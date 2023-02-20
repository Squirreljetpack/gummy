#ifndef API_HPP
#define API_HPP

#include <nlohmann/json_fwd.hpp>
#include <string_view>

namespace gummy {
namespace api {
    bool daemon_start();
    bool daemon_stop();
    bool daemon_is_running();
    void daemon_send(const std::string &s);

    nlohmann::json config_get_current();
    int config_invalid_val();
    bool config_is_valid(int);
    bool config_is_valid(double);
    bool config_is_valid(std::string);

    std::pair<int, int> brightness_range();
    std::pair<int, int> temperature_range();
}
}

#endif // API_HPP
