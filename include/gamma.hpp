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

#ifndef GAMMA_H
#define GAMMA_H

#include <tuple>
#include "xorg.hpp"
#include "channel.hpp"

namespace core {
void set_gamma(Xorg *xorg, int brt_step, int temp_step, int screen_index);
void refresh_gamma(Xorg*, Channel<>&);
}

#endif
