// Copyright 2021-2024 Francesco Fusco <f.fusco@pm.me>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <functional>
#include <stdexcept>
#include <fstream>
#include <utility>
#include <unistd.h>

#include <nlohmann/json.hpp>
#include <fmt/core.h>
#include <gummyd/api.hpp>
#include <gummyd/file.hpp>
#include <gummyd/constants.hpp>
#include <gummyd/utils.hpp>

namespace gummyd {

namespace {
void _daemon_send(std::string_view data) {
    file_write(xdg_runtime_dir() / gummyd::constants::fifo_filename, data);
}

std::string _daemon_get(std::string_view data) {
    _daemon_send(data);
    return file_read(xdg_runtime_dir() / gummyd::constants::fifo_filename);
}
}

bool daemon_start() {
    if (daemon_is_running()) {
        return false;
    }

    const pid_t pid = fork();

    if (pid > 0) {
        return true;
    }

    if (pid == 0) {
        execl(CMAKE_INSTALL_DAEMON_PATH, "", nullptr);
        throw std::runtime_error(fmt::format("execl({}) fail\n", CMAKE_INSTALL_DAEMON_PATH));
    }

    throw std::runtime_error("fork() fail");
}

bool daemon_stop() {
    if (daemon_is_running()) {
        _daemon_send("stop");
        return true;
    }
    return false;
}

void daemon_reset() {
    if (daemon_is_running()) {
        _daemon_send("reset");
    }
}

nlohmann::json daemon_screen_status() {
    return nlohmann::json::from_cbor(_daemon_get("status"));
}

bool daemon_is_running() {
    lockfile flock(xdg_runtime_dir() / gummyd::constants::flock_filename, F_SETLK);
    return flock.locked();
}

nlohmann::json config_get() {
    std::ifstream ifs(xdg_config_dir() / gummyd::constants::config_filename);
    ifs.exceptions(std::fstream::failbit);
    nlohmann::json ret;
    ifs >> ret;
    return ret;
}

void config_write(nlohmann::json &json) {
    std::ofstream fs(xdg_config_dir() / gummyd::constants::config_filename);
    fs.exceptions(std::fstream::failbit);
    fs << std::setw(4) << json;
}

void daemon_send_config(nlohmann::json &json) {
    _daemon_send(json.dump());
}

std::pair<int, int> brightness_range() {
    return {0, gummyd::constants::brt_steps_max};
}

std::pair<int, int> temperature_range() {
    return {gummyd::constants::temp_k_min, gummyd::constants::temp_k_max};
}

std::function<int(int)> brightness_perc_to_step = [] (int perc) {
    const std::pair<int, int> brt_range_real = gummyd::brightness_range();
    const std::pair<int, int> brt_perc_range = {0, brt_range_real.second / 10};

    const int abs_val     = std::abs(perc);
    const int clamped_val = std::clamp(abs_val, brt_perc_range.first, brt_perc_range.second);
    const int out_val     = gummyd::remap(clamped_val,
                                          brt_perc_range.first, brt_perc_range.second,
                                          brt_range_real.first, brt_range_real.second);
    return perc >= 0 ? out_val : -out_val;
};

}
