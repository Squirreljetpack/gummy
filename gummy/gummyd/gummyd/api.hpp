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

#ifndef API_HPP
#define API_HPP

#include <nlohmann/json_fwd.hpp>

namespace gummyd {
    bool daemon_start();
    bool daemon_stop();
    bool daemon_is_running();
    void daemon_send(const std::string &s);

    nlohmann::json config_get_current();
    int config_invalid_val();
    bool config_is_valid(int);
    bool config_is_valid(double);
    bool config_is_valid(std::string);

    std::pair<int, int> brightness_range();
    std::pair<int, int> temperature_range();
}

#endif // API_HPP
