/**
* gummy
* Copyright (C) 2022  Francesco Fusco
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

#include <syslog.h>
#include <latch>

#include "config.hpp"
#include "file.hpp"
#include "utils.hpp"
#include "xorg.hpp"
#include "sysfs_devices.hpp"
#include "gamma.hpp"
#include "server.hpp"

void start(Xorg &xorg, config conf, std::stop_token stoken)
{
	assert(xorg.scr_count() == conf.screens.size());

	gamma_state gamma_state(xorg, config(xorg.scr_count()).screens);

	std::vector<std::jthread> threads;
	threads.emplace_back([&] {
		gamma_state.refresh(stoken);
	});

	std::vector<Sysfs::Backlight> vec = Sysfs::get_backlights();

	if (!vec.empty()) {
		vec[0].set_step(conf.screens[0].models[config::screen::model_idx::BACKLIGHT].val);
	}

	//////////////////////////////////////////////////////////////////////

	const size_t screenshot_clients = conf.clients_for(config::screen::mode::SCREENSHOT);
	const size_t als_clients        = conf.clients_for(config::screen::mode::ALS);
	const size_t time_clients       = conf.clients_for(config::screen::mode::TIME);
	printf("screenshot: %zu, als: %zu, time: %zu\n", screenshot_clients, als_clients, time_clients);

	server_channel<time_data> time_ch(time_clients, {0,0,0,0});

	// start time service
	threads.emplace_back([&] {
		time_server(time_ch, conf.time, stoken);
	});

	const auto model_fn = [&] (size_t idx, config::screen::model_idx model_idx, int val) {
		switch (model_idx) {
		case config::screen::model_idx::BACKLIGHT:
			printf("model fn: set backlight\n");
			if (!vec.empty()) {
				vec[0].set_step(val);
			}
			break;
		case config::screen::model_idx::BRIGHTNESS:
			printf("model fn: set brightness\n");
			gamma_state.set_brightness(idx, val);
			break;
		case config::screen::model_idx::TEMPERATURE:
			printf("model fn: set temperature\n");
			gamma_state.set_temperature(idx, val);
			break;
		}
	};

	// read config.screens
	for (size_t idx = 0; idx < conf.screens.size(); ++idx) {

		for (size_t model_idx = 0; model_idx < conf.screens[idx].models.size(); ++model_idx) {

			const auto &model = conf.screens[idx].models[model_idx];

			if (model.mode == config::screen::TIME) {
				printf("screen %zu has model %zu on mode TIME\n", idx, model_idx);
				const auto x = [&] (int val) {
					gamma_state.set_temperature(idx, val);
				};
				threads.emplace_back(std::jthread(time_client, std::ref(time_ch), model, x));
			}
		}
	}

	for (auto &t : threads)
		t.join();
}

int message_loop()
{
	Xorg xorg;

	std::jthread thr([&] (std::stop_token stoken) {
		start(xorg, config(xorg.scr_count()), stoken);
	});

	while (true) {

		const std::string data(file_read(constants::fifo_filepath));

		if (data == "stop")
			return EXIT_SUCCESS;

		const json msg = [&] {
			try {
				return json::parse(data);
			} catch (json::exception &e) {
				return json({"exception", e.what()});
			}
		}();

		if (msg.contains("exception")) {
			printf("%s\n", msg["exception"].get<std::string>().c_str());
			continue;
		}

		thr.request_stop();
		thr.join();
		thr = std::jthread(start, std::ref(xorg), config(msg, xorg.scr_count()), thr.get_stop_token());
	}
}

int main(int argc, char **argv)
{
	if (argc > 1 && strcmp(argv[1], "-v") == 0) {
		puts(VERSION);
		exit(EXIT_SUCCESS);
	}

	openlog("gummyd", LOG_PID, LOG_DAEMON);

	if (set_flock(constants::flock_filepath) < 0) {
		syslog(LOG_ERR, "lockfile error");
		exit(EXIT_FAILURE);
	}

	make_fifo(constants::fifo_filepath);

	return message_loop();
}
