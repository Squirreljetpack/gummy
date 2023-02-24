// Copyright (c) 2021-2023, Francesco Fusco. All rights reserved.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <string>
#include <libudev.h>
#include <filesystem>
#include <gummyd/udev.hpp>
//#include <systemd/sd-device.h> @TODO: replace libudev with this

namespace gummyd {
namespace sysfs {

udev_context::udev_context() {
    _addr = udev_new();
}

udev_context::~udev_context() {
    udev_unref(_addr);
}

udev* udev_context::get() const {
    return _addr;
}

device::device(const udev_context &udev, std::filesystem::path path)
    : _addr(udev_device_new_from_syspath(udev.get(), path.generic_string().c_str())) {}

device::~device() {
    udev_device_unref(_addr);
};

std::string device::path() const {
    return udev_device_get_syspath(_addr);
}

std::string device::get(std::string_view attr) const {
    const char *s = udev_device_get_sysattr_value(_addr, attr.data());
    return s ? s : "";
}

void device::set(std::string_view attr, std::string_view val) {
    udev_device_set_sysattr_value(_addr, attr.data(), val.data());
}

} // namespace syfs
} // namespace gummyd
