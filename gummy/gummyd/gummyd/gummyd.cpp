// Copyright (c) 2021-2023, Francesco Fusco. All rights reserved.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <nlohmann/json.hpp>
#include <fmt/std.h>
#include <spdlog/spdlog.h>
#include <spdlog/cfg/env.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include <gummyd/file.hpp>
#include <gummyd/utils.hpp>
#include <gummyd/sdbus-util.hpp>
#include <gummyd/channel.hpp>
#include <gummyd/core.hpp>
#include <gummyd/config.hpp>
#include <gummyd/display.hpp>
#include <gummyd/gamma.hpp>
#include <gummyd/sysfs_devices.hpp>
#include <gummyd/constants.hpp>
#include <gummyd/ddc.hpp>

using namespace gummyd;

void run(display_server &dsp, std::vector<ddc::display> &ddc_displays, config conf, std::stop_token stoken)
{
	assert(dsp.scr_count() == conf.screens.size());

    const size_t screenlight_clients = conf.clients_for(config::screen::mode::SCREENLIGHT);
    const size_t als_clients         = conf.clients_for(config::screen::mode::ALS);
    const size_t time_clients        = conf.clients_for(config::screen::mode::TIME);
    spdlog::debug("clients: screenlight: {}, als: {}, time: {}", screenlight_clients, als_clients, time_clients);

	std::vector<std::jthread> threads;

	channel<double>    als_ch(-1.);
	channel<time_data> time_ch({-1, -1, -1});
	std::vector<channel<int>> brt_channels;
    brt_channels.reserve(screenlight_clients);

	if (time_clients > 0) {
        spdlog::debug("starting time_server...");
		threads.emplace_back([&] {
			time_server(time_ch, conf.time, stoken);
		});
	}

    std::vector<sysfs::backlight> backlights = sysfs::get_backlights();

    std::vector<sysfs::als> als = sysfs::get_als();
    spdlog::info("[sysfs] backlights: {}, als: {}", backlights.size(), als.size());

	if (als_clients > 0 && !als.empty()) {
        spdlog::debug("starting als_server...");
		threads.emplace_back([&] {
			als_server(als[0], als_ch, conf.als, stoken);
		});
	}

	gamma_state gamma_state(dsp, conf.screens);

	for (size_t idx = 0; idx < conf.screens.size(); ++idx) {

        if (conf.clients_for(config::screen::mode::SCREENLIGHT, idx) > 0) {
            spdlog::debug("[screen {}] starting screenlight server", idx);
			brt_channels.emplace_back(-1);
            threads.emplace_back(screenlight_server, std::ref(dsp), idx, std::ref(brt_channels.back()), conf.screenshot, stoken);
		}

        const auto state_dir = xdg_state_dir() / fmt::format("gummyd/screen-{}", idx);

        for (size_t model_idx = 0; model_idx < conf.screens[idx].models.size(); ++model_idx) {

			const auto &model = conf.screens[idx].models[model_idx];
            using enum config::screen::model_id;
            using enum config::screen::mode;
            using std::placeholders::_1;

			// dummy function
			std::function<void(int)> fn = [] ([[maybe_unused]] int val) { };

			switch (config::screen::model_id(model_idx)) {
			case BACKLIGHT:
				break;
			case BRIGHTNESS:
                fn = std::bind(&gamma_state::set_brightness, &gamma_state, idx, _1);
				break;
			case TEMPERATURE:
                fn = std::bind(&gamma_state::set_temperature, &gamma_state, idx, _1);
				break;
            }

            // if two threads are modifying two different gamma values,
            // they must both know beforehand the respective gamma values,
            // in order to avoid screen flickering
            const int val = [&] {
                if (model.mode == MANUAL)
                    return model.val;

                try {
                    return std::stoi(file_read(state_dir / config::screen::model_name(model.id)));
                } catch (std::exception &e) {
                    return model.max;
                }
            }();

            if (model.id == BRIGHTNESS || model.id == TEMPERATURE)
                fn(val);
        }

        for (size_t model_idx = 0; model_idx < conf.screens[idx].models.size(); ++model_idx) {

            const auto &model = conf.screens[idx].models[model_idx];

            using enum config::screen::model_id;
            using enum config::screen::mode;
            using std::placeholders::_1;

            // dummy function
            std::function<void(int)> fn = [] ([[maybe_unused]] int val) { };

            switch (config::screen::model_id(model_idx)) {

            case BACKLIGHT:
                if (idx < backlights.size()) {
                    fn = std::bind(&sysfs::backlight::set_step, &backlights[idx], _1);
                } else if (idx < ddc_displays.size()) {
                    fn = std::bind(&ddc::display::set_brightness_step, &ddc_displays[idx], _1);
                }
                break;
            case BRIGHTNESS:
                fn = std::bind(&gamma_state::apply_brightness, &gamma_state, idx, _1);
                break;
            case TEMPERATURE:
                fn = std::bind(&gamma_state::apply_temperature, &gamma_state, idx, _1);
                break;
            }

            const auto scr_model_id = fmt::format("screen-{}-{}", idx, config::screen::model_name(config::screen::model_id(model_idx)));

            switch (model.mode) {
            case MANUAL:
                spdlog::debug("[{}] setting manual value: {}", scr_model_id, model.val);
                fn(model.val);
                break;
            case ALS:
                if (als.empty()) {
                    spdlog::warn("[{}] ALS not found, skipping", scr_model_id);
                    break;
                }
                spdlog::debug("[{}] starting als_client", scr_model_id);
                threads.emplace_back(als_client, std::ref(als_ch), idx, model, fn, conf.als.adaptation_ms);
                break;
            case SCREENLIGHT:
                spdlog::debug("[{}] starting screenlight_client", scr_model_id);
                threads.emplace_back(screenlight_client, std::ref(brt_channels.back()), idx, model, fn, conf.screenshot.adaptation_ms);
                break;
            case TIME:
                spdlog::debug("[{}] starting time_client", scr_model_id);
                threads.emplace_back(time_client, std::ref(time_ch), idx, model, fn);
                break;
            }
        }
	}

	threads.emplace_back([&] {
        spdlog::debug("[gamma refresh] start");
		while (true) {
            jthread_wait_until(std::chrono::seconds(10), stoken);
            spdlog::debug("gamma refresh");
            gamma_state.apply_to_all_screens();
            if (stoken.stop_requested()) {
                spdlog::debug("[gamma refresh] stop requested");
				return;
            }
		}
	});

    spdlog::debug("joining {} threads", threads.size());
    for (auto &t : threads) {
        spdlog::debug("waiting for thread {}", t.get_id());
		t.join();
    }

    spdlog::debug("{:=^60}", "end");
    spdlog::flush_on(spdlog::level::debug);
}

int message_loop()
{
    display_server dsp;
    spdlog::debug("[display_server] found {} screen(s)", dsp.scr_count());

    // restore default gamma on exit.
    // in the unlikely event that display_server throws, gamma state is already at default
    const auto fn = [] () {
        display_server dsp;
        gamma_state(dsp).apply_to_all_screens();
    };

    std::atexit(fn);
	//std::set_terminate(fn);
    config conf(dsp.scr_count());

    const std::filesystem::path pipe_filepath = xdg_runtime_dir() / constants::fifo_filename;

    named_pipe pipe(pipe_filepath);

    const auto proxy = sdbus_util::on_system_sleep([&pipe_filepath] (sdbus::Signal &sig) {
	    bool sleep;
	    sig >> sleep;
	    if (!sleep)
            file_write(pipe_filepath, "reset");
    });

    std::vector<ddc::display> ddc_displays = ddc::get_displays();

	while (true) {

		std::jthread thr([&] (std::stop_token stoken) {
            run(dsp, ddc_displays, conf, stoken);
		});

        const std::string data(file_read(pipe_filepath));

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

		conf = config(msg, dsp.scr_count());
	}

	return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
	if (argc > 1 && strcmp(argv[1], "-v") == 0) {
		std::puts(VERSION);
		std::exit(EXIT_SUCCESS);
	}

    lockfile flock(xdg_runtime_dir() / constants::flock_filename);

    std::filesystem::create_directories(xdg_state_dir() / "gummyd");

    spdlog::cfg::load_env_levels();
    spdlog::set_default_logger(spdlog::rotating_logger_mt("gummyd", xdg_state_dir() / "gummyd/logs/gummyd.log", 1048576 * 5, 3));
	spdlog::info("gummyd v{}", VERSION);
	return message_loop();
}
