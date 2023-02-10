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

#ifndef CHANNEL_H
#define CHANNEL_H

#include <vector>
#include <memory>
#include <semaphore>
#include <mutex>
#include <atomic>

namespace fushko {

template <class T>
class channel {
	std::vector<std::unique_ptr<std::binary_semaphore>> clients_;
	std::unique_ptr<std::mutex> mtx_;

	T data_;
	size_t n_clients_;
	bool clients_ready_;
public:
	channel(int nclients) {
		clients_ready_ = false;
		n_clients_ = nclients;
		clients_.reserve(nclients);
		mtx_ = std::make_unique<std::mutex>();
	}

	size_t connect() {
		size_t id;

		mtx_->lock();

		    clients_.emplace_back(std::make_unique<std::binary_semaphore>(0));
			id = clients_.size() - 1;

			if (clients_.size() == n_clients_) {
				std::atomic_ref(clients_ready_).store(true);
				std::atomic_ref(clients_ready_).notify_one();
			}

		mtx_->unlock();

		return id;
	}

	T recv(size_t idx) {
		clients_[idx]->acquire();
		return std::atomic_ref(data_).load();
	}

	T data() {
		return std::atomic_ref(data_).load();
	}

	void send(T data) {
		std::atomic_ref(data_).store(data);

		std::atomic_ref(clients_ready_).wait(false);

		for (const auto &client : clients_) {
			client->release();
		}
	}
};

}

#endif
