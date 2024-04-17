// Copyright 2021-2024 Francesco Fusco <f.fusco@pm.me>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <spdlog/spdlog.h>
#include <sdbus-c++/IConnection.h>

#include <gummyd/utils.hpp>
#include <gummyd/gamma.hpp>
#include <gummyd/config.hpp>
#include <gummyd/display.hpp>
#include <gummyd/constants.hpp>
#include <gummyd/sd-dbus.hpp>

using namespace gummyd;

gamma_state::gamma_state(const std::vector<xcb::randr::output> &outputs)
  : x_connection_(std::make_unique<xcb::connection>()),
  randr_outputs_(outputs),
  outputs_settings_(outputs.size(), default_settings) {
}

gamma_state::gamma_state(const std::vector<dbus::mutter::output> &outputs)
: dbus_connection_(sdbus::createSessionBusConnection()),
  mutter_outputs_(outputs),
  outputs_settings_(outputs.size(), default_settings) {
}

// Color ramp by Ingo Thies.
// From Redshift: https://github.com/jonls/redshift/blob/master/README-colorramp
std::array<double, 3> kelvin_to_rgb(int val) {
    constexpr size_t nrows (56);
    constexpr size_t ncols (3);
    constexpr std::array<double, nrows * ncols> ingo_thies_table {
            1.00000000, 0.18172716, 0.00000000,
            1.00000000, 0.25503671, 0.00000000,
            1.00000000, 0.30942099, 0.00000000,
            1.00000000, 0.35357379, 0.00000000,
            1.00000000, 0.39091524, 0.00000000,
            1.00000000, 0.42322816, 0.00000000,
            1.00000000, 0.45159884, 0.00000000,
            1.00000000, 0.47675916, 0.00000000,
            1.00000000, 0.49923747, 0.00000000,
            1.00000000, 0.51943421, 0.00000000,
            1.00000000, 0.54360078, 0.08679949,
            1.00000000, 0.56618736, 0.14065513,
            1.00000000, 0.58734976, 0.18362641,
            1.00000000, 0.60724493, 0.22137978,
            1.00000000, 0.62600248, 0.25591950,
            1.00000000, 0.64373109, 0.28819679,
            1.00000000, 0.66052319, 0.31873863,
            1.00000000, 0.67645822, 0.34786758,
            1.00000000, 0.69160518, 0.37579588,
            1.00000000, 0.70602449, 0.40267128,
            1.00000000, 0.71976951, 0.42860152,
            1.00000000, 0.73288760, 0.45366838,
            1.00000000, 0.74542112, 0.47793608,
            1.00000000, 0.75740814, 0.50145662,
            1.00000000, 0.76888303, 0.52427322,
            1.00000000, 0.77987699, 0.54642268,
            1.00000000, 0.79041843, 0.56793692,
            1.00000000, 0.80053332, 0.58884417,
            1.00000000, 0.81024551, 0.60916971,
            1.00000000, 0.81957693, 0.62893653,
            1.00000000, 0.82854786, 0.64816570,
            1.00000000, 0.83717703, 0.66687674,
            1.00000000, 0.84548188, 0.68508786,
            1.00000000, 0.85347859, 0.70281616,
            1.00000000, 0.86118227, 0.72007777,
            1.00000000, 0.86860704, 0.73688797,
            1.00000000, 0.87576611, 0.75326132,
            1.00000000, 0.88267187, 0.76921169,
            1.00000000, 0.88933596, 0.78475236,
            1.00000000, 0.89576933, 0.79989606,
            1.00000000, 0.90198230, 0.81465502,
            1.00000000, 0.90963069, 0.82838210,
            1.00000000, 0.91710889, 0.84190889,
            1.00000000, 0.92441842, 0.85523742,
            1.00000000, 0.93156127, 0.86836903,
            1.00000000, 0.93853986, 0.88130458,
            1.00000000, 0.94535695, 0.89404470,
            1.00000000, 0.95201559, 0.90658983,
            1.00000000, 0.95851906, 0.91894041,
            1.00000000, 0.96487079, 0.93109690,
            1.00000000, 0.97107439, 0.94305985,
            1.00000000, 0.97713351, 0.95482993,
            1.00000000, 0.98305189, 0.96640795,
            1.00000000, 0.98883326, 0.97779486,
            1.00000000, 0.99448139, 0.98899179,
            1.00000000, 1.00000000, 1.00000000,
    };

    const double idx_lerp (remap(val, constants::temp_k_min, constants::temp_k_max, 0, nrows - 1));
    const size_t idx (std::floor(idx_lerp) * ncols);

    if (idx >= ingo_thies_table.size() - ncols) {
        return {1.,1.,1.};
    }

    const size_t idx_next (idx + ncols);
    const double r (lerp(ingo_thies_table[idx + 0], ingo_thies_table[idx_next + 0], mant(idx_lerp)));
    const double g (lerp(ingo_thies_table[idx + 1], ingo_thies_table[idx_next + 1], mant(idx_lerp)));
    const double b (lerp(ingo_thies_table[idx + 2], ingo_thies_table[idx_next + 2], mant(idx_lerp)));
    return {r, g, b};
}

double calc_brt_scale(int step, size_t ramp_sz) {
    const int ramp_step = (UINT16_MAX + 1) / ramp_sz;
    return (double(step) / constants::brt_steps_max) * ramp_step;
}

// The gamma ramp is a set of unsigned 16-bit values for each of the three color channels.
// Ramp size varies on different systems.
// Default values with ramp_sz = 2048 look like this for each channel:
// [ 0, 32, 64, 96, ... UINT16_MAX - 32 ]
// So, when ramp_sz = 2048, each value is increased in steps of 32,
// When ramp_sz = 1024, it's 64, and so on.
std::vector<uint16_t> gamma_state::create_ramps(gamma_state::settings settings, size_t sz) {
    settings = gamma_state::sanitize(settings);

    std::vector<uint16_t> ramps (sz * 3);
    const std::span r (ramps.begin(), sz);
    const std::span g (r.end(), sz);
    const std::span b (g.end(), sz);

    const double brt_scale (calc_brt_scale(settings.brightness, sz));
    const auto   rgb_scale (kelvin_to_rgb(settings.temperature));

    for (size_t i = 0; i < sz; ++i) {
        const int val (std::min(int(i * brt_scale), UINT16_MAX));
        r[i] = uint16_t(val * rgb_scale[0]);
        g[i] = uint16_t(val * rgb_scale[1]);
        b[i] = uint16_t(val * rgb_scale[2]);
    }

    return ramps;
}

void gamma_state::set(size_t screen_index, gamma_state::settings settings) {
    SPDLOG_TRACE("[gamma_state] [screen {}] set(brt: {}, temp: {})", screen_index, settings.brightness, settings.temperature);

    if (mutter_outputs_.size() > 0) {
        return dbus::mutter::set_gamma(
                    *dbus_connection_,
                    mutter_outputs_[screen_index].serial,
                    mutter_outputs_[screen_index].crtc,
                    gamma_state::create_ramps(settings, mutter_outputs_[screen_index].ramp_size)
        );
    }

    if (randr_outputs_.size() > 0) {
        return xcb::randr::set_gamma(*x_connection_,
                                     randr_outputs_[screen_index].crtc_id,
                                     gamma_state::create_ramps(settings, randr_outputs_[screen_index].ramp_size));
    }
}

gamma_state::settings gamma_state::sanitize(settings vals) {
    using namespace constants;
	return {
		std::clamp(vals.brightness, brt_steps_min, brt_steps_max),
		std::clamp(vals.temperature, temp_k_min, temp_k_max)
	};
}

void gamma_state::store_brightness(size_t idx, int val) {
    settings values = std::atomic_ref(outputs_settings_[idx]).load();
    values.brightness = val;
    std::atomic_ref(outputs_settings_[idx]).store(values);
}

void gamma_state::store_temperature(size_t idx, int val) {
    settings values = std::atomic_ref(outputs_settings_[idx]).load();
    values.temperature = val;
    std::atomic_ref(outputs_settings_[idx]).store(values);
}

void gamma_state::set_brightness(size_t idx, int val) {
    store_brightness(idx, val);
    set(idx, std::atomic_ref(outputs_settings_[idx]).load());
}

void gamma_state::set_temperature(size_t idx, int val) {
    store_temperature(idx, val);
    set(idx, std::atomic_ref(outputs_settings_[idx]).load());
}

void gamma_state::reset_gamma() {
    for (size_t i = 0; i < outputs_settings_.size(); ++i) {
        set(i, std::atomic_ref(outputs_settings_[i]).load());
    }
}

std::vector<gamma_state::settings> gamma_state::get_settings() {
    return outputs_settings_;
}

gamma_state::~gamma_state() {
    outputs_settings_.assign(outputs_settings_.size(), default_settings);
    reset_gamma();
}
