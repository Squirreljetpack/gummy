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

#ifndef CORE_HPP
#define CORE_HPP

#include <functional>
#include <stop_token>

#include "display.hpp"
#include "channel.hpp"
#include "sysfs_devices.hpp"
#include "time.hpp"
#include "config.hpp"

void brightness_server(display_server &dsp, size_t screen_idx, fushko::channel<int> &ch, struct config::screenshot conf, std::stop_token stoken);
void brightness_client(const fushko::channel<int> &ch, config::screen::model model, std::function<void(int)> model_fn, int adaptation_ms);

void als_server(sysfs::als &als, fushko::channel<double> &ch, struct config::als conf, std::stop_token stoken);
void als_client(const fushko::channel<double> &ch, config::screen::model model, std::function<void(int)> model_fn, int adaptation_ms);

struct time_data {
	long time_since_last_event;
	long adaptation_s;
	long in_range;
};

struct time_target {
	int val;
	int duration_ms;
};

void time_server(fushko::channel<time_data> &ch, struct config::time conf, std::stop_token stoken);
void time_client(const fushko::channel<time_data> &ch, config::screen::model model, std::function<void(int)> model_fn);
#endif // CORE_HPP
