/**
* gummy
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

#include "core.hpp"
#include "easing.hpp"

using namespace fushko;

// [0, 255]
int image_brightness(std::pair<uint8_t*, size_t> buf, int bytes_per_pixel = 4, int stride = 1024)
{
	uint8_t *arr      = buf.first;
	const uint64_t sz = buf.second;

	std::array<uint64_t, 3> rgb {0, 0, 0};
	for (uint64_t i = 0, inc = stride * bytes_per_pixel; i < sz; i += inc) {
		rgb[0] += arr[i + 2];
		rgb[1] += arr[i + 1];
		rgb[2] += arr[i];
	}

	return ((rgb[0] * 0.2126) + (rgb[1] * 0.7152) + (rgb[2] * 0.0722)) * stride / (sz / bytes_per_pixel);
}

void brightness_server(display_server &dsp, size_t screen_idx, channel<int> &ch, struct config::screenshot conf, std::stop_token stoken)
{
	int cur = constants::brt_steps_max;
	int prev;
	int delta = 0;

	while (true) {
		prev = cur;

		cur = std::clamp(image_brightness(dsp.screen_data(screen_idx)) + int(remap(conf.offset_perc, -100, 100, -255, 255)), 0, 255);
		delta += std::abs(prev - cur);

		if (delta > 8) {
			delta = 0;
			printf("[brightness_server]: sending %d\n", cur);
			ch.send(cur);
		}

		jthread_wait_until(conf.poll_ms, stoken);

		if (stoken.stop_requested()) {
			ch.send(-1);
			return;
		}
	}
}

int brt_target_als(int als_brt, int min, int max, int offset)
{
	const int offset_step = offset * constants::brt_steps_max / max;
	return std::clamp(als_brt + offset_step, min, max);
}

void brightness_client(channel<int> &ch, config::screen::model model, std::function<void(int)> model_fn, int adaptation_ms)
{
	int val = model.max;
	int prev_brt = -1;

	while (true) {

		const int brt = ch.recv(prev_brt);

		printf("[brightness client] received %d.\n", brt);

		if (brt < 0) {
			return;
		}

		const int target = remap(brt, 0, 255, model.max, model.min);

		const auto interrupt = [&] {
			return ch.read() != brt;
		};

		printf("[brightness client] easing from %d to %d...\n", val, target);
		val = easing::animate(val, target, adaptation_ms, easing::ease_out_expo, model_fn, interrupt);
		prev_brt = brt;
	}
}

void als_server(sysfs::als &als, channel<int> &ch, int sleep_ms, int prev, int cur, std::stop_token stoken)
{
	while (true) {

		prev = als.lux_step();
		als.update();
		cur = als.lux_step();

		if (prev != cur) {
			ch.send(cur);
		}

		jthread_wait_until(sleep_ms, stoken);

		if (stoken.stop_requested()) {
			ch.send(-1);
			return;
		}
	}
}

void time_server(channel<time_data> &ch, struct config::time conf, std::stop_token stoken)
{
	time_window tw(std::time(nullptr), conf.start, conf.end, -(conf.adaptation_minutes * 60));

	while (true) {

		tw.reference(std::time(nullptr));

		const int in_range = tw.in_range();

		printf("[time_server] writing: %d\n", in_range);

		ch.send({
		    tw.time_since_last(),
		    conf.adaptation_minutes * 60,
		    tw.in_range(),
		});

		if (!in_range && tw.reference() > tw.start())
			    tw.shift_dates();

		const int time_to_next_ms = std::abs(tw.time_to_next()) * 1000;
		//printf("time to next event: %d s\n", time_to_next_ms / 1000);

		jthread_wait_until(time_to_next_ms, stoken);

		if (stoken.stop_requested()) {
			ch.send({-1, -1, -1});
			puts("[time_server] exit");
			return;
		}
	}
}

time_target calc_time_target(bool step, time_data data, config::screen::model model)
{
	const std::time_t delta_s = std::min(std::abs(data.time_since_last_event), data.adaptation_s);

	const int target = [&] {
		const double lerp_fac = double(delta_s) / data.adaptation_s;
		if (data.in_range)
			return int(lerp(model.min, model.max, lerp_fac));
		else
			return int(lerp(model.max, model.min, lerp_fac));
	}();

	const std::time_t min_duration_ms = 2000;

	const int duration_ms = std::max(((data.adaptation_s - delta_s) * 1000), min_duration_ms);

	printf("in_range: %ld\n", data.in_range);
	printf("target: %d\n", target);
	printf("duration_ms: %d\n", duration_ms);

	if (!step)
		return { target, min_duration_ms };
	else
		return { data.in_range ? model.max : model.min, duration_ms };
}

void time_client(channel<time_data> &ch, config::screen::model model, std::function<void(int)> model_fn)
{
	int cur = model.max;

	time_data prev {-1, -1, -1};

	while (true) {

		puts("[time_client] reading...");
		const time_data data = ch.recv(prev);

		if (data.in_range < 0)
			return;

		const auto interrupt = [&] {
			return ch.read().time_since_last_event != data.time_since_last_event;
		};

		for (int step = 0; step < 2; ++step) {
			const time_target target = calc_time_target(step, data, model);
			printf("[time_client] animating from %d to %d (duration: %d ms)..\n", cur, target.val, target.duration_ms);
			cur = easing::animate(cur, target.val, target.duration_ms, easing::ease, model_fn, interrupt);
		}
		prev = data;
	}
}



