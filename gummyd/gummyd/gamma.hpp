// Copyright 2021-2024 Francesco Fusco <f.fusco@pm.me>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef GAMMA_HPP
#define GAMMA_HPP

#include <gummyd/display.hpp>
#include <gummyd/config.hpp>
#include <gummyd/constants.hpp>
#include <gummyd/sd-dbus.hpp>

namespace gummyd {

class gamma_state {
public:
    struct settings {
        int brightness;
        int temperature;
    };
    settings default_settings {
        gummyd::constants::brt_steps_max,
        6500
    };

    gamma_state(std::vector<xcb::randr::output>);
    gamma_state(std::vector<dbus::mutter::output>);
    ~gamma_state();
    gamma_state(const gamma_state &) = delete;
    gamma_state(gamma_state &&) = delete;

    void store_brightness(size_t screen_idx, int val);
    void set_brightness(size_t screen_idx, int val);

    void store_temperature(size_t screen_idx, int val);
    void set_temperature(size_t screen_idx, int val);

    void reset_gamma();
    std::vector<settings> get_settings();

private:
    xcb::connection x_connection_;
    std::vector<xcb::randr::output> randr_outputs_;
    std::vector<dbus::mutter::output> mutter_outputs_;
    std::vector<settings> outputs_settings_;

    std::vector<uint16_t> create_ramps(gamma_state::settings settings, size_t sz);
    static gamma_state::settings sanitize(settings);
    void set(size_t screen_idx, settings);
};
}

#endif // GAMMA_HPP
