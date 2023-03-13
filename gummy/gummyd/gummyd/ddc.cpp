// Copyright (c) 2021-2023, Francesco Fusco. All rights reserved.
// SPDX-License-Identifier: GPL-3.0-or-later

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

// currently unused
class ddc::display_refs {
    DDCA_Display_Ref *refs_;
public:
    display_refs() {
        ddca_get_display_refs(false, &refs_);
    }
    ~display_refs() {
        ddca_free_display_ref(refs_);
    }
    DDCA_Display_Ref *get() const {
        return refs_;
    }
};

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

class ddc_error {
    DDCA_Error_Detail *error_;
public:
    ddc_error() {
        error_ = ddca_get_error_detail();
    }
    ~ddc_error() {
        ddca_free_error_detail(error_);
    }
    DDCA_Error_Detail *get() const {
        return error_;
    }
    std::string get_error() {
        if (!error_) {
            return "ddca_get_error_detail unavailable";
        }
        auto ret = fmt::format("error count: {} detail: {}. Causes:\n", error_->cause_ct, error_->detail);
        for (int i = 0; i < error_->cause_ct; ++i) {
            ret += fmt::format("{}: {}\n", i, error_->causes[i]->detail);
        }
        return ret;
    }
};

std::string ddc::feature_name(DDCA_Vcp_Feature_Code vcp_code) {
    return ddca_get_feature_name(vcp_code);
}

std::vector<ddc::display> ddc::get_displays() {
    ddca_set_max_tries(DDCA_WRITE_READ_TRIES, ddca_max_max_tries());
    ddca_set_max_tries(DDCA_MULTI_PART_TRIES, ddca_max_max_tries());

    const ddc::display_list list;
    std::vector<ddc::display> vec;

    for (int i = 0; i < list.get()->ct; ++i) {
        const auto &info = list.get()->info[i];
        spdlog::info("[ddc] found model: {} [{}]", info.model_name, i);
        vec.emplace_back(info.dref);
    }

    return vec;
}

ddc::display::display(DDCA_Display_Ref ref) : max_brightness_(0) {
    const DDCA_Status st = ddca_open_display2(ref, true, &handle_);
    if (st != DDCRC_OK) {
        throw std::runtime_error(fmt::format("ddca_open_display2 {}", st));
    }
}

ddc::display::~display() {
    ddca_close_display(handle_);
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

DDCA_Display_Handle ddc::display::get() const {
    return handle_;
}

