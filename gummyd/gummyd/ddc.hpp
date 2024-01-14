// Copyright 2021-2024 Francesco Fusco
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DDC_HPP
#define DDC_HPP

#include <array>
#include <vector>
#include <string>
#include <ddcutil_c_api.h>
#include <ddcutil_macros.h>
#include <ddcutil_status_codes.h>

namespace ddc {

class display_list;
class display {
    DDCA_Display_Handle handle_;
    uint16_t max_brightness_;
public:
    display(DDCA_Display_Ref ref);
    ~display();
    display(display &&o);
    DDCA_Display_Handle get() const;
    DDCA_Non_Table_Vcp_Value get_brightness_vcp() const;
    void set_brightness_step(int val);
    int get_brightness() const;
};

std::vector<display> get_displays();
std::vector<display> get_displays(std::vector<std::array<uint8_t, 128>> edids);

} // namespace ddc

#endif // DDC_HPP
