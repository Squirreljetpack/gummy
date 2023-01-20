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

#include "screenctl.h"
#include "cfg.h"
#include "../common/utils.h"

#include <syslog.h>

core::Temp_Manager::Temp_Manager(Xorg *xorg)
    : xorg(xorg),
      current_step(temp_steps_max)
{}

void core::temp_start(Temp_Manager &t)
{
	const auto notify_on_sys_resume = [&] (sdbus::Signal &sig) {
		bool suspended;
		sig >> suspended;
		if (!suspended)
			t.ch.send(t.NOTIFIED);
	};

	t.dbus_proxy = dbus_register_signal_handler(
	    "org.freedesktop.login1",
	    "/org/freedesktop/login1",
	    "org.freedesktop.login1.Manager",
	    "PrepareForSleep",
	    notify_on_sys_resume);

	t.ch.send(t.WORKING);
	core::temp_adjust_loop(t);
}

void core::temp_adjust_loop(Temp_Manager &t)
{
	//printf("temp_adjust_loop: temp_auto: %d\n", cfg.temp_auto);

	t.ch.send(t.WORKING);

	if (cfg.temp_auto) {
		core::temp_adjust(t, timestamps_update(
		    cfg.temp_auto_sunrise,
		    cfg.temp_auto_sunset,
		    -(cfg.temp_auto_speed * 60)), false);
	}

	//printf("temp_adjust_loop: waiting until timeout\n");

	// retry without waiting if interrupted
	if (t.ch.data() != t.NOTIFIED) {
		if (t.ch.recv_timeout(60000) == t.EXIT)
			return;
	}

	core::temp_adjust_loop(t);
}

void core::temp_adjust(Temp_Manager &t, Timestamps ts, bool step)
{
	//printf("temp_adjust: step %d\n", step);

	ts.cur = std::time(nullptr);
	const bool daytime = ts.cur >= ts.start && ts.cur < ts.end;

	int target_temp;
	std::time_t time_to_subtract;
	if (daytime) {
		target_temp = cfg.temp_auto_high;
		time_to_subtract = ts.start;
	} else {
		target_temp = cfg.temp_auto_low;
		time_to_subtract = ts.end;
	}

	const double adaptation_time_s = double(cfg.temp_auto_speed) * 60;
	int time_since_start_s = std::abs(ts.cur - time_to_subtract);
	if (time_since_start_s > adaptation_time_s)
		time_since_start_s = adaptation_time_s;

	double animation_s = 2;
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
	    t.current_step,
	    target_step,
	    cfg.temp_auto_fps,
	    animation_s * 1000);

	core::temp_animation_loop(t, a, -1, t.current_step, target_step);

	//printf("temp_adjust: step %d done\n", step);
	if (!step)
		core::temp_adjust(t, ts, !step);
}

void core::temp_animation_loop(Temp_Manager &t, Animation a, int prev_step, int cur_step, int target_step)
{
	if (cur_step == target_step) {
		printf("temp_animation_loop: target reached\n");
		return;
	}

	if (t.ch.data() == t.NOTIFIED) {
		printf("temp_animation_loop: interrupted\n");
		return;
	}

	a.elapsed += a.slice;
	cur_step = int(ease_in_out_quad(a.elapsed, a.start_step, a.diff, a.duration_s));
	t.current_step = cur_step;

	printf("temp_animation_loop: animation_time: %f/%f\n", a.elapsed, a.duration_s);
	if (cur_step != prev_step) {
		for (size_t i = 0; i < t.xorg->scr_count(); ++i) {
			if (cfg.screens[i].temp_auto) {
				cfg.screens[i].temp_step = cur_step;
				t.xorg->set_gamma(i,
				                  cfg.screens[i].brt_step,
				                  cur_step);
			}
		}
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(1000 / a.fps));

	core::temp_animation_loop(t, a, cur_step, cur_step, target_step);
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
		m.ch.send(0);

	for (auto &t : threads)
		t.join();
}

void core::als_capture_loop(Sysfs::ALS &als, Channel &ch)
{
	const int prev_step = als.lux_step();
	als.update();

	if (prev_step != als.lux_step() || ch.data() == 2) {
		printf("als_capture_loop: sending signal\n");
		ch.send(1);
	}

	if (ch.recv_timeout(cfg.als_polling_rate) == 0) {
		printf("als_capture_loop: exit\n");
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
	std::thread thr([&] {
		core::monitor_brt_adjust_loop(mon, brt_steps_max);
	});
	core::monitor_is_auto_loop(mon);

	mon.brt_ch.send(-1);
	thr.join();
}

void core::monitor_is_auto_loop(Monitor &mon)
{
	printf("brt_mode: %d\n", cfg.screens[mon.id].brt_mode);
	mon.ch.send(1);

	if (cfg.screens[mon.id].brt_mode != MANUAL) {
		printf("monitor_is_auto_loop: starting monitor_capture_loop\n");
		core::monitor_capture_loop(mon, monitor_capture_state{0, 0, 0, 0}, 255);
	}

	if (mon.ch.data() != 2) {
		printf("monitor_is_auto_loop: awaiting\n");
		if (mon.ch.recv() == 0) {
			return;
		}
	}

	core::monitor_is_auto_loop(mon);
}

void core::monitor_capture_loop(Monitor &mon, monitor_capture_state state, int ss_delta)
{
	printf("monitor_capture_loop\n");
	const auto &scr = cfg.screens[mon.id];

	const int ss_brt = [&] {

		if (scr.brt_mode == ALS) {
			printf("monitor_capture_loop: awaiting ALS signal\n");
			mon.als_ch->recv();
			printf("monitor_capture_loop: received ALS signal\n");
			return mon.als->lux_step();
		}

		return mon.xorg->get_screen_brightness(mon.id);
	}();

	if (mon.ch.data() == 2) {
		printf("monitor_capture_loop: interrupted\n");
		return;
	}

	ss_delta += abs(state.ss_brt - ss_brt);

	if (ss_delta > scr.brt_auto_threshold || scr.brt_mode == ALS) {
		ss_delta = 0;
		mon.brt_ch.send(ss_brt);
	}

	if (scr.brt_auto_min != state.cfg_min
	    || scr.brt_auto_max != state.cfg_max
	    || scr.brt_auto_offset != state.cfg_offset) {
		ss_delta = 255;
	}

	state.ss_brt     = ss_brt;
	state.cfg_min    = scr.brt_auto_min;
	state.cfg_max    = scr.brt_auto_max;
	state.cfg_offset = scr.brt_auto_offset;

	if (scr.brt_mode == SCREENSHOT) {
		if (mon.backlight) {
			mon.als_ch->recv_timeout(scr.brt_auto_polling_rate);
		} else {
			std::this_thread::sleep_for(std::chrono::milliseconds(scr.brt_auto_polling_rate));
		}
	}

	core::monitor_capture_loop(mon, state, ss_delta);
}

void core::monitor_brt_adjust_loop(Monitor &mon, int cur_step)
{
	printf("monitor_brt_adjust_loop: awaiting\n");
	const int ss_brt = mon.brt_ch.recv();
	printf("monitor_brt_adjust_loop: received %d\n", ss_brt);

	if (ss_brt == -1) {
		printf("monitor_brt_adjust_loop: quitting\n");
		return;
	}

	const auto &scr = cfg.screens[mon.id];

	const int target_step = [&] {
		if (mon.als && scr.brt_mode == ALS)
			return calc_brt_target_als(ss_brt, scr.brt_auto_min, scr.brt_auto_max, scr.brt_auto_offset);
		else
			return calc_brt_target(ss_brt, scr.brt_auto_min, scr.brt_auto_max, scr.brt_auto_offset);
	}();

	if (scr.brt_mode == SCREENSHOT)
		cur_step = scr.brt_step;

	if (cur_step != target_step) {
		if (mon.backlight) {
			cur_step = target_step;
			mon.backlight->set(cur_step * mon.backlight->max_brt() / brt_steps_max);
		} else {
			Animation a = animation_init(cur_step, target_step, cfg.brt_auto_fps, scr.brt_auto_speed);
			cur_step = monitor_brt_animation_loop(mon, a, -1, cur_step, target_step, ss_brt);
		}
	}

	core::monitor_brt_adjust_loop(mon, cur_step);
}

int core::monitor_brt_animation_loop(Monitor &mon, Animation a, int prev_step, int cur_step, int target_step, int ss_brt)
{
	if (mon.ch.data() == 2) {
		printf("monitor_brt_animation_loop: interrupted\n");
		return cur_step;
	}

	a.elapsed += a.slice;
	cur_step = int(round(ease_out_expo(a.elapsed, a.start_step, a.diff, a.duration_s)));

	if (cur_step != prev_step) {
		cfg.screens[mon.id].brt_step = cur_step;
		mon.xorg->set_gamma(mon.id,
		                    cur_step,
		                    cfg.screens[mon.id].temp_step);
	}

	if (cur_step == target_step)
		return cur_step;

	std::this_thread::sleep_for(std::chrono::milliseconds(1000 / a.fps));
	return core::monitor_brt_animation_loop(mon, a, cur_step, cur_step, target_step, ss_brt);
}

int core::calc_brt_target_als(int als_brt, int min, int max, int offset)
{
	const int offset_step = offset * brt_steps_max / max;
	return std::clamp(als_brt + offset_step, min, max);
}

int core::calc_brt_target(int ss_brt, int min, int max, int offset)
{
	const int ss_step     = ss_brt * brt_steps_max / 255;
	const int offset_step = offset * brt_steps_max / max;
	return std::clamp(brt_steps_max - ss_step + offset_step, min, max);
}

void core::refresh_gamma(Xorg &xorg, Channel &ch)
{
	for (size_t i = 0; i < xorg.scr_count(); ++i) {
		xorg.set_gamma(i,
		               cfg.screens[i].brt_step,
		               cfg.screens[i].temp_step);
	}

	if (ch.recv_timeout(10000) == 0)
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
