#include "pch.h"
#include "Time.h"


void Time::init() {
	uinterval_count = 0;
	   utimer_count = 0;

	ticks = _start_timer(freq);
}

void Time::update() {
	float elapsed = (float)(_elapsed_timer(ticks, freq) * 0.001);

	for (int i = 0; i < uinterval_count; ++i) {
		intervals[i].elapsed = elapsed;
	}
}

const uint8 Time::start_timer() {
	uint8 ret = 0; // find the first available (0-value) slot
	while (timers[ret]) { ++ret; }
	if (ret >= utimer_count) { utimer_count = ret + 1; }

	timers[ret] = (float)_elapsed_timer(ticks, freq);

	return ret;
}

const float Time::stop_timer(const uint8 handle) {
	const float s = timers[handle];
	const float e = (float)_elapsed_timer(ticks, freq);

	timers[handle] = NULL;

	return e - s;
}

/// <summary>
/// Creates a repeating timer with a set duration
/// </summary>
/// <param name="duration">Duration in millisections</param>
/// <returns>A handle for the interval</returns>
const uint8 Time::set_interval(const float duration) {
	uint8 ret = 0; // find the first available (0-value) slot
	while (intervals[ret].last) { ++ret; }
	if (ret >= uinterval_count) { uinterval_count = ret + 1; }

	const float start = (float)(_elapsed_timer(ticks, freq) * 0.001);

	intervals[ret] = { start, duration };

	return ret;
}

const void Time::clear_interval(const uint8 handle) {
	intervals[handle] = {};
}

// Empty functions. These will get overridden later by whatever OS timer functions are available.
uint64 (*Time::  _start_timer)(double& freq) {};
double (*Time::_elapsed_timer)(uint64 start, double freq) {};