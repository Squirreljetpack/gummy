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
#include <fstream>
#include <sstream>

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

class named_pipe {
	int fd_;
	std::string filepath_;
public:
	named_pipe(std::string filepath) : filepath_(filepath) {
		fd_ = mkfifo(filepath.c_str(), S_IFIFO | 0640);
		if (fd_ < 0) {
			throw std::runtime_error("mkfifo error");
		}
	}
	~named_pipe() {
		close(fd_);
		unlink(filepath_.c_str());
	}
};

class lock_file {
	std::string filepath_;
	int fd_;
	int fnctl_op_;
public:
	lock_file(std::string filepath)
	: filepath_(filepath),
	  fd_(open(filepath.c_str(), O_WRONLY | O_CREAT, 0640)) {

		if (fd_ < 0) {
			throw std::runtime_error("open() error");
		}

		flock fl;
		fl.l_type   = F_WRLCK;
		fl.l_whence = SEEK_SET;
		fl.l_start  = 0;
		fl.l_len    = 1;
		fnctl_op_   = fcntl(fd_, F_SETLK, &fl);

		if (fnctl_op_ < 0) {
			throw std::runtime_error("set_flock error. Already running?\n");
		}
	}
	~lock_file() {
		close(fd_);

		if (fnctl_op_ > -1) {
			unlink(filepath_.c_str());
		}
	}
};

inline std::string file_read(std::string filepath)
{
	std::ifstream fs(filepath);
	fs.exceptions(std::ifstream::failbit);

	std::ostringstream buf;
	buf << fs.rdbuf();

	return buf.str();
}

inline void file_write(std::string filepath, const std::string &data)
{
	std::ofstream fs(filepath);
	fs.exceptions(std::ifstream::failbit);
	fs.write(data.c_str(), data.size());
}

inline std::string xdg_config_filepath(std::string filename)
{
	const char *home = getenv("XDG_CONFIG_HOME");
	std::string format = "/";

	if (!home) {
		home   = getenv("HOME");
		format = "/.config/";
	}

	std::ostringstream ss;
	ss << home << format << filename;

	return ss.str();
}

#endif // FILE_HPP
