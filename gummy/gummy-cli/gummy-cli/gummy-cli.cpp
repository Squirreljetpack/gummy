// Copyright (c) 2021-2023, Francesco Fusco. All rights reserved.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <regex>
#include <nlohmann/json.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/cfg/env.h>
#include <CLI/App.hpp>
#include <CLI/Formatter.hpp>
#include <CLI/Config.hpp>

#include <gummyd/api.hpp>
#include <gummyd/utils.hpp>

void start() {
    if (!gummyd::daemon_start())
        std::puts("already started");
    std::exit(EXIT_SUCCESS);
}

void stop() {
    if (gummyd::daemon_stop()) {
        std::puts("gummy stopped");;
    } else {
        std::puts("already stopped");
    }
	std::exit(EXIT_SUCCESS);
}

void status() {
    if (gummyd::daemon_is_running()) {
        std::puts("running");
    } else {
        std::puts("not running");
    }
	std::exit(EXIT_SUCCESS);
}

std::string check_time_format(const std::string &s) {
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

template <class T>
struct range {
    T min;
    T max;
    range(T mi, T mx) : min(mi), max(mx) {};

    std::string desc() const {
        return CLI::Range(min, max).get_description();
    }
};

template <class T>
void setif(nlohmann::json &val, T new_val) {
    if ((gummyd::config_is_valid(new_val))) {
		val = new_val;
	}
}

void setif(nlohmann::json &val, std::string new_val) {
    if (val.is_string() && !new_val.empty())
        val = new_val;
}

template <class T, std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T>, int> = 0>
void setif(nlohmann::json &val, T new_val, bool relative, range<T> range, std::function<T(T)> fn = [](T x){return x;}) {

    if (!(gummyd::config_is_valid(new_val))) {
        return;
    }

    spdlog::debug("new_val {} is relative: {}", new_val, relative);
    if (relative) {
        val = std::clamp(val.get<T>() + fn(new_val), range.min, range.max);
    } else {
        spdlog::debug("setting {} to {}", val.get<T>(), fn(new_val));
        val = fn(new_val);
	}
}


template <class T>
std::string relative_validator(const std::string &str, bool &relative_flag, range<T> range) {
    if (str.starts_with("+") || str.starts_with("-")) {
        relative_flag = true;
        return "";
	}

    const T val = std::is_integral<T>::value ? std::stoi(str) : std::stod(str);
    if (val >= range.min && val <= range.max) {
        return "";
    }

    return fmt::format("Value {} not in range [{} - {}]", val, range.min, range.max);
};

int interface(int argc, char **argv)
{
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
        TIME_ADAPTATION_MINUTES,

        SCREENSHOT_SCALE,
        SCREENSHOT_POLL_MS,
        SCREENSHOT_ADAPTATION_MS,

        ALS_SCALE,
        ALS_POLL_MS,
        ALS_ADAPTATION_MS
    };

    constexpr int option_count = 23;
    const std::array<std::array<std::string, 2>, option_count> options {{
    {"-v,--version", "Print version and exit"},
    {"-s,--screen", "Index on which to apply screen-related settings. If omitted, any changes will be applied on all screens."},

    {"-B,--backlight-mode", "Backlight mode. 0 = manual, 1 = screenlight, 2 = ALS (if available), 3 = time range"},
    {"-b,--backlight", "Set backlight brightness percentage."},
    {"--backlight-min", "Set minimum backlight (for auto modes)"},
    {"--backlight-max", "Set maximum backlight (for auto modes)"},

    {"-P,--brightness-mode", "Brightness mode. 0 = manual, 1 = screenlight, 2 = ALS (if available), 3 = time range"},
    {"-p,--brightness", "Set pixel brightness percentage."},
    {"--brightness-min", "Set minimum brightness (for auto modes)"},
    {"--brightness-max", "Set maximum brightness (for auto modes)"},

    {"-T,--temperature-mode", "Temperature mode. 0 = manual, 1 = screenlight, 2 = ALS (if available), 3 = time range"},
    {"-t,--temperature", "Set pixel temperature in kelvins."},
    {"--temperature-min", "Set minimum temperature (for auto modes)"},
    {"--temperature-max", "Set maximum temperature (for auto modes)"},

    {"-y,--time-start", "Starting time in 24h format, for example `06:00`."},
    {"-u,--time-end", "End time in 24h format, for example `16:30`."},
    {"-i,--time-adaptation-min", "Adaptation speed in minutes.\nFor example, if this is set to 30 minutes, time-based models start easing into their min. value 30 minutes before the end time."},

    {"--screenlight-scale", "Screenshot brightness multiplier. Useful for calibration."},
    {"--screenlight-poll-ms", "Time interval between each screenshot."},
    {"--screenlight-adaptation-ms", "Adaptation speed in milliseconds."},

    {"--als-scale", "ALS signal multiplier. Useful for calibration."},
    {"--als-poll-ms", "Time interval between each sensor reading."},
    {"--als-adaptation-ms", "Adaptation speed in milliseconds."},
    }};
    std::array<bool, option_count> rel_fl {};

    CLI::App app("Screen manager for X11.", "gummy");

	app.add_subcommand("start", "Start the background process.")->callback(start);
	app.add_subcommand("stop", "Stop the background process.")->callback(stop);
	app.add_subcommand("status", "Show app / screen status.")->callback(status);

	app.add_flag(options[VERS][0], [] ([[maybe_unused]] int64_t t) {
		std::puts(VERSION);
		std::exit(0);
	}, options[VERS][1]);

    const int invalid_val = gummyd::config_invalid_val();

    int scr_idx = invalid_val;
	app.add_option(options[SCREEN_NUM][0], scr_idx, options[SCREEN_NUM][1])->check(CLI::Range(0, 99));

    struct {
        std::array<int, 4> backlight;
        std::array<int, 4> brightness;
        std::array<int, 4> temperature;
    } models;

    models.backlight.fill(invalid_val);
    models.brightness.fill(invalid_val);
    models.temperature.fill(invalid_val);

    constexpr std::array scr_grp {
            "Screen backlight settings",
            "Screen brightness settings",
            "Screen temperature settings"
    };

    constexpr std::array service_grp {
            "ALS mode settings",
            "Screenlight mode settings",
            "Time range mode settings"
    };

    struct sensor {
        double scale;
        int poll_ms;
        int adaptation_ms;
    };
    struct service {
       std::string start = "";
       std::string end = "";
       int adaptation_minutes;
    };

    sensor  als { double(invalid_val), invalid_val, invalid_val };
    sensor  screenlight { double(invalid_val), invalid_val, invalid_val };
    service time {"", "", invalid_val};

    const CLI::Range mode_range(0, 3);

    const std::pair<int, int> brt_range_pair  = gummyd::brightness_range();
    const std::pair<int, int> temp_range_pair = gummyd::temperature_range();

    const range brightness_range_real(brt_range_pair.first, brt_range_pair.second);
    const range brightness_range(0, brightness_range_real.max / 10);
    const range temp_range(temp_range_pair.first, temp_range_pair.second);

    const range als_range_scale(0.0, 10.0);
    const range als_range_poll_ms(1, 10000 * 60 * 60);
    const range als_range_adaptation_ms(1, 10000);

    const range screenlight_range_scale(0.0, 10.0);
    const range screenlight_range_adaptation_ms(1, 10000);
    const range screenlight_range_poll_ms(1, 10000);

    const range time_range_adaptation_minutes(1, 60 * 12);

    app.add_option(options[BACKLIGHT_MODE][0], models.backlight[0], options[BACKLIGHT_MODE][1])->check(mode_range)->group(scr_grp[0]);
    app.add_option(options[BACKLIGHT_PERC][0], models.backlight[1], options[BACKLIGHT_PERC][1])->check(CLI::Validator([&] (const std::string &s) { return relative_validator(s, rel_fl[BACKLIGHT_PERC], brightness_range); }, brightness_range.desc()))->group(scr_grp[0]);
    app.add_option(options[BACKLIGHT_MIN][0], models.backlight[2], options[BACKLIGHT_MIN][1])->check(CLI::Validator([&] (const std::string &s) { return relative_validator(s, rel_fl[BACKLIGHT_MIN], brightness_range);    }, brightness_range.desc()))->group(scr_grp[0]);
    app.add_option(options[BACKLIGHT_MAX][0], models.backlight[3], options[BACKLIGHT_MAX][1])->check(CLI::Validator([&] (const std::string &s) { return relative_validator(s, rel_fl[BACKLIGHT_MAX], brightness_range);    }, brightness_range.desc()))->group(scr_grp[0]);

    app.add_option(options[BRT_MODE][0], models.brightness[0], options[BRT_MODE][1])->check(mode_range)->group(scr_grp[1]);
    app.add_option(options[BRT_PERC][0], models.brightness[1], options[BRT_PERC][1])->check(CLI::Validator([&] (const std::string &s) { return relative_validator(s, rel_fl[BRT_PERC], brightness_range); }, brightness_range.desc()))->group(scr_grp[1]);
    app.add_option(options[BRT_MIN][0], models.brightness[2], options[BRT_MIN][1])->check(CLI::Validator([&] (const std::string &s) { return relative_validator(s, rel_fl[BRT_MIN], brightness_range); }, brightness_range.desc()))->group(scr_grp[1]);
    app.add_option(options[BRT_MAX][0], models.brightness[3], options[BRT_MAX][1])->check(CLI::Validator([&] (const std::string &s) { return relative_validator(s, rel_fl[BRT_MAX], brightness_range); }, brightness_range.desc()))->group(scr_grp[1]);

    app.add_option(options[TEMP_MODE][0], models.temperature[0], options[TEMP_MODE][1])->check(mode_range)->group(scr_grp[2]);
    app.add_option(options[TEMP_KELV][0], models.temperature[1], options[TEMP_KELV][1])->check(CLI::Validator([&] (const std::string &s) { return relative_validator(s, rel_fl[TEMP_KELV], temp_range); }, temp_range.desc()))->group(scr_grp[2]);
    app.add_option(options[TEMP_MIN][0], models.temperature[2], options[TEMP_MIN][1])->check(CLI::Validator([&] (const std::string &s) { return relative_validator(s, rel_fl[TEMP_MIN], temp_range); }, temp_range.desc()))->group(scr_grp[2]);
    app.add_option(options[TEMP_MAX][0], models.temperature[3], options[TEMP_MAX][1])->check(CLI::Validator([&] (const std::string &s) { return relative_validator(s, rel_fl[TEMP_MAX], temp_range); }, temp_range.desc()))->group(scr_grp[2]);

    app.add_option(options[ALS_SCALE][0], als.scale, options[ALS_SCALE][1])->check(CLI::Validator([&] (const std::string &s) { return relative_validator(s, rel_fl[ALS_SCALE], als_range_scale); }, als_range_scale.desc()))->group(service_grp[0]);
    app.add_option(options[ALS_POLL_MS][0], als.poll_ms, options[ALS_POLL_MS][1])->check(CLI::Validator([&] (const std::string &s) { return relative_validator(s, rel_fl[ALS_POLL_MS], als_range_poll_ms); }, als_range_poll_ms.desc()))->group(service_grp[0]);
    app.add_option(options[ALS_ADAPTATION_MS][0], als.adaptation_ms, options[ALS_ADAPTATION_MS][1])->check(CLI::Validator([&] (const std::string &s) { return relative_validator(s, rel_fl[ALS_ADAPTATION_MS], als_range_adaptation_ms); }, als_range_adaptation_ms.desc()))->group(service_grp[0]);

    app.add_option(options[SCREENSHOT_SCALE][0], screenlight.scale, options[SCREENSHOT_SCALE][1])->check(CLI::Validator([&] (const std::string &s) { return relative_validator(s, rel_fl[SCREENSHOT_SCALE], screenlight_range_scale); }, screenlight_range_scale.desc()))->group(service_grp[1]);
    app.add_option(options[SCREENSHOT_POLL_MS][0], screenlight.poll_ms, options[SCREENSHOT_POLL_MS][1])->check(CLI::Validator([&] (const std::string &s) { return relative_validator(s, rel_fl[SCREENSHOT_POLL_MS], screenlight_range_poll_ms); }, screenlight_range_poll_ms.desc()))->group(service_grp[1]);
    app.add_option(options[SCREENSHOT_ADAPTATION_MS][0], screenlight.adaptation_ms, options[SCREENSHOT_ADAPTATION_MS][1])->check(CLI::Validator([&] (const std::string &s) { return relative_validator(s, rel_fl[SCREENSHOT_ADAPTATION_MS], screenlight_range_adaptation_ms); }, screenlight_range_adaptation_ms.desc()))->group(service_grp[1]);

    app.add_option(options[TIME_START][0], time.start, options[TIME_START][1])->check(check_time_format)->group(service_grp[2]);
    app.add_option(options[TIME_END][0], time.end, options[TIME_END][1])->check(check_time_format)->group(service_grp[2]);
    app.add_option(options[TIME_ADAPTATION_MINUTES][0], time.adaptation_minutes, options[TIME_ADAPTATION_MINUTES][1])->check(CLI::Validator([&] (const std::string &s) { return relative_validator(s, rel_fl[TIME_ADAPTATION_MINUTES], time_range_adaptation_minutes); }, time_range_adaptation_minutes.desc()))->group(service_grp[2]);

    spdlog::debug("parsing options...");
	try {
		if (argc == 1) {
			app.parse("-h");
		} else {
			app.parse(argc, argv);
		}
	} catch (const CLI::ParseError &e) {
		return app.exit(e);
	}

    if (!gummyd::daemon_is_running()) {
        std::puts("gummy is not running.\nType: `gummy start`");
        std::exit(EXIT_SUCCESS);
    }

    spdlog::debug("getting config...");
    nlohmann::json config_json = [&] {
        try {
            return gummyd::config_get_current();
        } catch (nlohmann::json::exception &e) {
            return nlohmann::json {{"error", e.what()}};
        }
    }();

    if (config_json.contains("error")) {
        spdlog::error("error while retrieving config:{}", config_json["error"].get<std::string>());
		return EXIT_FAILURE;
	}

    spdlog::debug("updating services...");
	setif(config_json["time"]["start"], time.start);
	setif(config_json["time"]["end"], time.end);
    setif(config_json["time"]["adaptation_minutes"], time.adaptation_minutes, rel_fl[TIME_ADAPTATION_MINUTES], time_range_adaptation_minutes);

    setif(config_json["screenlight"]["scale"], screenlight.scale, rel_fl[SCREENSHOT_SCALE], screenlight_range_scale);
    setif(config_json["screenlight"]["poll_ms"], screenlight.poll_ms, rel_fl[SCREENSHOT_POLL_MS], screenlight_range_poll_ms);
    setif(config_json["screenlight"]["adaptation_ms"], screenlight.adaptation_ms, rel_fl[SCREENSHOT_ADAPTATION_MS], screenlight_range_adaptation_ms);

    setif(config_json["als"]["scale"], als.scale, rel_fl[ALS_SCALE], als_range_scale);
    setif(config_json["als"]["poll_ms"], als.poll_ms, rel_fl[ALS_POLL_MS], als_range_poll_ms);
    setif(config_json["als"]["adaptation_ms"], als.adaptation_ms, rel_fl[ALS_ADAPTATION_MS], als_range_adaptation_ms);

    const auto update_screen_conf = [&] (size_t idx) {

        if (idx > config_json["screens"].size() - 1)
            return;

        auto &scr = config_json["screens"][idx];

        const std::function<int(int)> brightness_perc_to_step = [&] (int val) {
            const int abs_clamped_val = std::clamp(std::abs(val), brightness_range.min, brightness_range.max);
            const int x = gummyd::remap(abs_clamped_val, brightness_range.min, brightness_range.max, brt_range_pair.first, brt_range_pair.second);
            spdlog::debug("{} -> {}", val, x);
            return val >= 0 ? x : -x;
        };

        setif(scr["backlight"]["mode"], models.backlight[0]);
        setif(scr["backlight"]["val"], models.backlight[1], rel_fl[BACKLIGHT_PERC], brightness_range_real, brightness_perc_to_step);
        setif(scr["backlight"]["min"], models.backlight[2], rel_fl[BACKLIGHT_MIN], brightness_range_real, brightness_perc_to_step);
        setif(scr["backlight"]["max"], models.backlight[3], rel_fl[BACKLIGHT_MAX], brightness_range_real, brightness_perc_to_step);
        setif(scr["brightness"]["mode"], models.brightness[0]);
        setif(scr["brightness"]["val"], models.brightness[1], rel_fl[BRT_PERC], brightness_range_real, brightness_perc_to_step);
        setif(scr["brightness"]["min"], models.brightness[2], rel_fl[BRT_MIN], brightness_range_real, brightness_perc_to_step);
        setif(scr["brightness"]["max"], models.brightness[3], rel_fl[BRT_MAX], brightness_range_real, brightness_perc_to_step);
        setif(scr["temperature"]["mode"], models.temperature[0]);
        setif(scr["temperature"]["val"], models.temperature[1], rel_fl[TEMP_KELV], temp_range);
        setif(scr["temperature"]["min"], models.temperature[2], rel_fl[TEMP_MIN], temp_range);
        setif(scr["temperature"]["max"], models.temperature[3], rel_fl[TEMP_MAX], temp_range);

        if (gummyd::config_is_valid(models.backlight[1]))
            scr["backlight"]["mode"] = 0;
        if (gummyd::config_is_valid(models.brightness[1]))
            scr["brightness"]["mode"] = 0;
        if (gummyd::config_is_valid(models.temperature[1]))
            scr["temperature"]["mode"] = 0;
    };

    if (scr_idx == invalid_val) {
        spdlog::debug("updating all screens");
        for (size_t i = 0; i < config_json["screens"].size(); ++i)
            update_screen_conf(i);
    } else {
        spdlog::debug("updating screen {}", scr_idx);
        update_screen_conf(scr_idx);
    }

    spdlog::debug("writing to daemon...");
    gummyd::daemon_send(config_json.dump());

	return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
    spdlog::cfg::load_env_levels();
    return interface(argc, argv);
}
