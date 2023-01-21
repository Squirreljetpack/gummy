/**
* gummy
* Copyright (C) 2022  Francesco Fusco
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

#ifndef UTILS_H
#define UTILS_H

#include <cstddef>
#include <cstdint>
#include <ctime>
#include <string>
#include <condition_variable>
#include <functional>

int set_lock();
int calc_brightness(uint8_t *buf,
                    uint64_t buf_sz,
                    int bytes_per_pixel = 4,
                    int stride = 1024);

double lerp(double x, double a, double b);
double invlerp(double x, double a, double b);
double remap(double x, double a, double b, double ay, double by);
double remap_to_idx(int val, int val_min, int val_max, size_t arr_sz);
double mant(double x);

struct Animation
{
	double elapsed;
	double slice;
	double duration_s;
	int fps;
	int start_step;
	int diff;
};

Animation animation_init(int start, int end, int fps, int duration_ms);

double ease_out_expo(double t, double b , double c, double d);
double ease_in_out_quad(double t, double b, double c, double d);
int    ease_out_expo_loop(Animation a, int prev, int cur, int end, std::function<bool (int, int)> fn);
int    ease_in_out_quad_loop(Animation a, int prev, int cur, int end, std::function<bool (int, int)> fn);

struct Timestamps
{
    std::time_t cur;
	std::time_t start;
	std::time_t end;
};

Timestamps timestamps_update(const std::string &start, const std::string &end, int seconds);
std::time_t timestamp_modify(std::time_t ts, int h, int m, int s);
std::string timestamp_fmt(std::time_t ts);

class Channel
{
    std::condition_variable cv;
	std::mutex mtx;
	int _data;
public:
	int data();
	int recv();
	int recv_timeout(int ms);
	void send(int);
};

#endif // UTILS_H
