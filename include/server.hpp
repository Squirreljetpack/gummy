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

#ifndef SERVER_HPP
#define SERVER_HPP

#include <thread>
#include <functional>

#include "xorg.hpp"
#include "channel.hpp"
#include "sysfs_devices.hpp"
#include "time.hpp"
#include "config.hpp"

void brightness_server(Xorg &xorg, size_t screen_idx, Channel<> &brt_ch, Channel<> &sig, int sleep_ms, int prev, int cur);
void als_server(Sysfs::ALS &als, Channel<> &ch, Channel<> &sig, int sleep_ms, int prev, int cur);

struct time_server_message {
	bool in_range;
	std::time_t time_since_last_event;
	std::time_t adaptation_s;

	bool keep_alive;
};

struct time_target {
	int val;
	int duration_ms;
};

void time_server(Channel<time_server_message> &ch, Channel<> &sig, config::time_config conf);
void time_client(Channel<time_server_message> &time_ch, config::screen::model model, std::function<void(int)> model_fn);

#endif // SERVER_HPP
