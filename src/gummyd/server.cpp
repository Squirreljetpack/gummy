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

#include <thread>
#include "xorg.hpp"
#include "channel.hpp"
#include "sysfs_devices.hpp"
#include "server.hpp"
#include "cfg.hpp"
#include "utils.hpp"

// [0, 255]
int image_brightness(std::tuple<uint8_t*, size_t> buf, int bytes_per_pixel = 4, int stride = 1024)
{
	uint64_t rgb[3] {};
	uint8_t  *arr = std::get<0>(buf);
	uint64_t sz   = std::get<1>(buf);

	for (uint64_t i = 0, inc = stride * bytes_per_pixel; i < sz; i += inc) {
		rgb[0] += arr[i + 2];
		rgb[1] += arr[i + 1];
		rgb[2] += arr[i];
	}

	return ((rgb[0] * 0.2126) + (rgb[1] * 0.7152) + (rgb[2] * 0.0722)) * stride / (sz / bytes_per_pixel);
}

void brightness_server(Xorg &xorg, size_t screen_idx, Channel &brt_ch, Channel &sig, int sleep_ms, int prev, int cur)
{
	while (true) {
		prev = cur;
		cur = remap(image_brightness(xorg.screen_data(screen_idx)), 0, 255, brt_steps_min, brt_steps_max);

		if (prev != cur)
			brt_ch.send(cur);

		if (sig.recv_timeout(sleep_ms) < 0) {
			brt_ch.send(-1);
			return;
		}
	}
}

void als_server(Sysfs::ALS &als, Channel &ch, Channel &sig, int sleep_ms, int prev, int cur)
{
	while (true) {

		prev = als.lux_step();
		als.update();
		cur = als.lux_step();

		if (prev != cur) {
			ch.send(cur);
		}

		if (sig.recv_timeout(sleep_ms) < 0) {
			ch.send(-1);
			printf("als_server: exit\n");
			return;
		}
	}
}
