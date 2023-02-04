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

#include <fcntl.h>
#include <sys/stat.h>

inline int set_flock(std::string filepath)
{
	const int fd = open(filepath.c_str(), O_WRONLY | O_CREAT, 0640);

	if (fd < 0)
		return fd;

	flock fl;
	fl.l_type   = F_WRLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start  = 0;
	fl.l_len    = 1;

	return fcntl(fd, F_SETLK, &fl);
}

inline int make_fifo(std::string filepath)
{
	return mkfifo(filepath.c_str(), S_IFIFO | 0640);
}

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
