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

#include <filesystem>
#include <libudev.h>

#include <gummyd/utils.hpp>
#include <gummyd/sysfs_devices.hpp>
#include <gummyd/config.hpp>

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
		std::array<std::string, 2> lux_filenames = {
		    "in_illuminance_input",
		    "in_illuminance_raw"
		};
	} als;
} config;

std::vector<sysfs::backlight> sysfs::get_backlights()
{
	using namespace std::filesystem;
	std::vector<sysfs::backlight> vec;

	if (exists(config.backlight.path)) {
		for (const auto &s : directory_iterator(config.backlight.path))
			vec.emplace_back(s.path());
	}

	return vec;
}

std::vector<sysfs::als> sysfs::get_als()
{
	using namespace std::filesystem;
	std::vector<sysfs::als> vec;

	if (exists(config.als.path)) {
		for (const auto &s : directory_iterator(config.als.path)) {
			const auto str = s.path().stem().string();
			if (str.find(config.als.name) != std::string::npos)
				vec.emplace_back(s.path());
		}
	}

	return vec;
}

sysfs::backlight::backlight(std::string path)
    : _dev(_udev, path),
      _val(std::stoi(_dev.get(config.backlight.brt))),
      _max(std::stoi(_dev.get(config.backlight.max_brt)))
{
}

void sysfs::backlight::set_step(int step)
{
	_val = remap(step, 0, constants::brt_steps_max, 0, _max);

	_dev.set(config.backlight.brt, std::to_string(_val).c_str());
}

int sysfs::backlight::val() const
{
	return _val;
}

int sysfs::backlight::step() const
{
	return remap(_val, 0, _max, 0, constants::brt_steps_max);
}

int sysfs::backlight::max() const
{
	return _max;
}

sysfs::als::als(std::string path)
    : _dev(_udev, path),
	  _lux_scale(1.0)
{
	for (const auto &fname : config.als.lux_filenames) {
		if (!_dev.get(fname).empty()) {
			_lux_filename = fname;
			break;
		}
	}

	if (_lux_filename.empty()) {
		throw std::runtime_error("Lux data not found for ALS at path: " + path);
	}

	_lux_scale = [this] {
		const std::string scale = _dev.get(config.als.lux_scale);
		return scale.empty() ? 1.0 : std::stod(scale);
	}();
}

double sysfs::als::read_lux()
{
	sysfs::device dev(_udev, _dev.path());
	return std::stod(dev.get(_lux_filename)) * _lux_scale;
}
