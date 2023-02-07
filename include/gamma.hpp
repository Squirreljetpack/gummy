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

#ifndef GAMMA_HPP
#define GAMMA_HPP

#include <atomic>

#include "xorg.hpp"
#include "channel.hpp"
#include "config.hpp"

class gamma_state {
	Xorg *_xorg;

	struct values {
		int brightness;
		int temperature;
	};

	std::vector<std::unique_ptr<std::atomic<values>>> _screens;

	void set(size_t screen_idx, values);
public:
	gamma_state(Xorg &xorg, std::vector<config::screen> screen_conf);
	void set_brightness(size_t screen_idx, int val);
	void set_temperature(size_t screen_idx, int val);
	void refresh(std::stop_token stoken);
};

#endif
