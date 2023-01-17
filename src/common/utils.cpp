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

#include "utils.h"
#include "defs.h"

#include <fcntl.h>
#include <cmath>

int calc_brightness(uint8_t *buf, uint64_t buf_sz, int bytes_per_pixel, int stride)
{
	uint64_t rgb[3] {};
	for (uint64_t i = 0, inc = stride * bytes_per_pixel; i < buf_sz; i += inc) {
		rgb[0] += buf[i + 2];
		rgb[1] += buf[i + 1];
		rgb[2] += buf[i];
	}

	return (rgb[0] * 0.2126 + rgb[1] * 0.7152 + rgb[2] * 0.0722) * stride / (buf_sz / bytes_per_pixel);
}

double lerp(double x, double a, double b)
{
	return ((1 - x) * a) + (x * b);
}

double invlerp(double x, double a, double b)
{
	return (x - a) / (b - a);
}

double remap(double x, double a, double b, double ay, double by)
{
	return lerp(invlerp(x, a, b), ay, by);
}

double temp_step_to_color_mult(int step, size_t color_ch)
{
	constexpr size_t max_rows_idx = ingo_thies_table.size() / 3 - 1;
	const double tmp = remap(step, temp_steps_min, temp_steps_max, 0, max_rows_idx);
	const size_t row = floor(tmp);
	const size_t upper_idx = (row + 1) * 3 > max_rows_idx * 3 ? max_rows_idx * 3 : (row + 1) * 3;
	return lerp(std::abs(row - tmp), ingo_thies_table[row * 3 + color_ch], ingo_thies_table[upper_idx + color_ch]);
}

Animation animation_init(int start, int end, int fps, int duration_ms)
{
	Animation a;
	a.start_step = start;
	a.diff       = end - start;
	a.fps        = fps;
	a.duration_s = duration_ms / 1000.;
	a.slice      = 1. / a.fps;
	a.elapsed    = 0.;
	return a;
}

double ease_out_expo(double t, double b , double c, double d)
{
	return (t == d) ? b + c : c * (-pow(2, -10 * t / d) + 1) + b;
}

double ease_in_out_quad(double t, double b, double c, double d)
{
	if ((t /= d / 2) < 1)
		return c / 2 * t * t + b;
	else
		return -c / 2 * ((t - 1) * (t - 3) - 1) + b;
}

int set_lock()
{
	int fd = open(lock_name, O_WRONLY | O_CREAT, 0666);
	if (fd == -1)
		return 1;

	flock fl;
	fl.l_type   = F_WRLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start  = 0;
	fl.l_len    = 1;

	if (fcntl(fd, F_SETLK, &fl) == -1)
		return 2;

	return 0;
}
