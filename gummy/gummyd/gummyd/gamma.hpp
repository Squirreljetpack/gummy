// Copyright (c) 2021-2023, Francesco Fusco. All rights reserved.
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef GAMMA_HPP
#define GAMMA_HPP

#include <gummyd/display.hpp>
#include <gummyd/config.hpp>
#include <gummyd/constants.hpp>

namespace gummyd {
class gamma_state {
	struct values {
        int brightness = gummyd::constants::brt_steps_max;
		int temperature = 6500;
	};

	display_server *dsp_;
	std::vector<values> screens_;

	static values sanitize(values);
    void apply(size_t screen_idx, values);
public:
	gamma_state(display_server &dsp);
    gamma_state(display_server &dsp, std::vector<gummyd::config::screen> conf);

    void set_brightness(size_t screen_idx, int val);
    void apply_brightness(size_t screen_idx, int val);

    void set_temperature(size_t screen_idx, int val);
    void apply_temperature(size_t screen_idx, int val);

    void apply_to_all_screens();
};
}

#endif // GAMMA_HPP
