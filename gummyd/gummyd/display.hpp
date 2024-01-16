// Copyright 2021-2023 Francesco Fusco <f.fusco@pm.me>
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
    std::vector<xcb::randr::output> randr_outputs_;
    xcb::shared_image  shimg_;
	std::mutex mutex_;
public:
	display_server();
    int     shared_image_data(size_t scr_idx, auto fn);
	void    set_gamma_ramp(int scr_idx, const std::vector<uint16_t> &ramps);
	size_t  ramp_size(int scr_idx) const;
	size_t  scr_count() const;
    std::vector<std::array<uint8_t, 128>> get_edids();
};
}

#endif // DISPLAY_HPP
