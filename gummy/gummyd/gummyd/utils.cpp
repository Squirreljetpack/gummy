#include <cmath>
#include <gummyd/utils.hpp>
#include <spdlog/spdlog.h>

namespace gummyd {

inline double invlerp(double val, double min, double max) {
    return (val - min) / (max - min);
}

double lerp(double a, double b, double t) {
    return ((1 - t) * a) + (t * b);
}

double remap(double val, double min, double max, double new_min, double new_max) {
    return lerp(new_min, new_max, invlerp(val, min, max));
}

double mant(double x) {
    return x - std::floor(x);
}

double remap_to_idx(int val, int min, int max, size_t arr_sz) {
    return remap(val, min, max, 0, arr_sz - 1);
}

spdlog::level::level_enum env_log_level() {
    const auto env = getenv("LOG_LEVEL");
    if (!env)
        return spdlog::level::off;

    try {
        return spdlog::level::level_enum(std::stoi(std::string(env)));
    } catch (std::invalid_argument &e) {
        return spdlog::level::off;
    }
}

}
