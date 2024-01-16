// Copyright 2021-2024 Francesco Fusco <f.fusco@pm.me>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UTILS_HPP
#define UTILS_HPP

#include <memory>

namespace gummyd {
// scale value in a [0, 1] range
double invlerp(double val, double min, double max);
// interpolate betweeen a and b
double lerp(double a, double b, double t);
// convert from one range to another
double remap(double val, double min, double max, double new_min, double new_max);
// get the fractional part of a floating point number
double mant(double x);

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
}

#endif // UTILS_HPP
