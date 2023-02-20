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

#ifndef SYSFS_DEVICES_HPP
#define SYSFS_DEVICES_HPP

#include <vector>
#include <gummyd/udev.hpp>

namespace gummy {

class backlight {
    sysfs::udev_context _udev;
	sysfs::device _dev;
	int _val;
	int _max;
public:
	backlight(std::string path);
	int val() const;
	int max() const;
	int step() const;
	void set_step(int);
};

class als {
    sysfs::udev_context _udev;
	sysfs::device _dev;
	std::string   _lux_filename;
	double        _lux_scale;
public:
	als(std::string path);
    double read_lux() const;
};

std::vector<backlight> get_backlights();
std::vector<als>       get_als();
}

#endif // SYSFS_DEVICES_HPP
