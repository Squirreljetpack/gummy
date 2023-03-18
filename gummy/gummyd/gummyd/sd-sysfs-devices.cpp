// Copyright 2021-2023 Francesco Fusco
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cerrno>
#include <cstring>
#include <array>
#include <vector>
#include <string>
#include <string_view>
#include <filesystem>
#include <fmt/std.h>

#include <gummyd/sd-sysfs.hpp>
#include <gummyd/sd-sysfs-devices.hpp>
#include <gummyd/utils.hpp>
#include <gummyd/constants.hpp>
#include <spdlog/spdlog.h>

namespace gummyd::constants {
namespace {

namespace backlight {
constexpr std::string_view path      = "/sys/class/backlight";
constexpr std::string_view name      = "brightness";
constexpr std::string_view max_name  = "max_brightness";
} // namespace backlight

namespace als {
constexpr std::string_view path      = "/sys/bus/iio/devices";
constexpr std::string_view name      = "iio:device";
constexpr std::string_view lux_scale = "in_illuminance_scale";
constexpr std::array<std::string_view, 2> lux_filenames = {
    "in_illuminance_input",
    "in_illuminance_raw"
};
} // namespace als

} // anonymous namespace
} // namespace gummyd::constants

using namespace gummyd;

std::vector<sysfs::backlight> sysfs::get_backlights() {
    using namespace std::filesystem;
	std::vector<sysfs::backlight> vec;

    if (exists(constants::backlight::path)) {
        for (const auto &s : directory_iterator(constants::backlight::path))
            vec.emplace_back(s.path());
	}

	return vec;
}

std::vector<sysfs::als> sysfs::get_als() {
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

sysfs::backlight::backlight(std::filesystem::path path)
    : _dev(path),
      _val(std::stoi(_dev.get(constants::backlight::name))),
      _max(std::stoi(_dev.get(constants::backlight::max_name))) {
}

void sysfs::backlight::set_step(int step) {
    _val = remap(step, 0, constants::brt_steps_max, 0, _max);
	const int res = _dev.set(constants::backlight::name, std::to_string(_val));

	if (res < 0) {
		spdlog::error("[sysfs] backlight error code {} ({})", errno, std::strerror(errno));
	}
}

int sysfs::backlight::val() const {
	return _val;
}

int sysfs::backlight::step() const {
    return remap(_val, 0, _max, 0, constants::brt_steps_max);
}

int sysfs::backlight::max() const {
	return _max;
}

sysfs::als::als(std::filesystem::path path)
    : _dev(path),
      _lux_scale(1.0) {
    for (std::string_view fname : constants::als::lux_filenames) {
		if (!_dev.get(fname).empty()) {
			_lux_filename = fname;
			break;
		}
	}

	if (_lux_filename.empty()) {
		throw std::runtime_error(fmt::format("Lux data not found for ALS at path: {}", path));
	}

	_lux_scale = [this] {
        const std::string scale = _dev.get(constants::als::lux_scale);
		return scale.empty() ? 1.0 : std::stod(scale);
	}();
}

double sysfs::als::read_lux() const {
	const sysfs::device copy(_dev.path());
	return std::stod(copy.get(_lux_filename)) * _lux_scale;
}
