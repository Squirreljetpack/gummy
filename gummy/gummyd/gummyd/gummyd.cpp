/**
* gummy
* Copyright (C) 2023  Francesco Fusco
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
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

using namespace gummyd;

void run(display_server &dsp, config conf, std::stop_token stoken)
{
	assert(dsp.scr_count() == conf.screens.size());

	const size_t screenshot_clients = conf.clients_for(config::screen::mode::SCREENSHOT);
	const size_t als_clients        = conf.clients_for(config::screen::mode::ALS);
	const size_t time_clients       = conf.clients_for(config::screen::mode::TIME);
    spdlog::debug("clients: als: {}, screenlight: {}, time: {}", als_clients, screenshot_clients, time_clients);

	std::vector<std::jthread> threads;

	channel<double>    als_ch(-1.);
	channel<time_data> time_ch({-1, -1, -1});
	std::vector<channel<int>> brt_channels;
	brt_channels.reserve(screenshot_clients);

	if (time_clients > 0) {
        spdlog::debug("starting time_server...");
		threads.emplace_back([&] {
			time_server(time_ch, conf.time, stoken);
		});
	}

    std::vector<backlight> backlights = get_backlights();
    std::vector<als> als = get_als();

	if (als_clients > 0 && !als.empty()) {
        spdlog::debug("starting time_server...");
		threads.emplace_back([&] {
			als_server(als[0], als_ch, conf.als, stoken);
		});
	}

	gamma_state gamma_state(dsp, conf.screens);

	for (size_t idx = 0; idx < conf.screens.size(); ++idx) {

		if (conf.clients_for(config::screen::mode::SCREENSHOT, idx) > 0) {
			brt_channels.emplace_back(-1);
            threads.emplace_back(screenlight_server, std::ref(dsp), idx, std::ref(brt_channels.back()), conf.screenshot, stoken);
		}

        const std::string xdg_state_dir = xdg_state_filepath(fmt::format("gummyd/screen-{}", idx));

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
                    const std::string filepath = fmt::format("{}/{}", xdg_state_dir, config::screen::model_name(model.id));
                    return std::stoi(file_read(filepath));
                } catch (std::exception &e) {
                    return model.max;
                }
            }();

            fn(val);
        }

        for (size_t model_idx = 0; model_idx < conf.screens[idx].models.size(); ++model_idx) {

            const auto &model = conf.screens[idx].models[model_idx];

            using enum config::screen::model_id;
            using enum config::screen::mode;
            using std::placeholders::_1;

            const auto model_name = config::screen::model_name(config::screen::model_id(model_idx));

            // dummy function
            std::function<void(int)> fn = [] ([[maybe_unused]] int val) { };

            switch (config::screen::model_id(model_idx)) {

            case BACKLIGHT:
                if (idx < backlights.size())
                    fn = std::bind(&backlight::set_step, &backlights[idx], _1);
                break;
            case BRIGHTNESS:
                fn = std::bind(&gamma_state::apply_brightness, &gamma_state, idx, _1);
                break;
            case TEMPERATURE:
                fn = std::bind(&gamma_state::apply_temperature, &gamma_state, idx, _1);
                break;
            }

            switch (model.mode) {
            case MANUAL:
                spdlog::debug("[{} model] setting manual value: {}", model_name, model.val);
                fn(model.val);
                break;
            case ALS:
                spdlog::debug("[{} model] starting als_client", model_name);
                threads.emplace_back(als_client, std::ref(als_ch), idx, model, fn, conf.als.adaptation_ms);
                break;
            case SCREENSHOT:
                spdlog::debug("[{} model] starting screenlight_client", model_name);
                threads.emplace_back(screenlight_client, std::ref(brt_channels.back()), idx, model, fn, conf.screenshot.adaptation_ms);
                break;
            case TIME:
                spdlog::debug("[{} model] starting time_client", model_name);
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
	for (auto &t : threads)
		t.join();

    spdlog::debug("{:=^60}", "end");
}

int message_loop()
{
	display_server dsp;
    spdlog::debug("[display_server] found {} screen(s)", dsp.scr_count());

	config conf(dsp.scr_count());

	named_pipe pipe(constants::fifo_filepath);

    const auto proxy = sdbus_util::on_system_sleep([] (sdbus::Signal &sig) {
	    bool sleep;
	    sig >> sleep;
	    if (!sleep)
	        file_write(constants::fifo_filepath, "reset");
    });

	while (true) {

		std::jthread thr([&] (std::stop_token stoken) {
			run(dsp, conf, stoken);
		});

		const std::string data(file_read(constants::fifo_filepath));

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

    auto logger = spdlog::rotating_logger_mt("gummyd", xdg_state_filepath("gummyd/logs/gummyd.log"), 1048576 * 5, 1);
    spdlog::set_default_logger(logger);

    spdlog::set_level(gummyd::env_log_level());

    std::filesystem::create_directories(xdg_state_filepath("gummyd/"));

	lock_file flock(constants::flock_filepath);

	return message_loop();
}
