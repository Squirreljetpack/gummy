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

#include <stdexcept>
#include <vector>
#include <tuple>

#include <xcb/xcb.h>
#include <xcb/randr.h>
//#include <xcb/shm.h>
//#include <xcb/xcb_image.h>

class XCB
{
    xcb_connection_t *conn;
	xcb_screen_t *screen;
	int pref_screen;
public:
	XCB(); ~XCB();
	auto crtcs()                                -> std::tuple<xcb_randr_crtc_t*, size_t>;
	auto crtc_data(xcb_randr_crtc_t crtc)       -> xcb_randr_get_crtc_info_reply_t*;
	auto gamma_ramp_size(xcb_randr_crtc_t crtc) -> int;
	auto set_gamma(xcb_randr_crtc_t crtc, const std::vector<uint16_t> &ramps) -> void;
};

inline XCB::XCB() : conn(xcb_connect(nullptr, &pref_screen))
{
	int err = xcb_connection_has_error(conn);
	if (err > 0)
		throw std::runtime_error("XCB connection error: " + std::to_string(err));

	xcb_screen_iterator_t iter = xcb_setup_roots_iterator(xcb_get_setup(conn));

	for (int i = 0; iter.rem > 0; --i) {
		if (i == pref_screen) {
			screen = iter.data;
			break;
		}
		xcb_screen_next(&iter);
	}
}

inline XCB::~XCB()
{
	xcb_disconnect(conn);
}

inline std::tuple<xcb_randr_crtc_t*, size_t> XCB::crtcs()
{
	xcb_generic_error_t *e;
	auto cookie = xcb_randr_get_screen_resources(conn, screen->root);
	auto reply  = xcb_randr_get_screen_resources_reply(conn, cookie, &e);

	if (e)
		throw std::runtime_error("xcb_randr_get_screen_resources_reply error " + std::to_string(e->error_code));

	// "get_crtc_info_reply_t" doesn't work if an std::vector is used for this.
	std::tuple ret = std::make_tuple(xcb_randr_get_screen_resources_crtcs(reply), reply->num_crtcs);

	free(reply);

	return ret;
}

inline xcb_randr_get_crtc_info_reply_t *XCB::crtc_data(xcb_randr_crtc_t crtc)
{
	xcb_generic_error_t *e;
	auto cookie = xcb_randr_get_crtc_info(conn, crtc, 0);
	auto reply  = xcb_randr_get_crtc_info_reply(conn, cookie, &e);

	if (e)
		throw std::runtime_error("xcb_randr_get_crtc_info_reply error " + std::to_string(e->error_code));

	// needs to be freed
	return reply;
}

inline int XCB::gamma_ramp_size(xcb_randr_crtc_t crtc)
{
	xcb_generic_error_t *e;
	auto cookie = xcb_randr_get_crtc_gamma(conn, crtc);
	auto reply  = xcb_randr_get_crtc_gamma_reply(conn, cookie, &e);

	if (e)
		throw std::runtime_error("xcb_randr_get_crtc_gamma_reply error: %d\n" + std::to_string(e->error_code));

	const int ret = reply->size;

	free(reply);

	return ret;
}

inline void XCB::set_gamma(xcb_randr_crtc_t crtc, const std::vector<uint16_t> &ramps)
{
	const size_t sz = ramps.size() / 3;

	auto req = xcb_randr_set_crtc_gamma_checked(conn, crtc, sz,
	    &ramps[0 * sz],
	    &ramps[1 * sz],
	    &ramps[2 * sz]);

	xcb_generic_error_t *e = xcb_request_check(conn, req);
	if (e) {
		throw std::runtime_error("xcb_randr_set_crtc_gamma_checked error: %d\n" + std::to_string(e->error_code));
	}
}
