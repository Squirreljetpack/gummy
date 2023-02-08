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
	if (time_clients > 0) {
		threads.emplace_back([&] {
			time_server(time_ch, conf.time, stoken);
		});
	}
	// read config.screens
	for (size_t idx = 0; idx < conf.screens.size(); ++idx) {

		for (size_t model_idx = 0; model_idx < conf.screens[idx].models.size(); ++model_idx) {

			const auto &model = conf.screens[idx].models[model_idx];

			using std::placeholders::_1;
			using model_name = config::screen::model_idx;

			// dummy function
			std::function<void(int)> fn = [] ([[maybe_unused]] int val) { };

			switch (model_idx) {
			case model_name::BACKLIGHT:
				if (!vec.empty())
					fn = std::bind(&Sysfs::Backlight::set_step, &vec[0], _1);
				break;
			case model_name::BRIGHTNESS:
				fn = std::bind(&gamma_state::set_brightness, &gamma_state, idx, _1);
				break;
			case model_name::TEMPERATURE:
				fn = std::bind(&gamma_state::set_temperature, &gamma_state, idx, _1);
				break;
			}

			switch (model.mode) {
			case config::screen::UNINITIALIZED: {
				break;
			}
			case config::screen::MANUAL: {
				break;
			}
			case config::screen::ALS: {
				break;
			}
			case config::screen::SCREENSHOT: {
				server_channel<int> ch(1, 0);
				threads.emplace_back(std::jthread(brightness_server, std::ref(xorg), idx, std::ref(ch), conf.screenshot, stoken));
				//threads.emplace_back(std::jthread(brightness_client, std::ref(ch), model, fn, conf.screenshot.adaptation_ms));
				break;
			}
			case config::screen::TIME: {
				printf("screen %zu has model %zu on mode TIME\n", idx, model_idx);
				threads.emplace_back(std::jthread(time_client, std::ref(time_ch), model, fn));
				break;
			}
			}
		}
	}

	for (auto &t : threads)
		t.join();

	puts("==========================end=============================");
}

int message_loop()
{
	Xorg xorg;

	config conf(xorg.scr_count());

	while (true) {

		std::jthread thr([&] (std::stop_token stoken) {
			start(xorg, conf, stoken);
		});

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
		conf = config(msg, xorg.scr_count());
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
