/**
* gummy
* Copyright (C) 2023  Francesco Fusco
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

#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

namespace gummy {
namespace constants {
constexpr const char *flock_filepath  = "/tmp/gummy.lock";
constexpr const char *fifo_filepath   = "/tmp/gummy.fifo";
constexpr const char *config_filename = "gummyconf.json";
constexpr int brt_steps_min  = 200;
constexpr int brt_steps_max  = 1000;
constexpr int temp_k_min     = 1000;
constexpr int temp_k_max     = 6500;
}}

#endif // CONSTANTS_HPP
