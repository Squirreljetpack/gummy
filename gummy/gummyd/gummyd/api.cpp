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

#include <stdexcept>
#include <limits>

#include <nlohmann/json.hpp>
#include <fmt/core.h>
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
    try {
        lock_file flock(gummyd::constants::flock_filepath);
        return false;
    } catch (std::runtime_error &e) {
        daemon_send("stop");
        return true;
    }
}

bool daemon_is_running() {
    try {
        lock_file flock(gummyd::constants::flock_filepath);
        return false;
    } catch (std::runtime_error &e) {
        return true;
    }
}

void daemon_send(const std::string &s) {
    file_write(gummyd::constants::fifo_filepath, s);
}

nlohmann::json config_get_current() {
    std::ifstream ifs(xdg_config_filepath(gummyd::constants::config_filename));
    nlohmann::json ret;
    ifs >> ret;
    return ret;
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
