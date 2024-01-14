// Copyright 2021-2024 Francesco Fusco
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef API_HPP
#define API_HPP

#include <utility>
#include <functional>
#include <nlohmann/json_fwd.hpp>

namespace gummyd {
    // Get current configuration from disk.
    nlohmann::json config_get();

    // Write new configuration to disk. Daemon will read once started.
    void config_write(nlohmann::json&);

    // Returns false if it's already started.
    bool daemon_start();

    // Returns false if it's already stopped.
    bool daemon_stop();

    // Check if it's running.
    bool daemon_is_running();

    // Get current settings for each screen.
    nlohmann::json daemon_screen_status();

    // Update daemon configuration.
    void daemon_send_config(nlohmann::json&);

    // Get min/max values for brightness and temperature.
    std::pair<int, int> brightness_range();
    std::pair<int, int> temperature_range();

    // Convert backlight/brightness percentage to the format used by the daemon.
    extern std::function<int(int)> brightness_perc_to_step;
}

#endif // API_HPP
