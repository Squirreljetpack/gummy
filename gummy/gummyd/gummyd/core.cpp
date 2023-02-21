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

#include <fmt/chrono.h>

#include <gummyd/time.hpp>
#include <gummyd/easing.hpp>
#include <gummyd/utils.hpp>
#include <gummyd/core.hpp>
#include <gummyd/constants.hpp>
#include <gummyd/file.hpp>

using namespace fushko;

// [0, 255]
int image_brightness(uint8_t *buf, size_t sz, int bytes_per_pixel = 4, int stride = 1024)
{
	std::array<uint64_t, 3> rgb {};
	for (uint64_t i = 0, inc = stride * bytes_per_pixel; i < sz; i += inc) {
		rgb[0] += buf[i + 2];
		rgb[1] += buf[i + 1];
		rgb[2] += buf[i];
	}
	return ((rgb[0] * 0.2126) + (rgb[1] * 0.7152) + (rgb[2] * 0.0722)) * stride / (sz / bytes_per_pixel);
}

void gummy::screenlight_server(display_server &dsp, size_t screen_idx, channel<int> &ch, struct config::screenshot conf, std::stop_token stoken)
{
    int cur = constants::brt_steps_max;
	int prev;
	int delta = 0;

	while (true) {
		prev = cur;

		const auto buf = [&dsp, screen_idx] {
			for (int tries = 0; tries < 10; ++tries) {
				const auto ret = dsp.screen_data(screen_idx);
				if (ret.data)
					return ret;
				LOG_FMT_("failed to get screen data [error: {}], retrying ({})...\n", ret.size, tries + 1);
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
			}
			throw std::runtime_error("failed to get screen data after 10 tries");
		}();

		cur = std::clamp(double(image_brightness(buf.data, buf.size)) * conf.scale, 0., 255.);
		delta += std::abs(prev - cur);

		if (delta > 8) {
			delta = 0;
            LOG_FMT_("[screenlight_server] sending: {}\n", cur);
			ch.send(cur);
		}

		jthread_wait_until(std::chrono::milliseconds(conf.poll_ms), stoken);

		if (stoken.stop_requested()) {
			ch.send(-1);
			return;
		}
	}
}

void gummy::screenlight_client(const channel<int> &ch, size_t screen_idx, config::screen::model model, std::function<void(int)> model_fn, int adaptation_ms)
{
    const std::string xdg_state_dir = xdg_state_filepath(fmt::format("gummyd/screen-{}", screen_idx));
    const std::string filepath      = fmt::format("{}/{}", xdg_state_dir, config::screen::model_name(model.id));

    const int start_val = [&] {
        try {
            return std::stoi(file_read(filepath));
        } catch (std::runtime_error &e) {
            std::filesystem::create_directories(xdg_state_dir);
            return model.max;
        }
    }();

    int val = start_val;
	int prev_brt = -1;

	while (true) {
		const int brt = ch.recv(prev_brt);

        LOG_FMT_("[model: {}, client: screenlight] received {}.\n", config::screen::model_name(model.id), brt);

		if (brt < 0) {
            break;
		}

		const int target = remap(brt, 0, 255, model.max, model.min);

		const auto interrupt = [&] {
			return ch.read() != brt;
		};

        LOG_FMT_("[model: {}, client: screenlight] easing from {} to {}...\n", config::screen::model_name(model.id), val, target);
		val = easing::animate(val, target, adaptation_ms, easing::ease_out_expo, model_fn, interrupt);
		prev_brt = brt;
	}

    file_write(filepath, fmt::format("{}\n", val));
}

void gummy::als_server(const als &als, channel<double> &ch, struct config::als conf, std::stop_token stoken)
{
	double prev = -1;
	double cur;

	while (true) {

		// have at least 1 lux to play with
		const double lux = std::max(als.read_lux(), 1.);

		// the human eye's perception of light intensity is roughly logarithmic
		cur = std::log10(std::max(lux * conf.scale, 1.));

		if (std::abs(prev - cur) > 0.01) {
			LOG_FMT_("[als] sending: {}\n", cur);
			ch.send(cur);
		}

		jthread_wait_until(std::chrono::milliseconds(conf.poll_ms), stoken);

		if (stoken.stop_requested()) {
			ch.send(-1);
			return;
		}

		prev = cur;
	}
}

void gummy::als_client(const channel<double> &ch, size_t screen_idx, config::screen::model model, std::function<void(int)> model_fn, int adaptation_ms)
{
    const std::string xdg_state_dir = xdg_state_filepath(fmt::format("gummyd/screen-{}", screen_idx));
    const std::string filepath      = fmt::format("{}/{}", xdg_state_dir, config::screen::model_name(model.id));

	int val = model.max;
	double prev_brt = -1;

	while (true) {

		const double brt = ch.recv(prev_brt);

		LOG_FMT_("[als client] received {} (prev: {}).\n", brt, prev_brt);

		if (brt < 0) {
			return;
		}

		const int target = lerp(model.min, model.max, std::clamp(brt, 0., 1.));

		const auto interrupt = [&] {
			return ch.read() != brt;
		};

		LOG_FMT_("[als client] easing from {} to {}...\n", val, target);
		val = easing::animate(val, target, adaptation_ms, easing::ease_out_expo, model_fn, interrupt);
		prev_brt = brt;
	}

    file_write(filepath, fmt::format("{}\n", screen_idx));
}

void gummy::time_server(channel<time_data> &ch, struct config::time conf, std::stop_token stoken)
{
	time_window tw(std::time(nullptr), conf.start, conf.end, -(conf.adaptation_minutes * 60));

	while (true) {

		tw.reference(std::time(nullptr));

		const int in_range = tw.in_range();

		LOG_FMT_("[time_server] in range: {}\n", in_range);

		ch.send({
		    tw.time_since_last(),
		    conf.adaptation_minutes * 60,
		    tw.in_range(),
		});

		if (!in_range && tw.reference() > tw.start()) {
			LOG_("[time_server] adding 1 day to time range\n");
			tw.shift_dates();
		}

		const std::chrono::seconds time_to_next(std::abs(tw.time_to_next()));
		LOG_FMT_("[time_server] [{}] sleeping until next event in: {} (~{})\n", timestamp_fmt(std::time(nullptr)), std::chrono::duration_cast<std::chrono::minutes>(time_to_next), std::chrono::duration_cast<std::chrono::hours>(time_to_next));

		jthread_wait_until(time_to_next, stoken);

		if (stoken.stop_requested()) {
			ch.send({-1, -1, -1});
			LOG_("[time_server] exit\n");
			return;
		}
	}
}

gummy::time_target calc_time_target(bool step, gummy::time_data data, gummy::config::screen::model model)
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

	if (!step)
		return { target, min_duration_ms };
	else
		return { data.in_range ? model.max : model.min, duration_ms };
}

void gummy::time_client(const channel<time_data> &ch, size_t screen_idx, config::screen::model model, std::function<void(int)> model_fn)
{
    const std::string xdg_state_dir = xdg_state_filepath(fmt::format("gummyd/screen-{}", screen_idx));
    const std::string filepath      = fmt::format("{}/{}", xdg_state_dir, config::screen::model_name(model.id));

    const int start_val = [&] {
        try {
            return std::stoi(file_read(filepath));
        } catch (std::runtime_error &e) {
            std::filesystem::create_directories(xdg_state_dir);
            return model.max;
        }
    }();

    int cur = start_val;

	time_data prev {-1, -1, -1};

	while (true) {

		LOG_("[time_client] reading...\n");
		const time_data data = ch.recv(prev);

		if (data.in_range < 0)
            break;

		const auto interrupt = [&] {
			return ch.read().time_since_last_event != data.time_since_last_event;
		};

		for (int step = 0; step < 2; ++step) {
			const time_target target = calc_time_target(step, data, model);
			LOG_FMT_("[time_client] easing from {} to {} (duration: {})...\n", cur, target.val, std::chrono::duration_cast<std::chrono::minutes>(std::chrono::milliseconds(target.duration_ms)));
			cur = easing::animate(cur, target.val, target.duration_ms, easing::ease, model_fn, interrupt);
		}
		prev = data;
	}

    file_write(filepath, fmt::format("{}\n", cur));
}
