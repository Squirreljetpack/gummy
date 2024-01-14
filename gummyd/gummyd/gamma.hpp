// Copyright 2021-2024 Francesco Fusco
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef GAMMA_HPP
#define GAMMA_HPP

#include <gummyd/display.hpp>
#include <gummyd/config.hpp>
#include <gummyd/constants.hpp>

namespace gummyd {

class gamma_state {
public:
    struct settings {
        int brightness = gummyd::constants::brt_steps_max;
        int temperature = 6500;
    };

    gamma_state(std::vector<xcb::randr::output>);
    gamma_state(std::vector<xcb::randr::output>, std::vector<gummyd::config::screen> conf);

    void store_brightness(size_t screen_idx, int val);
    void set_brightness(size_t screen_idx, int val);

    void store_temperature(size_t screen_idx, int val);
    void set_temperature(size_t screen_idx, int val);

    void reset_gamma();
    std::vector<settings> get_settings();

private:
    xcb::connection x_connection_;
    std::vector<xcb::randr::output> randr_outputs_;

    std::vector<settings> screen_settings_;
    static settings sanitize(settings);
    void set(size_t screen_idx, settings);
};
}

#endif // GAMMA_HPP
