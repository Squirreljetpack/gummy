// Copyright 2021-2023 Francesco Fusco
// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <regex>
#include <limits>

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
    if (!gummyd::daemon_is_running()) {
        std::puts("not running");
        std::exit(EXIT_SUCCESS);
    }

    const auto mode_str = [] (int mode) {
        switch (mode) {
        case 0: return "";
        case 1: return " (screenlight)";
        case 2: return " (ALS)";
        case 3: return " (time)";
        }
        return "error";
    };

    static constexpr std::string not_available = "N/A";

    const nlohmann::json screens = nlohmann::json::from_cbor(gummyd::daemon_get("status"));
    std::string rows;

    for (size_t idx = 0; idx < screens.size(); ++idx) {
        const std::string backlight_str = [&] {
            if (screens[idx]["bl"].get<int>() > - 1) {
                return fmt::format("{}%{}", screens[idx]["bl"].get<int>(), mode_str(screens[idx]["bl_mode"].get<int>()));
            } else {
                return not_available;
            }
        }();
        const std::string brightness_str = [&] {
            if (screens[idx]["brt"].get<int>() > - 1) {
                return fmt::format("{}%{}", screens[idx]["brt"].get<int>(), mode_str(screens[idx]["brt_mode"].get<int>()));
            } else {
                return not_available;
            }
        }();
        const std::string temperature_str = [&] {
            if (screens[idx]["temp"].get<int>() > - 1) {
                return fmt::format("{}K{}", screens[idx]["temp"].get<int>(), mode_str(screens[idx]["temp_mode"].get<int>()));
            } else {
                return not_available;
            }
        }();

        fmt::format_to(std::back_inserter(rows),
                       "[screen {}] backlight: {}, brightness: {}, temperature: {}\n",
                       idx, backlight_str, brightness_str, temperature_str);

    }

    fmt::print("{}", rows);
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

constexpr int option_count = 25;
static constexpr std::array<std::array<const char*, 2>, option_count> options {{
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

    {"--gamma-enable", "Toggle gamma functionality. A value of 0 allows gummy to co-exist with other programs that handle gamma."},
    {"--gamma-refresh-s", "Interval between each gamma refresh. A value of 0 disables refreshes."},
}};

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
    ALS_ADAPTATION_MS,

    GAMMA_ENABLED,
    GAMMA_REFRESH_S,
};

constexpr std::array screen_group_strings {
    "Screen backlight settings",
    "Screen brightness settings",
    "Screen temperature settings"
};

constexpr std::array service_group_strings {
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

struct {
    std::array<int, 4> backlight;
    std::array<int, 4> brightness;
    std::array<int, 4> temperature;
} models;

// Signifies a value not changed by the user and therefore to ignore when updating the config file.
constexpr int unset (std::numeric_limits<int>().min());

// What is this, PHP?
bool isset(int x) {
    return x != unset;
}

// Bad, but at least it's explicit
bool isset(double x) {
    return int(x) != unset;
}

bool isset(std::string_view x) {
    return !x.empty();
}

template <class T>
void setif(nlohmann::json &val, T new_val) {
    if (isset(new_val)) {
        val = new_val;
    }
}

void setif(nlohmann::json &val, const std::string &new_val) {
    if (!val.is_string()) {
        throw std::runtime_error("updating non-string configuration with a string");
    }

    if (isset(new_val)) {
        val = new_val;
    }
}

// The callback is currently used by the caller when we need to convert
// the backlight/brightness percentage to the config format.
// Otherwise it does nothing.
template <class T, std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T>, int> = 0>
void setif(nlohmann::json &val, T new_val, bool relative, range<T> range, std::function<T(T)> fn = [](T x){ return x; }) {
    if (!isset(new_val)) {
        return;
    }

    const T fn_val = fn(new_val);

    if (relative) {
        spdlog::debug("adding: {} + {}", val.get<T>(), fn_val);
        val = std::clamp(val.get<T>() + fn_val, range.min, range.max);
    } else {
        spdlog::debug("setting {} to {}", val.get<T>(), fn_val);
        val = fn_val;
    }
}

int interface(int argc, char **argv) {
    CLI::App app("Screen manager for X11.", "gummy");
	app.add_subcommand("start", "Start the background process.")->callback(start);
	app.add_subcommand("stop", "Stop the background process.")->callback(stop);
	app.add_subcommand("status", "Show app / screen status.")->callback(status);

	app.add_flag(options[VERS][0], [] ([[maybe_unused]] int64_t t) {
		std::puts(VERSION);
		std::exit(0);
	}, options[VERS][1]);

    int screen_idx (unset);
    models.backlight.fill(unset);
    models.brightness.fill(unset);
    models.temperature.fill(unset);
    int gamma_enabled   (unset);
    int gamma_refresh_s (unset);
    sensor  als { double(unset), unset, unset };
    sensor  screenlight { double(unset), unset, unset };
    service time {"", "", unset};

    const std::pair<int, int> brt_range_pair  = gummyd::brightness_range();
    const std::pair<int, int> temp_range_pair = gummyd::temperature_range();
    const CLI::Range mode_range(0, 3);
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

    // Flags signaling relative increments/decrements.
    std::array<bool, option_count> rel_fl {};

    app.add_option(options[SCREEN_NUM][0], screen_idx, options[SCREEN_NUM][1])->check(CLI::Range(0, 99));

    // Per-screen settings
    app.add_option(options[BACKLIGHT_MODE][0], models.backlight[0], options[BACKLIGHT_MODE][1])->check(mode_range)->group(screen_group_strings[0]);
    app.add_option(options[BACKLIGHT_PERC][0], models.backlight[1], options[BACKLIGHT_PERC][1])->check(CLI::Validator([&] (const std::string &s) { return relative_validator(s, rel_fl[BACKLIGHT_PERC], brightness_range); }, brightness_range.desc()))->group(screen_group_strings[0]);
    app.add_option(options[BACKLIGHT_MIN][0], models.backlight[2], options[BACKLIGHT_MIN][1])->check(CLI::Validator([&] (const std::string &s) { return relative_validator(s, rel_fl[BACKLIGHT_MIN], brightness_range);    }, brightness_range.desc()))->group(screen_group_strings[0]);
    app.add_option(options[BACKLIGHT_MAX][0], models.backlight[3], options[BACKLIGHT_MAX][1])->check(CLI::Validator([&] (const std::string &s) { return relative_validator(s, rel_fl[BACKLIGHT_MAX], brightness_range);    }, brightness_range.desc()))->group(screen_group_strings[0]);

    app.add_option(options[BRT_MODE][0], models.brightness[0], options[BRT_MODE][1])->check(mode_range)->group(screen_group_strings[1]);
    app.add_option(options[BRT_PERC][0], models.brightness[1], options[BRT_PERC][1])->check(CLI::Validator([&] (const std::string &s) { return relative_validator(s, rel_fl[BRT_PERC], brightness_range); }, brightness_range.desc()))->group(screen_group_strings[1]);
    app.add_option(options[BRT_MIN][0], models.brightness[2], options[BRT_MIN][1])->check(CLI::Validator([&] (const std::string &s) { return relative_validator(s, rel_fl[BRT_MIN], brightness_range); }, brightness_range.desc()))->group(screen_group_strings[1]);
    app.add_option(options[BRT_MAX][0], models.brightness[3], options[BRT_MAX][1])->check(CLI::Validator([&] (const std::string &s) { return relative_validator(s, rel_fl[BRT_MAX], brightness_range); }, brightness_range.desc()))->group(screen_group_strings[1]);

    app.add_option(options[TEMP_MODE][0], models.temperature[0], options[TEMP_MODE][1])->check(mode_range)->group(screen_group_strings[2]);
    app.add_option(options[TEMP_KELV][0], models.temperature[1], options[TEMP_KELV][1])->check(CLI::Validator([&] (const std::string &s) { return relative_validator(s, rel_fl[TEMP_KELV], temp_range); }, temp_range.desc()))->group(screen_group_strings[2]);
    app.add_option(options[TEMP_MIN][0], models.temperature[2], options[TEMP_MIN][1])->check(CLI::Validator([&] (const std::string &s) { return relative_validator(s, rel_fl[TEMP_MIN], temp_range); }, temp_range.desc()))->group(screen_group_strings[2]);
    app.add_option(options[TEMP_MAX][0], models.temperature[3], options[TEMP_MAX][1])->check(CLI::Validator([&] (const std::string &s) { return relative_validator(s, rel_fl[TEMP_MAX], temp_range); }, temp_range.desc()))->group(screen_group_strings[2]);

    // Global settings
    app.add_option(options[ALS_SCALE][0], als.scale, options[ALS_SCALE][1])->check(CLI::Validator([&] (const std::string &s) { return relative_validator(s, rel_fl[ALS_SCALE], als_range_scale); }, als_range_scale.desc()))->group(service_group_strings[0]);
    app.add_option(options[ALS_POLL_MS][0], als.poll_ms, options[ALS_POLL_MS][1])->check(CLI::Validator([&] (const std::string &s) { return relative_validator(s, rel_fl[ALS_POLL_MS], als_range_poll_ms); }, als_range_poll_ms.desc()))->group(service_group_strings[0]);
    app.add_option(options[ALS_ADAPTATION_MS][0], als.adaptation_ms, options[ALS_ADAPTATION_MS][1])->check(CLI::Validator([&] (const std::string &s) { return relative_validator(s, rel_fl[ALS_ADAPTATION_MS], als_range_adaptation_ms); }, als_range_adaptation_ms.desc()))->group(service_group_strings[0]);

    app.add_option(options[SCREENSHOT_SCALE][0], screenlight.scale, options[SCREENSHOT_SCALE][1])->check(CLI::Validator([&] (const std::string &s) { return relative_validator(s, rel_fl[SCREENSHOT_SCALE], screenlight_range_scale); }, screenlight_range_scale.desc()))->group(service_group_strings[1]);
    app.add_option(options[SCREENSHOT_POLL_MS][0], screenlight.poll_ms, options[SCREENSHOT_POLL_MS][1])->check(CLI::Validator([&] (const std::string &s) { return relative_validator(s, rel_fl[SCREENSHOT_POLL_MS], screenlight_range_poll_ms); }, screenlight_range_poll_ms.desc()))->group(service_group_strings[1]);
    app.add_option(options[SCREENSHOT_ADAPTATION_MS][0], screenlight.adaptation_ms, options[SCREENSHOT_ADAPTATION_MS][1])->check(CLI::Validator([&] (const std::string &s) { return relative_validator(s, rel_fl[SCREENSHOT_ADAPTATION_MS], screenlight_range_adaptation_ms); }, screenlight_range_adaptation_ms.desc()))->group(service_group_strings[1]);

    app.add_option(options[TIME_START][0], time.start, options[TIME_START][1])->check(check_time_format)->group(service_group_strings[2]);
    app.add_option(options[TIME_END][0], time.end, options[TIME_END][1])->check(check_time_format)->group(service_group_strings[2]);
    app.add_option(options[TIME_ADAPTATION_MINUTES][0], time.adaptation_minutes, options[TIME_ADAPTATION_MINUTES][1])->check(CLI::Validator([&] (const std::string &s) { return relative_validator(s, rel_fl[TIME_ADAPTATION_MINUTES], time_range_adaptation_minutes); }, time_range_adaptation_minutes.desc()))->group(service_group_strings[2]);

    app.add_option(options[GAMMA_ENABLED][0], gamma_enabled, options[GAMMA_ENABLED][1])->check(CLI::Range(0, 1));
    app.add_option(options[GAMMA_REFRESH_S][0], gamma_refresh_s, options[GAMMA_REFRESH_S][1])->check(CLI::Range(0, 60));

    spdlog::debug("parsing options");
	try {
		if (argc == 1) {
			app.parse("-h");
		} else {
			app.parse(argc, argv);
		}
	} catch (const CLI::ParseError &e) {
		return app.exit(e);
	}

    spdlog::debug("getting config");
    nlohmann::json config_json = [&] {
        try {
            return gummyd::config_get_current();
        } catch (const nlohmann::json::exception &e) {
            fmt::print("Error: {}\n", e.what());
            std::exit(EXIT_FAILURE);
        }
    }();

	setif(config_json["time"]["start"], time.start);
	setif(config_json["time"]["end"], time.end);
    setif(config_json["time"]["adaptation_minutes"], time.adaptation_minutes, rel_fl[TIME_ADAPTATION_MINUTES], time_range_adaptation_minutes);
    setif(config_json["screenlight"]["scale"], screenlight.scale, rel_fl[SCREENSHOT_SCALE], screenlight_range_scale);
    setif(config_json["screenlight"]["poll_ms"], screenlight.poll_ms, rel_fl[SCREENSHOT_POLL_MS], screenlight_range_poll_ms);
    setif(config_json["screenlight"]["adaptation_ms"], screenlight.adaptation_ms, rel_fl[SCREENSHOT_ADAPTATION_MS], screenlight_range_adaptation_ms);
    setif(config_json["als"]["scale"], als.scale, rel_fl[ALS_SCALE], als_range_scale);
    setif(config_json["als"]["poll_ms"], als.poll_ms, rel_fl[ALS_POLL_MS], als_range_poll_ms);
    setif(config_json["als"]["adaptation_ms"], als.adaptation_ms, rel_fl[ALS_ADAPTATION_MS], als_range_adaptation_ms);
    setif(config_json["gamma"]["enabled"], gamma_enabled);
    setif(config_json["gamma"]["refresh_s"], gamma_refresh_s);

    const auto update_screen_config = [&] (size_t idx) {
        if (idx > config_json["screens"].size() - 1) {
            fmt::print("Invalid screen number. Run `gummy status` to check for valid ones.\n");
            std::exit(EXIT_FAILURE);
        }

        const std::function<int(int)> brightness_perc_to_step = [&] (int val) {
            const int abs_clamped_val = std::clamp(std::abs(val), brightness_range.min, brightness_range.max);
            const int out_val = gummyd::remap(abs_clamped_val, brightness_range.min, brightness_range.max, brt_range_pair.first, brt_range_pair.second);
            spdlog::debug("{}% -> {}", val, out_val);
            return val >= 0 ? out_val : -out_val;
        };

        auto &scr = config_json["screens"][idx];

        setif(scr["backlight"]["mode"], models.backlight[0]);
        setif(scr["backlight"]["val"],  models.backlight[1], rel_fl[BACKLIGHT_PERC], brightness_range_real, brightness_perc_to_step);
        setif(scr["backlight"]["min"],  models.backlight[2], rel_fl[BACKLIGHT_MIN], brightness_range_real, brightness_perc_to_step);
        setif(scr["backlight"]["max"],  models.backlight[3], rel_fl[BACKLIGHT_MAX], brightness_range_real, brightness_perc_to_step);

        setif(scr["brightness"]["mode"], models.brightness[0]);
        setif(scr["brightness"]["val"],  models.brightness[1], rel_fl[BRT_PERC], brightness_range_real, brightness_perc_to_step);
        setif(scr["brightness"]["min"],  models.brightness[2], rel_fl[BRT_MIN], brightness_range_real, brightness_perc_to_step);
        setif(scr["brightness"]["max"],  models.brightness[3], rel_fl[BRT_MAX], brightness_range_real, brightness_perc_to_step);

        setif(scr["temperature"]["mode"],models.temperature[0]);
        setif(scr["temperature"]["val"], models.temperature[1], rel_fl[TEMP_KELV], temp_range);
        setif(scr["temperature"]["min"], models.temperature[2], rel_fl[TEMP_MIN], temp_range);
        setif(scr["temperature"]["max"], models.temperature[3], rel_fl[TEMP_MAX], temp_range);

        // If the user passed a manual backlight value, set backlight mode to manual.
        if (isset(models.backlight[1])) {
            scr["backlight"]["mode"] = 0;
        }
        if (isset(models.brightness[1])) {
            scr["brightness"]["mode"] = 0;
        }
        if (isset(models.temperature[1])) {
            scr["temperature"]["mode"] = 0;
        }
    };

    if (isset(screen_idx)) {
        spdlog::debug("updating screen {}", screen_idx);
        update_screen_config(screen_idx);
    } else {
        spdlog::debug("updating all screens");
        for (size_t i = 0; i < config_json["screens"].size(); ++i) {
            update_screen_config(i);
        }
    }

    if (gummyd::daemon_is_running()) {
        spdlog::debug("sending config to daemon");
        gummyd::daemon_send(config_json.dump());
        return EXIT_SUCCESS;
    }

    spdlog::debug("daemon not running, updating config file instead");
    gummyd::config_write(config_json);
    std::puts("Configuration updated. Run 'gummy start' to apply.");

	return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
    spdlog::cfg::load_env_levels();
    return interface(argc, argv);
}
