/**
* gummy
* Copyright (C) 2022  Francesco Fusco
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

#ifndef SYSFS_DEVICES_H
#define SYSFS_DEVICES_H

#include <vector>
#include "sysfs.hpp"

namespace Sysfs {

class Backlight
{
    Sysfs::Device _dev;
	int _val;
	int _max;
public:
	Backlight(std::string path);
	int val() const;
	int max() const;
	int step() const;
	void set_step(int);
};

class ALS
{
    Sysfs::Device _dev;
	std::string _lux_name;
	double      _lux_scale;
	int         _lux_step;
public:
	ALS(std::string path);
	int lux_step() const;
	void update();
};

int calc_lux_step(double lux);
std::vector<Backlight> get_backlights();
std::vector<ALS>       get_als();
}

#endif
