#include <ctime>
#include <stdexcept>
#include <gummyd/time.hpp>
#include <gummyd/utils.hpp>

namespace gummyd {
time_t timestamp_modify(std::time_t ts, int h, int m, int s) {
    std::tm tm = *std::localtime(&ts);
    tm.tm_hour = h;
    tm.tm_min  = m;
    tm.tm_sec  = 0;
    tm.tm_sec += s;
    return std::mktime(&tm);
}

time_t add_day(std::time_t ts) {
    std::tm tm = *std::localtime(&ts);
    tm.tm_hour += 24;
    return std::mktime(&tm);
}

std::string timestamp_fmt(std::time_t ts) {
    std::string str(std::asctime(std::localtime(&ts)));
    str.pop_back();
    return str;
}

time_window::time_window(std::time_t reference, std::string start, std::string end, int seconds) {
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

void time_window::shift_dates() {
    _start = add_day(_start);
    _end   = add_day(_end);
}

bool time_window::in_range() const {
    return _reference >= _start && _reference < _end;
}

// If negative, it's the time since the start
std::time_t time_window::time_to_start() const {
    return _start - _reference;
}

// If negative, it's the time since the end
std::time_t time_window::time_to_end() const {
    return _end - _reference;
}

std::time_t time_window::time_to_next() const {
    return in_range() ? time_to_end() : time_to_start();
}

std::time_t time_window::time_since_last() const {
    return in_range() ? time_to_start() : time_to_end();
}

void time_window::reference(std::time_t t) {
    _reference = t;
}

std::time_t time_window::reference() const {
    return _reference;
}

std::time_t time_window::start() const {
    return _start;
}

std::time_t time_window::end() const {
    return _end;
}

} // namespace gummyd
