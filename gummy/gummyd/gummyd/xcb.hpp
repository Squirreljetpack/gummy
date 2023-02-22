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

#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <xcb/shm.h>
#include <xcb/xcb_image.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <spdlog/spdlog.h>
#include <gummyd/utils.hpp>

namespace gummyd {
namespace xcb {

inline void throw_if(xcb_generic_error_t *err, std::string err_str) {
	if (err)
		throw std::runtime_error(err_str + " " + std::to_string(err->error_code));
}

inline size_t screen_size(xcb_screen_t *scr) {
	return scr->width_in_pixels * scr->height_in_pixels * scr->root_depth;
}

class connection {
    xcb_connection_t *addr_;
	std::vector<xcb_screen_t*> screens_;

public:
	connection() : addr_(xcb_connect(nullptr, nullptr)) {
		if (const int err = xcb_connection_has_error(addr_) > 0)
			throw std::runtime_error("xcb_connect failed with error " + std::to_string(err));

		auto setup = xcb_get_setup(addr_);
		auto it    = xcb_setup_roots_iterator(setup);

		while (it.rem > 0) {
			screens_.emplace_back(it.data);
			xcb_screen_next(&it);
		}

		if (screens_.empty()) {
			throw std::runtime_error("XCB: no screens found");
		}
	}

	~connection() {
		xcb_disconnect(addr_);
	}

	xcb_connection_t* get() const {
		return addr_;
	}

	xcb_screen_t* first_screen() const {
		return screens_[0];
	}

	bool extension_present(std::string name) const {
		auto query_c = xcb_query_extension(addr_, name.size(), name.c_str());
		auto query_r = c_unique_ptr<xcb_query_extension_reply_t>(xcb_query_extension_reply(addr_, query_c, nullptr));
		return query_r && query_r->present;
	}
};

class shared_memory {
	xcb::connection conn_;
	xcb_shm_segment_info_t shminfo_;
public:
	shared_memory(size_t size) {
		shminfo_.shmseg  = xcb_generate_id(conn_.get());
		shminfo_.shmid   = shmget(IPC_PRIVATE, size, IPC_CREAT | 0600);
		shminfo_.shmaddr = reinterpret_cast<uint8_t*>(shmat(shminfo_.shmid, nullptr, 0));

		auto req = xcb_shm_attach_checked(conn_.get(), shminfo_.shmseg, shminfo_.shmid, 0);
		throw_if(xcb_request_check(conn_.get(), req), "xcb_shm_attach_checked");
	}

	unsigned int seg() {
		return shminfo_.shmseg;
	}

	unsigned int id() {
		return shminfo_.shmid;
	}

	uint8_t* addr() {
		return shminfo_.shmaddr;
	}

	~shared_memory() {
		xcb_request_check(conn_.get(), xcb_shm_detach_checked(conn_.get(), shminfo_.shmseg));
		shmdt(shminfo_.shmaddr);
		shmctl(shminfo_.shmseg, IPC_RMID, 0);
	}
};

class randr
{
public:

    struct output {
	    xcb_randr_crtc_t crtc_id;
		uint16_t width;
		uint16_t height;
		int16_t x;
		int16_t y;
		uint16_t ramp_size;
	};

	std::vector<output> outputs(const connection &conn, xcb_screen_t *screen) {
		xcb_generic_error_t *err;

		auto res_c = xcb_randr_get_screen_resources_current(conn.get(), screen->root);
		auto res_r = c_unique_ptr<xcb_randr_get_screen_resources_current_reply_t> (xcb_randr_get_screen_resources_current_reply(conn.get(), res_c, &err));
		throw_if(err, "xcb_randr_get_screen_resources_current");

		xcb_randr_output_t *outputs = xcb_randr_get_screen_resources_current_outputs(res_r.get());
		std::vector<output> ret;

		for (size_t i = 0; i < res_r.get()->num_outputs; ++i) {
			auto output_c = xcb_randr_get_output_info(conn.get(), outputs[i], XCB_CURRENT_TIME);
			auto output_r = c_unique_ptr<xcb_randr_get_output_info_reply_t>(xcb_randr_get_output_info_reply(conn.get(), output_c, &err));
			throw_if(err, "xcb_randr_get_output_info");

			auto crtc_info_c = xcb_randr_get_crtc_info(conn.get(), output_r->crtc, XCB_CURRENT_TIME);
			auto crtc_info_r = c_unique_ptr<xcb_randr_get_crtc_info_reply_t>(xcb_randr_get_crtc_info_reply(conn.get(), crtc_info_c, nullptr));
			if (!crtc_info_r)
				continue;

			auto gamma_c = xcb_randr_get_crtc_gamma_size(conn.get(), output_r->crtc);
			auto gamma_r = c_unique_ptr<xcb_randr_get_crtc_gamma_size_reply_t>(xcb_randr_get_crtc_gamma_size_reply(conn.get(), gamma_c, &err));
			throw_if(err, "xcb_randr_get_crtc_gamma_size");

			ret.push_back({
			    output_r->crtc,
			    crtc_info_r->width,
			    crtc_info_r->height,
			    crtc_info_r->x,
			    crtc_info_r->y,
			    gamma_r->size,
			});
		}

		return ret;
	}

	void set_gamma(const connection &conn, xcb_randr_crtc_t crtc, const std::vector<uint16_t> &ramps) {
		const size_t sz = ramps.size() / 3;
		auto req = xcb_randr_set_crtc_gamma_checked(conn.get(), crtc, sz,
		&ramps[0 * sz],
		&ramps[1 * sz],
		&ramps[2 * sz]);
		throw_if(xcb_request_check(conn.get(), req), "xcb_randr_set_crtc_gamma_checked");
	}
};

class shared_image
{
    connection conn_;
	shared_memory *shmem_;
	xcb_image_t *image;
public:
	struct buffer {
	    uint8_t *data;
		size_t size;
	};

	shared_image(shared_memory &shmem, unsigned int width, unsigned int height)
	: shmem_(&shmem),
	image(xcb_image_create_native(
	                conn_.get(),
	                width, height,
	                XCB_IMAGE_FORMAT_Z_PIXMAP,
	                conn_.first_screen()->root_depth,
	                shmem_->addr(),
	                screen_size(conn_.first_screen()),
	                nullptr))
	{
	}

	buffer get(int16_t x, int16_t y, uint16_t w, uint16_t h) noexcept {
		auto image_c = xcb_shm_get_image(
		               conn_.get(),
		               conn_.first_screen()->root,
		               x, y,
		               w, h,
		               ~0, XCB_IMAGE_FORMAT_Z_PIXMAP,
		               shmem_->seg(), 0);

		xcb_generic_error_t *err;

		auto image_r = xcb_shm_get_image_reply(conn_.get(), image_c, &err);

		if (!image_r) {
			return { nullptr, err->error_code };
		}

        SPDLOG_TRACE("[XSHM] screenshot: {} * {} | x: {} y: {} size: {}\n", w, h, x, y, image_r->size);

		return {
			image->data,
			image_r->size,
		};
	}

	~shared_image() {
		// fails with "free(): invalid pointer"
		//xcb_image_destroy(image);
	}
};

}
}

#endif // XCB_HPP
