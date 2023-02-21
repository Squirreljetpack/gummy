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

//#include <syslog.h>
#include <nlohmann/json.hpp>

#include <gummyd/file.hpp>
#include <gummyd/utils.hpp>
#include <gummyd/sdbus-util.hpp>
#include <gummyd/channel.hpp>
#include <gummyd/core.hpp>
#include <gummyd/config.hpp>
#include <gummyd/display.hpp>
#include <gummyd/gamma.hpp>
#include <gummyd/sysfs_devices.hpp>
#include <gummyd/constants.hpp>

using namespace fushko;
using namespace gummy;

void run(display_server &dsp, config conf, std::stop_token stoken)
{
	assert(dsp.scr_count() == conf.screens.size());

	const size_t screenshot_clients = conf.clients_for(config::screen::mode::SCREENSHOT);
	const size_t als_clients        = conf.clients_for(config::screen::mode::ALS);
	const size_t time_clients       = conf.clients_for(config::screen::mode::TIME);
	LOG_FMT_("clients: als: {}, screenlight: {}, time: {}\n", als_clients, screenshot_clients, time_clients);

	std::vector<std::jthread> threads;

	channel<double>    als_ch(-1.);
	channel<time_data> time_ch({-1, -1, -1});
	std::vector<channel<int>> brt_channels;
	brt_channels.reserve(screenshot_clients);

	if (time_clients > 0) {
		threads.emplace_back([&] {
			time_server(time_ch, conf.time, stoken);
		});
	}

    std::vector<backlight> backlights = get_backlights();
    std::vector<als> als = get_als();

	if (als_clients > 0 && !als.empty()) {
		threads.emplace_back([&] {
			als_server(als[0], als_ch, conf.als, stoken);
		});
	}

	gamma_state gamma_state(dsp, conf.screens);

	for (size_t idx = 0; idx < conf.screens.size(); ++idx) {

		if (conf.clients_for(config::screen::mode::SCREENSHOT, idx) > 0) {
			brt_channels.emplace_back(-1);
            threads.emplace_back(screenlight_server, std::ref(dsp), idx, std::ref(brt_channels.back()), conf.screenshot, stoken);
		}

        const std::string xdg_state_dir = xdg_state_filepath(fmt::format("gummyd/screen-{}", idx));

        for (size_t model_idx = 0; model_idx < conf.screens[idx].models.size(); ++model_idx) {

			const auto &model = conf.screens[idx].models[model_idx];
            using enum config::screen::model_id;
            using enum config::screen::mode;
            using std::placeholders::_1;

			// dummy function
			std::function<void(int)> fn = [] ([[maybe_unused]] int val) { };

			switch (config::screen::model_id(model_idx)) {
			case BACKLIGHT:
			    if (idx < backlights.size())
                    fn = std::bind(&backlight::set_step, &backlights[idx], _1);
				break;
			case BRIGHTNESS:
                fn = std::bind(&gamma_state::set_brightness, &gamma_state, idx, _1);
				break;
			case TEMPERATURE:
                fn = std::bind(&gamma_state::set_temperature, &gamma_state, idx, _1);
				break;
            }

            // if two threads are modifying two different gamma values,
            // they must both know beforehand the respective gamma values,
            // in order to avoid screen flickering
            const int val = [&] {
                if (model.mode == MANUAL)
                    return model.val;

                try {
                    const std::string filepath = fmt::format("{}/{}", xdg_state_dir, config::screen::model_name(model.id));
                    return std::stoi(file_read(filepath));
                } catch (std::exception &e) {
                    return model.max;
                }
            }();

            fn(val);
        }

        for (size_t model_idx = 0; model_idx < conf.screens[idx].models.size(); ++model_idx) {

            const auto &model = conf.screens[idx].models[model_idx];

            using enum config::screen::model_id;
            using enum config::screen::mode;
            using std::placeholders::_1;

            const auto model_name = config::screen::model_name(config::screen::model_id(model_idx));

            // dummy function
            std::function<void(int)> fn = [] ([[maybe_unused]] int val) { };

            switch (config::screen::model_id(model_idx)) {

            case BACKLIGHT:
                if (idx < backlights.size())
                    fn = std::bind(&backlight::set_step, &backlights[idx], _1);
                break;
            case BRIGHTNESS:
                fn = std::bind(&gamma_state::apply_brightness, &gamma_state, idx, _1);
                break;
            case TEMPERATURE:
                fn = std::bind(&gamma_state::apply_temperature, &gamma_state, idx, _1);
                break;
            }

            switch (model.mode) {
            case MANUAL:
                LOG_FMT_("{} setting manual value: {}\n", model_name, model.val);
                fn(model.val);
                break;
            case ALS:
                LOG_FMT_("{} starting als_client...\n", model_name);
                threads.emplace_back(als_client, std::ref(als_ch), idx, model, fn, conf.als.adaptation_ms);
                break;
            case SCREENSHOT:
                LOG_FMT_("{} starting screenlight_client...\n", model_name);
                threads.emplace_back(screenlight_client, std::ref(brt_channels.back()), idx, model, fn, conf.screenshot.adaptation_ms);
                break;
            case TIME:
                LOG_FMT_("{} starting time_client...\n", model_name);
                threads.emplace_back(time_client, std::ref(time_ch), idx, model, fn);
                break;
            }
        }
	}

	threads.emplace_back([&] {
		while (true) {
            jthread_wait_until(std::chrono::seconds(10), stoken);
            LOG_("refreshing gamma...\n");
            gamma_state.apply_to_all_screens();
			if (stoken.stop_requested())
				return;
		}
	});

	for (auto &t : threads)
		t.join();

	LOG_FMT_("{:=^60}\n", "end");
}

int message_loop()
{
	display_server dsp;
	LOG_FMT_("[display_server] found {} screen(s)\n", dsp.scr_count());

	config conf(dsp.scr_count());

	named_pipe pipe(constants::fifo_filepath);

    std::filesystem::remove_all(xdg_state_filepath("gummyd/"));
    std::filesystem::create_directories(xdg_state_filepath("gummyd/"));

    const auto proxy = sdbus_util::on_system_sleep([] (sdbus::Signal &sig) {
	    bool sleep;
	    sig >> sleep;
	    if (!sleep)
	        file_write(constants::fifo_filepath, "reset");
    });

	while (true) {

		std::jthread thr([&] (std::stop_token stoken) {
			run(dsp, conf, stoken);
		});

		const std::string data(file_read(constants::fifo_filepath));

		if (data == "stop")
			break;

		if (data == "reset")
			continue;

		const nlohmann::json msg = [&] {
			try {
				return nlohmann::json::parse(data);
			} catch (nlohmann::json::exception &e) {
                return nlohmann::json {{"error", e.what()}};
			}
		}();

        if (msg.contains("error")) {
            LOG_ERR_FMT_("{}\n", msg["error"].get<std::string>());
			continue;
		}

		conf = config(msg, dsp.scr_count());
	}

	return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
	if (argc > 1 && strcmp(argv[1], "-v") == 0) {
		std::puts(VERSION);
		std::exit(EXIT_SUCCESS);
	}

	lock_file flock(constants::flock_filepath);
	//openlog("gummyd", LOG_FMT_PID, LOG_FMT_DAEMON);

	return message_loop();
}
