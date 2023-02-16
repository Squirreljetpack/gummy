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
#include "json.hpp"
#include "file.hpp"

using nlohmann::json;

namespace constants {
const char *config_filename = "gummyconf.json";
const char *fifo_filepath   = "/tmp/gummy.fifo";
const char *flock_filepath  = "/tmp/gummy.lock";

constexpr int brt_steps_min  = 200;
constexpr int brt_steps_max  = 1000;
constexpr int temp_k_min     = 1000;
constexpr int temp_k_max     = 6500;
}

void config::defaults()
{
	filepath = xdg_config_filepath(constants::config_filename);

	time.start               = "06:00";
	time.end                 = "16:00";
	time.adaptation_minutes  = 60;

	screenshot.scale         = 1.0;
	screenshot.poll_ms       = 200;
	screenshot.adaptation_ms = 5000;

	als.scale                = 1.0;
	als.poll_ms              = 5000;
	als.adaptation_ms        = 5000;
}

config::screen::screen()
{
	using namespace constants;

	auto &backlight = models[BACKLIGHT];
	backlight.mode = MANUAL;
	backlight.val  = brt_steps_max;
	backlight.min  = 500;
	backlight.max  = brt_steps_max;

	auto &brightness = models[BRIGHTNESS];
	brightness.mode = MANUAL;
	brightness.val  = brt_steps_max;
	brightness.min  = 500;
	brightness.max  = brt_steps_max;

	auto &temperature = models[TEMPERATURE];
	temperature.mode = MANUAL;
	temperature.val  = temp_k_max;
	temperature.min  = 3400;
	temperature.max  = temp_k_max;
}

config::screen::screen(json in)
{
	auto &backlight = models[BACKLIGHT];
	backlight.mode = in["backlight"]["mode"];
	backlight.val  = in["backlight"]["val"];
	backlight.min  = in["backlight"]["min"];
	backlight.max  = in["backlight"]["max"];

	auto &brightness = models[BRIGHTNESS];
	brightness.mode = in["brightness"]["mode"];
	brightness.val  = in["brightness"]["val"];
	brightness.min  = in["brightness"]["min"];
	brightness.max  = in["brightness"]["max"];

	auto &temperature = models[TEMPERATURE];
	temperature.mode = in["temperature"]["mode"];
	temperature.val  = in["temperature"]["val"];
	temperature.min  = in["temperature"]["min"];
	temperature.max  = in["temperature"]["max"];
}

json config::screen::to_json() const
{
	const auto &backlight   = models[BACKLIGHT];
	const auto &brightness  = models[BRIGHTNESS];
	const auto &temperature = models[TEMPERATURE];

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

config::config(size_t scr_no)
{
	defaults();

	file_parse();

	screen_diff(scr_no);

	file_pretty_write();
}

config::config(json in, size_t scr_no)
{
	defaults();

	from_json(in);

	screen_diff(scr_no);

	file_pretty_write();
}

void config::from_json(json in)
{
	for (const auto &s : in["screens"]) {
		screens.emplace_back(s);
	}

	time.start               = in["time"]["start"];
	time.end                 = in["time"]["end"];
	time.adaptation_minutes  = in["time"]["adaptation_minutes"];

	screenshot.scale         = in["screenshot"]["scale"];
	screenshot.poll_ms       = in["screenshot"]["poll_ms"];
	screenshot.adaptation_ms = in["screenshot"]["adaptation_ms"];

	als.scale                = in["als"]["scale"];
	als.poll_ms              = in["als"]["poll_ms"];
	als.adaptation_ms        = in["als"]["adaptation_ms"];
}

json config::to_json() const
{
	json ret {
		{"screens", json::array()},

		{"time", {
				{"start", time.start},
				{"end", time.end},
				{"adaptation_minutes", time.adaptation_minutes},
		}},

		{"screenshot", {
				{"scale", screenshot.scale},
				{"poll_ms", screenshot.poll_ms},
				{"adaptation_ms", screenshot.adaptation_ms},
		}},

		{"als", {
				{"scale", als.scale},
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
}

void config::file_pretty_write() const
{
	std::ofstream fs(filepath);
	fs.exceptions(std::fstream::failbit);
	fs << std::setw(4) << config::to_json();
}

void config::file_parse()
{
	const std::string data = [&] {
		try {
			return file_read(filepath);
		} catch (std::system_error &e) {
			return std::string();
		}
	}();

	const json jdata = [&] {
		try {
			return json::parse(data);
		} catch (json::exception &e) {
			return json();
		}
	}();

	if (!jdata.empty()) {
		from_json(jdata);
	}
}

size_t config::clients_for(config::screen::mode mode)
{
	size_t c = 0;
	for (size_t i = 0; i < screens.size(); ++i)
		c += clients_for(mode, i);
	return c;
}

size_t config::clients_for(config::screen::mode mode, size_t screen_index)
{
	size_t c = 0;
	for (const auto &model : screens[screen_index].models)
		if (model.mode == mode)
			++c;
	return c;
}

bool config::valid_f64(double val) {
	return val > std::numeric_limits<double>::min();
}

bool config::valid_int(int val) {
	return val > std::numeric_limits<int>::min();
}
