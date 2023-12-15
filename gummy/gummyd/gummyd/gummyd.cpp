// Copyright 2021-2023 Francesco Fusco
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

using namespace gummyd;

std::optional<xcb::shared_image> shared_image(bool cond) {
    return cond ? std::optional<xcb::shared_image>(std::in_place) : std::nullopt;
}

void run(std::vector<xcb::randr::output> &randr_outputs,
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

    std::optional gamma_state = [&randr_outputs] {
        return randr_outputs.size() > 0 ?
                    std::optional<gummyd::gamma_state>(std::in_place, randr_outputs) : std::nullopt;
    }();
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
        const auto state_dir = xdg_state_dir() / fmt::format("gummyd/screen-{}", idx);

        // individual gamma settings must be already stored to avoid screen flickering,
        // when multiple threads are modifying gamma
        for (size_t model_idx = 0; model_idx < conf.screens[idx].models.size(); ++model_idx) {

            const auto &model = conf.screens[idx].models[model_idx];

            // dummy function
            std::function<void(int)> model_fn = [] ([[maybe_unused]] int val) {};

            switch (config::screen::model_id(model_idx)) {
            case BACKLIGHT:
                break;
            case BRIGHTNESS:
                if (gamma_state.has_value())
                    model_fn = std::bind(&gamma_state::store_brightness, &gamma_state.value(), idx, _1);
                break;
            case TEMPERATURE:
                if (gamma_state.has_value())
                    model_fn = std::bind(&gamma_state::store_temperature, &gamma_state.value(), idx, _1);
                break;
            }

            model_fn([&model, &state_dir] {
                if (model.mode == MANUAL)
                    return model.val;
                try {
                    return std::stoi(file_read(state_dir / config::screen::model_name(model.id)));
                } catch (std::exception &e) {
                    return model.max;
                }
            }());
        }

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
                if (gamma_state.has_value())
                    fn = std::bind(&gamma_state::set_brightness, &gamma_state.value(), idx, _1);
                break;
            case TEMPERATURE:
                if (gamma_state.has_value())
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

    if (gamma_state.has_value()) {
        threads.emplace_back([&] {
            spdlog::debug("[gamma refresh] start");
            while (true) {
                jthread_wait_until(std::chrono::seconds(10), stoken);
                spdlog::trace("gamma refresh");
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

int message_loop() {
    std::vector randr_outputs = [] {
        if (!gummyd::env("WAYLAND_DISPLAY").empty()) {
            return std::vector<xcb::randr::output>();
        }
        xcb::connection conn;
        return xcb::randr::outputs(conn, conn.first_screen());
    } ();

    const std::vector randr_edids = [&randr_outputs] {
        std::vector<decltype(xcb::randr::output::edid)> vec(randr_outputs.size());
        std::ranges::transform(randr_outputs, vec.begin(), &xcb::randr::output::edid);
        return vec;
    }();
    std::vector ddc_displays     = ddc::get_displays(randr_edids);
    std::vector sysfs_als        = sysfs::get_als();
    std::vector sysfs_backlights = sysfs::get_backlights();

    spdlog::info("[x11] found {} screen(s)", randr_outputs.size());
    spdlog::info("[sysfs] backlights: {}, als: {}", sysfs_backlights.size(), sysfs_als.size());

    if (ddc_displays.size() == 0) {
        spdlog::warn("No DDC displays found. i2c-dev module not loaded?");
    } else if (randr_outputs.size() > 0 && randr_outputs.size() > ddc_displays.size()) {
        throw std::runtime_error(fmt::format("could not match all x11 displays with ddc. x11: {}, ddc {}", randr_outputs.size(), ddc_displays.size()));
    }

    const size_t screen_count = [&] {
        if (ddc_displays.size() > 0)
            return ddc_displays.size();
        if (randr_outputs.size() > 0)
            return randr_outputs.size();
        if (sysfs_backlights.size() > 0)
            return sysfs_backlights.size();
        return 0ul;
    }();

    if (screen_count == 0) {
        fmt::print("No displays detected. There is nothing to do!\n");
        exit(EXIT_SUCCESS);
    }

    if (randr_outputs.size() > 0) {
        std::atexit([] () {
            xcb::connection conn;
            gamma_state(xcb::randr::outputs(conn, conn.first_screen())).reset_gamma();
        });
    }

    config conf(screen_count);

    const std::filesystem::path pipe_filepath = xdg_runtime_dir() / constants::fifo_filename;

    named_pipe pipe(pipe_filepath);

    const auto proxy = dbus::on_system_sleep([&pipe_filepath] (sdbus::Signal &sig) {
	    bool sleep;
	    sig >> sleep;
	    if (!sleep)
            file_write(pipe_filepath, "reset");
    });

	while (true) {

		std::jthread thr([&] (std::stop_token stoken) {
            run(randr_outputs, sysfs_backlights, sysfs_als, ddc_displays, conf, stoken);
		});

        soft_reset:
        const std::string data(file_read(pipe_filepath));

        if (data == "status") {
            std::string result;
            for (size_t idx = 0; const auto &x : ddc_displays) {
                fmt::format_to(std::back_inserter(result), "[screen {}] backlight: {}\n", idx++, x.get_brightness_string());
            }
            // Will block execution until the client reads from the pipe.
            file_write(pipe_filepath, result);
            goto soft_reset;
        }

		if (data == "stop")
			break;

		if (data == "reset")
			continue;

		const nlohmann::json msg = [&] {
			try {
				return nlohmann::json::parse(data);
			} catch (nlohmann::json::exception &e) {
                return nlohmann::json {{"error", e.what()}};
			}
		}();

        if (msg.contains("error")) {
            spdlog::error("{}", msg["error"].get<std::string>());
			continue;
		}

        conf = config(msg, screen_count);
	}

	return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "-v") == 0) {
        std::puts(VERSION);
        std::exit(EXIT_SUCCESS);
    }

    std::filesystem::create_directories(xdg_state_dir() / "gummyd");
    spdlog::set_level(spdlog::level::warn);
    spdlog::cfg::load_env_levels();
    spdlog::set_default_logger(spdlog::rotating_logger_mt("gummyd", xdg_state_dir() / "gummyd/logs/gummyd.log", 1048576 * 5, 3));
    spdlog::flush_every(std::chrono::seconds(10));
    spdlog::info("gummyd v{}", VERSION);

    lockfile flock(xdg_runtime_dir() / constants::flock_filename, false);
    if (flock.locked()) {
        return -1;
    }

    return message_loop();
}
