#include <stdexcept>
#include <limits>

#include <nlohmann/json.hpp>
#include <fmt/core.h>
#include <gummyd/api.hpp>
#include <gummyd/file.hpp>
#include <gummyd/constants.hpp>

namespace gummy {
namespace api {

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
        lock_file flock(constants::flock_filepath);
        return false;
    } catch (std::runtime_error &e) {
        daemon_send("stop");
        return true;
    }
}

bool daemon_is_running() {
    try {
        lock_file flock(constants::flock_filepath);
        return false;
    } catch (std::runtime_error &e) {
        return true;
    }
}

void daemon_send(const std::string &s) {
    file_write(constants::fifo_filepath, s);
}

nlohmann::json config_get_current() {
    std::ifstream ifs(xdg_config_filepath(constants::config_filename));
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
    return {0, constants::brt_steps_max};
}

std::pair<int, int> temperature_range() {
    return {constants::temp_k_min, constants::temp_k_max};
}

}}

