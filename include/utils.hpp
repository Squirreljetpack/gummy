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

#ifndef UTILS_HPP
#define UTILS_HPP

#include <cmath>
#include <chrono>
#include <thread>
#include <condition_variable>

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

inline void jthread_wait_until(int ms, std::stop_token stoken)
{
	using namespace std::chrono;
	std::mutex mutex;
	std::unique_lock lock(mutex);
	std::condition_variable_any()
	        .wait_until(lock, stoken, system_clock::now() + milliseconds(ms), [&] { return stoken.stop_requested(); });
}

template <class T, auto fn>
struct deleter {
	void operator()(T *ptr) { fn(ptr); }
};

template <class T>
struct c_deleter {
	void operator()(T *ptr) { std::free(ptr); }
};

template <class T>
using c_unique_ptr = std::unique_ptr<T, c_deleter<T>>;
//using c_unique_ptr = std::unique_ptr<T, deleter<T, std::free>>;

#endif // UTILS_H
