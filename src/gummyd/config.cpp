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

#include <fstream>
#include "config.hpp"
#include "syslog.h"

using nlohmann::json;

namespace constants {
const char *config_name = "gummyconf.json";
const char *fifo_name   = "/tmp/gummy.fifo";
const char *lock_name   = "/tmp/gummy.lock";

constexpr int brt_steps_min  = 100;
constexpr int brt_steps_max  = 500;
constexpr int temp_steps_min = 0;
constexpr int temp_steps_max = 500;
constexpr int temp_k_min     = 1000;
constexpr int temp_k_max     = 6500;
}

void config::defaults()
{
	screens.emplace_back(screen());

	time.start               = "06:00";
	time.end                 = "16:00";
	time.adaptation_min      = 60;

	screenshot.offset_perc   = 0;
	screenshot.poll_ms       = 200;
	screenshot.adaptation_ms = 1000;

	als.offset_perc          = 0;
	als.poll_ms              = 1000;
	als.adaptation_ms        = 5000;
}

config::screen::screen()
{
	using namespace constants;
	backlight.mode = MANUAL;
	backlight.val  = brt_steps_max;
	backlight.min  = brt_steps_min;
	backlight.max  = brt_steps_max;

	brightness.mode = MANUAL;
	brightness.val  = brt_steps_max;
	brightness.min  = brt_steps_min;
	brightness.max  = brt_steps_max;

	temperature.mode = MANUAL;
	temperature.val  = temp_k_max;
	temperature.min  = temp_k_min;
	temperature.max  = temp_k_max;
}

config::screen::screen(json in)
{
	backlight.mode = in["bl_mode"];
	backlight.val  = in["bl_perc"];
	backlight.min  = in["bl_min"];
	backlight.max  = in["bl_max"];

	brightness.mode = in["brt_mode"];
	brightness.val  = in["brt_perc"];
	brightness.min  = in["brt_min"];
	brightness.max  = in["brt_max"];

	temperature.mode = in["temp_mode"];
	temperature.val  = in["temp_kelv"];
	temperature.min  = in["temp_min"];
	temperature.max  = in["temp_max"];
}

json config::screen::to_json() const
{
	return {
		{"backlight", {
				{"mode", backlight.mode},
				{"val", backlight.val},
				{"min", backlight.min},
				{"max", backlight.max},
		}},

		{"brightness", {
				{"mode", brightness.mode},
				{"val", brightness.val},
				{"min", brightness.min},
				{"max", brightness.max},
		}},

		{"temperature", {
				{"mode", temperature.mode},
				{"val", temperature.val},
				{"min", temperature.min},
				{"max", temperature.max},
		}},
	};
}

json config::to_json() const
{
	json ret {
		{"screens", json::array()},

		{"time", {
				{"start", time.start},
				{"end", time.end},
				{"adaptation_min", time.adaptation_min},
		}},

		{"screenshot", {
				{"offset_perc", screenshot.offset_perc},
				{"poll_ms", screenshot.poll_ms},
				{"adaptation_ms", screenshot.adaptation_ms},
		}},

		{"als", {
				{"offset_perc", als.offset_perc},
				{"poll_ms", als.poll_ms},
				{"adaptation_ms", als.adaptation_ms}
		}},
	};

	for (const auto &s : screens)
		ret["screens"].emplace_back(s.to_json());

	return ret;
}

void config::screen_diff(size_t scr_no)
{
	// positive: new screens
	// negative: screens removed
	const int screen_diff = scr_no - screens.size();

	if (screen_diff == 0)
		return;

	if (screen_diff > 0) {
		for (int i = 0; i < screen_diff; ++i)
			screens.emplace_back();
	} else {
		for (int i = screen_diff; i < 0; ++i)
			screens.pop_back();
	}

	disk_write(path(constants::config_name));
}

config::config(size_t scr_no)
{
	defaults();
	disk_read(path(constants::config_name));
	screen_diff(scr_no);
}

config::config(json in, size_t scr_no)
{
	defaults();
	from_json(in);
	screen_diff(scr_no);
	disk_write(path(constants::config_name));
}

void config::disk_write(std::string path) const
{
	std::ofstream ofs(path);

	if (ofs.fail()) {
		syslog(LOG_ERR, "disk_write fail\n");
		return;
	}

	try {
		ofs << std::setw(4) << config::to_json();
	} catch (json::exception &e) {
		syslog(LOG_ERR, "%s\n", e.what());
		return;
	}
}

void config::disk_read(std::string path)
{
	std::ifstream fs(path, std::fstream::in | std::fstream::app);

	if (fs.fail()) {
		syslog(LOG_ERR, "disk_read error\n");
		return;
	}

	fs.seekg(0, std::ios::end);

	if (fs.tellg() == 0) {
		disk_write(path);
		return;
	}

	fs.seekg(0);

	json data;

	try {
		fs >> data;
	} catch (json::exception &e) {
		syslog(LOG_ERR, "%s\n", e.what());
		disk_write(path);
		return;
	}

	from_json(data);
}

void config::from_json(json in)
{
	for (const auto &s : in["screens"]) {
		screens.emplace_back(s);
	}

	time.start          = in["time_start"];
	time.end            = in["time_end"];
	time.adaptation_min = in["time_adaptation_min"];

	screenshot.offset_perc   = in["screenshot_offset_perc"];
	screenshot.poll_ms       = in["screenshot_poll_ms"];
	screenshot.adaptation_ms = in["screenshot_adaptation_ms"];

	als.offset_perc   = in["als_offset_perc"];
	als.poll_ms       = in["als_poll_ms"];
	als.adaptation_ms = in["als_adaptation_ms"];
}

std::string config::path(std::string filename)
{
	const char *home   = getenv("XDG_CONFIG_HOME");
	const char *format = "/";

	if (!home) {
		format = "/.config/";
		home = getenv("HOME");
	}

	std::ostringstream ss;
	ss << home << format << filename;
	return ss.str();
}
