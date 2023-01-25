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
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <cmath>
#include <fcntl.h>



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

inline std::string timestamp_fmt(std::time_t ts)
{
	std::string str(std::asctime(std::localtime(&ts)));
	str.pop_back();
	return str;
}

class time_window
{
    std::time_t _cur;
	std::time_t _start;
	std::time_t _end;
public:
	time_window(std::time_t cur, std::string start, std::string end, int seconds);
	bool in_range();
	std::time_t delta();
};

inline time_window::time_window(std::time_t cur, std::string start, std::string end, int seconds)
{
	_cur = cur;

	_start = timestamp_modify(_cur,
	    std::stoi(start.substr(0, 2)),
	    std::stoi(start.substr(3, 2)),
	    seconds);

	_end = timestamp_modify(_cur,
	    std::stoi(end.substr(0, 2)),
	    std::stoi(end.substr(3, 2)),
	    seconds);

	printf("cur: %s\n", timestamp_fmt(_cur).c_str());
	printf("srt: %s\n", timestamp_fmt(_start).c_str());
	printf("end: %s\n", timestamp_fmt(_end).c_str());
}

// Time passed since the start/end timestamp.
inline std::time_t time_window::delta()
{
	return std::abs(_cur - (in_range() ? _start : _end));
}

inline bool time_window::in_range()
{
	return _cur >= _start && _cur < _end;
}



#endif // UTILS_H
