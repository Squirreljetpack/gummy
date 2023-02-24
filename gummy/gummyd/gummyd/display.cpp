// Copyright (c) 2021-2023, Francesco Fusco. All rights reserved.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gummyd/display.hpp>
#include <gummyd/xcb.hpp>
#include <functional>
#include <mutex>

using namespace gummyd;

display_server::display_server()
: xcb_conn_(),
  randr_(),
  randr_outputs_(randr_.outputs(xcb_conn_, xcb_conn_.first_screen())),
  shmem_(xcb::screen_size(xcb_conn_.first_screen())),
  shimg_(shmem_, xcb_conn_.first_screen()->width_in_pixels, xcb_conn_.first_screen()->height_in_pixels) {
}

int display_server::shared_image_data(size_t scr_idx, std::function<int(xcb::shared_image::buffer)> fn) {
	std::lock_guard lock(mutex_);
	const auto o = randr_outputs_[scr_idx];
	const auto buf = shimg_.get(o.x, o.y, o.width, o.height);
	return buf.data ? fn(buf) : -(buf.size);
}

void display_server::set_gamma_ramp(int scr_idx, const std::vector<uint16_t> &ramps) {
	randr_.set_gamma(xcb_conn_, randr_outputs_[scr_idx].crtc_id, ramps);
}

size_t display_server::ramp_size(int scr_idx) const {
	return randr_outputs_[scr_idx].ramp_size;
}

size_t display_server::scr_count() const {
	return randr_outputs_.size();
}
