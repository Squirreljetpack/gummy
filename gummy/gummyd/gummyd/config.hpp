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

#include <string>
#include <nlohmann/json_fwd.hpp>

namespace gummy {
class config {

	void defaults();

	void file_parse();
	void screen_diff(size_t scr_no);
	void file_pretty_write() const;

	void from_json(nlohmann::json data);
	nlohmann::json to_json() const;

	std::string filepath_;
public:

    struct time {
        std::string start;
        std::string end;
        int adaptation_minutes;
    } time;

    struct screenshot {
        double scale;
        int poll_ms;
        int adaptation_ms;
    } screenshot;

    struct als {
        double scale;
        int poll_ms;
        int adaptation_ms;
    } als;

	struct screen {

		enum class model_id {
			BACKLIGHT,
			BRIGHTNESS,
			TEMPERATURE
		};

		enum class mode {
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

		std::array<model, 3> models;

		screen();
		screen(nlohmann::json data);
		nlohmann::json to_json() const;
	};
	std::vector<screen> screens;

	config(size_t scr_no);
	config(nlohmann::json data, size_t scr_no);
	size_t clients_for(config::screen::mode);
	size_t clients_for(config::screen::mode, size_t screen_index);
};
}

#endif // CONFIG_HPP
