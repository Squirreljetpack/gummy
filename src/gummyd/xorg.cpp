/**
* gummy
* Copyright (C) 2022  Francesco Fusco
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

#include "xorg.hpp"

Xorg::Output::Output(xcb_randr_crtc_t c, size_t sz,
XLib &xlib, unsigned int width, unsigned int height) // todo: construct image in ctor call (shared_image needs a move ctor)
    : crtc(c),
    ramp_size(sz),
    image(xlib, width, height)
{
}

Xorg::Xorg()
{
	const std::tuple crtcs = xcb.crtcs();

	for (size_t i = 0; i < std::get<1>(crtcs); ++i) {

		auto crtc = std::get<0>(crtcs)[i];
		auto info = xcb.crtc_data(crtc);

		if (info->num_outputs < 1)
			continue;

		outputs.emplace_back(crtc,
		    xcb.gamma_ramp_size(crtc) * 3,
		    xlib, info->width, info->height);
	}
}

std::tuple<uint8_t*, size_t> Xorg::screen_data(int scr_idx)
{
	outputs[scr_idx].image.update(xlib);
	// The cast to uint8_t is important
	return std::make_tuple(
	    reinterpret_cast<uint8_t*>(outputs[scr_idx].image.data()),
	    outputs[scr_idx].image.length());
}

void Xorg::set_gamma_ramp(int scr_idx, const std::vector<uint16_t> &ramps)
{
	xcb.set_gamma(outputs[scr_idx].crtc, ramps);
}

size_t Xorg::ramp_size(int scr_idx)
{
	return outputs[scr_idx].ramp_size;
}

size_t Xorg::scr_count() const
{
	return outputs.size();
}
