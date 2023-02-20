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

#include <gummyd/api.hpp>
#include <gummyd/utils.hpp>

void start() {
    if (!libgummyd::daemon_start())
        std::puts("already started");
    std::exit(EXIT_SUCCESS);
}

void stop() {
    if (libgummyd::daemon_stop()) {
        std::puts("gummy stopped");;
    } else {
        std::puts("already stopped");
    }
	std::exit(EXIT_SUCCESS);
}

void status() {
    if (libgummyd::daemon_is_running()) {
        std::puts("running");
    } else {
        std::puts("not running");
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

	return "";
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

constexpr int option_count = 23;

const std::array<std::array<std::string, 2>, option_count> options {{
{"-v, --version", "Print version and exit"},
{"-s,--screen", "Index on which to apply screen-related settings. If omitted, any changes will be applied on all screens."},

{"-B,--backlight-mode", "Backlight mode. 0 = manual, 1 = screenshot, 2 = ALS (if available), 3 = time range"},
{"-b,--backlight", "Set backlight brightness percentage."},
{"--backlight-min", "Set minimum backlight (for non-manual modes)"},
{"--backlight-max", "Set maximum backlight (for non-manual modes)"},

{"-P,--brightness-mode", "Brightness mode. 0 = manual, 1 = screenshot, 2 = ALS (if available), 3 = time range"},
{"-p,--brightness", "Set pixel brightness percentage."},
{"--brightness-min", "Set minimum brightness (for non-manual modes)"},
{"--brightness-max", "Set maximum brightness (for non-manual modes)"},

{"-T,--temperature-mode", "Temperature mode. 0 = manual, 1 = screenshot, 2 = ALS (if available), 3 = time range"},
{"-t,--temperature", "Set pixel temperature in kelvins."},
{"--temperature-min", "Set minimum temperature (for non-manual modes)"},
{"--temperature-max", "Set maximum temperature (for non-manual modes)"},

{"-y,--time-start", "Starting time in 24h format, for example `06:00`."},
{"-u,--time-end", "End time in 24h format, for example `16:30`."},
{"-i,--time-adaptation-min", "Adaptation speed in minutes.\nFor example, if this is set to 30 minutes, time-based models start easing into their min. value 30 minutes before the end time."},

{"--screenshot-scale", "Screenshot brightness multiplier. Useful for calibration."},
{"--screenshot-poll-ms", "Time interval between each screenshot."},
{"--screenshot-adaptation-ms", "Adaptation speed in milliseconds."},

{"--als-scale", "ALS signal multiplier. Useful for calibration."},
{"--als-poll-ms", "Time interval between each sensor reading."},
{"--als-adaptation-ms", "Adaptation speed in milliseconds."},
}};

const std::function<int(int)> brightness_perc_to_step = [] (int val) {
    const auto [min, max] = libgummyd::brightness_range();

	if (val >= 0) {
        return remap(std::clamp(val, 0, 100), 0, 100, min, max);
	} else {
        return -remap(std::clamp(std::abs(val), 0, 100), 0, 100, min, max);
	}
};

template <class T>
void setif(nlohmann::json &val, T new_val) {
    if ((libgummyd::config_is_valid(new_val))) {
		val = new_val;
	}
}

template <class T, std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T>, int> = 0>
void setif(nlohmann::json &val, T new_val, bool relative, T min, T max, std::function<T(T)> fn = [](T x){return x;}) {

    if (!(libgummyd::config_is_valid(new_val))) {
	    return;
    }

    if (relative) {
	    val = std::clamp(val.get<T>() + fn(new_val), min, max);
    } else {
        val = fn(new_val);
	}
}

void setif(nlohmann::json &val, std::string new_val) {
	if (val.is_string() && !new_val.empty())
		val = new_val;
}

template <class T>
std::string check_range(const std::string &s, T min, T max) {
	const T val = std::is_integral<T>::value ? std::stoi(s) : std::stod(s);
	if (!(val >= min && val <= max)) {
		return fmt::format("Value {} not in range [{} - {}]", val, min, max);
	}
	return "";
}

std::array<bool, option_count> relative_flags {};

template <class T>
std::string range_or_relative(const std::string &s, option_id opt_id, T min, T max) {
	if (s.starts_with("+") || s.starts_with("-")) {
		relative_flags[opt_id] = true;
		return std::string("");
	}
	return check_range(s, min, max);
};

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

    constexpr std::string_view range_fmt   = "INT in [{} - {}]";
    constexpr std::string_view range_fmt_f = "FLOAT in [{} - {}]";

    const std::pair<int, int> brt_range  = libgummyd::brightness_range();
    const std::pair<int, int> temp_range = libgummyd::temperature_range();
    const int invalid_val                = libgummyd::config_invalid_val();

    const std::string cli_brt_range_str  = fmt::format(range_fmt, 0, 100);
    const std::string temp_range_str     = fmt::format(range_fmt, temp_range.first, temp_range.second);
    const std::string scale_range        = fmt::format(range_fmt_f, 0.0, 10.0);

    struct {
        std::array<int, 4> backlight;
        std::array<int, 4> brightness;
        std::array<int, 4> temperature;
    } models;

    models.backlight.fill(invalid_val);
    models.brightness.fill(invalid_val);
    models.temperature.fill(invalid_val);

	const std::string grp_bl("Screen backlight settings");
    app.add_option(options[BACKLIGHT_MODE][0], models.backlight[0], options[BACKLIGHT_MODE][1])->check(CLI::Range(0, 3))->group(grp_bl);
    app.add_option(options[BACKLIGHT_PERC][0], models.backlight[1], options[BACKLIGHT_PERC][1])->check(CLI::Validator([&] (const std::string &s) { return range_or_relative(s, BACKLIGHT_PERC, 0, 100); }, cli_brt_range_str))->group(grp_bl);
    app.add_option(options[BACKLIGHT_MIN][0], models.backlight[2], options[BACKLIGHT_MIN][1])->check(CLI::Validator([&] (const std::string &s) { return range_or_relative(s, BACKLIGHT_MIN, 0, 100); }, cli_brt_range_str))->group(grp_bl);
    app.add_option(options[BACKLIGHT_MAX][0], models.backlight[3], options[BACKLIGHT_MAX][1])->check(CLI::Validator([&] (const std::string &s) { return range_or_relative(s, BACKLIGHT_MAX, 0, 100); }, cli_brt_range_str))->group(grp_bl);

    const std::string grp_brt("Screen brightness settings");
    app.add_option(options[BRT_MODE][0], models.brightness[0], options[BRT_MODE][1])->check(CLI::Range(0, 3))->group(grp_brt);
    app.add_option(options[BRT_PERC][0], models.brightness[1], options[BRT_PERC][1])->check(CLI::Validator([&] (const std::string &s) { return range_or_relative(s, BRT_PERC, 0, 100); }, cli_brt_range_str))->group(grp_brt);
    app.add_option(options[BRT_MIN][0], models.brightness[2], options[BRT_MIN][1])->check(CLI::Validator([&] (const std::string &s) { return range_or_relative(s, BRT_MIN, 0, 100); }, cli_brt_range_str))->group(grp_brt);
    app.add_option(options[BRT_MAX][0], models.brightness[3], options[BRT_MAX][1])->check(CLI::Validator([&] (const std::string &s) { return range_or_relative(s, BRT_MAX, 0, 100); }, cli_brt_range_str))->group(grp_brt);

	const std::string grp_temp("Screen temperature settings");
    app.add_option(options[TEMP_MODE][0], models.temperature[0], options[TEMP_MODE][1])->check(CLI::Range(0, 3))->group(grp_temp);
    app.add_option(options[TEMP_KELV][0], models.temperature[1], options[TEMP_KELV][1])->check(CLI::Validator([&] (const std::string &s) { return range_or_relative(s, TEMP_KELV, temp_range.first, temp_range.second); }, temp_range_str))->group(grp_temp);
    app.add_option(options[TEMP_MIN][0], models.temperature[2], options[TEMP_MIN][1])->check(CLI::Validator([&] (const std::string &s) { return range_or_relative(s, TEMP_MIN, temp_range.first, temp_range.second); }, temp_range_str))->group(grp_temp);
    app.add_option(options[TEMP_MAX][0], models.temperature[3], options[TEMP_MAX][1])->check(CLI::Validator([&] (const std::string &s) { return range_or_relative(s, TEMP_MAX, temp_range.first, temp_range.second); }, temp_range_str))->group(grp_temp);

    struct sensor {
    double scale;
    int    poll_ms;
    int    adaptation_ms;
    };
    struct service {
    std::string start = "";
    std::string end = "";
    int adaptation_minutes;
    };

    sensor  als { double(invalid_val), invalid_val, invalid_val };
    sensor  screenshot { double(invalid_val), invalid_val, invalid_val };
    service time {"", "", invalid_val};

	const std::string grp_als("ALS mode settings");
	app.add_option(options[ALS_SCALE][0], als.scale, options[ALS_SCALE][1])->check(CLI::Validator([&] (const std::string &s) { return range_or_relative(s, ALS_SCALE, 0., 10.); }, scale_range))->group(grp_als);
	app.add_option(options[ALS_POLL_MS][0], als.poll_ms, options[ALS_POLL_MS][1])->check(CLI::Range(1, 10000 * 60 * 60))->group(grp_als);
	app.add_option(options[ALS_ADAPTATION_MS][0], als.adaptation_ms, options[ALS_ADAPTATION_MS][1])->check(CLI::Range(1, 10000))->group(grp_als);

    const std::string grp_ss("Screenshot mode settings");
	app.add_option(options[SCREENSHOT_SCALE][0], screenshot.scale, options[SCREENSHOT_SCALE][1])->check(CLI::Validator([&] (const std::string &s) { return range_or_relative(s, SCREENSHOT_SCALE, 0., 10.); }, scale_range))->group(grp_ss);
	app.add_option(options[SCREENSHOT_POLL_MS][0], screenshot.poll_ms, options[SCREENSHOT_POLL_MS][1])->check(CLI::Range(1, 10000))->group(grp_ss);
	app.add_option(options[SCREENSHOT_ADAPTATION_MS][0], screenshot.adaptation_ms, options[SCREENSHOT_ADAPTATION_MS][1])->check(CLI::Range(1, 10000))->group(grp_ss);

	const std::string grp_time("Time range mode settings");
	app.add_option(options[TIME_START][0], time.start, options[TIME_START][1])->check(check_time_format)->group(grp_time);
	app.add_option(options[TIME_END][0], time.end, options[TIME_END][1])->check(check_time_format)->group(grp_time);
	app.add_option(options[TIME_ADAPTATION_MS][0], time.adaptation_minutes, options[TIME_ADAPTATION_MS][1])->check(CLI::Range(1, 60 * 12))->group(grp_time);

    LOG_("parsing options...\n");
	try {
		if (argc == 1) {
			app.parse("-h");
		} else {
			app.parse(argc, argv);
		}
	} catch (const CLI::ParseError &e) {
		return app.exit(e);
	}

    if (!libgummyd::daemon_is_running()) {
        std::puts("gummy is not running.\nType: `gummy start`");
        std::exit(EXIT_SUCCESS);
    }

    LOG_("getting config...\n");
    nlohmann::json config_json = [&] {
        try {
            return libgummyd::config_get_current();
        } catch (nlohmann::json::exception &e) {
            return nlohmann::json {{"error", e.what()}};
        }
    }();

    if (config_json.contains("error")) {
        LOG_ERR_FMT_("error while retrieving config:\n{}\n", config_json["error"].get<std::string>());
		return EXIT_FAILURE;
	}

    LOG_("updating config...\n");
	const auto update_screen_conf = [&] (size_t idx) {

		if (idx > config_json["screens"].size() - 1)
			return;

        LOG_FMT_("updating screen {}...\n", idx);

        auto &scr = config_json["screens"][idx];

        setif(scr["backlight"]["mode"], models.backlight[0]);
        setif(scr["backlight"]["val"], models.backlight[1], relative_flags[BACKLIGHT_PERC], brt_range.first, brt_range.second, brightness_perc_to_step);
        setif(scr["backlight"]["min"], models.backlight[2], relative_flags[BACKLIGHT_MIN], brt_range.first, brt_range.second, brightness_perc_to_step);
        setif(scr["backlight"]["max"], models.backlight[3], relative_flags[BACKLIGHT_MAX], brt_range.first, brt_range.second, brightness_perc_to_step);

        setif(scr["brightness"]["mode"], models.brightness[0]);
        setif(scr["brightness"]["val"], models.brightness[1], relative_flags[BRT_PERC], brt_range.first, brt_range.second, brightness_perc_to_step);
        setif(scr["brightness"]["min"], models.brightness[2], relative_flags[BRT_MIN], brt_range.first, brt_range.second, brightness_perc_to_step);
        setif(scr["brightness"]["max"], models.brightness[3], relative_flags[BRT_MAX], brt_range.first, brt_range.second, brightness_perc_to_step);

        setif(scr["temperature"]["mode"], models.temperature[0]);
        setif(scr["temperature"]["val"], models.temperature[1], relative_flags[TEMP_KELV], temp_range.first, temp_range.second);
        setif(scr["temperature"]["min"], models.temperature[2], relative_flags[TEMP_MIN], temp_range.first, temp_range.second);
        setif(scr["temperature"]["max"], models.temperature[3], relative_flags[TEMP_MAX], temp_range.first, temp_range.second);

        if (libgummyd::config_is_valid(models.backlight[1]))
            scr["backlight"]["mode"] = 0;
        if (libgummyd::config_is_valid(models.brightness[1]))
            scr["brightness"]["mode"] = 0;
        if (libgummyd::config_is_valid(models.temperature[1]))
            scr["temperature"]["mode"] = 0;
	};

    LOG_("updating screens\n");
	if (scr_idx > -1) {
		update_screen_conf(scr_idx);
	} else {
		for (size_t i = 0; i < config_json["screens"].size(); ++i)
			update_screen_conf(i);
	}

    LOG_("updating time...\n");
	setif(config_json["time"]["start"], time.start);
	setif(config_json["time"]["end"], time.end);
	setif(config_json["time"]["adaptation_minutes"], time.adaptation_minutes);

    LOG_("updating screenshot...\n");
	setif(config_json["screenshot"]["scale"], screenshot.scale, relative_flags[SCREENSHOT_SCALE], 0., 10.);
	setif(config_json["screenshot"]["poll_ms"], screenshot.poll_ms);
	setif(config_json["screenshot"]["adaptation_ms"], screenshot.adaptation_ms);

    LOG_("updating als...\n");
	setif(config_json["als"]["scale"], als.scale, relative_flags[ALS_SCALE], 0., 10.);
	setif(config_json["als"]["poll_ms"], als.poll_ms);
	setif(config_json["als"]["adaptation_ms"], als.adaptation_ms);

    LOG_("writing to daemon...\n");
    libgummyd::daemon_send(config_json.dump());

	return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
	return interface(argc, argv);
}
