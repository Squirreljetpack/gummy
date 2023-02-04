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

#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <vector>
#include <string>

#include "json.hpp"
using nlohmann::json;

namespace constants {
extern const char* config_filename;
extern const char* fifo_filepath;
extern const char* flock_filepath;

extern const int brt_steps_min;
extern const int brt_steps_max;
extern const int temp_steps_min;
extern const int temp_steps_max;
extern const int temp_k_min;
extern const int temp_k_max;
}

std::string xdg_config_path(std::string config_name);

class config {

	void disk_read(std::string path);
	void disk_write(std::string path) const;

	void screen_diff(size_t scr_no);
	void defaults();

	void from_json(nlohmann::json data);
	nlohmann::json to_json() const;
public:

	struct screen {

		enum mode {
			UNINITIALIZED = -1,
			MANUAL,
			SCREENSHOT,
			ALS,
			TIME,
		};

		struct model {
			screen::mode mode;
			int val;
			int min;
			int max;
			model() : mode(UNINITIALIZED), val(-1), min(-1), max(-1) {};
		};

		enum model_idx {
			BACKLIGHT,
			BRIGHTNESS,
			TEMPERATURE
		};
		std::array<model, 3> models;

		screen();
		screen(nlohmann::json data);
		nlohmann::json to_json() const;
	};
	std::vector<screen> screens;

	struct time {
		std::string start;
		std::string end;
		int adaptation_minutes;
		time() : start(""), end(""), adaptation_minutes(-1) {};
	} time;

	struct screenshot {
		int offset_perc;
		int poll_ms;
		int adaptation_ms;
		screenshot() : offset_perc(-1), poll_ms(-1), adaptation_ms(-1) {};
	} screenshot;

	struct als {
		int offset_perc;
		int poll_ms;
		int adaptation_ms;
		als() : offset_perc(-1), poll_ms(-1), adaptation_ms(-1) {};
	} als;

	config(size_t scr_no);
	config(nlohmann::json data, size_t scr_no);
};

#endif // CONFIG_HPP
