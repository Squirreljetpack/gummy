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

void brightness_server(Xorg &xorg, size_t screen_idx, channel<int> &ch, struct config::screenshot conf, std::stop_token stoken);
void brightness_client(channel<int> &ch, config::screen::model model, std::function<void(int)> model_fn, int adaptation_ms);

void als_server(Sysfs::ALS &als, channel<int> &ch, int sleep_ms, int prev, int cur, std::stop_token stoken);

struct time_data {
	bool in_range;
	std::time_t time_since_last_event;
	std::time_t adaptation_s;
	bool keep_alive;
};

struct time_target {
	int val;
	int duration_ms;
};

void time_server(channel<time_data> &ch, struct config::time conf, std::stop_token stoken);
void time_client(channel<time_data> &ch, config::screen::model model, std::function<void(int)> model_fn);
#endif // SERVER_HPP
