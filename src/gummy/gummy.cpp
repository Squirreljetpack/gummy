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
#include <regex>
#include <unistd.h>

#include <nlohmann/json.hpp>
#include <CLI/App.hpp>
#include <CLI/Formatter.hpp>
#include <CLI/Config.hpp>

#include "config.hpp"
#include "utils.hpp"
#include "file.hpp"

using namespace constants;

void start()
{
	if (set_flock(flock_filepath) < 0) {
		std::puts("already started");
		std::exit(EXIT_SUCCESS);
	}

	std::puts("starting gummy");
	pid_t pid = fork();

	if (pid < 0) {
		std::puts("fork() failed");
		std::exit(EXIT_FAILURE);
	}

	// parent
	if (pid > 0) {
		std::exit(EXIT_SUCCESS);
	}

	execl(CMAKE_INSTALL_DAEMON_PATH, "", nullptr);

	std::puts("failed to start gummyd");
	std::exit(EXIT_FAILURE);
}

void stop()
{
	if (set_flock(flock_filepath) == 0) {
		std::puts("already stopped");
		std::exit(EXIT_SUCCESS);
	}

	file_write(constants::fifo_filepath, "stop");

	std::puts("gummy stopped");
	std::exit(EXIT_SUCCESS);
}

void status()
{
	if (set_flock(flock_filepath) == 0) {
		std::puts("not running");
	} else {
		std::puts("running");
	}

	std::exit(EXIT_SUCCESS);
}

std::string check_time_format(const std::string &s)
{
	std::regex pattern("^\\d{2}:\\d{2}$");
	std::string err("option should match the 24h format.");

	if (!std::regex_search(s, pattern))
		return err;
	if (std::stoi(s.substr(0, 2)) > 23)
		return err;
	if (std::stoi(s.substr(3, 2)) > 59)
		return err;

	return std::string("");
}

enum option_id {
	VERS,
	SCREEN_NUM,

	BACKLIGHT_MODE,
	BACKLIGHT_PERC,
	BACKLIGHT_MIN,
	BACKLIGHT_MAX,

	BRT_MODE,
	BRT_PERC,
	BRT_MIN,
	BRT_MAX,

	TEMP_MODE,
	TEMP_KELV,
	TEMP_MIN,
	TEMP_MAX,

	TIME_START,
	TIME_END,
	TIME_ADAPTATION_MS,

	SCREENSHOT_SCALE,
	SCREENSHOT_POLL_MS,
	SCREENSHOT_ADAPTATION_MS,

	ALS_SCALE,
	ALS_POLL_MS,
	ALS_ADAPTATION_MS
};

const std::array<std::array<std::string, 2>, 23> options {{
{"-v, --version", "Print version and exit"},
{"-s,--screen", "Index on which to apply screen-related settings. If omitted, any changes will be applied on all screens."},

{"-B,--backlight-mode", "Backlight mode. 0 = manual, 1 = screenshot, 2 = ALS (if available), 3 = time range"},
{"-b,--backlight-val", "Set backlight percentage."},
{"--backlight-min", "Set minimum backlight (for non-manual modes)"},
{"--backlight-max", "Set maximum backlight (for non-manual modes)"},

{"-P,--brightness-mode", "Brightness mode. 0 = manual, 1 = screenshot, 2 = ALS (if available), 3 = time range"},
{"-p,--brightness-val", "Set pixel brightness percentage."},
{"--brightness-min", "Set minimum brightness (for non-manual modes)"},
{"--brightness-max", "Set maximum brightness (for non-manual modes)"},

{"-T,--temperature-mode", "Temperature mode. 0 = manual, 1 = screenshot, 2 = ALS (if available), 3 = time range"},
{"-t,--temperature-val", "Set pixel temperature in kelvins."},
{"--temperature-min", "Set minimum temperature (for non-manual modes)"},
{"--temperature-max", "Set maximum temperature (for non-manual modes)"},

{"-y,--time-start", "Starting time in 24h format, for example `06:00`."},
{"-u,--time-end", "End time in 24h format, for example `16:30`."},
{"-i,--time-adaptation-min", "Adaptation speed in minutes.\nFor example, if this is set to 30 minutes, time-based values start easing into their min. value 30 minutes before the end time."},

{"--screenshot-scale", "Screenshot brightness multiplier. Useful for calibration."},
{"--screenshot-poll-ms", "Time interval between each screenshot."},
{"--screenshot-adaptation-ms", "Adaptation speed in milliseconds."},

{"--als-scale", "ALS signal multiplier. Useful for calibration."},
{"--als-poll-ms", "Time interval between each sensor reading."},
{"--als-adaptation-ms", "Adaptation speed in milliseconds."},
}};

int perc_to_step(int val) {
	return remap(val, 0, 100, 0, constants::brt_steps_max);
}

void setif(nlohmann::json &val, double new_val) {
	if (config::valid_f64(new_val) && val.is_number_float())
		val = new_val;
}

void setif(nlohmann::json &val, int new_val) {
	if (config::valid_int(new_val) && val.is_number_integer())
		val = new_val;
}

void setif(nlohmann::json &val, int new_val, std::function<int(int)> fn) {
	if (config::valid_int(new_val) && val.is_number_integer())
		val = fn(new_val);
}

void setif(nlohmann::json &val, std::string new_val)
{
	if (val.is_string() && !new_val.empty())
		val = new_val;
}

int interface(int argc, char **argv)
{
	CLI::App app("Screen manager for X11.", "gummy");

	app.add_subcommand("start", "Start the background process.")->callback(start);
	app.add_subcommand("stop", "Stop the background process.")->callback(stop);
	app.add_subcommand("status", "Show app / screen status.")->callback(status);

	app.add_flag(options[VERS][0], [] ([[maybe_unused]] int64_t t) {
		std::puts(VERSION);
		std::exit(0);
	}, options[VERS][1]);

	int scr_idx = -1;
	app.add_option(options[SCREEN_NUM][0], scr_idx, options[SCREEN_NUM][1])->check(CLI::Range(0, 99));

	const std::string grp_bl("Screen backlight settings");
	config::screen::model backlight;
	app.add_option(options[BACKLIGHT_MODE][0], backlight.mode, options[BACKLIGHT_MODE][1])->check(CLI::Range(0, 3))->group(grp_bl);
	app.add_option(options[BACKLIGHT_PERC][0], backlight.val, options[BACKLIGHT_PERC][1])->check(CLI::Range(0, 100))->group(grp_bl);
	app.add_option(options[BACKLIGHT_MIN][0], backlight.min, options[BACKLIGHT_MIN][1])->check(CLI::Range(0, 100))->group(grp_bl);
	app.add_option(options[BACKLIGHT_MAX][0], backlight.max, options[BACKLIGHT_MAX][1])->check(CLI::Range(0, 100))->group(grp_bl);

	const std::string grp_brt("Screen brightness settings");
	config::screen::model brightness;
	app.add_option(options[BRT_MODE][0], brightness.mode, options[BRT_MODE][1])->check(CLI::Range(0, 3))->group(grp_brt);
	app.add_option(options[BRT_PERC][0], brightness.val, options[BRT_PERC][1])->check(CLI::Range(0, 100))->group(grp_brt);
	app.add_option(options[BRT_MIN][0], brightness.min, options[BRT_MIN][1])->check(CLI::Range(0, 100))->group(grp_brt);
	app.add_option(options[BRT_MAX][0], brightness.max, options[BRT_MAX][1])->check(CLI::Range(0, 100))->group(grp_brt);

	const std::string grp_temp("Screen temperature settings");
	config::screen::model temperature;

	app.add_option(options[TEMP_MODE][0], temperature.mode, options[TEMP_MODE][1])->check(CLI::Range(0, 3))->group(grp_temp);
	app.add_option(options[TEMP_KELV][0], temperature.val, options[TEMP_KELV][1])->check(CLI::Range(temp_k_min, temp_k_max))->group(grp_temp);
	app.add_option(options[TEMP_MIN][0], temperature.min, options[TEMP_MIN][1])->check(CLI::Range(temp_k_min, temp_k_max))->group(grp_temp);
	app.add_option(options[TEMP_MAX][0], temperature.max, options[TEMP_MAX][1])->check(CLI::Range(temp_k_min, temp_k_max))->group(grp_temp);

	const std::string grp_als("ALS mode settings");
	struct config::als als;
	app.add_option(options[ALS_SCALE][0], als.scale, options[ALS_SCALE][1])->check(CLI::Range(0., 10.))->group(grp_als);
	app.add_option(options[ALS_POLL_MS][0], als.poll_ms, options[ALS_POLL_MS][1])->check(CLI::Range(1, 10000 * 60 * 60))->group(grp_als);
	app.add_option(options[ALS_ADAPTATION_MS][0], als.adaptation_ms, options[ALS_ADAPTATION_MS][1])->check(CLI::Range(1, 10000))->group(grp_als);

	const std::string grp_ss("Screenshot mode settings");
	struct config::screenshot screenshot;
	app.add_option(options[SCREENSHOT_SCALE][0], screenshot.scale, options[SCREENSHOT_SCALE][1])->check(CLI::Range(0., 10.))->group(grp_ss);
	app.add_option(options[SCREENSHOT_POLL_MS][0], screenshot.poll_ms, options[SCREENSHOT_POLL_MS][1])->check(CLI::Range(1, 10000))->group(grp_ss);
	app.add_option(options[SCREENSHOT_ADAPTATION_MS][0], screenshot.adaptation_ms, options[SCREENSHOT_ADAPTATION_MS][1])->check(CLI::Range(1, 10000))->group(grp_ss);

	const std::string grp_time("Time range mode settings");
	struct config::time time;
	app.add_option(options[TIME_START][0], time.start, options[TIME_START][1])->check(check_time_format)->group(grp_time);
	app.add_option(options[TIME_END][0], time.end, options[TIME_END][1])->check(check_time_format)->group(grp_time);
	app.add_option(options[TIME_ADAPTATION_MS][0], time.adaptation_minutes, options[TIME_ADAPTATION_MS][1])->check(CLI::Range(1, 60 * 12))->group(grp_time);

	try {
		if (argc == 1) {
			app.parse("-h");
		} else {
			app.parse(argc, argv);
		}
	} catch (const CLI::ParseError &e) {
		return app.exit(e);
	}

	if (set_flock(flock_filepath) == 0) {
		std::puts("gummy is not running.\nType: `gummy start`\n");
		std::exit(EXIT_SUCCESS);
	}

	nlohmann::json config_json = [&] {
		std::ifstream ifs(xdg_config_filepath(constants::config_filename));
		nlohmann::json j;
		try {
			ifs >> j;
		} catch (nlohmann::json::exception &e) {
			return nlohmann::json({"exception", e.what()});
		}
		return j;
	}();

	if (config_json.contains("exception")) {
		LOG_ERR_FMT_("{}\n", config_json["exception"].get<std::string>());
		return EXIT_FAILURE;
	}

	const auto update_screen_conf = [&] (size_t idx) {

		if (idx > config_json["screens"].size() - 1)
			return;

		auto &scr = config_json["screens"][idx];

		setif(scr["backlight"]["mode"], int(backlight.mode));
		setif(scr["backlight"]["val"], backlight.val, perc_to_step);
		setif(scr["backlight"]["min"], backlight.min, perc_to_step);
		setif(scr["backlight"]["max"], backlight.max, perc_to_step);

		setif(scr["brightness"]["mode"], int(brightness.mode));
		setif(scr["brightness"]["val"], brightness.val, perc_to_step);
		setif(scr["brightness"]["min"], brightness.min, perc_to_step);
		setif(scr["brightness"]["max"], brightness.max, perc_to_step);

		setif(scr["temperature"]["mode"], int(temperature.mode));
		setif(scr["temperature"]["val"], temperature.val);
		setif(scr["temperature"]["min"], temperature.min);
		setif(scr["temperature"]["max"], temperature.max);

		using enum config::screen::mode;
		if (config::valid_int(backlight.val))
			scr["backlight"]["mode"] = MANUAL;
		if (config::valid_int(brightness.val))
			scr["brightness"]["mode"] = MANUAL;
		if (config::valid_int(temperature.val))
			scr["temperature"]["mode"] = MANUAL;
	};

	if (scr_idx > -1) {
		update_screen_conf(scr_idx);
	} else {
		for (size_t i = 0; i < config_json["screens"].size(); ++i)
			update_screen_conf(i);
	}

	setif(config_json["time"]["start"], time.start);
	setif(config_json["time"]["end"], time.end);
	setif(config_json["time"]["adaptation_minutes"], time.adaptation_minutes);

	setif(config_json["screenshot"]["scale"], screenshot.scale);
	setif(config_json["screenshot"]["poll_ms"], screenshot.poll_ms);
	setif(config_json["screenshot"]["adaptation_ms"], screenshot.adaptation_ms);

	setif(config_json["als"]["scale"], als.scale);
	setif(config_json["als"]["poll_ms"], als.poll_ms);
	setif(config_json["als"]["adaptation_ms"], als.adaptation_ms);

	file_write(constants::fifo_filepath, config_json.dump());

	return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
	return interface(argc, argv);
}
