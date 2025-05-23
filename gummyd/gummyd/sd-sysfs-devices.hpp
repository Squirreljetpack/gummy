// Copyright 2021-2024 Francesco Fusco <f.fusco@pm.me>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SD_SYSFS_DEVICES_HPP
#define SD_SYSFS_DEVICES_HPP

#include <vector>
#include <filesystem>
#include <gummyd/sd-sysfs.hpp>

namespace gummyd {
namespace sysfs {

class backlight {
	sysfs::device _dev;
	int _val;
	int _max;
public:
    backlight(std::filesystem::path path);
	int val() const;
	int max() const;
    double perc() const;
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
} // namespace sysfs
} // namespace gummyd

#endif // SD_SYSFS_DEVICES_HPP
