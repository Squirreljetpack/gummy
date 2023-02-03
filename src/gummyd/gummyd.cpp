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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fstream>

#include "utils.hpp"
#include "xorg.hpp"
#include "sysfs_devices.hpp"
#include "screenctl.hpp"
#include "gamma.hpp"
#include "config.hpp"
#include "server.hpp"
#include "easing.hpp"

void init_fifo()
{
	if (mkfifo(constants::fifo_name, S_IFIFO|0640) == 1) {
		syslog(LOG_ERR, "mkfifo err %d, aborting\n", errno);
		std::exit(EXIT_FAILURE);
	}
}

std::string read_fifo()
{
	std::ifstream fs(constants::fifo_name);

	if (fs.fail()) {
		syslog(LOG_ERR, "unable to open fifo, aborting\n");
		std::exit(EXIT_FAILURE);
	}

	std::ostringstream stream;
	stream << fs.rdbuf();

	fs.close();
	return stream.str();
}

void start(Xorg &xorg, config conf)
{
	assert(xorg.scr_count() == conf.screens.size());

	gamma_state gamma_state(xorg, conf.screens);

	/*Channel stop_signal(0);

	Channel <time_server_message> time_ch;

	std::vector<std::thread> servers;
	std::vector<std::thread> clients;

	// start time service
	servers.emplace_back([&] {
		time_server(time_ch, stop_signal, conf.time);
	});

	// read config.screens
	for (size_t idx = 0; idx < conf.screens.size(); ++idx) {

		const auto set_temp = [&] (int val) {
			gamma_state.set_temperature(idx, val);
		};

		for (const auto &model : conf.screens[idx].models) {
			if (model.mode == config::screen::TIME) {
				clients.emplace_back([&] {
					time_client(time_ch, model, set_temp);
				});
			}
		}
	}

	stop_signal.send(-1);

	for (auto &t : servers)
		t.join();
	for (auto &t : clients)
		t.join();*/
}

int message_loop()
{
	bool bootstrap = true;
	Xorg xorg;

	config conf(std::string(constants::config_name), xorg.scr_count());

	while (true) {

		if (bootstrap) {
			bootstrap = false;
			start(xorg, conf);
			continue;
		}

		const std::string data(read_fifo());

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
			printf("%s\n", msg["error"].get<std::string>().c_str());
			continue;
		}

		start(xorg, config(msg, xorg.scr_count()));
	}
}

int main(int argc, char **argv)
{
	if (argc > 1 && strcmp(argv[1], "-v") == 0) {
		puts(VERSION);
		exit(EXIT_SUCCESS);
	}

	openlog("gummyd", LOG_PID, LOG_DAEMON);

	if (int err = set_lock(constants::lock_name) > 0) {
		syslog(LOG_ERR, "lockfile err %d", err);
		exit(EXIT_FAILURE);
	}

	init_fifo();

	return message_loop();
}
