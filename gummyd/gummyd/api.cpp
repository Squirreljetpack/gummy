// Copyright 2021-2023 Francesco Fusco
// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdexcept>
#include <limits>
#include <fstream>
#include <unistd.h>

#include <nlohmann/json.hpp>
#include <fmt/core.h>
#include <fmt/std.h>
#include <spdlog/spdlog.h>
#include <gummyd/api.hpp>
#include <gummyd/file.hpp>
#include <gummyd/constants.hpp>

namespace gummyd {

bool daemon_start() {

    if (daemon_is_running())
        return false;

    const pid_t pid = fork();

    if (pid > 0)
        return true;

    if (pid == 0) {
        execl(CMAKE_INSTALL_DAEMON_PATH, "", nullptr);
        throw std::runtime_error(fmt::format("execl({}) fail\n", CMAKE_INSTALL_DAEMON_PATH));
    }

    throw std::runtime_error("fork() fail");
}

bool daemon_stop() {
    if (daemon_is_running()) {
        daemon_send("stop");
        return true;
    }
    return false;
}

bool daemon_is_running() {
    lockfile flock(xdg_runtime_dir() / gummyd::constants::flock_filename, false);
    return flock.locked();
}

void daemon_send(const std::string &s) {
    lockfile flock(xdg_runtime_dir() / gummyd::constants::flock_filename_cli, true);
    file_write(xdg_runtime_dir() / gummyd::constants::fifo_filename, s);
}

std::string daemon_get(std::string_view s) {
    lockfile flock(xdg_runtime_dir() / gummyd::constants::flock_filename_cli, true);
    file_write(xdg_runtime_dir() / gummyd::constants::fifo_filename, s.data());
    return file_read((xdg_runtime_dir() / gummyd::constants::fifo_filename));
}

nlohmann::json config_get_current() {
    std::ifstream ifs(xdg_config_dir() / gummyd::constants::config_filename);
    ifs.exceptions(std::fstream::failbit);
    nlohmann::json ret;
    ifs >> ret;
    return ret;
}

void config_write(nlohmann::json json) {
    std::ofstream fs(xdg_config_dir() / gummyd::constants::config_filename);
    fs.exceptions(std::fstream::failbit);
    fs << std::setw(4) << json;
}

int config_invalid_val() {
    return std::numeric_limits<int>().min();
}

bool config_is_valid(int val) {
    return val > config_invalid_val();
}

bool config_is_valid(double val) {
    return val > config_invalid_val();
}

bool config_is_valid(std::string val) {
    return !val.empty();
}

std::pair<int, int> brightness_range() {
    return {0, gummyd::constants::brt_steps_max};
}

std::pair<int, int> temperature_range() {
    return {gummyd::constants::temp_k_min, gummyd::constants::temp_k_max};
}

}
