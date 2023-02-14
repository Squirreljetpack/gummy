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
#include <limits>

#include "json.hpp"

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

class config {

	void defaults();

	void file_parse();
	void screen_diff(size_t scr_no);
	void file_pretty_write() const;

	void from_json(nlohmann::json data);
	nlohmann::json to_json() const;

	std::string filepath;
public:

	struct screen {

		enum mode {
			UNINITIALIZED = std::numeric_limits<int>::min(),
			MANUAL = 0,
			SCREENSHOT,
			ALS,
			TIME,
		};

		struct model {
			screen::mode mode;
			int val;
			int min;
			int max;

			model() :
			mode(UNINITIALIZED),
			val(std::numeric_limits<int>::min()),
			min(std::numeric_limits<int>::min()),
			max(std::numeric_limits<int>::min()) {};
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

		time() :
		start(""),
		end(""),
		adaptation_minutes(std::numeric_limits<int>::min()) {};
	} time;

	struct screenshot {
		double scale;
		int poll_ms;
		int adaptation_ms;

		screenshot() :
		scale(std::numeric_limits<int>::min()),
		poll_ms(std::numeric_limits<int>::min()),
		adaptation_ms(std::numeric_limits<int>::min()) {};
	} screenshot;

	struct als {
	    double scale;
		int poll_ms;
		int adaptation_ms;

		als() :
		scale(std::numeric_limits<double>::min()),
		poll_ms(std::numeric_limits<int>::min()),
		adaptation_ms(std::numeric_limits<int>::min()) {};
	} als;

	config(size_t scr_no);
	config(nlohmann::json data, size_t scr_no);
	size_t clients_for(config::screen::mode);
	size_t clients_for(config::screen::mode, size_t screen_index);

	static bool valid_f64(double val);
	static bool valid_int(int val);
};

#endif // CONFIG_HPP
