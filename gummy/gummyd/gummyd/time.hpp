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
#include <stdexcept>

namespace gummyd {
inline time_t timestamp_modify(std::time_t ts, int h, int m, int s)
{
	std::tm tm = *std::localtime(&ts);
	tm.tm_hour = h;
	tm.tm_min  = m;
	tm.tm_sec  = 0;
	tm.tm_sec += s;
	return std::mktime(&tm);
}

inline time_t add_day(std::time_t ts)
{
	std::tm tm = *std::localtime(&ts);
	tm.tm_hour += 24;
	return std::mktime(&tm);
}

inline std::string timestamp_fmt(std::time_t ts)
{
	std::string str(std::asctime(std::localtime(&ts)));
	str.pop_back();
	return str;
}

class time_window
{
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

inline time_window::time_window(std::time_t reference, std::string start, std::string end, int seconds)
{
	_reference = reference;

	_start = timestamp_modify(_reference,
	    std::stoi(start.substr(0, 2)),
	    std::stoi(start.substr(3, 2)),
	    seconds);

	_end = timestamp_modify(_reference,
	    std::stoi(end.substr(0, 2)),
	    std::stoi(end.substr(3, 2)),
	    seconds);

	if (_start > _end) {
		throw std::logic_error("invalid timestamp range");
	}
}

inline void time_window::shift_dates()
{
	_start = add_day(_start);
	_end   = add_day(_end);
}

inline bool time_window::in_range() const
{
	return _reference >= _start && _reference < _end;
}

// If negative, it's the time since the start
inline std::time_t time_window::time_to_start() const
{
	return _start - _reference;
}

// If negative, it's the time since the end
inline std::time_t time_window::time_to_end() const
{
	return _end - _reference;
}

inline std::time_t time_window::time_to_next() const
{
	return in_range() ? time_to_end() : time_to_start();
}

inline std::time_t time_window::time_since_last() const
{
	return in_range() ? time_to_start() : time_to_end();
}

inline void time_window::reference(std::time_t t) {
	_reference = t;
}

inline std::time_t time_window::reference() const {
	return _reference;
}

inline std::time_t time_window::start() const {
	return _start;
}

inline std::time_t time_window::end() const {
	return _end;
}
}
#endif // TIME_HPP
