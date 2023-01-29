/**
* gummy
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

#ifndef SCREENCTL_H
#define SCREENCTL_H

#include <thread>

#include "xorg.hpp"
#include "channel.hpp"
#include "sysfs_devices.hpp"
#include "utils.hpp"
#include "time.hpp"
#include "dbus.hpp"

namespace core {

/**
 * Temperature is adjusted in two steps.
 * The first one is for quickly catching up to the proper temperature when:
 * - time is checked sometime after the start time
 * - the system wakes up
 * - temperature settings change */
class Temp_Manager
{
	std::unique_ptr<sdbus::IProxy> _dbus_proxy;
	enum ch_code {
		EXIT = -1,
		MANUAL,
		AUTO,
		NOTIFIED
	};
	Channel2<std::tuple<int, int>> _ch;
	Channel _sig;
	Xorg *xorg;

	int  _global_step;
	void adjust(bool step, bool daytime, std::time_t time_since_last);

public:
	Temp_Manager(Xorg*);
	int  global_step();

	void start();
	void notify();
	void stop();
};

class Monitor
{
	Xorg *xorg;
	size_t id;

	Sysfs::Backlight *backlight;
	Sysfs::ALS *als;

	Channel sig;
	Channel brt_ch;
	Channel *als_ch;

	void brightness_client(int cur_step, bool wait);

public:
	Monitor(Xorg*, Sysfs::Backlight*, Sysfs::ALS*, Channel *als_ch, int id);
	Monitor(Monitor&&);

	void start();
	void stop();
};

int brt_target(int ss_brt, int min, int max, int offset);
int brt_target_als(int als_brt, int min, int max, int offset);

class Brightness_Manager
{
	Channel als_ch;
	Channel sig;

	std::vector<Sysfs::Backlight> backlights;
	std::vector<Sysfs::ALS>       als;
	std::vector<Monitor>          monitors;
	std::vector<std::thread>      threads;
public:
	Brightness_Manager(Xorg&);
	void start();
	void stop();
};

}

#endif // SCREENCTL_H
