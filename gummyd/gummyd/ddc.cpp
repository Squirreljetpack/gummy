// Copyright 2021-2024 Francesco Fusco <f.fusco@pm.me>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <span>
#include <vector>
#include <ranges>
#include <string>
#include <ddcutil_c_api.h>
#include <ddcutil_macros.h>
#include <ddcutil_status_codes.h>
#include <fmt/core.h>
#include <spdlog/spdlog.h>
#include <gummyd/utils.hpp>
#include <gummyd/constants.hpp>
#include <gummyd/ddc.hpp>

namespace ddc {
static constexpr DDCA_Vcp_Feature_Code brightness_code = 0x10;
}

class ddc::display_list {
    DDCA_Display_Info_List *list_;
public:
    display_list() {
        ddca_get_display_info_list2(true, &list_);
    }
    ~display_list() {
        ddca_free_display_info_list(list_);
    }
    DDCA_Display_Info_List *get() const {
        return list_;
    }
};

std::vector<ddc::display> ddc::get_displays() {
    const ddc::display_list list;
    std::vector<ddc::display> vec;
    vec.reserve(list.get()->ct);

    for (int i = 0; i < list.get()->ct; ++i) {
        const auto &display_data = list.get()->info[i];
        spdlog::info("[ddc] found: {}-{}-{}", display_data.mfg_id, display_data.model_name, display_data.sn);
        vec.emplace_back(display_data.dref);
    }

    return vec;
}

std::vector<ddc::display> ddc::get_displays(std::vector<std::array<uint8_t, 128>> edids) {
    ddca_enable_verify(false);

    if (edids.size() == 0)
        return ddc::get_displays();

    const ddc::display_list list;
    if (list.get()->ct == 0)
        return {};

    if (size_t(list.get()->ct) != edids.size()) {
        spdlog::warn("[ddc] display count mismatch. input: {} DDC: {}.", edids.size(), list.get()->ct);
    }

    std::vector<ddc::display> vec;
    vec.reserve(list.get()->ct);

    // Displays not found in the input vector will be ignored.
    for (const auto &edid : edids) {
        for (const auto &display_data : std::span(list.get()->info, list.get()->ct)) {
            if (std::ranges::equal(edid, display_data.edid_bytes)) {
                vec.emplace_back(display_data.dref);
                spdlog::info("[ddc] found: {}-{}-{}", display_data.mfg_id, display_data.model_name, display_data.sn);
            }
        }
    }

    return vec;
}

ddc::display::display(DDCA_Display_Ref ref) : max_brightness_(0) {
    const DDCA_Status st = ddca_open_display2(ref, true, &handle_);
    if (st != DDCRC_OK) {
        throw std::runtime_error(fmt::format("ddca_open_display2 {}", st));
    }
}

ddc::display::display(ddc::display &&o) : handle_(o.handle_) {
    o.handle_ = nullptr;
}

ddc::display::~display() {
    ddca_close_display(handle_);
}

DDCA_Display_Handle ddc::display::get() const {
    return handle_;
}

int ddc::display::get_brightness() const {
    try {
        const DDCA_Non_Table_Vcp_Value brightness = get_brightness_vcp();
        return (brightness.sh << 8 | brightness.sl);
    } catch (std::runtime_error &e) {
        return -1;
    }
}

DDCA_Non_Table_Vcp_Value ddc::display::get_brightness_vcp() const {
    DDCA_Non_Table_Vcp_Value val;
    const DDCA_Status st = ddca_get_non_table_vcp_value(handle_, ddc::brightness_code, &val);
    if (st != DDCRC_OK) {
        throw std::runtime_error(fmt::format("ddca_get_non_table_vcp_value error {} ({})", st, ddca_rc_desc(st)));
    }
    return val;
}

void ddc::display::set_brightness_step(int val) {
    if (max_brightness_ == 0) {
        try {
            const DDCA_Non_Table_Vcp_Value brightness = get_brightness_vcp();
            max_brightness_ = brightness.mh << 8 | brightness.ml;
        } catch (std::runtime_error &e) {
            spdlog::error("[ddc] exception: {}.", e.what());
            return;
        }
    }

    const double tmp = gummyd::remap(val, 0, gummyd::constants::brt_steps_max, 0, max_brightness_);
    const uint16_t out_val = std::clamp(uint16_t(std::round(tmp)), uint16_t(0), max_brightness_);
    spdlog::debug("[ddc] setting brightness: {}/{}", out_val, max_brightness_);

    const DDCA_Status st = ddca_set_non_table_vcp_value(handle_, ddc::brightness_code, out_val >> 8, out_val & 0xFF);
    if (st != DDCRC_OK) {
        spdlog::error("[ddc] ddca_set_non_table_vcp_value error {}", st);
    }
}


