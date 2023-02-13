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

#ifndef DISPLAY_HPP
#define DISPLAY_HPP

#include <vector>
#include "xcb.hpp"

class display_server
{
	struct output
	{
		xcb_randr_crtc_t  crtc;
		size_t            ramp_size;
		xcb::shared_image image;

		output(xcb_randr_crtc_t crtc, size_t ramp_size, xcb &xcb, unsigned int width, unsigned int height);
	};

	xcb xcb_;
	std::vector<output> outputs;
public:
	display_server();
	std::pair<uint8_t*, size_t> screen_data(int scr_idx);
	void    set_gamma_ramp(int scr_idx, const std::vector<uint16_t> &ramps);
	size_t  ramp_size(int scr_idx);
	size_t  scr_count() const;
};

#endif // EASING_HPP
