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

//#include <syslog.h>

#include <nlohmann/json.hpp>

#include "core.hpp"

#include "dbus.hpp"
#include "file.hpp"
#include "utils.hpp"

#include "config.hpp"
#include "display.hpp"
#include "gamma.hpp"
#include "sysfs_devices.hpp"

using namespace fushko;

void run(display_server &dsp, config conf, std::stop_token stoken)
{
	assert(dsp.scr_count() == conf.screens.size());

	const size_t screenshot_clients = conf.clients_for(config::screen::mode::SCREENSHOT);
	const size_t als_clients        = conf.clients_for(config::screen::mode::ALS);
	const size_t time_clients       = conf.clients_for(config::screen::mode::TIME);
	LOG_FMT_("clients: als: {}, screenlight: {}, time: {}\n", als_clients, screenshot_clients, time_clients);

	std::vector<std::jthread> threads;

	channel<double>    als_ch(-1.);
	channel<time_data> time_ch({-1, -1, -1});
	std::vector<channel<int>> brt_channels;
	brt_channels.reserve(screenshot_clients);

	if (time_clients > 0) {
		threads.emplace_back([&] {
			time_server(time_ch, conf.time, stoken);
		});
	}

	std::vector<sysfs::backlight> backlights = sysfs::get_backlights();
	std::vector<sysfs::als> als = sysfs::get_als();

	if (als_clients > 0 && !als.empty()) {
		threads.emplace_back([&] {
			als_server(als[0], als_ch, conf.als, stoken);
		});
	}

	gamma_state gamma_state(dsp, conf.screens);

	for (size_t idx = 0; idx < conf.screens.size(); ++idx) {

		if (conf.clients_for(config::screen::mode::SCREENSHOT, idx) > 0) {
			brt_channels.emplace_back(-1);
			threads.emplace_back(std::jthread(brightness_server, std::ref(dsp), idx, std::ref(brt_channels.back()), conf.screenshot, stoken));
		}

		for (size_t model_idx = 0; model_idx < conf.screens[idx].models.size(); ++model_idx) {

			const auto &model = conf.screens[idx].models[model_idx];

			// dummy function
			std::function<void(int)> fn = [] ([[maybe_unused]] int val) { };

			switch (config::screen::model_id(model_idx)) {
			using enum config::screen::model_id;
			using std::placeholders::_1;
			case BACKLIGHT:
			    if (idx < backlights.size())
					fn = std::bind(&sysfs::backlight::set_step, &backlights[idx], _1);
				break;
			case BRIGHTNESS:
				fn = std::bind(&gamma_state::set_brightness, &gamma_state, idx, _1);
				break;
			case TEMPERATURE:
				fn = std::bind(&gamma_state::set_temperature, &gamma_state, idx, _1);
				break;
			}

			switch (model.mode) {
			using enum config::screen::mode;
			case UNINITIALIZED: {
				throw std::runtime_error("invalid model state");
			}
			case MANUAL: {
				fn(model.val);
				break;
			}
			case ALS: {
				threads.emplace_back(std::jthread(als_client, std::ref(als_ch), model, fn, conf.als.adaptation_ms));
				break;
			}
			case SCREENSHOT: {
				threads.emplace_back(std::jthread(brightness_client, std::ref(brt_channels.back()), model, fn, conf.screenshot.adaptation_ms));
				break;
			}
			case TIME: {
				threads.emplace_back(std::jthread(time_client, std::ref(time_ch), model, fn));
				break;
			}
			}
		}
	}

	threads.emplace_back([&] {
		while (true) {
			jthread_wait_until(std::chrono::seconds(10), stoken);
			gamma_state.refresh();
			if (stoken.stop_requested())
				return;
		}
	});

	for (auto &t : threads)
		t.join();

	LOG_FMT_("{:=^60}\n", "end");
}

int message_loop()
{
	display_server dsp;
	LOG_FMT_("[display_server] found {} screen(s)\n", dsp.scr_count());

	config conf(dsp.scr_count());

	named_pipe pipe(constants::fifo_filepath);

	const auto proxy = sdbus_on_system_sleep([] (sdbus::Signal &sig) {
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
				return nlohmann::json({"exception", e.what()});
			}
		}();

		if (msg.contains("exception")) {
			LOG_ERR_FMT_("{}\n", msg["exception"].get<std::string>());
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

	lock_file flock(constants::flock_filepath);
	//openlog("gummyd", LOG_FMT_PID, LOG_FMT_DAEMON);

	return message_loop();
}
