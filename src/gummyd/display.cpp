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

#include "display.hpp"

display_server::output::output(
    xcb_randr_crtc_t c,
    size_t sz,
    xcb &xcb,
    unsigned int width,
    unsigned int height)
    : crtc(c),
      ramp_size(sz),
      image(xcb, width, height) {}

display_server::display_server()
{
	for (const auto &crtc : xcb_.randr_crtcs()) {
		if (crtc.num_outputs > 0) {
			outputs.emplace_back(crtc.id,
			                     crtc.ramp_size * 3,
			                     xcb_,
			                     crtc.width,
			                     crtc.height);
		}
	}
}

std::pair<uint8_t*, size_t> display_server::screen_data(int scr_idx)
{
	outputs[scr_idx].image.update(xcb_);
	// The cast to uint8_t is important
	return std::make_pair(
	    reinterpret_cast<uint8_t*>(outputs[scr_idx].image.data()),
	    outputs[scr_idx].image.size());
}

void display_server::set_gamma_ramp(int scr_idx, const std::vector<uint16_t> &ramps)
{
	xcb_.randr_set_gamma(outputs[scr_idx].crtc, ramps);
}

size_t display_server::ramp_size(int scr_idx)
{
	return outputs[scr_idx].ramp_size;
}

size_t display_server::scr_count() const
{
	return outputs.size();
}
