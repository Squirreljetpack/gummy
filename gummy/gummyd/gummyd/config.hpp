// Copyright 2021-2023 Francesco Fusco
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <filesystem>
#include <nlohmann/json_fwd.hpp>

namespace gummyd {
class config {

	void defaults();

	void file_parse();
	void screen_diff(size_t scr_no);
	void file_pretty_write() const;

	void from_json(nlohmann::json data);
	nlohmann::json to_json() const;

    std::filesystem::path filepath_;
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
			SCREENLIGHT,
			ALS,
			TIME,
		};

		struct model {
            model_id id;
			screen::mode mode;
			int val;
			int min;
			int max;
		};

		std::array<model, 3> models;
        static std::string model_name(model_id);

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
