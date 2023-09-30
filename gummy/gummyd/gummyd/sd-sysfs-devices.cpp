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
#include <gummyd/file.hpp>

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

    if (!exists(constants::als::path)) {
        return {};
    }

	std::vector<als> vec;

    for (const auto &dir : directory_iterator(constants::als::path)) {

        const std::string filename = dir.path().filename();

        if (filename.find(constants::als::name) == std::string::npos) {
            continue;
        }

        try {
            const std::string device_name = gummyd::file_read(dir.path() / "name");
            if (device_name == "acpi-als" || device_name == "als") {
                vec.emplace_back(dir.path());
            }
        } catch (const std::exception &e) {
            spdlog::error(e.what());
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
        throw std::runtime_error(fmt::format("Lux data not found for ALS device at path: {}", path));
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
