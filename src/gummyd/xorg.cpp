/**
* gummy
* Copyright (C) 2022  Francesco Fusco
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

#include <sys/ipc.h>
#include <sys/shm.h>
#include <syslog.h>

#include "xorg.hpp"
#include "cfg.hpp"
#include "utils.hpp"

// Color ramp by Ingo Thies.
// From Redshift: https://github.com/jonls/redshift/blob/master/README-colorramp
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

Xorg::Xorg()
{
	const std::tuple crtcs = xcb.crtcs();

	for (size_t i = 0; i < std::get<1>(crtcs); ++i) {
		Output o;
		o.crtc = std::get<0>(crtcs)[i];
		o.info = xcb.crtc_data(o.crtc);

		if (o.info->num_outputs > 0)
			outputs.push_back(o);
	}

	for (auto &o : outputs) {

		o.ramps.resize(xcb.gamma_ramp_size(o.crtc) * 3);

		o.image = XShmCreateImage(
		   xlib.dsp,
		   DefaultVisual(xlib.dsp, DefaultScreen(xlib.dsp)),
		   DefaultDepth(xlib.dsp, DefaultScreen(xlib.dsp)),
		   ZPixmap,
		   nullptr,
		   &o.shminfo,
		   o.info->width,
		   o.info->height
		);

		o.image_len     = o.info->width * o.info->height * 4;
		o.shminfo.shmid = shmget(IPC_PRIVATE, o.image_len, IPC_CREAT | 0600);
		void *shm       = shmat(o.shminfo.shmid, nullptr, SHM_RDONLY);

		if (shm == reinterpret_cast<void*>(-1)) {
			syslog(LOG_ERR, "shmat failed");
			exit(1);
		}

		o.shminfo.shmaddr  = o.image->data = reinterpret_cast<char*>(shm);
		o.shminfo.readOnly = False;

		XShmAttach(xlib.dsp, &o.shminfo);
	}
}

int Xorg::screen_brightness(int scr_idx)
{
	Output *o = &outputs[scr_idx];
	XShmGetImage(xlib.dsp, DefaultRootWindow(xlib.dsp), o->image, o->info->x, o->info->y, AllPlanes);
	return calc_brightness(
	    reinterpret_cast<uint8_t*>(o->image->data),
	    o->image_len);
}

void Xorg::set_gamma(int scr_idx, int brt_step, int temp_step)
{
	fill_ramp(outputs[scr_idx],
	                 std::clamp(brt_step, brt_steps_min, brt_steps_max),
	                 temp_step);
}

/**
 * The gamma ramp is a set of unsigned 16-bit values for each of the three color channels.
 * Ramp size varies on different systems.
 * Default values with ramp_sz = 2048 look like this for each channel:
 * [ 0, 32, 64, 96, ... UINT16_MAX - 32 ]
 * So, when ramp_sz = 2048, each value is increased in steps of 32,
 * When ramp_sz = 1024 (usually on iGPUs), it's 64, and so on.
 */
void Xorg::fill_ramp(Output &o, int brt_step, int temp_step)
{
	const size_t sz        = o.ramps.size() / 3;
	const int    ramp_step = (UINT16_MAX + 1) / sz;
	const double brt_mult  = (brt_step / brt_steps_max) * ramp_step;

	const double idx_lerp = remap_to_idx(temp_step, temp_steps_min, temp_steps_max, ingo_thies_table.size());
	const int    idx    = std::floor(idx_lerp);
	const double r_mult = lerp(mant(idx_lerp), ingo_thies_table[idx][0], ingo_thies_table[idx + 1][0]);
	const double g_mult = lerp(mant(idx_lerp), ingo_thies_table[idx][1], ingo_thies_table[idx + 1][1]);
	const double b_mult = lerp(mant(idx_lerp), ingo_thies_table[idx][2], ingo_thies_table[idx + 1][2]);

	uint16_t *r = &o.ramps[0 * sz];
	uint16_t *g = &o.ramps[1 * sz];
	uint16_t *b = &o.ramps[2 * sz];

	for (size_t i = 0; i < sz; ++i) {
		const int val = std::clamp(int(i * brt_mult), 0, UINT16_MAX);
		r[i] = uint16_t(val * r_mult);
		g[i] = uint16_t(val * g_mult);
		b[i] = uint16_t(val * b_mult);
	}

	xcb.set_gamma(o.crtc, o.ramps);
}

size_t Xorg::scr_count() const
{
	return outputs.size();
}
