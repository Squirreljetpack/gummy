// Copyright 2021-2024 Francesco Fusco <f.fusco@pm.me>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <string>
#include <functional>
#include <sdbus-c++/IProxy.h>
#include <sdbus-c++/IConnection.h>
#include <sdbus-c++/Types.h>
#include <sdbus-c++/sdbus-c++.h>
#include <spdlog/spdlog.h>
#include <gummyd/sd-dbus.hpp>

namespace gummyd {
namespace dbus {

std::unique_ptr<sdbus::IProxy> register_signal_handler(
    std::string service,
    std::string obj_path,
    std::string interface,
    std::string signal_name,
    std::function<void(sdbus::Signal &signal)> handler) {
    auto proxy = sdbus::createProxy(service, obj_path);
    proxy->registerSignalHandler(interface, signal_name, handler);
    proxy->finishRegistration();
    return proxy;
}

std::unique_ptr<sdbus::IProxy> on_system_sleep(std::function<void(sdbus::Signal &signal)> fn) {
    return dbus::register_signal_handler(
            "org.freedesktop.login1",
            "/org/freedesktop/login1",
            "org.freedesktop.login1.Manager",
            "PrepareForSleep",
            fn);
}

// https://gitlab.gnome.org/GNOME/mutter/-/blob/main/data/dbus-interfaces/org.gnome.Mutter.DisplayConfig.xml
// https://dbus.freedesktop.org/doc/dbus-specification.html#type-system
// d-spy signature: (uint32 serial, a(uxiiiiiuaua{sv}) crtcs, a(uxiausauaua{sv}) outputs, a(uxuudu) modes, int32 max_screen_width, int32 max_screen_height)
// abbreviated:     (ua(uxiiiiiuaua{sv})a(uxiausauaua{sv})a(uxuudu)ii)
// crtcs:           a(ux iiiii u au a{sv})
// outputs:         a(uxi au s au au a{sv})
// modes:           a(uxu udu)
std::vector<mutter::output> mutter::display_config_get_resources() {
    using au   = std::vector<uint32_t>;
    using a_sv = std::map<std::string, sdbus::Variant>;
    using crtc_t   = sdbus::Struct<uint32_t, int64_t, int32_t, int32_t, int32_t, int32_t, int32_t, uint32_t, au, a_sv>;
    using output_t = sdbus::Struct<uint32_t, int64_t, int32_t, au, std::string, au, au, a_sv>;
    using mode_t   = sdbus::Struct<uint32_t, int64_t, uint32_t, uint32_t, double, uint32_t>;

    std::tuple<uint32_t, std::vector<crtc_t>, std::vector<output_t>, std::vector<mode_t>, int32_t, int32_t> reply;

    {
        const std::string destination ("org.gnome.Mutter.DisplayConfig");
        const std::string object_path ("/org/gnome/Mutter/DisplayConfig");
        const std::string interface   ("org.gnome.Mutter.DisplayConfig");
        const std::string method      ("GetResources");
        try {
            auto connection (sdbus::createSessionBusConnection());
            auto proxy (sdbus::createProxy(*connection, destination, object_path));
            proxy->callMethod(method).onInterface(interface).withArguments().storeResultsTo(reply);
        } catch (const sdbus::Error &e) {
            spdlog::error(e.what());
        }
    }

    const auto [serial, crtcs, outputs, modes, max_width, max_height] = reply;

    std::vector<mutter::output> out_vec;

    for (const output_t &output : outputs) {
        const a_sv properties (output.get<7>());
        mutter::output out;
        out.serial = serial;
        out.crtc   = output.get<2>();
        out.name   = properties.at("display-name").get<std::string>();
        const std::vector<uint8_t> edid (properties.at("edid").get<std::vector<uint8_t>>());
        std::copy_n(edid.begin(), 128, out.edid.begin());

        out_vec.push_back(out);
    }

    return out_vec;
}

void mutter::set_gamma(int serial, int crtc, const std::vector<uint16_t> &ramps) {
    const size_t sz = ramps.size() / 3;
    const std::vector<uint16_t> red   (ramps.begin(), ramps.begin() + sz);
    const std::vector<uint16_t> green (ramps.begin() + sz, ramps.begin() + (sz * 2));
    const std::vector<uint16_t> blue  (ramps.begin() + (sz * 2), ramps.end());

    std::tuple<uint32_t, uint32_t, std::vector<uint16_t>, std::vector<uint16_t>, std::vector<uint16_t>> args {
        serial, crtc, red, green, blue
    };

    const std::string destination ("org.gnome.Mutter.DisplayConfig");
    const std::string object_path ("/org/gnome/Mutter/DisplayConfig");
    const std::string interface   ("org.gnome.Mutter.DisplayConfig");
    const std::string method      ("SetCrtcGamma");
    try {
        auto connection (sdbus::createSessionBusConnection());
        auto proxy (sdbus::createProxy(*connection, destination, object_path));
        proxy->callMethod(method).onInterface(interface).withArguments(args);
    } catch (const sdbus::Error &e) {
        spdlog::error(e.what());
    }
}

void test_method_call() {
    const std::string destination ("org.gnome.Mutter.DisplayConfig");
    const std::string object_path ("/org/gnome/Mutter/IdleMonitor");
    const std::string interface   ("org.freedesktop.DBus.ObjectManager");
    const std::string method_str  ("GetManagedObjects");

    auto connection (sdbus::createSessionBusConnection());
    auto proxy      (sdbus::createProxy(*connection, destination, object_path));

    // return type taken straight from the wiki
    // a{oa{sa{sv}}}
    std::map<sdbus::ObjectPath, std::map<std::string, std::map<std::string, sdbus::Variant>>> output;
    proxy->callMethod(method_str).onInterface(interface).storeResultsTo(output);
}

} // namespace dbus
} // namespace gummyd
