/**
* channel: tiny thread concurrency library
*
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

#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <atomic>

namespace fushko {

template <class T>
class channel {
	T _data;
public:
	channel(T data) : _data(data) {};

	T read() const {
		return std::atomic_ref(_data).load();
	}

	T recv(T old) const {
		std::atomic_ref(_data).wait(old);
		return read();
	}

	void send(T in) {
		std::atomic_ref(_data).store(in);
		std::atomic_ref(_data).notify_all();
	}
};

}

#endif //CHANNEL_HPP
