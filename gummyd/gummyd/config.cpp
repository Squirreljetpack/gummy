// Copyright 2021-2024 Francesco Fusco <f.fusco@pm.me>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <fstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <gummyd/config.hpp>
#include <gummyd/file.hpp>
#include <gummyd/constants.hpp>

using nlohmann::json;
using namespace gummyd;

void config::defaults()
{
    filepath_ = xdg_config_dir() / constants::config_filename;

	time.start               = "06:00";
	time.end                 = "16:00";
	time.adaptation_minutes  = 60;

	screenshot.scale         = 1.0;
	screenshot.poll_ms       = 200;
	screenshot.adaptation_ms = 5000;

	als.scale                = 1.0;
	als.poll_ms              = 5000;
	als.adaptation_ms        = 5000;

    gamma.enabled            = true;
    gamma.refresh_s          = 10;
}

config::screen::screen()
{
	using enum config::screen::model_id;
	auto &backlight   = models[size_t(BACKLIGHT)];
	auto &brightness  = models[size_t(BRIGHTNESS)];
	auto &temperature = models[size_t(TEMPERATURE)];

	using enum config::screen::mode;
	backlight.mode   = MANUAL;
	brightness.mode  = MANUAL;
	temperature.mode = MANUAL;

	using namespace constants;
	backlight.min    = 350;
	backlight.max    = brt_steps_max;
	backlight.val    = brt_steps_max;

	brightness.min   = 350;
	brightness.max   = brt_steps_max;
	brightness.val   = brt_steps_max;

	temperature.min  = 3200;
	temperature.max  = temp_k_max;
	temperature.val  = temp_k_max;
}

config::screen::screen(json in)
{
	for (size_t i = 0; i < models.size(); ++i) {
        const std::string key = model_name(model_id(i));
        models[i].id   = model_id(i);
        models[i].mode = mode(in[key]["mode"].get<int>());
		models[i].val  = in[key]["val"].get<int>();
		models[i].min  = in[key]["min"].get<int>();
		models[i].max  = in[key]["max"].get<int>();
	}
}

std::string config::screen::model_name(model_id id) {
    using enum config::screen::model_id;
    switch (id) {
    case BACKLIGHT:
        return "backlight";
    case BRIGHTNESS:
        return "brightness";
    case TEMPERATURE:
        return "temperature";
    }
    return "error: more models than keys";
}

std::string config::screen::mode_name(mode id) {
    using enum config::screen::mode;
    switch (id) {
    case MANUAL:
        return "manual";
    case SCREENLIGHT:
        return "screenlight";
    case ALS:
        return "ALS";
    case TIME:
        return "time";
    }
    return "error: more modes than keys";
}

json config::screen::to_json() const
{
	using enum config::screen::model_id;
	const auto &backlight   = models[size_t(BACKLIGHT)];
	const auto &brightness  = models[size_t(BRIGHTNESS)];
	const auto &temperature = models[size_t(TEMPERATURE)];

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

    try {
        time.start               = in["time"]["start"].get<std::string>();
        time.end                 = in["time"]["end"].get<std::string>();
        time.adaptation_minutes  = in["time"]["adaptation_minutes"].get<int>();

        screenshot.scale         = in["screenlight"]["scale"].get<double>();
        screenshot.poll_ms       = in["screenlight"]["poll_ms"].get<int>();
        screenshot.adaptation_ms = in["screenlight"]["adaptation_ms"].get<int>();

        als.scale                = in["als"]["scale"].get<double>();
        als.poll_ms              = in["als"]["poll_ms"].get<int>();
        als.adaptation_ms        = in["als"]["adaptation_ms"].get<int>();

        gamma.enabled            = in["gamma"]["enabled"].get<int>();
        gamma.refresh_s          = in["gamma"]["refresh_s"].get<int>();
    } catch (const nlohmann::json::exception &e) {
        spdlog::error(e.what());
    }
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

		{"screenlight", {
				{"scale", screenshot.scale},
				{"poll_ms", screenshot.poll_ms},
				{"adaptation_ms", screenshot.adaptation_ms},
		}},

		{"als", {
				{"scale", als.scale},
				{"poll_ms", als.poll_ms},
				{"adaptation_ms", als.adaptation_ms}
		}},

        {"gamma", {
                {"enabled", gamma.enabled},
                {"refresh_s", gamma.refresh_s}
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
	std::ofstream fs(filepath_);
	fs.exceptions(std::fstream::failbit);
	fs << std::setw(4) << config::to_json();
}

void config::file_parse()
{
	const std::string data = [&] {
		try {
			return file_read(filepath_);
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
