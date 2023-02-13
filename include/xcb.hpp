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
#include <memory>

#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <xcb/shm.h>
#include <xcb/xcb_image.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include "utils.hpp"

class xcb_connection {
	xcb_connection_t *addr_;
public:
	xcb_connection() : addr_(xcb_connect(nullptr, nullptr)) {
		if (const int err = xcb_connection_has_error(addr_) > 0)
			throw std::runtime_error("xcb_connect failed with error " + std::to_string(err));
	}

	~xcb_connection() {
		xcb_disconnect(addr_);
	}

	xcb_connection_t* get() const {
		return addr_;
	}
};

class xcb
{
	xcb_connection conn;
	std::vector<std::unique_ptr<xcb_screen_t>> screens;

	void throw_if(xcb_generic_error_t *err, std::string err_str) {
		if (err)
			throw std::runtime_error(err_str + " " + std::to_string(err->error_code));
	}

public:

	struct randr_crtc_data {
		xcb_randr_crtc_t id;
		int num_outputs;
		unsigned int width;
		unsigned int height;
		int ramp_size;
	};

	xcb() {
		auto setup = xcb_get_setup(conn.get());
		auto it    = xcb_setup_roots_iterator(setup);

		while (it.rem > 0) {
			screens.emplace_back(it.data);
			xcb_screen_next(&it);
		}
	}

	bool extension_available(std::string name) {
		auto cookie = xcb_query_extension(conn.get(), name.size(), name.c_str());
		c_unique_ptr<xcb_query_extension_reply_t> reply(xcb_query_extension_reply(conn.get(), cookie, nullptr));
		return reply && reply->present;
	}

	std::vector<xcb::randr_crtc_data> randr_crtcs() {
		xcb_generic_error_t *err;

		auto res_c = xcb_randr_get_screen_resources_current(conn.get(), screens[0]->root);
		auto res_r = c_unique_ptr<xcb_randr_get_screen_resources_current_reply_t> (xcb_randr_get_screen_resources_current_reply(conn.get(), res_c, &err));
		throw_if(err, "xcb_randr_get_screen_resources_current");

		std::vector<randr_crtc_data> ret;
		xcb_randr_crtc_t *crtcs = xcb_randr_get_screen_resources_current_crtcs(res_r.get());

		for (xcb_randr_crtc_t i = 0; i < res_r->num_crtcs; ++i) {

			auto info_c = xcb_randr_get_crtc_info(conn.get(), crtcs[i], 0);
			auto info_r = c_unique_ptr<xcb_randr_get_crtc_info_reply_t>(xcb_randr_get_crtc_info_reply(conn.get(), info_c, &err));
			throw_if(err, "xcb_randr_get_crtc_info");

			auto gamma_c = xcb_randr_get_crtc_gamma_size(conn.get(), crtcs[i]);
			auto gamma_r = c_unique_ptr<xcb_randr_get_crtc_gamma_size_reply_t>(xcb_randr_get_crtc_gamma_size_reply(conn.get(), gamma_c, &err));
			throw_if(err, "xcb_randr_get_crtc_gamma_size");

			ret.emplace_back(crtcs[i],
			info_r->num_outputs,
			info_r->width,
			info_r->height,
			gamma_r->size);
		}

		return ret;
	}

	void randr_set_gamma(xcb_randr_crtc_t crtc, const std::vector<uint16_t> &ramps) {
		const size_t sz = ramps.size() / 3;
		auto req = xcb_randr_set_crtc_gamma_checked(conn.get(), crtc, sz,
		&ramps[0 * sz],
		&ramps[1 * sz],
		&ramps[2 * sz]);
		throw_if(xcb_request_check(conn.get(), req), "randr gamma error");
	}

	class shared_image
	{
	    xcb_image_t *image;
		xcb_shm_segment_info_t shminfo;
	public:
		shared_image(xcb &xcb, unsigned int width, unsigned int height);
		~shared_image();
		void update(xcb &xcb);
		uint8_t *data();
		uint32_t size();
	};
};

inline xcb::shared_image::shared_image(xcb &xcb, unsigned int width, unsigned int height)
{
	shminfo.shmid   = shmget(IPC_PRIVATE, width * height * 4, IPC_CREAT | 0600);
	shminfo.shmaddr = reinterpret_cast<unsigned char*>(shmat(shminfo.shmid, nullptr, 0));
	shminfo.shmseg  = xcb_generate_id(xcb.conn.get());

	xcb_shm_attach(xcb.conn.get(), shminfo.shmseg, shminfo.shmid, false);

	image = xcb_image_create_native(xcb.conn.get(), width, height, XCB_IMAGE_FORMAT_Z_PIXMAP,
	                                xcb.screens[0]->root_depth, shminfo.shmaddr, width * height * 4, nullptr);
}

inline xcb::shared_image::~shared_image()
{
	shmdt(shminfo.shmaddr);
	shmctl(shminfo.shmseg, IPC_RMID, 0);
	//xcb_image_destroy(image);
}

inline void xcb::shared_image::update(xcb &xcb)
{
	xcb_shm_get_image_reply(xcb.conn.get(), xcb_shm_get_image_unchecked(
	    xcb.conn.get(),
	    xcb.screens[0]->root,
	    0,
	    0,
	    image->width,
	    image->height,
	    ~0,
	    XCB_IMAGE_FORMAT_Z_PIXMAP,
	    shminfo.shmseg,
	    0), nullptr);
}

inline uint8_t *xcb::shared_image::data()
{
	return image->data;
}

inline uint32_t xcb::shared_image::size()
{
	return image->size;
}

#endif
