// Copyright 2021-2023 Francesco Fusco
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SDBUS_UTIL_HPP
#define SDBUS_UTIL_HPP

#include <string>
#include <functional>
#include <sdbus-c++/IProxy.h>

namespace gummyd {
namespace sdbus_util {

std::unique_ptr<sdbus::IProxy> register_signal_handler(
    std::string service,
    std::string obj_path,
    std::string interface,
    std::string signal_name,
    std::function<void(sdbus::Signal &signal)> handler);

std::unique_ptr<sdbus::IProxy> on_system_sleep(std::function<void(sdbus::Signal &signal)> fn);

}
}

#endif // SDBUS_UTIL_HPP
