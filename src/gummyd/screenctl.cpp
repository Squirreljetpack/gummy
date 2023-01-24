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
#include "xorg.hpp"
#include "cfg.hpp"
#include "utils.hpp"

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
		printf("temp adjusting\n");
		_ch.send(AUTO);
		adjust(timestamps_update(
		    cfg.temp_auto_sunrise,
		    cfg.temp_auto_sunset,
		    -(cfg.temp_auto_speed * 60)), false);
	}

	// if notified, retry without waiting
	if (mode == NOTIFIED) {
		printf("temp notified\n");
		return check_mode_loop();
	}

	printf("temp waiting\n");
	if (_ch.recv_timeout(60000) == EXIT)
		return;

	check_mode_loop();
}

void core::Temp_Manager::adjust(Timestamps ts, bool step)
{
	//printf("temp_adjust: step %d\n", step);
	const bool   daytime = ts.cur >= ts.start && ts.cur < ts.end;
	const double adaptation_time_s = double(cfg.temp_auto_speed) * 60;

	int          target_temp;
	std::time_t  time_to_subtract;
	double       animation_s = 2;

	if (daytime) {
		target_temp = cfg.temp_auto_high;
		time_to_subtract = ts.start;
	} else {
		target_temp = cfg.temp_auto_low;
		time_to_subtract = ts.end;
	}

	int time_since_start_s = std::abs(ts.cur - time_to_subtract);

	if (time_since_start_s > adaptation_time_s)
		time_since_start_s = adaptation_time_s;

	if (!step) {
		if (daytime) {
			target_temp = remap(time_since_start_s, 0, adaptation_time_s, cfg.temp_auto_low, cfg.temp_auto_high);
		} else {
			target_temp = remap(time_since_start_s, 0, adaptation_time_s, cfg.temp_auto_high, cfg.temp_auto_low);
		}
	} else {
		animation_s = adaptation_time_s - time_since_start_s;
		if (animation_s < 2)
			animation_s = 2;
	}

	const int target_step = int(remap(target_temp, temp_k_min, temp_k_max, temp_steps_min, temp_steps_max));

	Animation a = animation_init(
	    _global_step,
	    target_step,
	    cfg.temp_auto_fps,
	    animation_s * 1000);

	_global_step = ease_in_out_quad_loop(a, -1, _global_step, target_step, [&] (int cur, int prev) {

		if (cur != prev) {
			for (size_t i = 0; i < xorg->scr_count(); ++i) {
				cfg.screens[i].temp_step = cur;
				xorg->set_gamma(i, cfg.screens[i].brt_step, cur);
			}
		}

		return _ch.data() == AUTO;
	});

	//printf("temp_adjust: step %d done\n", step);
	if (!step)
		adjust(ts, !step);
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
		als_ch.send(1);
		threads.emplace_back([&] {
			core::als_capture_loop(als[0], als_ch);
		});
	}

	for (auto &m : monitors)
		threads.emplace_back([&] { core::monitor_init(m); });
}

void core::Brightness_Manager::stop()
{
	als_ch.send(0);

	for (auto &m : monitors)
		m.ch.send(-1);

	for (auto &t : threads)
		t.join();
}

void core::als_capture_loop(Sysfs::ALS &als, Channel &ch)
{
	const int prev_step = als.lux_step();
	als.update();

	if (prev_step != als.lux_step() || ch.data() == 2) {
		//printf("als_capture_loop: sending signal\n");
		ch.send(1);
	}

	if (ch.recv_timeout(cfg.als_polling_rate) == 0) {
		//printf("als_capture_loop: exit\n");
		return;
	}

	core::als_capture_loop(als, ch);
}

core::Monitor::Monitor(Xorg *xorg,
		Sysfs::Backlight *bl,
        Sysfs::ALS *als,
        Channel *als_ch,
		int id)
   :  xorg(xorg),
      backlight(bl),
      als(als),
      als_ch(als_ch),
      id(id)
{
	ch.send(cfg.screens[id].brt_mode);

	if (!backlight) {
		xorg->set_gamma(id,
		                brt_steps_max,
		                cfg.screens[id].temp_step);
	}
}

core::Monitor::Monitor(Monitor &&o)
    :  xorg(o.xorg),
       backlight(o.backlight),
       id(o.id)
{
}

void core::monitor_init(Monitor &mon)
{
	core::monitor_check_brt_mode_loop(mon);
}

void core::monitor_check_brt_mode_loop(Monitor &mon)
{
	using mode = Config::Screen::Brt_mode;

	const int brt_mode = mon.ch.data();
	printf("brt_mode: %d\n", brt_mode);

	if (brt_mode < 0) {
		return;
	}

	if (brt_mode == mode::MANUAL) {
		mon.ch.recv();
		return core::monitor_check_brt_mode_loop(mon);
	}

	if (brt_mode != mode::MANUAL) {
		std::thread thr([&] {
			core::monitor_brt_adjust_loop(mon, brt_steps_max, true);
		});

		core::monitor_capture_loop(mon, Monitor::capture_state{0, 0, 0, 0, 0, 0});

		mon.brt_ch.send(-1);
		thr.join();
	}

	core::monitor_check_brt_mode_loop(mon);
}

void core::monitor_capture_loop(Monitor &mon, Monitor::capture_state state)
{
	using mode = Config::Screen::Brt_mode;
	const auto &scr = cfg.screens[mon.id];

	const int ss_brt = [&] {

		if (scr.brt_mode == mode::ALS) {
			mon.als_ch->recv();
			return mon.als->lux_step();
		}

		return mon.xorg->screen_brightness(mon.id);
	}();

	if (mon.ch.data() <= mode::MANUAL)
		return;

	state.diff += abs(state.ss_brt - ss_brt);

	if (state.diff > scr.brt_auto_threshold || scr.brt_mode == mode::ALS) {
		state.diff = 0;

		const int target_step = [&] {
			if (mon.als && scr.brt_mode == mode::ALS)
				return brt_target_als(ss_brt, scr.brt_auto_min, scr.brt_auto_max, scr.brt_auto_offset);
			else
				return brt_target(ss_brt, scr.brt_auto_min, scr.brt_auto_max, scr.brt_auto_offset);
		}();

		printf("monitor_capture_loop: sending %d...\n", target_step);
		mon.brt_ch.send(target_step);
	}

	if (scr.brt_auto_min != state.cfg_min
	    || scr.brt_auto_max != state.cfg_max
	    || scr.brt_auto_offset != state.cfg_offset) {
		state.diff = 255;
	}

	state.ss_brt     = ss_brt;
	state.cfg_min    = scr.brt_auto_min;
	state.cfg_max    = scr.brt_auto_max;
	state.cfg_offset = scr.brt_auto_offset;

	if (scr.brt_mode == mode::SCREENSHOT) {
		if (mon.backlight) {
			mon.als_ch->recv_timeout(scr.brt_auto_polling_rate);
		} else {
			std::this_thread::sleep_for(std::chrono::milliseconds(scr.brt_auto_polling_rate));
		}
	}

	core::monitor_capture_loop(mon, state);
}

void core::monitor_brt_adjust_loop(Monitor &mon, int cur_step, bool wait)
{
	printf("monitor_brt_adjust_loop: fetching (wait: %d)...\n", wait);

	const int target_step = wait ? mon.brt_ch.recv() : mon.brt_ch.data();

	printf("monitor_brt_adjust_loop: received %d.\n", target_step);

	if (target_step < 0)
		return;

	if (cur_step == target_step)
		return core::monitor_brt_adjust_loop(mon, cur_step, true);

	Animation a = animation_init(
	    cur_step,
	    target_step,
	    cfg.temp_auto_fps,
	    cfg.screens[mon.id].brt_auto_speed);

	ease_out_expo_loop(a, -1, cur_step, target_step,
	[&] (int cur, int prev) {
		if (cur != prev) {
			for (size_t i = 0; i < mon.xorg->scr_count(); ++i) {
				cur_step = cur;
				if (mon.backlight) {
					mon.backlight->set(cur_step * mon.backlight->max_brt() / brt_steps_max);
				} else {
					cfg.screens[i].brt_step = cur;
					mon.xorg->set_gamma(i, cur, cfg.screens[i].temp_step);
				}
			}
		}

		if (target_step == mon.brt_ch.data()) {
			return true;
		} else {
			wait = false;
			return false;
		}
	});

	core::monitor_brt_adjust_loop(mon, cur_step, wait);
}

int core::brt_target(int ss_brt, int min, int max, int offset)
{
	const int ss_step     = ss_brt * brt_steps_max / 255;
	const int offset_step = offset * brt_steps_max / max;
	return std::clamp(brt_steps_max - ss_step + offset_step, min, max);
}

int core::brt_target_als(int als_brt, int min, int max, int offset)
{
	const int offset_step = offset * brt_steps_max / max;
	return std::clamp(als_brt + offset_step, min, max);
}

void core::refresh_gamma(Xorg &xorg, Channel &ch)
{
	for (size_t i = 0; i < xorg.scr_count(); ++i) {
		xorg.set_gamma(i,
		               cfg.screens[i].brt_step,
		               cfg.screens[i].temp_step);
	}

	if (ch.recv_timeout(10000) < 0)
		return;

	core::refresh_gamma(xorg, ch);
}

std::unique_ptr<sdbus::IProxy> dbus_register_signal_handler(
    const std::string &service,
    const std::string &obj_path,
    const std::string &interface,
    const std::string &signal_name,
    std::function<void(sdbus::Signal &signal)> handler)
{
	auto proxy = sdbus::createProxy(service, obj_path);
	try {
		proxy->registerSignalHandler(interface, signal_name, handler);
		proxy->finishRegistration();
	} catch (sdbus::Error &e) {
		syslog(LOG_ERR, "sdbus error: %s", e.what());
	}
	return proxy;
}
