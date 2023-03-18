// Copyright 2021-2023 Francesco Fusco
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DISPLAY_HPP
#define DISPLAY_HPP

#include <vector>
#include <mutex>
#include <functional>

#include <gummyd/x11-xcb.hpp>

namespace gummyd {
class display_server
{
    xcb::connection xcb_conn_;
	xcb::randr randr_;
	std::vector<xcb::randr::output> randr_outputs_;
	xcb::shared_memory shmem_;
	xcb::shared_image  shimg_;
	std::mutex mutex_;
public:
	display_server();
	int     shared_image_data(size_t scr_idx, std::function<int(xcb::shared_image::buffer)> fn);
	void    set_gamma_ramp(int scr_idx, const std::vector<uint16_t> &ramps);
	size_t  ramp_size(int scr_idx) const;
	size_t  scr_count() const;
};
}

#endif // DISPLAY_HPP
