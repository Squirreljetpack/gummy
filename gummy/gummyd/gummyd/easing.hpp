// Copyright (c) 2021-2023, Francesco Fusco. All rights reserved.
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef EASING_HPP
#define EASING_HPP

#include <functional>
#include <chrono>

namespace gummyd {
namespace easing {
double ease(double t);
double ease_out_expo(double t);
double ease_in_out_quad(double t);
int animate(int start, int end, int duration_ms, std::function<double(double t)> easing, std::function<void(int)> fn, std::function<bool()> interrupt);
}
}

#endif // EASING_HPP
