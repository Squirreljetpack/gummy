// Copyright 2021-2023 Francesco Fusco
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
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
    ddca_set_max_tries(DDCA_WRITE_READ_TRIES, ddca_max_max_tries());
    ddca_set_max_tries(DDCA_MULTI_PART_TRIES, ddca_max_max_tries());
    ddca_enable_verify(false);

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

std::vector<ddc::display> ddc::get_displays(std::vector<std::array<uint8_t, 256>> edids) {
    ddca_set_max_tries(DDCA_WRITE_READ_TRIES, ddca_max_max_tries());
    ddca_set_max_tries(DDCA_MULTI_PART_TRIES, ddca_max_max_tries());
    ddca_enable_verify(false);

    const ddc::display_list list;
    if (list.get()->ct == 0)
        return {};

    std::vector<ddc::display> vec;
    vec.reserve(list.get()->ct);

    if (size_t(list.get()->ct) != edids.size()) {
        throw std::runtime_error(fmt::format("[ddc] edid count mismatch: {} vs DDC: {}", edids.size(), list.get()->ct));
    }

    for (const auto &edid : edids) {
        for (size_t i = 0; i < edids.size(); ++i) {
            const auto &display_data = list.get()->info[i];

            if (std::equal(edid.begin(), edid.begin() + 128, std::begin(display_data.edid_bytes), std::end(display_data.edid_bytes))) {
                vec.emplace_back(display_data.dref);
                spdlog::info("[ddc] found: {}-{}-{}", display_data.mfg_id, display_data.model_name, display_data.sn);
                break;
            }

            if (i == edids.size() - 1) {
                const auto ddc_edid_strings = [&list] {
                    std::vector<std::string> ret(list.get()->ct);
                    for (int i = 0; i < list.get()->ct; ++i) {
                        const auto &display_data = list.get()->info[i];
                        ret[i] = fmt::format("{:02x}\n\n", fmt::join(display_data.edid_bytes, " "));
                    }
                    return ret;
                }();
                throw std::runtime_error(
                            fmt::format("could not match the first 128 bytes of this edid with any DDC edid.\n{:02x}\n\nDDC edids:\n{}",
                                        fmt::join(edid, " "), fmt::join(ddc_edid_strings, " "))
                            );
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

    const uint16_t out_val = std::clamp(uint16_t(gummyd::remap(val, 0, gummyd::constants::brt_steps_max, 0, max_brightness_)), uint16_t(0), max_brightness_);
    spdlog::debug("[ddc] setting brightness: {}/{}", out_val, max_brightness_);

    const DDCA_Status st = ddca_set_non_table_vcp_value(handle_, ddc::brightness_code, out_val >> 8, out_val & 0xFF);
    if (st != DDCRC_OK) {
        spdlog::error("[ddc] ddca_set_non_table_vcp_value error {}", st);
    }
}


