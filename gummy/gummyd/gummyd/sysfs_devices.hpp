// Copyright (c) 2021-2023, Francesco Fusco. All rights reserved.
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SYSFS_DEVICES_HPP
#define SYSFS_DEVICES_HPP

#include <vector>
#include <filesystem>
#include <gummyd/udev.hpp>

namespace gummyd {

class backlight {
	sysfs::device _dev;
	int _val;
	int _max;
public:
    backlight(std::filesystem::path path);
	int val() const;
	int max() const;
	int step() const;
	void set_step(int);
};

class als {
    sysfs::device _dev;
	std::string   _lux_filename;
	double        _lux_scale;
public:
    als(std::filesystem::path path);
    double read_lux() const;
};

std::vector<backlight> get_backlights();
std::vector<als>       get_als();
}

#endif // SYSFS_DEVICES_HPP
