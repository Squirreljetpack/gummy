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
#include <ctime>
#include <string>

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

/* Interpolate val to an array index.
   The integral part of the return value is the index itself,
   while the fractional part is the interpolation factor
   between the index and the next one. */
double remap_to_idx(int val, int val_min, int val_max, size_t arr_sz)
{
	return remap(val, val_min, val_max, 0, arr_sz - 1);
}

// Get the fractional part of a floating point number.
double mant(double x)
{
	return x - std::floor(x);
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

time_t timestamp_modify(std::time_t ts, int h, int m, int s)
{
	std::tm tm = *std::localtime(&ts);
	tm.tm_hour = h;
	tm.tm_min  = m;
	tm.tm_sec  = 0;
	tm.tm_sec += s;
	return std::mktime(&tm);
}

Timestamps timestamps_update(const std::string &start, const std::string &end, int seconds)
{
	Timestamps ts;
	ts.cur = std::time(nullptr);
	ts.start = timestamp_modify(ts.cur,
	    std::stoi(start.substr(0, 2)),
	    std::stoi(start.substr(3, 2)),
	    seconds);
	ts.end = timestamp_modify(ts.cur,
	    std::stoi(end.substr(0, 2)),
	    std::stoi(end.substr(3, 2)),
	    seconds);
	return ts;
}

void print_timestamp(std::time_t ts)
{
	printf("%s\n", std::asctime(std::localtime(&ts)));
}
