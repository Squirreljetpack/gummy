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

class Channel
{
    std::condition_variable cv;
	std::mutex mtx;
	int _data{0};
public:
	int data();
	int recv();
	int recv_timeout(int ms);
	void send(int);
};

inline int Channel::data()
{
	int out;

	{
		std::lock_guard lk(mtx);
		out = _data;
	}

	return out;
}

inline void Channel::send(int data)
{
	{
		std::lock_guard lk(mtx);
		_data = data;
	}

	cv.notify_all();
}

inline int Channel::recv()
{
	int out;

	{
		std::unique_lock lk(mtx);
		cv.wait(lk);
		out = _data;
	}

	return out;
}

inline int Channel::recv_timeout(int ms)
{
	using namespace std::chrono;

	int out;

	{
		std::unique_lock lk(mtx);
		cv.wait_until(lk, system_clock::now() + milliseconds(ms));
		out = _data;
	}

	return out;
}

template <typename T>
class Channel2
{
	std::condition_variable cv;
	std::mutex mtx;
	T _data;
public:
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

#endif
