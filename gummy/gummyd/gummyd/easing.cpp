// Copyright 2021-2023 Francesco Fusco
// SPDX-License-Identifier: GPL-3.0-or-later

#include <fmt/chrono.h>
#include <spdlog/spdlog.h>

#include <gummyd/easing.hpp>
#include <gummyd/utils.hpp>

namespace gummyd {
namespace easing {

// Easing functions adapted from:
// https://github.com/warrenm/AHEasing/blob/master/AHEasing/easing.c

double ease(double t) {
    return t;
};

double ease_out_expo(double t) {
    return (t == 1.0) ? t : 1 - pow(2, -10 * t);
};

double ease_in_out_quad(double t) {
    if (t < 0.5)
        return 2 * t * t;
    else
        return (-2 * t * t) + (4 * t) - 1;
}

int animate(int start, int end, int duration_ms, std::function<double(double t)> easing, std::function<void(int)> fn, std::function<bool()> interrupt) {
    if (start == end) {
        fn(end);
        return end;
    }

    using namespace std::chrono;
    using namespace std::chrono_literals;

    int cur = start;
    int prev;

    const nanoseconds animation_time((milliseconds(duration_ms)));
    nanoseconds progress(0);

    const time_point<steady_clock> begin(steady_clock::now());

    while (true) {

        prev = cur;
        cur = lerp(start, end, std::min(easing(double(progress.count()) / animation_time.count()), 1.));

        if (prev != cur) {
            SPDLOG_TRACE("[easing] cur: {}, progress {}/{}", cur, duration_cast<seconds>(progress), duration_cast<seconds>(animation_time));
            fn(cur);
        }

        if (interrupt() || progress.count() >= animation_time.count()) {
            break;
        }

        std::this_thread::sleep_for(1ms);
        progress = (steady_clock::now() - begin);
    }

    spdlog::debug("[easing] animation over in {}", duration_cast<milliseconds>(steady_clock::now() - begin));
    return cur;
}

}
}
