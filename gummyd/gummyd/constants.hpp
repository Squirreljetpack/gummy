// Copyright 2021-2024 Francesco Fusco <f.fusco@pm.me>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <string_view>

namespace gummyd {
namespace constants {
extern const std::string_view flock_filename;
extern const std::string_view fifo_filename;
extern const std::string_view config_filename;
extern const int brt_steps_min;
extern const int brt_steps_max;
extern const int temp_k_min;
extern const int temp_k_max;
}}

#endif // CONSTANTS_HPP
