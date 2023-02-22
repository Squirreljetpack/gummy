// Copyright (c) 2021-2023, Francesco Fusco. All rights reserved.
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef FILE_HPP
#define FILE_HPP

#include <string>
#include <fcntl.h>

namespace gummyd {

class named_pipe {
	int fd_;
	std::string filepath_;
public:
    named_pipe(std::string filepath);
    ~named_pipe();
};

class lock_file {
	std::string filepath_;
	int fd_;
	int fnctl_op_;
	flock fl_;
public:
    lock_file(std::string filepath);
    ~lock_file();
};

std::string file_read(std::string filepath);
void file_write(std::string filepath, const std::string &data);

// https://specifications.freedesktop.org/basedir-spec/0.8/ar01s03.html
std::string xdg_config_filepath(std::string filename);
std::string xdg_state_filepath(std::string filename);
}

#endif // FILE_HPP
