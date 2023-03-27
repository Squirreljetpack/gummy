// Copyright 2021-2023 Francesco Fusco
// SPDX-License-Identifier: GPL-3.0-or-later

#include <span>
#include <algorithm>
#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <xcb/shm.h>
#include <xcb/xcb_image.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <spdlog/spdlog.h>
#include <gummyd/x11-xcb.hpp>
#include <gummyd/utils.hpp>

namespace gummyd {
namespace xcb {

void throw_if(xcb_generic_error_t *err, std::string err_str) {
    if (err)
        throw std::runtime_error(err_str + " " + std::to_string(err->error_code));
}

size_t screen_size(xcb_screen_t *scr) {
    return scr->width_in_pixels * scr->height_in_pixels * scr->root_depth;
}

connection::connection() : addr_(xcb_connect(nullptr, nullptr)) {
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

connection::~connection() {
    xcb_disconnect(addr_);
}

xcb_connection_t* connection::get() const {
    return addr_;
}

xcb_screen_t* connection::first_screen() const {
    return screens_[0];
}

bool connection::extension_present(std::string name) const {
    auto query_c = xcb_query_extension(addr_, name.size(), name.c_str());
    auto query_r = c_unique_ptr<xcb_query_extension_reply_t>(xcb_query_extension_reply(addr_, query_c, nullptr));
    return query_r && query_r->present;
}

shared_memory::shared_memory(size_t size) {
    shminfo_.shmseg  = xcb_generate_id(conn_.get());
    shminfo_.shmid   = shmget(IPC_PRIVATE, size, IPC_CREAT | 0600);
    shminfo_.shmaddr = reinterpret_cast<uint8_t*>(shmat(shminfo_.shmid, nullptr, 0));

    auto req = xcb_shm_attach_checked(conn_.get(), shminfo_.shmseg, shminfo_.shmid, 0);
    throw_if(xcb_request_check(conn_.get(), req), "xcb_shm_attach_checked");
}

shared_memory::~shared_memory() {
    xcb_request_check(conn_.get(), xcb_shm_detach_checked(conn_.get(), shminfo_.shmseg));
    shmdt(shminfo_.shmaddr);
    shmctl(shminfo_.shmseg, IPC_RMID, 0);
}

unsigned int shared_memory::seg() const {
    return shminfo_.shmseg;
}

unsigned int shared_memory::id() const {
    return shminfo_.shmid;
}

uint8_t* shared_memory::addr() const {
    return shminfo_.shmaddr;
}

std::vector<randr::output> randr::outputs(const connection &conn, xcb_screen_t *screen) {
    xcb_generic_error_t *err;

    auto res_c = xcb_randr_get_screen_resources_current(conn.get(), screen->root);
    auto res_r = c_unique_ptr<xcb_randr_get_screen_resources_current_reply_t> (xcb_randr_get_screen_resources_current_reply(conn.get(), res_c, &err));
    throw_if(err, "xcb_randr_get_screen_resources_current");

    xcb_randr_output_t *outputs = xcb_randr_get_screen_resources_current_outputs(res_r.get());
    std::vector<output> ret;

    auto atom_c = xcb_intern_atom(conn.get(), 1, strlen("EDID"), "EDID");
    auto atom_r = c_unique_ptr<xcb_intern_atom_reply_t>(xcb_intern_atom_reply(conn.get(), atom_c, &err));
    throw_if(err, "xcb_intern_atom_reply");

    // https://en.wikipedia.org/wiki/Extended_Display_Identification_Data
    static constexpr int edid_base_block_size = 128;

    for (size_t i = 0; i < res_r.get()->num_outputs; ++i) {
        auto output_c = xcb_randr_get_output_info(conn.get(), outputs[i], XCB_CURRENT_TIME);
        auto output_r = c_unique_ptr<xcb_randr_get_output_info_reply_t>(xcb_randr_get_output_info_reply(conn.get(), output_c, &err));
        throw_if(err, "xcb_randr_get_output_info");

        auto crtc_info_c = xcb_randr_get_crtc_info(conn.get(), output_r->crtc, XCB_CURRENT_TIME);
        auto crtc_info_r = c_unique_ptr<xcb_randr_get_crtc_info_reply_t>(xcb_randr_get_crtc_info_reply(conn.get(), crtc_info_c, nullptr));
        if (!crtc_info_r)
            continue;

        const uint8_t *buf = xcb_randr_get_output_info_name(output_r.get());
        const std::string dsp_id(buf, buf + output_r->name_len);
        spdlog::info("[x11] found: {}", dsp_id);

        const std::array<uint8_t, edid_base_block_size> edid = [&] () {
            auto outprop_c = xcb_randr_get_output_property(conn.get(), outputs[i], atom_r->atom, XCB_GET_PROPERTY_TYPE_ANY, 0, edid_base_block_size, 0, 0);
            auto outprop_r = c_unique_ptr<xcb_randr_get_output_property_reply_t>(xcb_randr_get_output_property_reply(conn.get(), outprop_c, &err));
            throw_if(err, "xcb_randr_get_output_property");

            const std::span edid(
                        xcb_randr_get_output_property_data(outprop_r.get()),
                        xcb_randr_get_output_property_data_length(outprop_r.get()));

            const auto log = fmt::format("[x11] {} edid is {} bytes", dsp_id, edid.size());

            std::array<uint8_t, edid_base_block_size> out;
            if (edid.size() >= edid_base_block_size) {
                spdlog::info(log);
                std::copy_n(edid.begin(), edid_base_block_size, out.begin());
            } else {
                spdlog::warn(log);
            }

            return out;
        }();

        auto gamma_c = xcb_randr_get_crtc_gamma_size(conn.get(), output_r->crtc);
        auto gamma_r = c_unique_ptr<xcb_randr_get_crtc_gamma_size_reply_t>(xcb_randr_get_crtc_gamma_size_reply(conn.get(), gamma_c, &err));
        throw_if(err, "xcb_randr_get_crtc_gamma_size");

        ret.push_back({
                          dsp_id,
                          edid,
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

void randr::set_gamma(const connection &conn, xcb_randr_crtc_t crtc, const std::vector<uint16_t> &ramps) {
    const size_t sz = ramps.size() / 3;
    auto req = xcb_randr_set_crtc_gamma_checked(conn.get(), crtc, sz,
                                                &ramps[0 * sz],
            &ramps[1 * sz],
            &ramps[2 * sz]);
    throw_if(xcb_request_check(conn.get(), req), "xcb_randr_set_crtc_gamma_checked");
}

shared_image::shared_image()
    : shmem_(xcb::screen_size(conn_.first_screen())),
      image_(xcb_image_create_native(
                conn_.get(),
                conn_.first_screen()->width_in_pixels,
                conn_.first_screen()->height_in_pixels,
                XCB_IMAGE_FORMAT_Z_PIXMAP,
                conn_.first_screen()->root_depth,
                shmem_.addr(),
                xcb::screen_size(conn_.first_screen()),
                nullptr))
{
}

shared_image::~shared_image() {
    // fails with "free(): invalid pointer".
    // this is not needed, shared memory takes care of it.
    //xcb_image_destroy(image);
}

int shared_image::get(int16_t x, int16_t y, uint16_t w, uint16_t h, std::function<int(std::span<uint8_t>)> fn) {
    std::lock_guard lk(mutex_);
    auto image_c = xcb_shm_get_image(
                conn_.get(),
                conn_.first_screen()->root,
                x, y,
                w, h,
                ~0, XCB_IMAGE_FORMAT_Z_PIXMAP,
                shmem_.seg(), 0);

    xcb_generic_error_t *err;
    auto image_r = xcb_shm_get_image_reply(conn_.get(), image_c, &err);
    if (!image_r) {
        return err ? err->error_code : -1;
    }

    SPDLOG_TRACE("[XSHM] got image: {} * {} | x: {} y: {} size: {}", w, h, x, y, image_r->size);

    return fn(std::span(image_->data, image_r->size));
}

size_t shared_image::size() const {
    return image_->size;
}

} // namespace xcb
} // namespace gummyd
