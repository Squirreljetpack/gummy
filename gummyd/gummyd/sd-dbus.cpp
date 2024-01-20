// Copyright 2021-2024 Francesco Fusco <f.fusco@pm.me>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <string>
#include <functional>
#include <span>

#include <spdlog/spdlog.h>
#include <sdbus-c++/sdbus-c++.h>
#include <gummyd/sd-dbus.hpp>

// std::span specialization for sdbus-c++ < v1.3.0.
namespace sdbus {

// serialization
template <typename _ElementType>
sdbus::Message& operator<<(sdbus::Message& msg, const std::span<_ElementType>& items) {
    msg.openContainer(sdbus::signature_of<_ElementType>::str());
    for (const auto& item : items) {
        msg << item;
    }
    msg.closeContainer();
    return msg;
}

// deserialization
template <typename _ElementType>
sdbus::Message& operator>>(sdbus::Message& msg, std::span<_ElementType>& items) {
    if (!msg.enterContainer(sdbus::signature_of<_ElementType>::str())) {
        return msg;
    }
    while (true) {
        _ElementType elem;
        if (msg >> elem) {
            items.emplace_back(std::move(elem));
        } else {
            break;
        }
    }
    msg.clearFlags();
    msg.exitContainer();
    return msg;
}
} // namespace sdbus

// signature
template <typename _Element, std::size_t _Extent>
struct sdbus::signature_of<std::span<_Element, _Extent>> : sdbus::signature_of<std::vector<_Element>>{};

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

size_t mutter::get_gamma_ramp_size(sdbus::IConnection &conn, uint32_t serial, uint32_t crtc) {
    std::tuple<std::vector<uint16_t>, std::vector<uint16_t>, std::vector<uint16_t>> reply;
    {
        const std::string destination ("org.gnome.Mutter.DisplayConfig");
        const std::string object_path ("/org/gnome/Mutter/DisplayConfig");
        const std::string interface   ("org.gnome.Mutter.DisplayConfig");
        const std::string method      ("GetCrtcGamma");
        try {
            auto proxy (sdbus::createProxy(conn, destination, object_path));
            proxy->callMethod(method).onInterface(interface).withArguments(serial, crtc).storeResultsTo(reply);
        } catch (const sdbus::Error &e) {
            spdlog::error(e.what());
        }
    }
    const auto [r, g, b] = reply;
    return r.size();
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

    const auto connection (sdbus::createSessionBusConnection());

    {
        const std::string destination ("org.gnome.Mutter.DisplayConfig");
        const std::string object_path ("/org/gnome/Mutter/DisplayConfig");
        const std::string interface   ("org.gnome.Mutter.DisplayConfig");
        const std::string method      ("GetResources");
        try {
            auto proxy (sdbus::createProxy(*connection, destination, object_path));
            proxy->callMethod(method).onInterface(interface).withArguments().storeResultsTo(reply);
        } catch (const sdbus::Error &e) {
            spdlog::error(e.what());
            return {};
        }
    }

    const auto [serial, crtcs, outputs, modes, max_width, max_height] = reply;

    std::vector<mutter::output> out_vec;

    for (const output_t &output : outputs) {
        const a_sv &properties (output.get<7>());
        const std::vector<uint8_t> &edid (properties.at("edid").get<std::vector<uint8_t>>());

        mutter::output out;
        out.serial     = serial;
        out.crtc       = output.get<2>();
        out.ramp_size  = mutter::get_gamma_ramp_size(*connection, out.serial, out.crtc);
        out.name       = properties.at("display-name").get<std::string>();
        std::copy_n(edid.begin(), 128, out.edid.begin());

        out_vec.push_back(out);
    }

    return out_vec;
}

void mutter::set_gamma(sdbus::IConnection &conn, uint32_t serial, uint32_t crtc, std::vector<uint16_t> ramps) {
    const std::string destination ("org.gnome.Mutter.DisplayConfig");
    const std::string object_path ("/org/gnome/Mutter/DisplayConfig");
    const std::string interface   ("org.gnome.Mutter.DisplayConfig");
    const std::string method      ("SetCrtcGamma");

    const size_t sz (ramps.size() / 3);
    const std::span r (ramps.begin(), sz);
    const std::span g (r.end(), sz);
    const std::span b (g.end(), sz);

    try {
        const auto proxy (sdbus::createProxy(conn, destination, object_path));
        proxy->callMethod(method).onInterface(interface).withArguments(std::tuple{serial, crtc, r, g, b});
    } catch (const sdbus::Error &e) {
        spdlog::error("[mutter] [set_gamma] {} ", e.what());
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
