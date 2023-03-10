// Copyright (c) 2021-2023, Francesco Fusco. All rights reserved.
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DDC_HPP
#define DDC_HPP

#include <vector>
#include <string>
#include <ddcutil_c_api.h>
#include <ddcutil_macros.h>
#include <ddcutil_status_codes.h>

namespace ddc {

class display_refs;
class display_list;

class display {
    DDCA_Display_Handle handle_;
    uint16_t max_brightness_;
public:
    display(DDCA_Display_Ref ref);
    ~display();
    DDCA_Display_Handle get() const;
    DDCA_Non_Table_Vcp_Value get_brightness_vcp() const;
    void set_brightness_step(int val);
};

std::string feature_name(DDCA_Vcp_Feature_Code vcp_code);
std::vector<display> get_displays();

} // namespace ddc

#endif // DDC_HPP
