/**
* gummy
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

#include <array>

#include "gamma.hpp"
#include "config.hpp"
#include "utils.hpp"
#include "channel.hpp"
#include "xorg.hpp"

using namespace constants;

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

	const double idx_lerp = remap_to_idx(val, temp_k_min, temp_k_max, ingo_thies_table.size());
	const size_t idx = std::floor(idx_lerp);
	const double r = lerp(ingo_thies_table[idx][0], ingo_thies_table[idx + 1][0], mant(idx_lerp));
	const double g = lerp(ingo_thies_table[idx][1], ingo_thies_table[idx + 1][1], mant(idx_lerp));
	const double b = lerp(ingo_thies_table[idx][2], ingo_thies_table[idx + 1][2], mant(idx_lerp));
	return std::make_tuple(r, g, b);
}

double calc_brt_mult(int step, size_t ramp_sz)
{
	const int ramp_step = (UINT16_MAX + 1) / ramp_sz;
	return (double(step) / brt_steps_max) * ramp_step;
}

gamma_state::gamma_state(Xorg &xorg, std::vector<config::screen> screens_conf)
{
	_xorg = &xorg;

	_screens.reserve(xorg.scr_count());

	for (size_t i = 0; i < xorg.scr_count(); ++i) {

		int brt  = brt_steps_max;
		int temp = temp_k_max;

		const auto &brt_model  = screens_conf[i].models[config::screen::model_idx::BRIGHTNESS];
		const auto &temp_model = screens_conf[i].models[config::screen::model_idx::TEMPERATURE];

		if (brt_model.mode == config::screen::mode::MANUAL)
			brt = brt_model.val;

		if (temp_model.mode == config::screen::mode::MANUAL)
			temp = temp_model.val;

		set(i, brt, temp);

		_screens.emplace_back(std::make_unique<values>(brt, temp));
	}
}

/**
 * The gamma ramp is a set of unsigned 16-bit values for each of the three color channels.
 * Ramp size varies on different systems.
 * Default values with ramp_sz = 2048 look like this for each channel:
 * [ 0, 32, 64, 96, ... UINT16_MAX - 32 ]
 * So, when ramp_sz = 2048, each value is increased in steps of 32,
 * When ramp_sz = 1024 (usually on iGPUs), it's 64, and so on.
 */
void gamma_state::set(size_t screen_index, int brt_step, int temp_step)
{
	std::vector<uint16_t> ramps(_xorg->ramp_size(screen_index));

	const size_t sz = ramps.size() / 3;
	const double brt_mult = calc_brt_mult(brt_step, sz);
	const auto [r_mult, g_mult, b_mult] = kelvin_to_rgb(temp_step);

	uint16_t *r = &ramps[0 * sz];
	uint16_t *g = &ramps[1 * sz];
	uint16_t *b = &ramps[2 * sz];

	for (size_t i = 0; i < sz; ++i) {
		const int val = std::clamp(int(i * brt_mult), 0, UINT16_MAX);
		r[i] = uint16_t(val * r_mult);
		g[i] = uint16_t(val * g_mult);
		b[i] = uint16_t(val * b_mult);
	}

	_xorg->set_gamma_ramp(screen_index, ramps);
}

void gamma_state::refresh(Channel<> &ch)
{
	while (true) {

		for (size_t i = 0; i < _xorg->scr_count(); ++i)
			set(i, _screens[i]->brightness, _screens[i]->temperature);

		// this return fails if we are still refreshing
		if (ch.recv_timeout(10000) < 0)
			return;
	}
}

void gamma_state::set_temperature(size_t idx, int val)
{
	_screens[idx]->temperature = val;
	set(idx, _screens[idx]->brightness, _screens[idx]->temperature);
}
