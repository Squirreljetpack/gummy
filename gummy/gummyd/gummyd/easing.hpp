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
