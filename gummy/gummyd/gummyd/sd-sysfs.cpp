// Copyright 2021-2023 Francesco Fusco
// SPDX-License-Identifier: GPL-3.0-or-later

#include <string>
#include <string_view>
#include <stdexcept>
#include <filesystem>
#include <systemd/sd-device.h>
#include <gummyd/sd-sysfs.hpp>

namespace gummyd {
namespace sysfs {

device::device(std::filesystem::path path) {
	const int ret = sd_device_new_from_syspath(&addr_, path.generic_string().c_str());
	if (ret < 0) {
		throw std::runtime_error("sd_device_new_from_syspath " + std::to_string(ret));
	}
}

device::~device() {
	sd_device_unref(addr_);
};

std::string device::path() const {
	const char *path = nullptr;
	const int ret = sd_device_get_syspath(addr_, &path);
	if (ret < 0) {
		throw std::runtime_error("sd_device_get_syspath " + std::to_string(ret));
	}
	return path;
}

std::string device::get(std::string_view attr) const {
	const char *val = nullptr;
	sd_device_get_sysattr_value(addr_, attr.data(), &val);
	return val ? val : "";
}

int device::set(std::string_view attr, std::string_view val) {
	return sd_device_set_sysattr_value(addr_, attr.data(), val.data());
}

} // namespace sysfs
} // namespace gummyd
