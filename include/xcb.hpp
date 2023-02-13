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

#ifndef XCB_HPP
#define XCB_HPP

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

class xcb_shared_memory {
	xcb_shm_segment_info_t shminfo;
public:
	xcb_shared_memory(const xcb_connection &conn, size_t size) {
		shminfo.shmseg  = xcb_generate_id(conn.get());
		shminfo.shmid   = shmget(IPC_PRIVATE, size, IPC_CREAT | 0600);
		shminfo.shmaddr = reinterpret_cast<uint8_t*>(shmat(shminfo.shmid, nullptr, 0));
		xcb_shm_attach(conn.get(), shminfo.shmseg, shminfo.shmid, 0);
	}

	unsigned int seg() {
		return shminfo.shmseg;
	}

	unsigned int id() {
		return shminfo.shmid;
	}

	uint8_t* addr() {
		return shminfo.shmaddr;
	}

	~xcb_shared_memory() {
		shmdt(shminfo.shmaddr);
		shmctl(shminfo.shmseg, IPC_RMID, 0);
	}
};

class xcb
{
	xcb_connection conn;
	std::vector<xcb_screen_t*> screens;

	void throw_if(xcb_generic_error_t *err, std::string err_str) {
		if (err)
			throw std::runtime_error(err_str + " " + std::to_string(err->error_code));
	}

public:

	xcb() {
		auto setup = xcb_get_setup(conn.get());
		auto it    = xcb_setup_roots_iterator(setup);

		while (it.rem > 0) {
			screens.emplace_back(it.data);
			xcb_screen_next(&it);
		}
	}

	bool extension_present(std::string name) {
		auto query_c = xcb_query_extension(conn.get(), name.size(), name.c_str());
		auto query_r = c_unique_ptr<xcb_query_extension_reply_t>(xcb_query_extension_reply(conn.get(), query_c, nullptr));
		return query_r && query_r->present;
	}

	struct randr_crtc_data {
		xcb_randr_crtc_t id;
		int num_outputs;
		unsigned int width;
		unsigned int height;
		int ramp_size;
	};

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
		xcb_shared_memory shmem;
		xcb_image_t *image;
	public:
		shared_image(xcb &xcb, unsigned int width, unsigned int height)
		    : shmem(xcb_shared_memory(xcb.conn, width * height * 4)),
		      image(xcb_image_create_native(
		                xcb.conn.get(), width, height, XCB_IMAGE_FORMAT_Z_PIXMAP,
		                xcb.screens[0]->root_depth, shmem.addr(), width * height * 4, nullptr))
		{
		}

		void update(xcb &xcb) {
			auto image_c = xcb_shm_get_image_unchecked(
			               xcb.conn.get(),
			               xcb.screens[0]->root,
			               0, 0,
			               image->width,
			               image->height,
			               ~0, XCB_IMAGE_FORMAT_Z_PIXMAP, shmem.seg(), 0);

			xcb_shm_get_image_reply(xcb.conn.get(), image_c, nullptr);
		}

		uint8_t *data() {
			return image->data;
		}

		uint32_t size() {
			return image->size;
		}

		~shared_image() {
			// fails with "free(): invalid pointer"
			//xcb_image_destroy(image);
		}
	};
};
#endif // XCB_HPP
