/**
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
#include <regex>
#include <unistd.h>

#include "CLI11.hpp"
#include "json.hpp"
#include "config.hpp"
#include "utils.hpp"

using std::cout;
using namespace constants;

void send(const std::string &s)
{
	std::ofstream fs(fifo_name);

	if (!fs.good()) {
		std::cout << "fifo open error, aborting\n";
		std::exit(1);
	}

	fs.write(s.c_str(), s.size());

	if (!fs.good()) {
		std::cout << "fifo write error, aborting\n";
		std::exit(1);
	}
}

void start()
{
	if (set_lock(lock_name) > 0) {
		cout << "already started\n";
		std::exit(0);
	}

	cout << "starting gummy\n";
	pid_t pid = fork();

	if (pid < 0) {
		cout << "fork() failed\n";
		std::exit(1);
	}

	// parent
	if (pid > 0) {
		std::exit(0);
	}

	execl(CMAKE_INSTALL_DAEMON_PATH, "", nullptr);
	cout << "failed to start gummyd\n";
	std::exit(1);
}

void stop()
{
	if (set_lock(lock_name) == 0) {
		cout << "already stopped\n";
		std::exit(0);
	}

	send("stop");

	cout << "gummy stopped\n";
	std::exit(0);
}

void status()
{
	cout << (set_lock(lock_name) == 0 ? "not running" : "running") << "\n";
	std::exit(0);
}

std::string time_format_callback(const std::string &s)
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
/*
int interface_old(int argc, const char **argv)
{
	CLI::App app("Screen manager for X11.", "gummy");

	app.add_subcommand("start", "Start the background process.")->callback(start);
	app.add_subcommand("stop", "Stop the background process.")->callback(stop);
	app.add_subcommand("status", "Show app / screen status.")->callback(status);

	int scr_no          = -1;
	int brt             = -1;
	int bm              = -1;
	int brt_auto_min    = -1;
	int brt_auto_max    = -1;
	int brt_auto_offset = -1;
	int brt_auto_speed  = -1;
	int scr_rate        = -1;
	int als_poll        = -1;
	int temp            = -1;
	int tm              = -1;
	int temp_day        = -1;
	int temp_night      = -1;
	int adapt_time      = -1;
	std::string sunrise_time;
	std::string sunset_time;

	int add = -1;
	int sub = -1;

	app.add_flag("-v,--version", [] ([[maybe_unused]] int64_t t) {
		cout << VERSION << '\n';
		std::exit(0);
	}, "Print version and exit");

	app.add_flag("--add", add, "Interpret value as increment");
	app.add_flag("--sub", sub, "Interpret value as decrement");

	app.add_option("-s,--screen", scr_no,
	               "Screen on which to act. If omitted, any changes will be applied on all screens.")->check(CLI::Range(0, 99));

	std::string brt_grp("Brightness options");
	app.add_option("-b,--brightness", brt,
	               "Set screen brightness percentage.")->check(CLI::Range(1, 100))->group(brt_grp);
	app.add_option("-B,--brt-mode", bm,
	               "Brightness mode. 0 = manual, 1 = screenshot, 2 = ALS (if available)")->check(CLI::Range(0, 2))->group(brt_grp);
	app.add_option("-N,--brt-auto-min", brt_auto_min,
	               "Set minimum automatic brightness.")->check(CLI::Range(1, 100))->group(brt_grp);
	app.add_option("-M,--brt-auto-max", brt_auto_max,
	               "Set maximum automatic brightness.")->check(CLI::Range(1, 100))->group(brt_grp);
	app.add_option("-L,--brt-auto-offset", brt_auto_offset,
	               "Set automatic brightness offset. Higher = brighter image.")->check(CLI::Range(-100, 100))->group(brt_grp);
	app.add_option("--brt-auto-speed", brt_auto_speed,
	               "Set brightness adaptation speed in milliseconds. Default is 1000 ms.")->check(CLI::Range(1, 10000))->group(brt_grp);
	app.add_option("--screen-poll-rate", scr_rate,
	               "How often to check for screen image changes in milliseconds. Only relevant for screens with brightness mode 1.")->check(CLI::Range(1, 5000))->group(brt_grp);
	app.add_option("--als-poll-rate", als_poll,
	               "How often to check for ambient light changes in milliseconds. Only relevant for screens with brightness mode 2.")->check(CLI::Range(1, 30000))->group(brt_grp);

	auto temp_range_callback = [&] (std::string &x) {
		int val = std::stoi(x);
		if (app.count("--add") == 0 && app.count("--sub") == 0) {
			if (val < temp_k_min || val > temp_k_max)
				return std::string("Value not in range " + std::to_string(temp_k_min) + " to " + std::to_string(temp_k_max));
		}
		if (val < 0)
			return std::string("Invalid value");
		return std::string("");
	};

	std::string temp_grp("Temperature options");
	app.add_option("-t,--temperature", temp,
	               "Set screen temperature in kelvins.\nSetting this option will disable automatic temperature if enabled.")
	               ->check(CLI::Validator(temp_range_callback, "INT in [2000 - 6500]"))
	               ->group(temp_grp);

	app.add_option("-T,--temp-mode", tm,
	               "Temperature mode. 0 for manual, 1 for automatic.")->check(CLI::Range(0, 1))->group(temp_grp);
	app.add_option("-j,--temp-day", temp_day,
	               "Set day time temperature in kelvins.")->check(CLI::Range(temp_k_min, temp_k_max))->group(temp_grp);
	app.add_option("-k,--temp-night", temp_night,
	               "Set night time temperature in kelvins.")->check(CLI::Range(temp_k_min, temp_k_max))->group(temp_grp);
	app.add_option("-y,--sunrise-time", sunrise_time,
	               "Set sunrise time in 24h format, for example `06:00`.")->check(time_format_callback)->group(temp_grp);
	app.add_option("-u,--sunset-time", sunset_time,
	               "Set sunset time in 24h format, for example `16:30`.")->check(time_format_callback)->group(temp_grp);
	app.add_option("-i,--temp-adaptation-time", adapt_time,
	               "Temperature adaptation time in minutes.\nFor example, if this option is set to 30 min. and the sunset time is at 16:30,\ntemperature starts adjusting at 16:00, going down gradually until 16:30.")->group(temp_grp);

	// show help with no args
	if (argc == 1) {
		++argc;
		argv[1] = "-hpp";
	}

	try {
		app.parse(argc, argv);
	} catch(const CLI::ParseError &e) {
		return app.exit(e);
	}

	if (set_lock(lock_name) == 0) {
		cout << "gummy is not running.\nType: `gummy start`\n";
		std::exit(1);
	}

	nlohmann::json msg {
		{"scr_no", scr_no},
		{"brt_mode", bm},
		{"brt_auto_min", brt_auto_min},
		{"brt_auto_max", brt_auto_max},
		{"brt_auto_offset", brt_auto_offset},
		{"brt_auto_speed", brt_auto_speed},
		{"brt_auto_screenshot_rate", scr_rate},
		{"brt_auto_als_poll_rate", als_poll},
		{"temp_mode", tm},
		{"brt_perc", brt},
		{"temp_k", temp},
		{"temp_day_k", temp_day},
		{"temp_night_k", temp_night},
		{"sunrise_time", sunrise_time},
		{"sunset_time", sunset_time},
		{"temp_adaptation_time", adapt_time},
		{"add", add},
		{"sub", sub}
	};

	send(msg.dump());
	return 0;
}
*/
enum {
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

	SCREENSHOT_OFFSET,
	SCREENSHOT_POLL_MS,
	SCREENSHOT_ADAPTATION_MS,

	ALS_OFFSET,
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
{"-i,--time-adaptation-min", "Adaptation speed in minutes.\nFor example, if this is set to 30 minutes, time-based values start easing into their minimum value 30 minutes before the end time."},

{"--screenshot-offset-perc", "Adds to the screenshot brightness calculation."},
{"--screenshot-poll-ms", "Time interval between each screenshot."},
{"--screenshot-adaptation-ms", "Adaptation speed in milliseconds."},

{"--als-offset-perc", "Adds to the ALS brightness calculation."},
{"--als-poll-ms", "Time interval between each sensor reading."},
{"--als-adaptation-ms", "Adaptation speed in milliseconds."},
}};

void setif(json &val, int new_val)
{
	if (val.is_number_integer() && new_val > -1)
		val = new_val;
}

void setif(json &val, std::string new_val)
{
	if (val.is_string() && !new_val.empty())
		val = new_val;
}

int interface(int argc, const char **argv)
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
	app.add_option(options[BACKLIGHT_MODE][0], backlight.mode, options[BACKLIGHT_MODE][1])->check(CLI::Range(0, 2))->group(grp_bl);
	app.add_option(options[BACKLIGHT_PERC][0], backlight.val, options[BACKLIGHT_PERC][1])->check(CLI::Range(0, 100))->group(grp_bl);
	app.add_option(options[BACKLIGHT_MIN][0], backlight.min, options[BACKLIGHT_MIN][1])->check(CLI::Range(0, 100))->group(grp_bl);
	app.add_option(options[BACKLIGHT_MAX][0], backlight.max, options[BACKLIGHT_MAX][1])->check(CLI::Range(0, 100))->group(grp_bl);

	const std::string grp_brt("Screen brightness settings");
	config::screen::model brightness;
	app.add_option(options[BRT_MODE][0], brightness.mode, options[BRT_MODE][1])->check(CLI::Range(0, 2))->group(grp_brt);
	app.add_option(options[BRT_PERC][0], brightness.val, options[BRT_PERC][1])->check(CLI::Range(0, 100))->group(grp_brt);
	app.add_option(options[BRT_MIN][0], brightness.min, options[BRT_MIN][1])->check(CLI::Range(0, 100))->group(grp_brt);
	app.add_option(options[BRT_MAX][0], brightness.max, options[BRT_MAX][1])->check(CLI::Range(0, 100))->group(grp_brt);

	const std::string grp_temp("Screen temperature settings");
	config::screen::model temperature;

	app.add_option(options[TEMP_MODE][0], temperature.mode, options[TEMP_MODE][1])->check(CLI::Range(0, 2))->group(grp_temp);
	app.add_option(options[TEMP_KELV][0], temperature.val, options[TEMP_KELV][1])->check(CLI::Range(temp_k_min, temp_k_max))->group(grp_temp);
	app.add_option(options[TEMP_MIN][0], temperature.min, options[TEMP_MIN][1])->check(CLI::Range(temp_k_min, temp_k_max))->group(grp_temp);
	app.add_option(options[TEMP_MAX][0], temperature.max, options[TEMP_MAX][1])->check(CLI::Range(temp_k_min, temp_k_max))->group(grp_temp);

	const std::string grp_time("Time range mode settings");
	struct config::time time;
	app.add_option(options[TIME_START][0], time.start, options[TIME_START][1])->check(time_format_callback)->group(grp_time);
	app.add_option(options[TIME_END][0], time.end, options[TIME_END][1])->check(time_format_callback)->group(grp_time);
	app.add_option(options[TIME_ADAPTATION_MS][0], time.adaptation_minutes, options[TIME_ADAPTATION_MS][1])->check(CLI::Range(1, 60 * 12))->group(grp_time);

	const std::string grp_ss("Screenshot mode settings");
	struct config::screenshot screenshot;
	app.add_option(options[SCREENSHOT_OFFSET][0], screenshot.offset_perc, options[SCREENSHOT_OFFSET][1])->check(CLI::Range(0, 100))->group(grp_ss);
	app.add_option(options[SCREENSHOT_POLL_MS][0], screenshot.poll_ms, options[SCREENSHOT_POLL_MS][1])->check(CLI::Range(1, 10000))->group(grp_ss);
	app.add_option(options[SCREENSHOT_ADAPTATION_MS][0], screenshot.adaptation_ms, options[SCREENSHOT_ADAPTATION_MS][1])->check(CLI::Range(1, 10000))->group(grp_ss);

	const std::string grp_als("ALS mode settings");
	struct config::als als;
	app.add_option(options[ALS_OFFSET][0], als.offset_perc, options[ALS_OFFSET][1])->check(CLI::Range(0, 100))->group(grp_als);
	app.add_option(options[ALS_POLL_MS][0], als.poll_ms, options[ALS_POLL_MS][1])->check(CLI::Range(1, 10000 * 60 * 60))->group(grp_als);
	app.add_option(options[ALS_ADAPTATION_MS][0], als.adaptation_ms, options[ALS_ADAPTATION_MS][1])->check(CLI::Range(1, 10000))->group(grp_als);

	try {
		app.parse(argc, argv);
	} catch(const CLI::ParseError &e) {
		return app.exit(e);
	}

	if (set_lock(lock_name) == 0) {
		std::puts("gummy is not running.\nType: `gummy start`\n");
		std::exit(EXIT_SUCCESS);
	}

	json config_json = [&] {
		std::ifstream ifs(xdg_config_path(constants::config_name));
		json j;
		try {
			ifs >> j;
		} catch (json::exception &e) {
			return json({"exception", e.what()});
		}
		return j;
	}();

	if (config_json.contains("exception")) {
		printf("%s\n", config_json["exception"].get<std::string>().c_str());
		return EXIT_FAILURE;
	}

	const auto update_screen_conf = [&] (size_t idx) {

		if (idx > config_json["screens"].size() - 1)
			return;

		auto &scr = config_json["screens"][idx];

		setif(scr["backlight"]["mode"], backlight.mode);
		setif(scr["backlight"]["val"], remap(backlight.val, 0, 100, 0, 1000));
		setif(scr["backlight"]["min"], remap(backlight.min, 0, 100, 0, 1000));
		setif(scr["backlight"]["max"], remap(backlight.max, 0, 100, 0, 1000));

		setif(scr["brightness"]["mode"], brightness.mode);
		setif(scr["brightness"]["val"], remap(brightness.val, 0, 100, 0, 1000));
		setif(scr["brightness"]["min"], remap(brightness.min, 0, 100, 0, 1000));
		setif(scr["brightness"]["max"], remap(brightness.max, 0, 100, 0, 1000));

		setif(scr["temperature"]["mode"], temperature.mode);
		setif(scr["temperature"]["val"], temperature.val);
		setif(scr["temperature"]["min"], temperature.min);
		setif(scr["temperature"]["max"], temperature.max);
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

	setif(config_json["screenshot"]["poll_ms"], screenshot.poll_ms);
	setif(config_json["screenshot"]["adaptation_ms"], screenshot.adaptation_ms);
	setif(config_json["screenshot"]["offset_perc"], screenshot.offset_perc);

	setif(config_json["als"]["poll_ms"], als.poll_ms);
	setif(config_json["als"]["adaptation_ms"], als.adaptation_ms);
	setif(config_json["als"]["offset_perc"], als.offset_perc);

	send(config_json.dump());
	return 0;
}

int main(int argc, const char **argv)
{
	return interface(argc, argv);
}
