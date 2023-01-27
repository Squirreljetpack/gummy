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
#include <cmath>
#include <fcntl.h>

// scale value in a [0, 1] range
inline double invlerp(double val, double min, double max)
{
	return (val - min) / (max - min);
}

// interpolate betweeen a and b. t = [0, 1]
inline double lerp(double a, double b, double t)
{
	return ((1 - t) * a) + (t * b);
}

inline double remap(double val, double min, double max, double new_min, double new_max)
{
	return lerp(new_min, new_max, invlerp(val, min, max));
}

// Get the fractional part of a floating point number.
inline double mant(double x)
{
	return x - std::floor(x);
}

/* Interpolate val to an array index.
   The integral part of the return value is the index itself,
   while the fractional part is the interpolation factor
   between the index and the next one. */
inline double remap_to_idx(int val, int min, int max, size_t arr_sz)
{
	return remap(val, min, max, 0, arr_sz - 1);
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

#endif // UTILS_H
