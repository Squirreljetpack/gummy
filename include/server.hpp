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

#include <thread>

#include "xorg.hpp"
#include "channel.hpp"
#include "sysfs_devices.hpp"
#include "time.hpp"

#ifndef SERVER_HPP
#define SERVER_HPP

void brightness_server(Xorg &xorg, size_t screen_idx, Channel &brt_ch, Channel &sig, int sleep_ms, int prev, int cur);
void als_server(Sysfs::ALS &als, Channel &ch, Channel &sig, int sleep_ms, int prev, int cur);
void time_server(time_window tw, Channel2<std::tuple<int, int>> &ch, Channel &sig);

#endif // SERVER_HPP¨
