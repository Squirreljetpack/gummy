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

void throw_if(xcb_generic_error_t *err, std::string err_str);
size_t screen_size(xcb_screen_t *scr);

class connection {
    xcb_connection_t *addr_;
    std::vector<xcb_screen_t*> screens_;
public:
    connection();
    ~connection();
    xcb_connection_t* get() const;
    xcb_screen_t* first_screen() const;
    bool extension_present(std::string name) const;
};

// Copyright (c) 2021-2023, Francesco Fusco. All rights reserved.
// SPDX-License-Identifier: GPL-3.0-or-later

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

class randr {
public:
    struct output {
        xcb_randr_crtc_t crtc_id;
        uint16_t width;
        uint16_t height;
        int16_t x;
        int16_t y;
        uint16_t ramp_size;
    };

    std::vector<output> outputs(const connection &conn, xcb_screen_t *screen);
    void set_gamma(const connection &conn, xcb_randr_crtc_t crtc, const std::vector<uint16_t> &ramps);
};

class shared_image {
    connection conn_;
    shared_memory *shmem_;
    xcb_image_t *image;
public:
    struct buffer {
        uint8_t *data;
        size_t size;
    };
    shared_image(shared_memory &shmem, unsigned int width, unsigned int height);
    ~shared_image();
    buffer get(int16_t x, int16_t y, uint16_t w, uint16_t h);
};

}
}

#endif // XCB_HPP
