// Copyright 2021-2024 Francesco Fusco <f.fusco@pm.me>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cmath>
#include <gummyd/utils.hpp>

namespace gummyd {

double invlerp(double val, double min, double max) {
    return (val - min) / (max - min);
}

double lerp(double a, double b, double t) {
    return ((1 - t) * a) + (t * b);
}

double remap(double val, double min, double max, double new_min, double new_max) {
    return lerp(new_min, new_max, invlerp(val, min, max));
}

double mant(double x) {
    return x - std::floor(x);
}

}
