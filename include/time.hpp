#ifndef TIME_HPP
#define TIME_HPP

#include <ctime>
#include <string>

inline time_t timestamp_modify(std::time_t ts, int h, int m, int s)
{
	std::tm tm = *std::localtime(&ts);
	tm.tm_hour = h;
	tm.tm_min  = m;
	tm.tm_sec  = 0;
	tm.tm_sec += s;
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
	std::time_t time_since_last() const;
	std::time_t reference() const;
	std::time_t start() const;
	std::time_t end() const;
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

	//printf("ref: %s\n", timestamp_fmt(_reference).c_str());
	//printf("srt: %s\n", timestamp_fmt(_start).c_str());
	//printf("end: %s\n", timestamp_fmt(_end).c_str());
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

inline std::time_t time_window::time_since_last() const
{
	return in_range() ? time_to_start() : time_to_end();
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

#endif // TIME_HPP
