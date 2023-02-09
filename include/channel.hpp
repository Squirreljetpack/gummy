/**
* Channel: tiny thread concurrency library.
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

#include <condition_variable>
#include <atomic>

template <typename T = int>
class Channel
{
	std::condition_variable cv;
	std::mutex mtx;
	T _data;
public:
	Channel() : _data({}) {}
	Channel(T data) : _data(data) {}

	T data() {
		return _data;
	}
	T recv() {
		T out;

		{
			std::unique_lock lk(mtx);
			cv.wait(lk);
			out = _data;
		}

		return out;
	}

	T recv_timeout(int ms) {
		using namespace std::chrono;

		T out;

		{
			std::unique_lock lk(mtx);
			cv.wait_until(lk, system_clock::now() + milliseconds(ms));
			out = _data;
		}

		return out;
	};

	void send(T data) {
		{
			std::lock_guard lk(mtx);
			_data = data;
		}

		cv.notify_all();
	}
};

template <class T>

class channel {
	int _nclients;
	std::atomic_int _nclients_to_notify;
	std::atomic<T> _data;

public:
	channel(int nclients) :
	    _nclients(nclients),
	    _nclients_to_notify(0) {
		puts("constructed");
	}

	channel(const channel &src) :
	    _nclients(_nclients),
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

	void update(T data) {
		_data.store(data);
		_nclients_to_notify.store(_nclients);
		_nclients_to_notify.notify_all();
	}
};

#endif
