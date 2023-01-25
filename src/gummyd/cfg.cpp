﻿/**
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

#include <fstream>
#include <syslog.h>

#include "cfg.hpp"

Config cfg;

const char *config_name = "gummyconf";
const char *fifo_name   = "/tmp/gummy.fifo";
const char *lock_name   = "/tmp/gummy.lock";

constexpr int brt_steps_min  = 100;
constexpr int brt_steps_max  = 500;
constexpr int temp_steps_min = 0;
constexpr int temp_steps_max = 500;
constexpr int temp_k_min     = 1000;
constexpr int temp_k_max     = 6500;

Config::Config()
: _path             (path()),
  brt_auto_fps      (60),
  als_polling_rate  (5000),
  temp_auto         (false),
  temp_auto_fps     (45),
  temp_auto_speed   (60), // minutes
  temp_auto_high    (temp_k_max),
  temp_auto_low     (3400),
  temp_auto_sunrise ("06:00"),
  temp_auto_sunset  ("16:00")
{}

Config::Screen::Screen()
: brt_mode              (MANUAL),
  brt_auto_min          (brt_steps_max / 4),
  brt_auto_max          (brt_steps_max),
  brt_auto_offset       (0),
  brt_auto_speed        (1000),
  brt_auto_threshold    (8),
  brt_auto_polling_rate (1000),
  brt_step              (brt_steps_max),
  temp_auto             (false),
  temp_step             (temp_steps_max)
{}

void Config::init(size_t detected_screens)
{
	read();

	// Remove unplugged screens from the config
	while (screens.size() > detected_screens)
		screens.pop_back();

	int new_screens = detected_screens - screens.size();
	while (new_screens--)
		screens.emplace_back(Screen());

	// If temp auto is on, set temp_step to 6500K for a smooth transition
	if (temp_auto) {
		for (size_t i = 0; i < screens.size(); ++i) {
			if (screens[i].temp_auto) {
				screens[i].temp_step = temp_steps_max;
			}
		}
	}

	write();
}

void Config::read()
{
	std::ifstream fs(_path, std::fstream::in | std::fstream::app);
	if (fs.fail()) {
		syslog(LOG_ERR, "Unable to open config\n");
		return;
	}

	fs.seekg(0, std::ios::end);
	if (fs.tellg() == 0) {
		Config::write();
		return;
	}
	fs.seekg(0);

	json jfs;
	try {
		fs >> jfs;
	} catch (json::exception &e) {
		syslog(LOG_ERR, "%s\n", e.what());
		Config::write();
		return;
	}
	Config::from_json(json_sanitize(jfs));
}

void Config::write()
{
	std::ofstream fs(_path);

	if (fs.fail()) {
		syslog(LOG_ERR, "Unable to open config\n");
		return;
	}

	try {
		fs << std::setw(4) << to_json();
	} catch (json::exception &e) {
		syslog(LOG_ERR, "%s\n", e.what());
		return;
	}
}

void Config::from_json(const json &in)
{
	brt_auto_fps      = in["brt_auto_fps"];
	als_polling_rate  = in["als_polling_rate"];
	temp_auto         = in["temp_auto"];
	temp_auto_fps     = in["temp_auto_fps"];
	temp_auto_speed   = in["temp_auto_speed"];
	temp_auto_sunrise = in["temp_auto_sunrise"];
	temp_auto_sunset  = in["temp_auto_sunset"];
	temp_auto_high    = in["temp_auto_highpp"];
	temp_auto_low     = in["temp_auto_low"];
	screens.clear();
	for (size_t i = 0; i < in["screens"].size(); ++i) {
		screens.emplace_back(
		    in["screens"][i]["brt_mode"],
		    in["screens"][i]["brt_auto_min"],
		    in["screens"][i]["brt_auto_max"],
		    in["screens"][i]["brt_auto_offset"],
		    in["screens"][i]["brt_auto_speed"],
		    in["screens"][i]["brt_auto_threshold"],
		    in["screens"][i]["brt_auto_polling_rate"],
		    in["screens"][i]["brt_step"],
		    in["screens"][i]["temp_auto"],
		    in["screens"][i]["temp_step"]
		);
	}
}

json json_sanitize(const json &j)
{
	Config default_config = Config();
	size_t input_json_screen_size = j["screens"].size();
	while (input_json_screen_size--)
		default_config.screens.emplace_back();

	// update the default json structure with the values found in the config file.
	json ret = default_config.to_json();
	ret.update(j);

	// Do the same for the screens (json::update() doesn't work with nested objects).
	for (size_t i = 0; i < j["screens"].size(); ++i) {
		json screen_json = screen_to_json(Config::Screen());
		screen_json.update(j["screens"][i]);
		ret["screens"][i] = screen_json;
	}

	return ret;
}

json screen_to_json(const Config::Screen &s)
{
	return json({
	     {"brt_mode", s.brt_mode},
	     {"brt_auto_min", s.brt_auto_min},
	     {"brt_auto_max", s.brt_auto_max},
	     {"brt_auto_offset", s.brt_auto_offset},
	     {"brt_auto_speed", s.brt_auto_speed},
	     {"brt_auto_threshold", s.brt_auto_threshold},
	     {"brt_auto_polling_rate", s.brt_auto_polling_rate},
	     {"brt_step", s.brt_step},
	     {"temp_auto", s.temp_auto},
	     {"temp_step", s.temp_step},
	});
}

json Config::to_json()
{
	json ret({
	    {"brt_auto_fps", brt_auto_fps},
	    {"als_polling_rate", als_polling_rate},
	    {"temp_auto", temp_auto},
	    {"temp_auto_fps", temp_auto_fps},
	    {"temp_auto_speed", temp_auto_speed},
	    {"temp_auto_sunrise", temp_auto_sunrise},
	    {"temp_auto_sunset", temp_auto_sunset},
	    {"temp_auto_high", temp_auto_high},
	    {"temp_auto_low", temp_auto_low},
	    {"screens", json::array()}
	});
	for (const auto &s : screens)
		ret["screens"].emplace_back(screen_to_json(s));
	return ret;
}

std::string Config::path()
{
	const char *home   = getenv("XDG_CONFIG_HOME");
	const char *format = "/";

	if (!home) {
		format = "/.config/";
		home = getenv("HOME");
	}

	std::ostringstream ss;
	ss << home << format << config_name;
	return ss.str();
}

Config::Screen::Screen(
    Brt_mode brt_mode,
    int brt_auto_min,
    int brt_auto_max,
    int brt_auto_offset,
    int brt_auto_speed,
    int brt_auto_threshold,
    int brt_auto_polling_rate,
    int brt_step,
    bool temp_auto,
    int temp_step
    ) : brt_mode(brt_mode),
    brt_auto_min(brt_auto_min),
    brt_auto_max(brt_auto_max),
    brt_auto_offset(brt_auto_offset),
    brt_auto_speed(brt_auto_speed),
    brt_auto_threshold(brt_auto_threshold),
    brt_auto_polling_rate(brt_auto_polling_rate),
    brt_step(brt_step),
    temp_auto(temp_auto),
    temp_step(temp_step)
{
}

Message::Message(const std::string &j)
{
	json msg = json::parse(j);
	scr_no               = msg["scr_no"];
	brt_perc             = msg["brt_perc"];
	brt_mode             = msg["brt_mode"];
	brt_auto_min         = msg["brt_auto_min"];
	brt_auto_max         = msg["brt_auto_max"];
	brt_auto_offset      = msg["brt_auto_offset"];
	brt_auto_speed       = msg["brt_auto_speed"];
	screenshot_rate_ms   = msg["brt_auto_screenshot_rate"];
	als_poll_rate_ms     = msg["brt_auto_als_poll_rate"];
	temp_k               = msg["temp_k"];
	temp_auto            = msg["temp_mode"];
	temp_day_k           = msg["temp_day_k"];
	temp_night_k         = msg["temp_night_k"];
	sunrise_time         = msg["sunrise_time"];
	sunset_time          = msg["sunset_time"];
	temp_adaptation_time = msg["temp_adaptation_time"];
	add                  = msg["add"];
	sub                  = msg["sub"];
}
