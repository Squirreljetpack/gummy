#ifndef EASING_HPP
#define EASING_HPP

#include <chrono>
#include <thread>
#include <functional>
#include <cmath>

struct Animation
{
    double frame;
	double time;
	double duration_s;
	int start;
	int diff;
	int fps;
	Animation(int start, int end, int fps, int duration_ms);
};

inline Animation::Animation(int start, int end, int fps, int duration_ms)
{
	this->start      = start;
	this->diff       = end - start;
	this->duration_s = duration_ms / 1000.;
	this->fps   = fps;
	this->frame = 1. / fps;
	this->time  = 0.;
}

inline double ease_out_expo(double t, double b , double c, double d)
{
	return (t == d) ? b + c : c * (-pow(2, -10 * t / d) + 1) + b;
}

inline double ease_in_out_quad(double t, double b, double c, double d)
{
	if ((t /= d / 2) < 1)
		return c / 2 * t * t + b;
	else
		return -c / 2 * ((t - 1) * (t - 3) - 1) + b;
}

inline int ease_in_out_quad_loop(Animation a, int prev, int cur, int end, std::function<bool(int, int)> fn)
{
	if (!fn(cur, prev) || cur == end)
		return cur;

	std::this_thread::sleep_for(std::chrono::milliseconds(1000 / a.fps));

	return ease_in_out_quad_loop(a,
	    cur,
	    int(ease_in_out_quad(a.time += a.frame, a.start, a.diff, a.duration_s)),
	    end,
	    fn);
}

inline int ease_out_expo_loop(Animation a, int prev, int cur, int end, std::function<bool(int, int)> fn)
{
	if (!fn(cur, prev) || cur == end)
		return cur;

	std::this_thread::sleep_for(std::chrono::milliseconds(1000 / a.fps));

	return ease_out_expo_loop(a,
	    cur,
	    int(round(ease_out_expo(a.time += a.frame, a.start, a.diff, a.duration_s))),
	    end,
	    fn);
}


#endif // EASING_HPP
