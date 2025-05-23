// Copyright 2021-2023 Francesco Fusco <f.fusco@pm.me>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <functional>
#include <mutex>
#include <gummyd/x11-xcb.hpp>
#include <gummyd/display.hpp>

using namespace gummyd;

display_server::display_server()
: xcb_conn_(),
  randr_outputs_(xcb::randr::outputs(xcb_conn_, xcb_conn_.first_screen())),
  shimg_() {
}

int display_server::shared_image_data(size_t scr_idx, auto fn) {
	const auto o = randr_outputs_[scr_idx];
    return shimg_.get(o.x, o.y, o.width, o.height, fn);
}

void display_server::set_gamma_ramp(int scr_idx, const std::vector<uint16_t> &ramps) {
    xcb::randr::set_gamma(xcb_conn_, randr_outputs_[scr_idx].crtc_id, ramps);
}

size_t display_server::ramp_size(int scr_idx) const {
	return randr_outputs_[scr_idx].ramp_size;
}

size_t display_server::scr_count() const {
	return randr_outputs_.size();
}

std::vector<std::array<uint8_t, 128>> display_server::get_edids() {
    std::vector<std::array<uint8_t, 128>> vec;
    vec.reserve(randr_outputs_.size());
    for (const auto &out : randr_outputs_) {
        vec.push_back(out.edid);
    }
    return vec;
}
