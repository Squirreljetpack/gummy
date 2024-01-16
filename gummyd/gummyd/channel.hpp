// Copyright 2021-2024 Francesco Fusco <f.fusco@pm.me>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <atomic>

namespace gummyd {

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
