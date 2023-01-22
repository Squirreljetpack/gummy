/**
* Device: libudev wrapper
* Copyright (C) 2022  Francesco Fusco
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

namespace Sysfs
{

class Device
{
public:
    Device(udev*, std::string path);
	Device(Device&&);
	~Device();
	std::string get(std::string attr) const;
	void        set(std::string attr, std::string val);
	std::string path() const;
private:
	udev_device *_addr;
};

inline Sysfs::Device::Device(udev *udev, std::string path)
{
	_addr = udev_device_new_from_syspath(udev, path.c_str());
}

inline Sysfs::Device::Device(Device &&d) : _addr(d._addr)
{
}

inline std::string Sysfs::Device::path() const
{
	return udev_device_get_syspath(_addr);
}

inline Sysfs::Device::~Device()
{
	udev_device_unref(_addr);
}

inline std::string Sysfs::Device::get(std::string attr) const
{
	const char *s = udev_device_get_sysattr_value(_addr, attr.c_str());
	return s ? s : "";
}

inline void Sysfs::Device::set(std::string attr, std::string val)
{
	udev_device_set_sysattr_value(_addr, attr.c_str(), val.c_str());
}

};

#endif // SYSFS_H
