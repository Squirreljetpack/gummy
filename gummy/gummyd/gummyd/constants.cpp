// Copyright (c) 2021-2023, Francesco Fusco. All rights reserved.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gummyd/constants.hpp>

namespace gummyd {
namespace constants {
constexpr const char *flock_filepath  = "/tmp/gummy.lock";
constexpr const char *fifo_filepath   = "/tmp/gummy.fifo";
constexpr const char *config_filename = "gummyconf.json";
constexpr int brt_steps_min  = 200;
constexpr int brt_steps_max  = 1000;
constexpr int temp_k_min     = 1000;
constexpr int temp_k_max     = 6500;
}}
