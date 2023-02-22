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
