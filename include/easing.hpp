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

#ifndef EASING_HPP
#define EASING_HPP

#include <functional>
#include <chrono>
#include <thread>
#include "utils.hpp"

// Easing functions adapted from:
// https://github.com/warrenm/AHEasing/blob/master/AHEasing/easing.c

namespace easing {

const auto ease_out_expo = [] (double t) {
	return (t == 1.0) ? t : 1 - pow(2, -10 * t);
};

const auto ease_in_out_quad = [] (double t) {
	if (t < 0.5)
		return 2 * t * t;
	else
		return (-2 * t * t) + (4 * t) - 1;
};

void animate(std::function<double(double t)> easing, int start, int end, int duration_ms, int progress_ms, int prev_step, int cur_step, std::function<void(int)> fn, std::function<bool()> interrupt)
{
	while (true) {
		if (cur_step == end) {
			return;
		}

		prev_step = cur_step;
		cur_step = lerp(start, end, easing(double(progress_ms) / duration_ms));
		//printf("cur step %d, progress %d/%d\n", cur_step, progress_ms, duration_ms);

		if (prev_step != cur_step)
			fn(cur_step);

		if (progress_ms == duration_ms) {
			return;
		}

		if (interrupt()) {
			return;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		progress_ms += 1;
	}
}

}

#endif // EASING_HPP
