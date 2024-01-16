// Copyright 2021-2024 Francesco Fusco <f.fusco@pm.me>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef X11_XCB_HPP
#define X11_XCB_HPP

#include <span>
#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <xcb/shm.h>
#include <xcb/xcb_image.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <spdlog/spdlog.h>

namespace gummyd {
namespace xcb {

void throw_if(xcb_generic_error_t *err, std::string err_str);
size_t screen_size(xcb_screen_t *scr);

class connection {
    xcb_connection_t *addr_;
    std::vector<xcb_screen_t*> screens_;
public:
    connection();
    ~connection();
    connection(const connection&&) = delete;
    xcb_connection_t* get() const;
    xcb_screen_t* first_screen() const;
    bool extension_present(std::string name) const;
};

class shared_memory {
    xcb::connection conn_;
    xcb_shm_segment_info_t shminfo_;
public:
    shared_memory(size_t size);
    ~shared_memory();
    unsigned int seg() const;
    unsigned int id() const;
    uint8_t* addr() const;
};

namespace randr {
    struct output {
        std::string id;
        std::array<uint8_t, 128> edid;
        xcb_randr_crtc_t crtc_id;
        uint16_t width;
        uint16_t height;
        int16_t x;
        int16_t y;
        uint16_t ramp_size;
    };

    std::vector<output> outputs(const connection &conn, xcb_screen_t *screen);
    void set_gamma(const connection &conn, xcb_randr_crtc_t crtc, const std::vector<uint16_t> &ramps);
} // namespace randr

class shared_image {
    connection conn_;
    shared_memory shmem_;
    xcb_image_t *image_;
    std::mutex mutex_;
public:
    shared_image();
    shared_image(shared_image&&) = delete;
    ~shared_image();
    int get(int16_t x, int16_t y, uint16_t w, uint16_t h, std::function<int(std::span<uint8_t>)> fn);
	size_t size() const;
};

} // namespace xcb
} // namespace gummyd

#endif // X11_XCB_HPP
