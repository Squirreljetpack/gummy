// Copyright (c) 2021-2023, Francesco Fusco. All rights reserved.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <string>
#include <libudev.h>
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

device::device(const udev_context &udev, std::string path)
    : _addr(udev_device_new_from_syspath(udev.get(), path.c_str())) {}

device::~device() {
    udev_device_unref(_addr);
};

std::string device::path() const {
    return udev_device_get_syspath(_addr);
}

std::string device::get(std::string attr) const {
    const char *s = udev_device_get_sysattr_value(_addr, attr.c_str());
    return s ? s : "";
}

void device::set(std::string attr, std::string val) {
    udev_device_set_sysattr_value(_addr, attr.c_str(), val.c_str());
}

} // namespace syfs
} // namespace gummyd
