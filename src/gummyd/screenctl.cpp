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

#include <syslog.h>

#include "screenctl.hpp"
#include "gamma.hpp"
#include "cfg.hpp"
#include "utils.hpp"
#include "easing.hpp"
#include "server.hpp"

core::Temp_Manager::Temp_Manager(Xorg *xorg)
    : xorg(xorg),
      _global_step(temp_steps_max)
{}

void core::Temp_Manager::start()
{
	const auto notify_on_sys_resume = [&] (sdbus::Signal &sig) {
		bool suspended;
		sig >> suspended;
		if (!suspended)
			_ch.send(NOTIFIED);
	};

	_dbus_proxy = dbus_register_signal_handler(
	    "org.freedesktop.login1",
	    "/org/freedesktop/login1",
	    "org.freedesktop.login1.Manager",
	    "PrepareForSleep",
	    notify_on_sys_resume);

	_ch.send(MANUAL);
	check_mode_loop();
}

void core::Temp_Manager::notify()
{
	_ch.send(NOTIFIED);
}

void core::Temp_Manager::stop()
{
	_ch.send(EXIT);
}

int core::Temp_Manager::global_step()
{
	return _global_step;
}

void core::Temp_Manager::check_mode_loop()
{
	const int mode = _ch.data();
	printf("temp mode: %d\n", mode);

	if (mode == EXIT)
		return;

	// todo: replace with channel data
	if (cfg.temp_auto) {

		//printf("temp adjusting\n");
		_ch.send(AUTO);

		adjust(time_window(std::time(nullptr),
		cfg.temp_auto_sunrise, cfg.temp_auto_sunset, -(cfg.temp_auto_speed * 60)), false);

		//printf("temp finished\n");
	}

	// if notified, retry without waiting
	if (mode == NOTIFIED) {
		_ch.send(MANUAL);
		//printf("temp notified\n");
		return check_mode_loop();
	}

	//printf("temp waiting\n");
	if (_ch.recv_timeout(60000) == EXIT)
		return;

	check_mode_loop();
}

void core::Temp_Manager::adjust(time_window tw, bool step)
{
	const bool daytime = tw.in_range();
	const std::time_t max_speed_s = cfg.temp_auto_speed * 60;
	const std::time_t delta_s = std::clamp(std::abs(tw.time_since_last()), 0l, max_speed_s);

	const int target_temp = [step, daytime, delta_s, max_speed_s] {
		if (!step) {
			if (daytime) {
				return int(remap(delta_s, 0, max_speed_s, cfg.temp_auto_low, cfg.temp_auto_high));
			} else {
				return int(remap(delta_s, 0, max_speed_s, cfg.temp_auto_high, cfg.temp_auto_low));
			}
		} else {
			return daytime ? cfg.temp_auto_high : cfg.temp_auto_low;
		}
	}();

	const int duration_ms = [step, max_speed_s, delta_s] {
		const int min = 2000;
		int ret = (!step) ? min : (max_speed_s - delta_s) * 1000;
		if (ret < min)
			ret = min;
		return ret;
	}();

	//printf("daytime: %d\n", daytime);
	//printf("target_temp: %d\n", target_temp);
	//printf("duration_ms: %d\n", duration_ms);

	const int target_step = int(remap(target_temp, temp_k_min, temp_k_max, temp_steps_min, temp_steps_max));

	const auto fn = [&] (int step) {
		_global_step = step;
		for (size_t i = 0; i < xorg->scr_count(); ++i) {
			cfg.screens[i].temp_step = step;
			core::set_gamma(xorg, cfg.screens[i].brt_step, cfg.screens[i].temp_step, i);
		}
	};

	const auto interrupt = [&] { return _ch.data() != AUTO; };

	easing::animate(easing::ease_in_out_quad, _global_step, target_step, duration_ms, 0, _global_step, _global_step, fn, interrupt);

	if (!step)
		adjust(tw, !step);
}

core::Brightness_Manager::Brightness_Manager(Xorg &xorg)
     : backlights(Sysfs::get_bl()),
       als(Sysfs::get_als())
{
	monitors.reserve(xorg.scr_count());
	threads.reserve(xorg.scr_count());

	for (size_t i = 0; i < xorg.scr_count(); ++i) {
		monitors.emplace_back(&xorg,
		                      i < backlights.size() ? &backlights[i] : nullptr,
		                      als.size() > 0 ? &als[0] : nullptr,
		                      &als_ch,
		                      i);
	}

	assert(monitors.size() == xorg.scr_count());
}

void core::Brightness_Manager::start()
{
	if (als.size() > 0) {
		threads.emplace_back([&] {
			als_server(als[0], als_ch, sig, cfg.als_polling_rate, 0, 0);
		});
	}

	for (auto &m : monitors)
		threads.emplace_back([&m] { m.start(); });
}

void core::Brightness_Manager::stop()
{
	sig.send(-1);

	for (auto &m : monitors)
		m.stop();

	for (auto &t : threads)
		t.join();
}

core::Monitor::Monitor(Xorg *xorg,
		Sysfs::Backlight *bl,
        Sysfs::ALS *als,
        Channel *als_ch,
		int id)
   :  xorg(xorg),
      id(id),
      backlight(bl),
      als(als),
      als_ch(als_ch)
{
	sig.send(cfg.screens[id].brt_mode);

	core::set_gamma(xorg, brt_steps_max, cfg.screens[id].temp_step, id);
}

core::Monitor::Monitor(Monitor &&o)
    :  xorg(o.xorg),
       id(o.id),
       backlight(o.backlight)
{
}

void core::Monitor::start()
{
	using mode = Config::Screen::Brt_mode;
	while (true) {
		const int brt_mode = sig.data();
		printf("brt_mode: %d\n", brt_mode);

		if (brt_mode < 0) {
			return;
		}

		if (brt_mode == mode::MANUAL) {
			sig.recv();
			return start();
		}

		if (brt_mode == mode::SCREENSHOT) {

			std::thread thr([&] {
				brightness_client(brt_steps_max, true);
			});

			brightness_server(*xorg, id, brt_ch, sig, cfg.screens[id].brt_auto_polling_rate, 0, 0);

			thr.join();
		}

		// there is one ALS server (started above) sending the same value to N clients
		/*if (brt_mode == mode::ALS) {

			std::thread thr([&] {
				brightness_client(brt_steps_max, true);
			});


			thr.join();
		}*/
	}
}

void core::Monitor::stop()
{
	sig.send(-1);
}

void core::Monitor::brightness_client(int cur_step, bool wait)
{
	while (true) {

		printf("monitor_brt_adjust_loop: fetching (wait: %d)...\n", wait);

		const int brightness = wait ? brt_ch.recv() : brt_ch.data();

		printf("monitor_brt_adjust_loop: received %d.\n", brightness);

		if (brightness < 0) {
			return;
		}

		const int target_step = brt_target(brightness, cfg.screens[id].brt_auto_min, cfg.screens[id].brt_auto_max, cfg.screens[id].brt_auto_offset);

		if (cur_step == target_step) {
			wait = true;
			continue;
		}

		const auto fn = [&] (int new_step) {
			cur_step = new_step;
			cfg.screens[id].brt_step = cur_step;
			core::set_gamma(xorg, cfg.screens[id].brt_step, cfg.screens[id].temp_step, id);
		};

		const auto interrupt = [&] {
			if (brightness != brt_ch.data()) {
				wait = false;
				return true;
			}
			return false;
		};

		printf("Easing from %d to %d...\n", cur_step, target_step);
		easing::animate(easing::ease_out_expo, cur_step, target_step, cfg.screens[id].brt_auto_speed, 0, cur_step, cur_step, fn, interrupt);
	}
}

int core::brt_target(int ss_brt, int min, int max, int offset)
{
	const int offset_step = offset * brt_steps_max / max;
	return std::clamp(brt_steps_max - ss_brt + offset_step, min, max);
}

int core::brt_target_als(int als_brt, int min, int max, int offset)
{
	const int offset_step = offset * brt_steps_max / max;
	return std::clamp(als_brt + offset_step, min, max);
}
