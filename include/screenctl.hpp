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

#include <sdbus-c++/IProxy.h>
#include <thread>

#include "xorg.hpp"
#include "channel.hpp"
#include "sysfs_devices.hpp"
#include "utils.hpp"
#include "time.hpp"

std::unique_ptr<sdbus::IProxy> dbus_register_signal_handler(
    const std::string &service,
    const std::string &obj_path,
    const std::string &interface,
    const std::string &signal_name,
    std::function<void(sdbus::Signal &signal)> handler);

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
	Channel _ch;
	Xorg *xorg;

	int  _global_step;
	void adjust(time_window tw, bool catch_up);
	void check_mode_loop();

public:
	Temp_Manager(Xorg*);
	int  global_step();

	void start();
	void notify();
	void stop();
};

struct Monitor
{
    Monitor(Xorg*, Sysfs::Backlight*, Sysfs::ALS*, Channel *als_ch, int id);
	Monitor(Monitor&&);
	Xorg *xorg;
	Sysfs::Backlight *backlight;
	Sysfs::ALS *als;
	Channel ch;
	Channel brt_ch;
	Channel *als_ch;
	int id;
	struct capture_state
	{
	    int ss_brt;
		int cfg_min;
		int cfg_max;
		int cfg_offset;
		int target;
		int diff;
	};
};

void monitor_init(Monitor&);

void monitor_check_brt_mode_loop(Monitor&);
void monitor_capture_loop(Monitor&, Monitor::capture_state);
void monitor_brt_adjust_loop(Monitor&, int cur_step, bool wait);

int  brt_target(int ss_brt, int min, int max, int offset);
int  brt_target_als(int als_brt, int min, int max, int offset);

struct Brightness_Manager
{
	Brightness_Manager(Xorg&);
	void start();
	void stop();
	Channel als_ch;
	std::vector<Sysfs::Backlight> backlights;
	std::vector<Sysfs::ALS>       als;
	std::vector<std::thread>      threads;
	std::vector<Monitor>          monitors;
};

void als_capture_loop(Sysfs::ALS &als, Channel &ch);

}

#endif // SCREENCTL_H
