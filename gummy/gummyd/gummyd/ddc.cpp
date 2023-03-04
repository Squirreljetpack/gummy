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

class ddc::info_list {
    DDCA_Display_Info_List *list_;
public:
    info_list() {
        ddca_get_display_info_list2(false, &list_);
    }
    ~info_list() {
        ddca_free_display_info_list(list_);
    }
    DDCA_Display_Info_List *get() const {
        return list_;
    }
};

constexpr DDCA_Vcp_Feature_Code ddc::brightness_code = 0x10;

std::string ddc::feature_name(DDCA_Vcp_Feature_Code vcp_code) {
    return ddca_get_feature_name(vcp_code);
}

std::vector<ddc::display> ddc::get_displays() {
    if (ddc::feature_name(ddc::brightness_code) != "Brightness") {
        throw std::runtime_error(fmt::format("DDC: {} not Brightness", ddc::brightness_code));
    }

    ddc::info_list list;
    std::vector<ddc::display> vec;

    for (int i = 0; i < list.get()->ct; ++i) {
        const auto &info = list.get()->info[i];
        spdlog::debug("ddc [{}] = disp_no: {}, model: {}", i, info.dispno, info.model_name);
        vec.emplace_back(info.dref);
    }

    return vec;
}

ddc::display::display(DDCA_Display_Ref ref) {
    DDCA_Status st = ddca_open_display2(ref, true, &handle_);
    if (st != 0) {
        throw std::runtime_error("ddca_open_display2 " + std::to_string(st));
    }

    st = ddca_get_feature_metadata_by_dh(ddc::brightness_code, handle_, false, &info_);
    if (st != 0) {
        throw std::runtime_error("ddca_get_feature_metadata_by_dh " + std::to_string(st));
    }

    spdlog::info("[ddc] Feature is simple NC: {}", info_->feature_flags & DDCA_SIMPLE_NC);
}

DDCA_Non_Table_Vcp_Value ddc::display::get_brightness_vcp() const {
    DDCA_Non_Table_Vcp_Value val;
    const DDCA_Status st = ddca_get_non_table_vcp_value(handle_, info_->feature_code, &val);
    if (st != 0) {
        throw std::runtime_error("ddca_get_non_table_vcp_value " + std::to_string(st));
    }
    return val;
}

void ddc::display::set_brightness_vcp(uint16_t new_val) {

    const DDCA_Non_Table_Vcp_Value old_val = get_brightness_vcp();

    const uint16_t max_val = old_val.mh << 8 | old_val.ml;
    const uint16_t cur_val = old_val.sh << 8 | old_val.sl;

    const uint16_t out_val = std::clamp(uint16_t(gummyd::remap(new_val, 0, gummyd::constants::brt_steps_max, 0, 100)), uint16_t(0), max_val);

    if (out_val == cur_val) {
        return;
    }

    const uint8_t out_sh = out_val >> 8;
    const uint8_t out_sl = out_val & 0xff;

    spdlog::debug("[ddc] setting brightness: {}/{}, prev: {}", out_val, max_val, cur_val);
    const DDCA_Status st = ddca_set_non_table_vcp_value(handle_, info_->feature_code, out_sh, out_sl);

    if (st != 0) {
        spdlog::error("[ddc] ddca_set_non_table_vcp_value", st);
        if (st == DDCRC_VERIFY) {
            spdlog::error("[ddc] value verification failed");
        }
        return;
    }
}

DDCA_Display_Handle ddc::display::get() const {
    return handle_;
}

ddc::display::~display() {
    std::free(info_);
    ddca_close_display(handle_);
}
