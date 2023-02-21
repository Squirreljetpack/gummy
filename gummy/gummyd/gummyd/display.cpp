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

#include <gummyd/display.hpp>

using namespace gummyd;

display_server::display_server()
: xcb_conn_(),
  randr_(),
  randr_outputs_(randr_.outputs(xcb_conn_, xcb_conn_.first_screen())),
  shmem_(xcb::screen_size(xcb_conn_.first_screen())),
  shimg_(shmem_, xcb_conn_.first_screen()->width_in_pixels, xcb_conn_.first_screen()->height_in_pixels)
{
}

xcb::shared_image::buffer display_server::screen_data(int scr_idx) {
	const auto o = randr_outputs_[scr_idx];
	return shimg_.get(o.x, o.y, o.width, o.height);
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
