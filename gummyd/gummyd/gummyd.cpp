﻿// Copyright 2021-2024 Francesco Fusco <f.fusco@pm.me>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <vector>
#include <functional>
#include <thread>
#include <optional>
#include <ranges>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/cfg/env.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include <gummyd/file.hpp>
#include <gummyd/utils.hpp>
#include <gummyd/sd-dbus.hpp>
#include <gummyd/channel.hpp>
#include <gummyd/core.hpp>
#include <gummyd/config.hpp>
#include <gummyd/display.hpp>
#include <gummyd/gamma.hpp>
#include <gummyd/sd-sysfs-devices.hpp>
#include <gummyd/constants.hpp>
#include <gummyd/ddc.hpp>
#include <gummyd/api.hpp>

using namespace gummyd;

std::optional<xcb::shared_image> shared_image(bool cond) {
    return cond ? std::optional<xcb::shared_image>(std::in_place) : std::nullopt;
}

std::optional<gummyd::gamma_state> opt_gamma_state (
    const std::vector<gummyd::xcb::randr::output> &randr_outputs,
    const std::vector<dbus::mutter::output> &mutter_outputs
) {
    if (mutter_outputs.size() > 0) {
        return std::optional<gummyd::gamma_state>(std::in_place, mutter_outputs);
    }

    if (randr_outputs.size() > 0) {
        return std::optional<gummyd::gamma_state>(std::in_place, randr_outputs);
    }

    return std::nullopt;
}

void run(std::vector<xcb::randr::output> &randr_outputs,
         std::optional<gummyd::gamma_state> &gamma_state,
         std::vector<sysfs::backlight> &sysfs_backlights,
         std::vector<sysfs::als>       &sysfs_als,
         std::vector<ddc::display>     &ddc_displays,
         config conf,
         std::stop_token stoken) {

    const size_t screenlight_clients = conf.clients_for(config::screen::mode::SCREENLIGHT);
    const size_t als_clients         = conf.clients_for(config::screen::mode::ALS);
    const size_t time_clients        = conf.clients_for(config::screen::mode::TIME);
    spdlog::info("clients: screenlight: {}, als: {}, time: {}", screenlight_clients, als_clients, time_clients);

	channel<double>    als_ch(-1.);
	channel<time_data> time_ch({-1, -1, -1});
    std::vector<channel<int>> screenlight_channels;
    screenlight_channels.reserve(screenlight_clients);
    std::vector<std::jthread> threads;

    std::optional shared_screen_image = shared_image(screenlight_clients > 0 && randr_outputs.size() > 0);

	if (time_clients > 0) {
        spdlog::debug("starting time_server...");
		threads.emplace_back([&] {
			time_server(time_ch, conf.time, stoken);
		});
	}

    if (als_clients > 0 && !sysfs_als.empty()) {
        spdlog::debug("starting als_server...");
        threads.emplace_back([&] {
            als_server(sysfs_als[0], als_ch, conf.als, stoken);
		});
	}

	for (size_t idx = 0; idx < conf.screens.size(); ++idx) {
        if (conf.clients_for(config::screen::mode::SCREENLIGHT, idx) > 0 && idx < randr_outputs.size()) {
            spdlog::debug("[screen {}] starting screenlight server", idx);
            screenlight_channels.emplace_back(-1);
            threads.emplace_back(screenlight_server, std::ref(*shared_screen_image), std::ref(randr_outputs[idx]), std::ref(screenlight_channels.back()), conf.screenshot, stoken);
		}

        using std::placeholders::_1;
        using enum config::screen::model_id;
        using enum config::screen::mode;

        for (size_t model_idx = 0; model_idx < conf.screens[idx].models.size(); ++model_idx) {

            const auto &model = conf.screens[idx].models[model_idx];

            // dummy function
            std::function<void(int)> fn = [] ([[maybe_unused]] int val) { };

            switch (config::screen::model_id(model_idx)) {

            case BACKLIGHT:
                if (idx < sysfs_backlights.size()) {
                    fn = std::bind(&sysfs::backlight::set_step, &sysfs_backlights[idx], _1);
                } else if (idx < ddc_displays.size()) {
                    fn = std::bind(&ddc::display::set_brightness_step, &ddc_displays[idx], _1);
                }
                break;
            case BRIGHTNESS:
                if (gamma_state.has_value() && conf.gamma.enabled)
                    fn = std::bind(&gamma_state::set_brightness, &gamma_state.value(), idx, _1);
                break;
            case TEMPERATURE:
                if (gamma_state.has_value() && conf.gamma.enabled)
                    fn = std::bind(&gamma_state::set_temperature, &gamma_state.value(), idx, _1);
                break;
            }

            const auto scr_model_id = fmt::format("screen-{}-{}", idx, config::screen::model_name(config::screen::model_id(model_idx)));

            switch (model.mode) {
            case MANUAL:
                spdlog::debug("[{}] setting manual value: {}", scr_model_id, model.val);
                fn(model.val);
                break;
            case ALS:
                if (sysfs_als.empty()) {
                    spdlog::warn("[{}] ALS not found, skipping", scr_model_id);
                    break;
                }
                spdlog::debug("[{}] starting als_client", scr_model_id);
                threads.emplace_back(als_client, std::ref(als_ch), idx, model, fn, conf.als.adaptation_ms);
                break;
            case SCREENLIGHT:
                if (screenlight_channels.empty()) {
                    spdlog::warn("[{}] screenlight unavailable, skipping", scr_model_id);
                    break;
                }
                spdlog::debug("[{}] starting screenlight_client", scr_model_id);
                threads.emplace_back(screenlight_client, std::ref(screenlight_channels.back()), idx, model, fn, conf.screenshot.adaptation_ms);
                break;
            case TIME:
                spdlog::debug("[{}] starting time_client", scr_model_id);
                threads.emplace_back(time_client, std::ref(time_ch), idx, model, fn);
                break;
            }
        }
	}

    if (gamma_state.has_value()
    && conf.gamma.enabled
    && conf.gamma.refresh_s > 0) {
        threads.emplace_back([&] {
            spdlog::debug("[gamma refresh] start");
            while (true) {
                jthread_wait_until(std::chrono::seconds(conf.gamma.refresh_s), stoken);
                spdlog::debug("[gamma refresh] refreshing...");
                gamma_state->reset_gamma();
                if (stoken.stop_requested()) {
                    spdlog::debug("[gamma refresh] stop requested");
                    return;
                }
            }
        });
    }

    spdlog::debug("joining {} threads", threads.size());
    std::ranges::for_each(threads, &std::jthread::join);

    spdlog::debug("{:=^60}", "end");
    spdlog::flush_on(spdlog::level::info);
}

int init() {
    std::vector randr_outputs ([] {
        if (!gummyd::env("WAYLAND_DISPLAY").empty()) {
            return std::vector<xcb::randr::output>();
        } else {
            xcb::connection conn;
            return xcb::randr::outputs(conn, conn.first_screen());
        }
    }());
    std::vector mutter_outputs (dbus::mutter::display_config_get_resources());

    // If we are on X11/mutter
    if (randr_outputs.size() > 0 && mutter_outputs.size() > 0) {
        // Sort mutter's outputs based on randr's order. They should already line up, but let's do it just in case.
        // This is bad and unnecessary if we used map lookups.
        std::vector<dbus::mutter::output> temp;
        for (const auto &randr_output : randr_outputs) {
            for (const auto &mutter_output : mutter_outputs) {
                if (randr_output.edid == mutter_output.edid) {
                    temp.push_back(mutter_output);
                }
            }
        }
        mutter_outputs = temp;

        if (mutter_outputs.size() != randr_outputs.size()) {
            throw std::runtime_error(fmt::format("randr and mutter display mismatch ({} vs {}).\n", randr_outputs.size(), mutter_outputs.size()));
        }
    }

    // Will be empty if we are neither on X11 or Wayland/Mutter.
    std::vector<std::array<uint8_t, 128>> edids;
    if (randr_outputs.size() > 0) {
        std::ranges::transform(randr_outputs, std::back_inserter(edids), &xcb::randr::output::edid);
    } else {
        std::ranges::transform(mutter_outputs, std::back_inserter(edids), &dbus::mutter::output::edid);
    }

    std::vector ddc_displays (ddc::get_displays(edids));
    std::vector sysfs_als (sysfs::get_als());
    std::vector sysfs_backlights (sysfs::get_backlights());

    spdlog::info("[x11] found {} screen(s)", randr_outputs.size());
    spdlog::info("[dbus] found {} screen(s)", mutter_outputs.size());
    spdlog::info("[sysfs] backlights: {}, als: {}", sysfs_backlights.size(), sysfs_als.size());

    if (ddc_displays.size() == 0) {
        spdlog::warn("No DDC displays found. i2c-dev module not loaded?");
    }

    const size_t screen_count ([&] {
        // unlikely that we go past the first check.
        if (ddc_displays.size() > 0)
            return ddc_displays.size();
        if (randr_outputs.size() > 0)
            return randr_outputs.size();
        if (mutter_outputs.size() > 0)
            return mutter_outputs.size();
        if (sysfs_backlights.size() > 0)
            return sysfs_backlights.size();
        return 0ul;
    }());

    if (screen_count == 0) {
        fmt::print("No displays detected. There is nothing to do!\n");
        exit(EXIT_SUCCESS);
    }

    const named_pipe pipe (xdg_runtime_dir() / constants::fifo_filename);

    const auto proxy ([&] {
        try {
            return dbus::on_system_sleep([] (sdbus::Signal sig) {
                bool sleep;
                sig >> sleep;
                if (!sleep) {
                    daemon_reset();
                }
            });
        } catch (const sdbus::Error &e) {
            spdlog::error("[dbus] on_system_sleep error: {}.", e.what());
            return std::unique_ptr<sdbus::IProxy>();
        }
    }());

    gummyd::config conf (screen_count);
    std::optional gamma_state (opt_gamma_state(randr_outputs, mutter_outputs));

    while (true) {
		std::jthread thr([&] (std::stop_token stoken) {
            run(randr_outputs, gamma_state, sysfs_backlights, sysfs_als, ddc_displays, conf, stoken);
		});

        read:
        const std::string data (file_read(pipe.path()));

        if (data == "status") {
            const std::vector<gummyd::gamma_state::settings> gamma_settings = [&gamma_state, &conf] {
                if (gamma_state.has_value() && conf.gamma.enabled) {
                    return gamma_state.value().get_settings();
                } else {
                    return std::vector<gummyd::gamma_state::settings>();
                }
            }();

            using enum config::screen::model_id;

            nlohmann::json out;

            for (size_t idx = 0; idx < screen_count; ++idx) {
                out[idx]["bl"] = [&] {
                    if (idx < sysfs_backlights.size()) {
                        return int(std::round(sysfs_backlights[idx].perc()));
                    } else if (idx < ddc_displays.size()) {
                        return ddc_displays[idx].get_brightness();
                    }
                    return -1;
                } ();
                out[idx]["brt"] = [&] {
                    if (!gamma_settings.empty()) {
                        return gamma_settings[idx].brightness / 10;
                    } else {
                        return -1;
                    }
                } ();
                out[idx]["temp"] = [&] {
                    if (!gamma_settings.empty()) {
                        return gamma_settings[idx].temperature;
                    } else {
                        return -1;
                    }
                } ();
                out[idx]["bl_mode"]   = conf.screens[idx].models[size_t(BACKLIGHT)].mode;
                out[idx]["brt_mode"]  = conf.screens[idx].models[size_t(BRIGHTNESS)].mode;
                out[idx]["temp_mode"] = conf.screens[idx].models[size_t(TEMPERATURE)].mode;
            }

            // Will block execution until the client reads from the pipe.
            // This is bad if the client crashes in the meantime.
            // Ideally we'd spawn a new thread for the status command.
            // But we'd need 2 pipes for reading/writing respectively.
            file_write(pipe.path(), nlohmann::json::to_cbor(out));
            goto read;
        }

        if (data == "stop") {
			break;
        }

        if (data == "reset") {
			continue;
        }

        try {
            conf = config(nlohmann::json::parse(data), screen_count);
        } catch (const nlohmann::json::exception &e) {
            spdlog::error("{}", e.what());
        }
	}

	return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "-v") == 0) {
        std::puts(VERSION);
        std::exit(EXIT_SUCCESS);
    }

    if (daemon_is_running()) {
        return 2;
    }

    std::filesystem::create_directories(xdg_state_dir() / "gummyd");
    spdlog::set_level(spdlog::level::warn);
    spdlog::cfg::load_env_levels();
    spdlog::set_default_logger(spdlog::rotating_logger_mt("gummyd", xdg_state_dir() / "gummyd/logs/gummyd.log", 1048576 * 5, 3));
    spdlog::flush_every(std::chrono::seconds(10));
    spdlog::info("gummyd v{}", VERSION);

    return init();
}
