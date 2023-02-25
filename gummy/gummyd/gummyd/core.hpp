// Copyright (c) 2021-2023, Francesco Fusco. All rights reserved.
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef CORE_HPP
#define CORE_HPP

#include <functional>
#include <stop_token>

#include <gummyd/channel.hpp>
#include <gummyd/display.hpp>
#include <gummyd/config.hpp>
#include <gummyd/sysfs_devices.hpp>

namespace gummyd {

void jthread_wait_until(std::chrono::milliseconds ms, std::stop_token stoken);

void screenlight_server(display_server &dsp, size_t screen_idx, channel<int> &ch, struct config::screenshot conf, std::stop_token stoken);
void screenlight_client(const channel<int> &ch, size_t screen_idx, config::screen::model model, std::function<void(int)> model_fn, int adaptation_ms);

void als_server(const sysfs::als &als, channel<double> &ch, struct config::als conf, std::stop_token stoken);
void als_client(const channel<double> &ch, size_t screen_idx, config::screen::model model, std::function<void(int)> model_fn, int adaptation_ms);

struct time_data {
	long time_since_last_event;
	long adaptation_s;
	long in_range;
};

struct time_target {
	int val;
	int duration_ms;
};

void time_server(channel<time_data> &ch, struct config::time conf, std::stop_token stoken);
void time_client(const channel<time_data> &ch, size_t screen_idx, config::screen::model model, std::function<void(int)> model_fn);

}

#endif // CORE_HPP
