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

#ifndef XCB_H
#define XCB_H

#include <vector>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

#include "xlib.hpp"
#include "xcb.hpp"

class Xorg
{
    struct Output
	{
	    xcb_randr_crtc_t crtc;
		size_t ramp_size;
		XLib::shared_image image;
		Output(xcb_randr_crtc_t crtc, size_t ramp_size,
		XLib &xlib, unsigned int width, unsigned int height);
	};

	std::vector<Output> outputs;
	XLib xlib;
	XCB  xcb;
	void set_ramp(Output &o);
	void fill_ramp(Output &, int brt_step, int temp_step);

public:
    Xorg();
	std::tuple<uint8_t*, size_t> screen_data(int scr_idx);
	void    set_gamma_ramp(int scr_idx, const std::vector<uint16_t> &ramps);
	size_t  ramp_size(int scr_idx);
	size_t  scr_count() const;
};

#endif // XCB_H
