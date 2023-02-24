// Copyright (c) 2021-2023, Francesco Fusco. All rights reserved.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <vector>
#include <string>
#include <string_view>
#include <filesystem>
#include <fmt/std.h>

#include <gummyd/sysfs_devices.hpp>
#include <gummyd/udev.hpp>
#include <gummyd/utils.hpp>
#include <gummyd/constants.hpp>

namespace gummyd::constants {

namespace backlight {
constexpr std::string_view path      = "/sys/class/backlight";
constexpr std::string_view name      = "brightness";
constexpr std::string_view max_name  = "max_brightness";
}

namespace als {
constexpr std::string_view path      = "/sys/bus/iio/devices";
constexpr std::string_view name      = "iio:device";
constexpr std::string_view lux_scale = "in_illuminance_scale";
constexpr std::array<std::string_view, 2> lux_filenames = {
    "in_illuminance_input",
    "in_illuminance_raw"
};
}
}

using namespace gummyd;

std::vector<backlight> gummyd::get_backlights() {
	using namespace std::filesystem;
    std::vector<backlight> vec;

    if (exists(constants::backlight::path)) {
        for (const auto &s : directory_iterator(constants::backlight::path))
            vec.emplace_back(s.path());
	}

	return vec;
}

std::vector<als> gummyd::get_als() {
	using namespace std::filesystem;
    std::vector<als> vec;
    if (exists(constants::als::path)) {
        for (const auto &s : directory_iterator(constants::als::path)) {
            const std::string stem = s.path().stem();
            if (stem.find(constants::als::name) != std::string::npos)
				vec.emplace_back(s.path());
		}
	}

	return vec;
}

backlight::backlight(std::filesystem::path path)
    : _dev(_udev, path),
      _val(std::stoi(_dev.get(constants::backlight::name))),
      _max(std::stoi(_dev.get(constants::backlight::max_name))) {
}

void backlight::set_step(int step) {
	_val = remap(step, 0, constants::brt_steps_max, 0, _max);
    _dev.set(constants::backlight::name, std::to_string(_val));
}

int backlight::val() const {
	return _val;
}

int backlight::step() const {
	return remap(_val, 0, _max, 0, constants::brt_steps_max);
}

int backlight::max() const {
	return _max;
}

als::als(std::filesystem::path path)
    : _dev(_udev, path),
      _lux_scale(1.0) {
    for (std::string_view fname : constants::als::lux_filenames) {
		if (!_dev.get(fname).empty()) {
			_lux_filename = fname;
			break;
		}
	}

	if (_lux_filename.empty()) {
        throw std::runtime_error(fmt::format("Lux data not found for ALS at path: {} ", path));
	}

	_lux_scale = [this] {
        const std::string scale = _dev.get(constants::als::lux_scale);
		return scale.empty() ? 1.0 : std::stod(scale);
	}();
}

double als::read_lux() const {
    const sysfs::device dev(_udev, _dev.path());
	return std::stod(dev.get(_lux_filename)) * _lux_scale;
}
