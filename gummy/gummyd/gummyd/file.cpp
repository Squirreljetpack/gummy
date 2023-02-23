// Copyright (c) 2021-2023, Francesco Fusco. All rights reserved.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <fstream>
#include <filesystem>
#include <string>
#include <string_view>
#include <sstream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <fmt/core.h>
#include <spdlog/spdlog.h>
#include <gummyd/file.hpp>

namespace gummyd {

named_pipe::named_pipe(std::filesystem::path filepath) : filepath_(filepath) {
    fd_ = mkfifo(filepath_.c_str(), S_IFIFO | 0640);
    if (fd_ < 0) {
        spdlog::error("[named_pipe] mkfifo error");
    }
}

named_pipe::~named_pipe() {
    close(fd_);
    std::filesystem::remove(filepath_);
}

lockfile::lockfile(std::filesystem::path filepath)
    : filepath_(filepath),
      fd_(open(filepath_.c_str(), O_WRONLY | O_CREAT, 0640)) {

    if (fd_ < 0) {
        throw std::runtime_error("[lockfile] open() failed");
    }

    fl_.l_type   = F_WRLCK;
    fl_.l_whence = SEEK_SET;
    fl_.l_start  = 0;
    fl_.l_len    = 1;
    fnctl_op_    = fcntl(fd_, F_SETLK, &fl_);

    if (fnctl_op_ < 0) {
        throw std::runtime_error("[lockfile] F_SETLK failed");
    }
}

lockfile::~lockfile() {
    if (fnctl_op_ > 0)
        fcntl(fd_, F_UNLCK, &fl_);
    close(fd_);
    std::filesystem::remove(filepath_);
}

std::string file_read(std::filesystem::path filepath) {
    std::ifstream fs(filepath);
    fs.exceptions(std::ifstream::failbit);

    std::ostringstream buf;
    buf << fs.rdbuf();

    return buf.str();
}

void file_write(std::filesystem::path filepath, const std::string &data) {
    std::ofstream fs(filepath);
    fs.exceptions(std::ifstream::failbit);
    fs.write(data.c_str(), data.size());
}

std::string env(std::string_view var) {
    const auto s = std::getenv(var.data());
    return s ? s : "";
}

std::filesystem::path xdg_config_dir() {
    constexpr std::array<std::array<std::string_view, 2>, 2> env_vars {{
        {"XDG_CONFIG_HOME", ""},
        {"HOME", "/.config"}
    }};

    std::filesystem::path ret;

    for (const auto &arr : env_vars) {
        const std::string env_var = env(arr[0]);
        if (!env_var.empty()) {
            ret = fmt::format("{}{}", env_var, arr[1]);
            break;
        }
    }

    if (ret.is_relative())
        throw std::runtime_error("xdg_config_dir should be absolute");

    return ret;
}

std::filesystem::path xdg_state_dir() {
    constexpr std::array<std::array<std::string_view, 2>, 2> env_vars {{
        {"XDG_STATE_HOME", ""},
        {"HOME", "/.local/state"}
    }};

    std::filesystem::path ret;

    for (const auto &arr : env_vars) {
        const std::string env_var = env(arr[0]);
        if (!env_var.empty()) {
            ret = fmt::format("{}{}", env_var, arr[1]);
            break;
        }
    }

    if (ret.is_relative())
        throw std::runtime_error("xdg_state_dir should be absolute");

    return ret;
}

std::filesystem::path xdg_runtime_dir() {
    constexpr std::array<std::array<std::string_view, 2>, 2> env_vars {{
        {"XDG_RUNTIME_DIR", ""},
        {"", "/var/run"}
    }};

    std::filesystem::path ret;

    for (const auto &arr : env_vars) {
        if (arr[0].empty()) {
            ret = arr[1];
            break;
        }

        const std::string env_var = env(arr[0]);
        if (!env_var.empty()) {
            ret = fmt::format("{}{}", env_var, arr[1]);
            break;
        }
    }

    if (ret.is_relative())
        throw std::runtime_error("xdg_runtime_dir should be absolute");

    return ret;
}

} // namespace gummyd
