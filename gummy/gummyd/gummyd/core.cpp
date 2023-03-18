// Copyright 2021-2023 Francesco Fusco
// SPDX-License-Identifier: GPL-3.0-or-later

#include <filesystem>
#include <fmt/chrono.h>
#include <spdlog/spdlog.h>

#include <gummyd/core.hpp>
#include <gummyd/sd-sysfs.hpp>
#include <gummyd/time.hpp>
#include <gummyd/easing.hpp>
#include <gummyd/utils.hpp>
#include <gummyd/constants.hpp>
#include <gummyd/file.hpp>

// [0, 255]
int image_brightness(uint8_t *buf, size_t sz, int bytes_per_pixel = 4, int stride = 1024) {
    std::array<uint64_t, 3> rgb {};
    for (uint64_t i = 0, inc = stride * bytes_per_pixel; i < sz; i += inc) {
        rgb[0] += buf[i + 2];
        rgb[1] += buf[i + 1];
        rgb[2] += buf[i];
    }
    return ((rgb[0] * 0.2126) + (rgb[1] * 0.7152) + (rgb[2] * 0.0722)) * stride / (sz / bytes_per_pixel);
}

void gummyd::jthread_wait_until(std::chrono::milliseconds ms, std::stop_token stoken) {
    using namespace std::chrono;
    std::mutex mutex;
    std::unique_lock lock(mutex);
    std::condition_variable_any()
            .wait_until(lock, stoken, system_clock::now() + ms, [&] { return stoken.stop_requested(); });
}

void gummyd::screenlight_server(display_server &dsp, size_t screen_idx, channel<int> &ch, struct config::screenshot conf, std::stop_token stoken)
{
    int cur = constants::brt_steps_max;
	int prev;
	int delta = 0;
	while (true) {
		prev = cur;

		const int brightness = [&] {
			for (int tries = 0; tries < 10; ++tries) {
				const int ret = dsp.shared_image_data(screen_idx, [] (xcb::shared_image::buffer b) { return image_brightness(b.data, b.size); });
				if (ret > -1)
					return ret;
				spdlog::error("failed to get screen data [error: {}], retrying ({})...", ret, tries + 1);
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
			}
			throw std::runtime_error("failed to get screen data after 10 tries");
		}();

		cur = std::clamp(double(brightness) * conf.scale, 0., 255.);
		delta += std::abs(prev - cur);

		if (delta > 8) {
			delta = 0;
			spdlog::debug("[screenlight_server (scr: {})] sending: {}", screen_idx, cur);
			ch.send(cur);
		}

		jthread_wait_until(std::chrono::milliseconds(conf.poll_ms), stoken);

		if (stoken.stop_requested()) {
			ch.send(-1);
			return;
		}
	}
}

void gummyd::screenlight_client(const channel<int> &ch, size_t screen_idx, config::screen::model model, std::function<void(int)> model_fn, int adaptation_ms)
{
    const auto state_dir = xdg_state_dir() / fmt::format("gummyd/screen-{}", screen_idx);
    std::filesystem::create_directories(state_dir);
    const auto filepath = state_dir / config::screen::model_name(model.id);

    const int start_val = [&] {
        try {
            return std::stoi(file_read(filepath));
        } catch (std::runtime_error &e) {
            return model.max;
        }
    }();

    int val = start_val;
	int prev_brt = -1;

	while (true) {
		const int brt = ch.recv(prev_brt);

        spdlog::debug("[model: {}, client: screenlight] received {}.", config::screen::model_name(model.id), brt);

		if (brt < 0) {
            break;
		}

		const int target = remap(brt, 0, 255, model.max, model.min);

		const auto interrupt = [&] {
			return ch.read() != brt;
		};

        spdlog::debug("[model: {}, client: screenlight] easing from {} to {}...", config::screen::model_name(model.id), val, target);
		val = easing::animate(val, target, adaptation_ms, easing::ease_out_expo, model_fn, interrupt);
		prev_brt = brt;
	}

    file_write(filepath, std::to_string(val));
}

void gummyd::als_server(const sysfs::als &als, channel<double> &ch, struct config::als conf, std::stop_token stoken)
{
	double prev = -1;
	double cur;

	while (true) {

		// have at least 1 lux to play with
		const double lux = std::max(als.read_lux(), 1.);
		spdlog::debug("[als] got {} lux", lux);

		// the human eye's perception of light intensity is roughly logarithmic
		cur = std::log10(std::max(lux * conf.scale, 1.));

		if (std::abs(prev - cur) > 0.01) {
            spdlog::debug("[als] sending: {}", cur);
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

void gummyd::als_client(const channel<double> &ch, size_t screen_idx, config::screen::model model, std::function<void(int)> model_fn, int adaptation_ms)
{
    const auto state_dir = xdg_state_dir() / fmt::format("gummyd/screen-{}", screen_idx);
    std::filesystem::create_directories(state_dir);
    const auto filepath = state_dir / config::screen::model_name(model.id);

    const int start_val = [&] {
        try {
            return std::stoi(file_read(filepath));
        } catch (std::runtime_error &e) {
            return model.max;
        }
    }();

    int val = start_val;
	double prev_brt = -1;

	while (true) {

		const double brt = ch.recv(prev_brt);

        spdlog::debug("[als client] received {} (prev: {}).", brt, prev_brt);

		if (brt < 0) {
			break;
		}

		const int target = lerp(model.min, model.max, std::clamp(brt, 0., 1.));

		const auto interrupt = [&] {
			return ch.read() != brt;
		};

        spdlog::debug("[als client] easing from {} to {}...", val, target);
		val = easing::animate(val, target, adaptation_ms, easing::ease_out_expo, model_fn, interrupt);
		prev_brt = brt;
	}

    file_write(filepath, std::to_string(val));
}

void gummyd::time_server(channel<time_data> &ch, struct config::time conf, std::stop_token stoken)
{
    time_window tw(std::time(nullptr), conf.start, conf.end, -(conf.adaptation_minutes * 60));

	while (true) {

		tw.reference(std::time(nullptr));

		const int in_range = tw.in_range();

        spdlog::debug("[time_server] in range: {}", in_range);

		ch.send({
		    tw.time_since_last(),
		    conf.adaptation_minutes * 60,
		    tw.in_range(),
		});

		if (!in_range && tw.reference() > tw.start()) {
            spdlog::debug("[time_server] adding 1 day to time range");
			tw.shift_dates();
		}

		const std::chrono::seconds time_to_next(std::abs(tw.time_to_next()));
        spdlog::debug("[time_server] sleeping until next event in: {} (~{})", std::chrono::duration_cast<std::chrono::minutes>(time_to_next), std::chrono::duration_cast<std::chrono::hours>(time_to_next));

		jthread_wait_until(time_to_next, stoken);

		if (stoken.stop_requested()) {
			ch.send({-1, -1, -1});
            spdlog::debug("[time_server] exit");
			return;
		}
	}
}

gummyd::time_target calc_time_target(bool step, gummyd::time_data data, gummyd::config::screen::model model)
{
	const std::time_t delta_s = std::min(std::abs(data.time_since_last_event), data.adaptation_s);

	const int target = [&] {
		const double lerp_fac = double(delta_s) / data.adaptation_s;
		if (data.in_range)
            return int(gummyd::lerp(model.min, model.max, lerp_fac));
		else
            return int(gummyd::lerp(model.max, model.min, lerp_fac));
	}();

	const std::time_t min_duration_ms = 2000;

	const int duration_ms = std::max(((data.adaptation_s - delta_s) * 1000), min_duration_ms);

	if (!step)
		return { target, min_duration_ms };
	else
		return { data.in_range ? model.max : model.min, duration_ms };
}

void gummyd::time_client(const channel<time_data> &ch, size_t screen_idx, config::screen::model model, std::function<void(int)> model_fn)
{
    const auto state_dir = xdg_state_dir() / fmt::format("gummyd/screen-{}", screen_idx);
    std::filesystem::create_directories(state_dir);
    const auto filepath = state_dir / config::screen::model_name(model.id);

    const int start_val = [&] {
        try {
            return std::stoi(file_read(filepath));
        } catch (std::runtime_error &e) {
            return model.max;
        }
    }();

    int cur = start_val;

	time_data prev {-1, -1, -1};

	while (true) {

        spdlog::debug("[time_client] reading...");
		const time_data data = ch.recv(prev);

		if (data.in_range < 0)
            break;

		const auto interrupt = [&] {
			return ch.read().time_since_last_event != data.time_since_last_event;
		};

		for (int step = 0; step < 2; ++step) {
			const time_target target = calc_time_target(step, data, model);
            spdlog::debug("[time_client] easing from {} to {} (duration: {})...", cur, target.val, std::chrono::duration_cast<std::chrono::minutes>(std::chrono::milliseconds(target.duration_ms)));
			cur = easing::animate(cur, target.val, target.duration_ms, easing::ease, model_fn, interrupt);
		}
		prev = data;
	}

    file_write(filepath, std::to_string(cur));
}
