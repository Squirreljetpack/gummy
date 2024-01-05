// Copyright 2021-2023 Francesco Fusco
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef API_HPP
#define API_HPP

#include <nlohmann/json_fwd.hpp>
#include <spdlog/spdlog.h>

namespace gummyd {
    bool daemon_start();
    bool daemon_stop();
    bool daemon_is_running();
    void daemon_send(const std::string &s);
    std::string daemon_get(std::string_view s);

    nlohmann::json config_get_current();
    void config_write(nlohmann::json);
    int config_invalid_val();
    bool config_is_valid(int);
    bool config_is_valid(double);
    bool config_is_valid(std::string);

    std::pair<int, int> brightness_range();
    std::pair<int, int> temperature_range();
}

#endif // API_HPP
