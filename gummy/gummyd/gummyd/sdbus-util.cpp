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
    std::function<void(sdbus::Signal &signal)> handler) {
    auto proxy = sdbus::createProxy(service, obj_path);
    proxy->registerSignalHandler(interface, signal_name, handler);
    proxy->finishRegistration();
    return proxy;
}

std::unique_ptr<sdbus::IProxy> on_system_sleep(std::function<void(sdbus::Signal &signal)> fn) {
    return sdbus_util::register_signal_handler(
            "org.freedesktop.login1",
            "/org/freedesktop/login1",
            "org.freedesktop.login1.Manager",
            "PrepareForSleep",
            fn);
}

}
}
