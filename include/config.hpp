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

#include <vector>
#include <string>
#include <json.hpp>

#ifndef CONFIG_HPP
#define CONFIG_HPP

namespace constants {
extern const char* config_name;
extern const char* fifo_name;
extern const char* lock_name;

extern const int brt_steps_min;
extern const int brt_steps_max;
extern const int temp_steps_min;
extern const int temp_steps_max;
extern const int temp_k_min;
extern const int temp_k_max;
}

class config {

	class screen {
		enum mode {
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
		};
		model backlight;
		model brightness;
		model temperature;
	public:
		screen();
		screen(nlohmann::json data);
		nlohmann::json to_json() const;
	};

	std::vector<screen> screens;

	struct {
		std::string start;
		std::string end;
		int adaptation_min;
	} time;

	struct {
		int offset_perc;
		int poll_ms;
		int adaptation_ms;
	} screenshot;

	struct {
		int offset_perc;
		int poll_ms;
		int adaptation_ms;
	} als;

	static std::string path(std::string config_name);

	void disk_read(std::string path);
	void disk_write(std::string path) const;

	void screen_diff(size_t scr_no);
	void defaults();

	void from_json(nlohmann::json data);
	nlohmann::json to_json() const;
public:
	config(size_t scr_no);
	config(nlohmann::json data, size_t scr_no);
};

#endif // CONFIG_HPP
