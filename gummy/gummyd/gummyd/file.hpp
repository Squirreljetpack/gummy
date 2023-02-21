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
#include <filesystem>
#include <sstream>

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <fmt/core.h>

namespace gummyd {

class named_pipe {
	int fd_;
	std::string filepath_;
public:
	named_pipe(std::string filepath) : filepath_(filepath) {
		fd_ = mkfifo(filepath.c_str(), S_IFIFO | 0640);
		if (fd_ < 0) {
			//LOG_FMT_("named_pipe: mkfifo error");
		}
	}
	~named_pipe() {
		close(fd_);
		std::filesystem::remove(filepath_);
	}
};

class lock_file {
	std::string filepath_;
	int fd_;
	int fnctl_op_;
	flock fl_;
public:
	lock_file(std::string filepath)
	: filepath_(filepath),
	  fd_(open(filepath.c_str(), O_WRONLY | O_CREAT, 0640)) {

		if (fd_ < 0) {
			throw std::runtime_error("open() error");
		}

		fl_.l_type   = F_WRLCK;
		fl_.l_whence = SEEK_SET;
		fl_.l_start  = 0;
		fl_.l_len    = 1;
		fnctl_op_    = fcntl(fd_, F_SETLK, &fl_);

		if (fnctl_op_ < 0) {
			throw std::runtime_error("set_flock error. Already running?\n");
		}
	}
	~lock_file() {
		if (fnctl_op_ > 0)
			fcntl(fd_, F_UNLCK, &fl_);
		close(fd_);
		std::filesystem::remove(filepath_);
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

// https://specifications.freedesktop.org/basedir-spec/0.8/ar01s03.html
// $XDG_STATE_HOME defines the base directory relative to which user-specific state files should be stored.
// If $XDG_STATE_HOME is either not set or empty, a default equal to $HOME/.local/state should be used.
// The $XDG_STATE_HOME contains state data that should persist between (application) restarts,
// but that is not important or portable enough to the user that it should be stored in $XDG_DATA_HOME. It may contain:
// - actions history (logs, history, recently used files, …)
// - current state of the application that can be reused on a restart (view, layout, open files, undo history, …)
inline std::string xdg_state_filepath(std::string filename) {

    constexpr std::array<std::array<std::string_view, 2>, 2> env_vars {{
        {"XDG_STATE_HOME", ""},
        {"HOME", "/.local/state"}
    }};

    for (const auto &var : env_vars) {
        if (auto env = getenv(var[0].data()))
            return fmt::format("{}{}/{}", env, var[1], filename);
    }

    throw std::runtime_error("HOME env not found!");
}

}

#endif // FILE_HPP
