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

#ifndef TIME_HPP
#define TIME_HPP

#include <ctime>
#include <string>

namespace gummyd {

time_t timestamp_modify(std::time_t ts, int h, int m, int s);
time_t add_day(std::time_t ts);
std::string timestamp_fmt(std::time_t ts);

class time_window {
    std::time_t _reference;
	std::time_t _start;
	std::time_t _end;
public:
	time_window(std::time_t cur, std::string start, std::string end, int seconds);
	bool in_range() const;
	std::time_t time_to_start() const;
	std::time_t time_to_end() const;
	std::time_t time_to_next() const;
	std::time_t time_since_last() const;
	std::time_t reference() const;
	std::time_t start() const;
	std::time_t end() const;

	void reference(std::time_t t);
	void shift_dates();
};

}
#endif // TIME_HPP
