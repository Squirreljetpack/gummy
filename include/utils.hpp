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

#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <chrono>
#include <thread>
#include <functional>

#include <cstddef>
#include <cstdint>
#include <ctime>
#include <cmath>
#include <fcntl.h>

struct Animation
{
	double elapsed;
	double slice;
	double duration_s;
	int fps;
	int start_step;
	int diff;
};

struct Timestamps
{
    std::time_t cur;
	std::time_t start;
	std::time_t end;
};

inline double lerp(double x, double a, double b)
{
	return ((1 - x) * a) + (x * b);
}

inline double invlerp(double x, double a, double b)
{
	return (x - a) / (b - a);
}

inline double remap(double x, double a, double b, double ay, double by)
{
	return lerp(invlerp(x, a, b), ay, by);
}

/* Interpolate val to an array index.
   The integral part of the return value is the index itself,
   while the fractional part is the interpolation factor
   between the index and the next one. */
inline double remap_to_idx(int val, int val_min, int val_max, size_t arr_sz)
{
	return remap(val, val_min, val_max, 0, arr_sz - 1);
}

// Get the fractional part of a floating point number.
inline double mant(double x)
{
	return x - std::floor(x);
}

inline Animation animation_init(int start, int end, int fps, int duration_ms)
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

inline double ease_out_expo(double t, double b , double c, double d)
{
	return (t == d) ? b + c : c * (-pow(2, -10 * t / d) + 1) + b;
}

inline double ease_in_out_quad(double t, double b, double c, double d)
{
	if ((t /= d / 2) < 1)
		return c / 2 * t * t + b;
	else
		return -c / 2 * ((t - 1) * (t - 3) - 1) + b;
}

inline int ease_in_out_quad_loop(Animation a, int prev, int cur, int end, std::function<bool(int, int)> fn)
{
	if (!fn(cur, prev) || cur == end)
		return cur;

	std::this_thread::sleep_for(std::chrono::milliseconds(1000 / a.fps));

	return ease_in_out_quad_loop(a,
	    cur,
	    int(ease_in_out_quad(a.elapsed += a.slice, a.start_step, a.diff, a.duration_s)),
	    end,
	    fn);
}

inline int ease_out_expo_loop(Animation a, int prev, int cur, int end, std::function<bool(int, int)> fn)
{
	if (!fn(cur, prev) || cur == end)
		return cur;

	std::this_thread::sleep_for(std::chrono::milliseconds(1000 / a.fps));

	return ease_out_expo_loop(a,
	    cur,
	    int(round(ease_out_expo(a.elapsed += a.slice, a.start_step, a.diff, a.duration_s))),
	    end,
	    fn);
}

inline int set_lock(std::string name)
{
	int fd = open(name.c_str(), O_WRONLY | O_CREAT, 0666);
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

inline time_t timestamp_modify(std::time_t ts, int h, int m, int s)
{
	std::tm tm = *std::localtime(&ts);
	tm.tm_hour = h;
	tm.tm_min  = m;
	tm.tm_sec  = 0;
	tm.tm_sec += s;
	return std::mktime(&tm);
}

inline Timestamps timestamps_update(const std::string &start, const std::string &end, int seconds)
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

inline std::string timestamp_fmt(std::time_t ts)
{
	std::string str(std::asctime(std::localtime(&ts)));
	str.pop_back();
	return str;
}

#endif // UTILS_H
