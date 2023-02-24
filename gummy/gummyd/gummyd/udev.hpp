// Copyright (c) 2021-2023, Francesco Fusco. All rights reserved.
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SYSFS_H
#define SYSFS_H

#include <string>
#include <string_view>
#include <filesystem>

#include <libudev.h>
//#include <systemd/sd-device.h>

namespace gummyd {
namespace sysfs {

class udev_context {
    udev *_addr;
public:
    udev_context();
    ~udev_context();
    udev* get() const;
};

class device {
    udev_device *_addr;
public:
    device(const udev_context &udev, std::filesystem::path path);
    ~device();
    std::string path() const;
    std::string get(std::string_view attr) const;
    void set(std::string_view attr, std::string_view val);
};

}
}

#endif // UDEV_HPP
