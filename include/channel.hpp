/**
* Channel: tiny thread concurrency library, loosely Rust-inspired.
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

#ifndef CHANNEL_H
#define CHANNEL_H

#include <atomic>

template <class T>
class channel {
	int _nclients;
	std::atomic_int _nclients_to_notify;
	std::atomic<T> _data;

public:
	channel(int nclients) :
	    _nclients(nclients),
	    _nclients_to_notify(0) {
	}

	channel(const channel &src) :
	    _nclients(src._nclients),
	    _nclients_to_notify(src._nclients_to_notify.load()),
	    _data(src._data.load()) {
	}

	T recv() {
		_nclients_to_notify.wait(0);
		--_nclients_to_notify;
		return _data.load();
	}

	T data() {
		return _data.load();
	}

	void send(T data) {
		_data.store(data);
		_nclients_to_notify.store(_nclients);
		_nclients_to_notify.notify_all();
	}
};

#endif
