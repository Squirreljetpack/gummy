// Copyright 2021-2024 Francesco Fusco <f.fusco@pm.me>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cmath>
#include <gummyd/utils.hpp>
#include <cassert>

namespace gummyd {

double lerp(double a, double b, double t) {
    return (b * t) + (a * (1. - t));
}

double invlerp(double x, double a, double b) {
    assert(b - a > 0.);
    return (x - a) / (b - a);
}

double remap(double x, double a, double b, double ka, double kb) {
    return lerp(ka, kb, invlerp(x, a, b));
}

double mant(double x) {
    return x - std::floor(x);
}

}
