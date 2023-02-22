/**
* Device: libudev wrapper
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

#ifndef SYSFS_H
#define SYSFS_H

#include <string>
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
    device(const udev_context &udev, std::string path);
    ~device();
    std::string path() const;
    std::string get(std::string attr) const;
    void set(std::string attr, std::string val);
};

}
}

#endif // UDEV_HPP
