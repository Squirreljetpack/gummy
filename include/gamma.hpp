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

#include "display.hpp"
#include "config.hpp"

class gamma_state {
	struct values {
		int brightness = constants::brt_steps_max;
		int temperature = 6500;
	};

	display_server *dsp_;
	std::vector<values> screens_;

	static values sanitize(values);
	void set(size_t screen_idx, values);
public:
	gamma_state(display_server &dsp);
	gamma_state(display_server &dsp, std::vector<config::screen> conf);
	void set_brightness(size_t screen_idx, int val);
	void set_temperature(size_t screen_idx, int val);
	void refresh();
};

#endif // GAMMA_HPP
