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

namespace gummyd {
namespace constants {
extern const char * const flock_filepath;
extern const char * const fifo_filepath;
extern const char * const config_filename;
extern const int brt_steps_min;
extern const int brt_steps_max;
extern const int temp_k_min;
extern const int temp_k_max;
}}

#endif // CONSTANTS_HPP
