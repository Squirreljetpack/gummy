// Copyright 2021-2024 Francesco Fusco <f.fusco@pm.me>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SD_DBUS_HPP
#define SD_DBUS_HPP

#include <string>
#include <functional>
#include <sdbus-c++/IProxy.h>

namespace gummyd {
namespace dbus {

std::unique_ptr<sdbus::IProxy> register_signal_handler(
    std::string service,
    std::string obj_path,
    std::string interface,
    std::string signal_name,
    std::function<void(sdbus::Signal &signal)> handler);

std::unique_ptr<sdbus::IProxy> on_system_sleep(std::function<void(sdbus::Signal &signal)> fn);

namespace mutter {
struct output {
    uint32_t serial;
    std::array<uint8_t, 128> edid;
    std::string name;
    int crtc;
    size_t ramp_size;
};
std::vector<mutter::output> display_config_get_resources();
size_t get_gamma_ramp_size(sdbus::IConnection&, uint32_t serial, uint32_t crtc);
void   set_gamma(sdbus::IConnection&, uint32_t serial, uint32_t crtc, std::vector<uint16_t> ramps);
} // namespace mutter

void test_method_call();
} // namespace dbus
} // namespace gummyd

#endif // SD_DBUS_HPP

