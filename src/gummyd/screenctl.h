﻿/**
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

#include "xorg.h"
#include "sysfs.h"
#include "../common/defs.h"
#include "../common/utils.h"
#include <sdbus-c++/IProxy.h>

#include <thread>
#include <condition_variable>

struct Sync
{
	std::condition_variable cv;
	std::mutex mtx;
	bool wake_up;
};

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
struct Temp_Manager
{
	Temp_Manager(Xorg*);
	enum ch_code {
		EXIT,
		WORKING,
		NOTIFIED
	};
	Channel ch;
	std::unique_ptr<sdbus::IProxy> dbus_proxy;
	Xorg *xorg;
	int current_step;
};
void temp_start(Temp_Manager&);
void temp_adjust(Temp_Manager&, Timestamps, bool catch_up);
void temp_adjust_loop(Temp_Manager&);
void temp_animation_loop(Temp_Manager&, Animation, int prev_step, int cur_step, int target_step);

struct Monitor
{
    Monitor(Xorg*, Sysfs::Backlight*, Sysfs::ALS*, Sync *als_ev, int id);
	Monitor(Monitor&&);
	std::condition_variable cv;
	Xorg                    *xorg;
	Sysfs::Backlight        *backlight;
	Sysfs::ALS              *als;
	Sync                    *als_ev;
	int id;
	int ss_brt;
	struct {
		bool paused;
		bool stopped;
		bool cfg_updated;
	} flags;
};

struct Previous_capture_state
{
	int ss_brt;
	int cfg_min;
	int cfg_max;
	int cfg_offset;
};

void monitor_init(Monitor&);
void monitor_pause(Monitor&);
void monitor_resume(Monitor&);
void monitor_stop(Monitor&);
void monitor_toggle(Monitor&, bool);

void monitor_is_auto_loop(Monitor&, Sync &brt_ev);
void monitor_capture_loop(Monitor&, Sync &brt_ev, Previous_capture_state, int ss_delta);
void monitor_brt_adjust_loop(Monitor&, Sync &brt_sync, int cur_step);
int  monitor_brt_animation_loop(Monitor&, Animation, int prev_step, int cur_step, int target_step, int ss_brt);

int  calc_brt_target(int ss_brt, int min, int max, int offset);
int  calc_brt_target_als(int als_brt, int min, int max, int offset);

struct Brightness_Manager
{
	Brightness_Manager(Xorg&);
	void start();
	void stop();
	std::vector<Sysfs::Backlight> backlights;
	std::vector<Sysfs::ALS>       als;
	std::vector<std::thread>      threads;
	std::vector<Monitor>          monitors;
	Sync als_ev;
	Sync als_stop;
};

void als_capture_loop(Sysfs::ALS&, Sync&, Sync&);
void als_capture_stop(Sync&);
void als_notify(Sync&);
int  als_await(Sysfs::ALS&, Sync&);

void refresh_gamma(Xorg&, Channel&);
}

#endif // SCREENCTL_H
