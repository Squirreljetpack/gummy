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

#ifndef XCB_H
#define XCB_H

#include <stdexcept>
#include <vector>
#include <tuple>

#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <xcb/shm.h>
#include <xcb/xcb_image.h>

#include <sys/ipc.h>
#include <sys/shm.h>

class XCB
{
    xcb_connection_t *conn;
	xcb_screen_t *screen;
	int pref_screen;
	auto query_extension(std::string) -> void;
public:

	XCB();
	~XCB();
	auto crtcs()                                -> std::tuple<xcb_randr_crtc_t*, size_t>;
	auto crtc_data(xcb_randr_crtc_t crtc)       -> xcb_randr_get_crtc_info_reply_t*;
	auto gamma_ramp_size(xcb_randr_crtc_t crtc) -> int;
	auto set_gamma(xcb_randr_crtc_t crtc, const std::vector<uint16_t> &ramps) -> void;

	class shared_image
	{
	    xcb_image_t *image;
		xcb_shm_segment_info_t shminfo;
	public:
		shared_image(XCB &xcb, unsigned int width, unsigned int height);
		~shared_image();
		void update(XCB &xcb);
		uint8_t *data();
		uint32_t size();
	};
};

inline XCB::XCB() : conn(xcb_connect(nullptr, &pref_screen))
{
	int err = xcb_connection_has_error(conn);

	if (err > 0)
		throw std::runtime_error("XCB connection: error " + std::to_string(err));

	xcb_screen_iterator_t iter = xcb_setup_roots_iterator(xcb_get_setup(conn));

	for (int i = 0; iter.rem > 0; --i) {
		if (i == pref_screen) {
			screen = iter.data;
			break;
		}
		xcb_screen_next(&iter);
	}

	query_extension("RANDR");
	query_extension("MIT-SHM");
}

inline void XCB::query_extension(std::string name)
{
	auto reply = xcb_query_extension_reply(conn, xcb_query_extension(conn, name.size(), name.c_str()), nullptr);
	const bool present = reply && reply->present;
	free(reply);

	if (!present)
		throw std::runtime_error(name + std::string(" not found"));
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
		throw std::runtime_error("xcb_randr_get_screen_resources_reply: error " + std::to_string(e->error_code));

	// "get_crtc_info_reply_t" doesn't work if an std::vector is used for this.
	return std::make_tuple(xcb_randr_get_screen_resources_crtcs(reply), reply->num_crtcs);
}

inline xcb_randr_get_crtc_info_reply_t *XCB::crtc_data(xcb_randr_crtc_t crtc)
{
	xcb_generic_error_t *e;
	auto cookie = xcb_randr_get_crtc_info(conn, crtc, 0);
	auto reply  = xcb_randr_get_crtc_info_reply(conn, cookie, &e);

	if (e)
		throw std::runtime_error("xcb_randr_get_crtc_info_reply: error " + std::to_string(e->error_code));

	// needs to be freed
	return reply;
}

inline int XCB::gamma_ramp_size(xcb_randr_crtc_t crtc)
{
	xcb_generic_error_t *e;
	auto cookie = xcb_randr_get_crtc_gamma(conn, crtc);
	auto reply  = xcb_randr_get_crtc_gamma_reply(conn, cookie, &e);

	if (e)
		throw std::runtime_error("xcb_randr_get_crtc_gamma_reply: error " + std::to_string(e->error_code));

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
		throw std::runtime_error("xcb_randr_set_crtc_gamma_checked: error " + std::to_string(e->error_code));
	}
}

inline XCB::shared_image::shared_image(XCB &xcb, unsigned int width, unsigned int height)
{
	shminfo.shmid   = shmget(IPC_PRIVATE, width * height * 4, IPC_CREAT | 0600);
	shminfo.shmaddr = reinterpret_cast<unsigned char*>(shmat(shminfo.shmid, nullptr, 0));
	shminfo.shmseg  = xcb_generate_id(xcb.conn);

	xcb_shm_attach(xcb.conn, shminfo.shmseg, shminfo.shmid, false);

	image = xcb_image_create_native(xcb.conn, width, height, XCB_IMAGE_FORMAT_Z_PIXMAP,
	                                xcb.screen->root_depth, shminfo.shmaddr, width * height * 4, nullptr);
}

inline XCB::shared_image::~shared_image()
{
	shmdt(shminfo.shmaddr);
	shmctl(shminfo.shmseg, IPC_RMID, 0);
	//xcb_image_destroy(image);
}

inline void XCB::shared_image::update(XCB &xcb)
{
	xcb_shm_get_image_reply(xcb.conn, xcb_shm_get_image_unchecked(
	    xcb.conn,
	    xcb.screen->root,
	    0,
	    0,
	    image->width,
	    image->height,
	    ~0,
	    XCB_IMAGE_FORMAT_Z_PIXMAP,
	    shminfo.shmseg,
	    0), nullptr);
}

inline uint8_t *XCB::shared_image::data()
{
	return image->data;
}

inline uint32_t XCB::shared_image::size()
{
	return image->size;
}

#endif
