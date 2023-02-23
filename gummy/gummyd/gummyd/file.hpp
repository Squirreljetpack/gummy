// Copyright (c) 2021-2023, Francesco Fusco. All rights reserved.
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef FILE_HPP
#define FILE_HPP

#include <string>
#include <string_view>
#include <filesystem>
#include <fcntl.h>

namespace gummyd {

class named_pipe {
	int fd_;
    std::filesystem::path filepath_;
public:
    named_pipe(std::filesystem::path filepath);
    ~named_pipe();
};

class lockfile {
    std::filesystem::path filepath_;
	int fd_;
	int fnctl_op_;
	flock fl_;
public:
    lockfile(std::filesystem::path filepath);
    ~lockfile();
};

std::string file_read(std::filesystem::path filepath);
void file_write(std::filesystem::path filepath, const std::string &data);

std::string env(std::string_view var);

// https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html
std::filesystem::path xdg_config_dir();
std::filesystem::path xdg_state_dir();
std::filesystem::path xdg_runtime_dir();
}

#endif // FILE_HPP
