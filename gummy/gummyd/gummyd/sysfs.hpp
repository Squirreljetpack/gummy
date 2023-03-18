// Copyright 2021-2023 Francesco Fusco
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SYSFS_H
#define SYSFS_H

#include <string>
#include <string_view>
#include <filesystem>
#include <systemd/sd-device.h>

namespace gummyd {
namespace sysfs {

class device {
    sd_device *addr_;
public:
	device(std::filesystem::path path);
    ~device();
    std::string path() const;
    std::string get(std::string_view attr) const;
	int set(std::string_view attr, std::string_view val);
};

}
}

#endif // SYSFS_HPP
