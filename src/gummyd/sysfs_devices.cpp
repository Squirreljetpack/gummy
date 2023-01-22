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

#include <filesystem>
#include <algorithm>
#include <cmath>
#include <syslog.h>
#include <libudev.h>

#include "sysfs_devices.hpp"
#include "cfg.hpp"
#include "utils.hpp"

const struct {
    struct {
	    std::string path    = "/sys/class/backlight";
		std::string brt     = "brightness";
		std::string max_brt = "max_brightness";
	} backlight;
	struct {
	    std::string path      = "/sys/bus/iio/devices";
		std::string name      = "iio:device";
		std::string lux_scale = "in_illuminance_scale";
		std::array<std::string, 2> lux_files = {
		    "in_illuminance_input",
		    "in_illuminance_raw"
		};
	} als;
} config;

std::vector<Sysfs::Backlight> Sysfs::get_bl()
{
	using namespace std::filesystem;
	std::vector<Sysfs::Backlight> vec;

	for (const auto &s : directory_iterator(config.backlight.path))
		vec.emplace_back(udev_new(), s.path());

	return vec;
}

std::vector<Sysfs::ALS> Sysfs::get_als()
{
	using namespace std::filesystem;
	std::vector<Sysfs::ALS> vec;

	for (const auto &s : directory_iterator(config.als.path)) {
		const auto str = s.path().stem().string();
		if (str.find(config.als.name) != std::string::npos)
			vec.emplace_back(udev_new(), s.path());
	}

	return vec;
}

Sysfs::Backlight::Backlight(udev *addr, std::string path)
    : _dev(addr, path),
      _max_brt(std::stoi(_dev.get(config.backlight.max_brt))),
      _cur(std::stoi(_dev.get(config.backlight.brt)))
{
}

void Sysfs::Backlight::set(int brt)
{
	_cur = std::clamp(brt, 0, _max_brt);
	_dev.set(config.backlight.brt, std::to_string(_cur).c_str());
}

int Sysfs::Backlight::step() const
{
	return remap(_cur, 0, _max_brt, brt_steps_min, brt_steps_max);
}

int Sysfs::Backlight::max_brt() const 
{
	return _max_brt;
}

Sysfs::ALS::ALS(udev *addr, std::string path)
    : _dev(addr, path),
	  _lux_scale(1.0)
{
	for (const auto &name : config.als.lux_files) {
		if (!_dev.get(name).empty()) {
			_lux_name = name;
			break;
		}
	}

	if (_lux_name.empty())
		syslog(LOG_ERR, "ALS output file not found");

	const std::string scale = _dev.get(config.als.lux_scale);

	if (!scale.empty())
		_lux_scale = std::stod(scale);
}

void Sysfs::ALS::update()
{
	Sysfs::Device dev(udev_new(), _dev.path());

	const double lux = std::stod(dev.get(_lux_name)) * _lux_scale;

	_lux_step = calc_lux_step(lux);
}

int Sysfs::ALS::lux_step() const
{
	return _lux_step;
}

// The human eye's perception of light intensity is roughly logarithmic.
// The highest illuminance detected by my laptop sensor is 21090.
// That's roughly 4.3 in log10. Assume the maximum is 5 as an approximation.
int Sysfs::calc_lux_step(double lux)
{
	if (lux == 0.)
		return 0;
	return log10(lux) / 5 * brt_steps_max;
}
