// Copyright (c) 2021-2023, Francesco Fusco. All rights reserved.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <spdlog/spdlog.h>

#include <gummyd/utils.hpp>
#include <gummyd/gamma.hpp>
#include <gummyd/config.hpp>
#include <gummyd/display.hpp>
#include <gummyd/constants.hpp>

using namespace gummyd;

gamma_state::gamma_state(display_server &dsp)
:	dsp_(&dsp),
    screens_(dsp.scr_count())
{}

gamma_state::gamma_state(display_server &dsp, std::vector<config::screen> conf)
:	dsp_(&dsp),
    screens_(dsp.scr_count())
{
	for (size_t i = 0; i < conf.size(); ++i) {
		using enum config::screen::mode;
		using enum config::screen::model_id;
		const auto &brt_model  = conf[i].models[size_t(BRIGHTNESS)];
		const auto &temp_model = conf[i].models[size_t(TEMPERATURE)];
		if (brt_model.mode == MANUAL) {
			screens_[i].brightness = brt_model.val;
		}
		if (temp_model.mode == MANUAL) {
			screens_[i].temperature = temp_model.val;
		}
	}

}

// Color ramp by Ingo Thies.
// From Redshift: https://github.com/jonls/redshift/blob/master/README-colorramp
std::tuple<double, double, double> kelvin_to_rgb(int val)
{
	constexpr std::array<std::array<double, 3>, 56> ingo_thies_table {{
		{1.00000000, 0.18172716, 0.00000000},
		{1.00000000, 0.25503671, 0.00000000},
		{1.00000000, 0.30942099, 0.00000000},
		{1.00000000, 0.35357379, 0.00000000},
		{1.00000000, 0.39091524, 0.00000000},
		{1.00000000, 0.42322816, 0.00000000},
		{1.00000000, 0.45159884, 0.00000000},
		{1.00000000, 0.47675916, 0.00000000},
		{1.00000000, 0.49923747, 0.00000000},
		{1.00000000, 0.51943421, 0.00000000},
		{1.00000000, 0.54360078, 0.08679949},
		{1.00000000, 0.56618736, 0.14065513},
		{1.00000000, 0.58734976, 0.18362641},
		{1.00000000, 0.60724493, 0.22137978},
		{1.00000000, 0.62600248, 0.25591950},
		{1.00000000, 0.64373109, 0.28819679},
		{1.00000000, 0.66052319, 0.31873863},
		{1.00000000, 0.67645822, 0.34786758},
		{1.00000000, 0.69160518, 0.37579588},
		{1.00000000, 0.70602449, 0.40267128},
		{1.00000000, 0.71976951, 0.42860152},
		{1.00000000, 0.73288760, 0.45366838},
		{1.00000000, 0.74542112, 0.47793608},
		{1.00000000, 0.75740814, 0.50145662},
		{1.00000000, 0.76888303, 0.52427322},
		{1.00000000, 0.77987699, 0.54642268},
		{1.00000000, 0.79041843, 0.56793692},
		{1.00000000, 0.80053332, 0.58884417},
		{1.00000000, 0.81024551, 0.60916971},
		{1.00000000, 0.81957693, 0.62893653},
		{1.00000000, 0.82854786, 0.64816570},
		{1.00000000, 0.83717703, 0.66687674},
		{1.00000000, 0.84548188, 0.68508786},
		{1.00000000, 0.85347859, 0.70281616},
		{1.00000000, 0.86118227, 0.72007777},
		{1.00000000, 0.86860704, 0.73688797},
		{1.00000000, 0.87576611, 0.75326132},
		{1.00000000, 0.88267187, 0.76921169},
		{1.00000000, 0.88933596, 0.78475236},
		{1.00000000, 0.89576933, 0.79989606},
		{1.00000000, 0.90198230, 0.81465502},
		{1.00000000, 0.90963069, 0.82838210},
		{1.00000000, 0.91710889, 0.84190889},
		{1.00000000, 0.92441842, 0.85523742},
		{1.00000000, 0.93156127, 0.86836903},
		{1.00000000, 0.93853986, 0.88130458},
		{1.00000000, 0.94535695, 0.89404470},
		{1.00000000, 0.95201559, 0.90658983},
		{1.00000000, 0.95851906, 0.91894041},
		{1.00000000, 0.96487079, 0.93109690},
		{1.00000000, 0.97107439, 0.94305985},
		{1.00000000, 0.97713351, 0.95482993},
		{1.00000000, 0.98305189, 0.96640795},
		{1.00000000, 0.98883326, 0.97779486},
		{1.00000000, 0.99448139, 0.98899179},
		{1.00000000, 1.00000000, 1.00000000},
	}};

    const double idx_lerp = remap_to_idx(val, constants::temp_k_min, constants::temp_k_max, ingo_thies_table.size());
	const size_t idx = std::floor(idx_lerp);
	const double r = lerp(ingo_thies_table[idx][0], ingo_thies_table[idx + 1][0], mant(idx_lerp));
	const double g = lerp(ingo_thies_table[idx][1], ingo_thies_table[idx + 1][1], mant(idx_lerp));
	const double b = lerp(ingo_thies_table[idx][2], ingo_thies_table[idx + 1][2], mant(idx_lerp));
    return {r, g, b};
}

double calc_brt_mult(int step, size_t ramp_sz)
{
	const int ramp_step = (UINT16_MAX + 1) / ramp_sz;
    return (double(step) / constants::brt_steps_max) * ramp_step;
}

/**
 * The gamma ramp is a set of unsigned 16-bit values for each of the three color channels.
 * Ramp size varies on different systems.
 * Default values with ramp_sz = 2048 look like this for each channel:
 * [ 0, 32, 64, 96, ... UINT16_MAX - 32 ]
 * So, when ramp_sz = 2048, each value is increased in steps of 32,
 * When ramp_sz = 1024 (usually on iGPUs), it's 64, and so on.
 */
void gamma_state::apply(size_t screen_index, values vals)
{
	vals = gamma_state::sanitize(vals);
	const size_t sz = dsp_->ramp_size(screen_index);
	std::vector<uint16_t> ramps(sz * 3);

	const double brt_mult = calc_brt_mult(vals.brightness, sz);
	const auto [r_mult, g_mult, b_mult] = kelvin_to_rgb(vals.temperature);

	uint16_t *r = &ramps[0 * sz];
	uint16_t *g = &ramps[1 * sz];
	uint16_t *b = &ramps[2 * sz];

	for (size_t i = 0; i < sz; ++i) {
		const int val = std::min(int(i * brt_mult), UINT16_MAX);
		r[i] = uint16_t(val * r_mult);
		g[i] = uint16_t(val * g_mult);
		b[i] = uint16_t(val * b_mult);
	}

    SPDLOG_TRACE("[screen {}] set_gamma_ramp(brt: {}, temp: {})", screen_index, vals.brightness, vals.temperature);

	dsp_->set_gamma_ramp(screen_index, ramps);
}

gamma_state::values gamma_state::sanitize(values vals) {
    using namespace constants;
	return {
		std::clamp(vals.brightness, brt_steps_min, brt_steps_max),
		std::clamp(vals.temperature, temp_k_min, temp_k_max)
	};
}

void gamma_state::apply_to_all_screens() {
	for (size_t i = 0; i < screens_.size(); ++i) {
        apply(i, std::atomic_ref(screens_[i]).load());
	}
}

void gamma_state::set_brightness(size_t idx, int val) {
	values values = std::atomic_ref(screens_[idx]).load();
	values.brightness = val;
	std::atomic_ref(screens_[idx]).store(values);
}

void gamma_state::set_temperature(size_t idx, int val) {
    values values = std::atomic_ref(screens_[idx]).load();
    values.temperature = val;
    std::atomic_ref(screens_[idx]).store(values);
}

void gamma_state::apply_brightness(size_t idx, int val) {
    set_brightness(idx, val);
    apply(idx, std::atomic_ref(screens_[idx]).load());
}

void gamma_state::apply_temperature(size_t idx, int val) {
    set_temperature(idx, val);
    apply(idx, std::atomic_ref(screens_[idx]).load());
}
