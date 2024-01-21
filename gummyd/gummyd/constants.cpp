// Copyright 2021-2024 Francesco Fusco <f.fusco@pm.me>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gummyd/constants.hpp>
#include <string_view>

namespace gummyd {
namespace constants {
constexpr std::string_view flock_filename  = "gummyd-lock";
constexpr std::string_view fifo_filename   = "gummyd-fifo";
constexpr std::string_view config_filename = "gummyconf.json";
constexpr int brt_steps_min  = 200;
constexpr int brt_steps_max  = 1000;
constexpr int temp_k_min     = 1000;
constexpr int temp_k_max     = 6500;
}}
