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

#include "xorg.h"
#include "cfg.h"
#include "../common/defs.h"
#include "../common/utils.h"

#include <sys/ipc.h>
#include <sys/shm.h>
#include <syslog.h>

XLib::XLib()
{
	if (XInitThreads() == 0) {
		syslog(LOG_ERR, "XInitThreads failed");
		exit(1);
	}
	if (!(dsp = XOpenDisplay(nullptr))) {
		syslog(LOG_ERR, "XOpenDisplay failed");
		exit(1);
	}
};

XLib::~XLib()
{
	XCloseDisplay(dsp);
}

XCB::XCB()
    : conn(xcb_connect(nullptr, &pref_screen))
{
	int err = xcb_connection_has_error(conn);
	if (err > 0) {
		syslog(LOG_ERR, "xcb_connect error %d", err);
		exit(1);
	}
	xcb_screen_iterator_t iter = xcb_setup_roots_iterator(xcb_get_setup(conn));
	for (int i = 0; iter.rem > 0; --i) {
		if (i == pref_screen) {
			screen = iter.data;
			break;
		}
		xcb_screen_next(&iter);
	}
}

XCB::~XCB()
{
	xcb_disconnect(conn);
}

Xorg::Xorg()
{
	auto scr_ck   = xcb_randr_get_screen_resources(xcb.conn, xcb.screen->root);
	auto *scr_rpl = xcb_randr_get_screen_resources_reply(xcb.conn, scr_ck, 0);
	xcb_randr_crtc_t *crtcs = xcb_randr_get_screen_resources_crtcs(scr_rpl);
	for (int i = 0; i < scr_rpl->num_crtcs; ++i) {
		Output o;
		o.crtc = crtcs[i];
		auto crtc_info_ck = xcb_randr_get_crtc_info(xcb.conn, o.crtc, 0);
		o.info            = xcb_randr_get_crtc_info_reply(xcb.conn, crtc_info_ck, nullptr);
		if (o.info->num_outputs == 0)
			continue;
		outputs.push_back(o);
	}
	free(scr_rpl);

	for (auto &o : outputs) {
		// gamma
		auto gamma_ck  = xcb_randr_get_crtc_gamma(xcb.conn, o.crtc);
		xcb_generic_error_t *e;
		auto gamma_rpl = xcb_randr_get_crtc_gamma_reply(xcb.conn, gamma_ck, &e);
		if (e) {
			syslog(LOG_ERR, "xcb_randr_get_crtc_gamma_reply error: %d\n", e->error_code);
			exit(1);
		}
		o.ramp_sz = gamma_rpl->size;
		o.ramps.resize(size_t(o.ramp_sz) * 3);
		free(gamma_rpl);

		// couldn't make the following work when refactored into a function
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

int Xorg::get_screen_brightness(int scr_idx)
{
	Output *o = &outputs[scr_idx];
	XShmGetImage(xlib.dsp, DefaultRootWindow(xlib.dsp), o->image, o->info->x, o->info->y, AllPlanes);
	return calc_brightness(
	    reinterpret_cast<uint8_t*>(o->image->data),
	    o->image_len);
}

void Xorg::set_gamma(int scr_idx, int brt_step, int temp_step)
{
	apply_gamma_ramp(outputs[scr_idx],
	                 std::clamp(brt_step, brt_steps_min, brt_steps_max),
	                 temp_step);
}

/**
 * The gamma ramp is a set of unsigned 16-bit values for each color channel.
 * Ramp size varies on different systems.
 * Default values with ramp_sz = 2048 look like this for each channel:
 * [ 0, 32, 64, 96, ... UINT16_MAX - 32 ]
 * So, when ramp_sz = 2048, each value is increased in steps of 32,
 * When ramp_sz = 1024 (usually on iGPUs), it's 64, and so on.
 */
void Xorg::apply_gamma_ramp(Output &o, int brt_step, int temp_step)
{
	uint16_t *r = &o.ramps[0 * o.ramp_sz];
	uint16_t *g = &o.ramps[1 * o.ramp_sz];
	uint16_t *b = &o.ramps[2 * o.ramp_sz];

	const int    ramp_step = (UINT16_MAX + 1) / o.ramp_sz;
	const double brt_mult = invlerp(brt_step, 0, brt_steps_max) * ramp_step;

	const double idx_lerp = remap_to_idx(temp_step, temp_steps_min, temp_steps_max, ingo_thies_table.size());
	const int    idx    = std::floor(idx_lerp);
	const double r_mult = lerp(mant(idx_lerp), ingo_thies_table[idx][0], ingo_thies_table[idx + 1][0]);
	const double g_mult = lerp(mant(idx_lerp), ingo_thies_table[idx][1], ingo_thies_table[idx + 1][1]);
	const double b_mult = lerp(mant(idx_lerp), ingo_thies_table[idx][2], ingo_thies_table[idx + 1][2]);

	for (int i = 0; i < o.ramp_sz; ++i) {
		const int val = std::clamp(int(i * brt_mult), 0, UINT16_MAX);
		r[i] = uint16_t(val * r_mult);
		g[i] = uint16_t(val * g_mult);
		b[i] = uint16_t(val * b_mult);
	}

	auto c = xcb_randr_set_crtc_gamma_checked(xcb.conn, o.crtc, o.ramp_sz, r, g, b);
	xcb_generic_error_t *e = xcb_request_check(xcb.conn, c);
	if (e) {
		syslog(LOG_ERR, "randr set gamma error: %d", int(e->error_code));
	}
}

size_t Xorg::scr_count() const
{
	return outputs.size();
}
