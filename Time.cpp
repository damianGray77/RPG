#include "pch.h"
#include "Time.h"


void Time::init() {
	uinterval_count = 0;
	   utimer_count = 0;
	
	current_timer = 0;

	ticks = _start_timer();
}

void Time::update() {
	float elapsed = (float)(_elapsed_timer(ticks) * 0.001);

	for (int i = 0; i < uinterval_count; ++i) {
		intervals[i].elapsed = elapsed;
	}
}

uint8 Time::start_timer() {
	const uint8 index = current_timer++;
	if (index >= utimer_count) { utimer_count = index + 1; }

	timers[index] = (float)_elapsed_timer(ticks);

	return index;
}

float Time::stop_timer(const uint8 handle) {
	const float start = timers[handle];
	const float end = (float)_elapsed_timer(ticks);

	timers[handle] = 0.0f;

	return end - start;
}

/// <summary>
/// Creates a repeating timer with a set duration
/// </summary>
/// <param name="duration">Duration in millisections</param>
/// <returns>A handle for the interval</returns>
uint8 Time::set_interval(const float duration) {
	uint8 ret = 0; // find the first available (0-value) slot
	while (intervals[ret].last) { ++ret; }
	if (ret >= uinterval_count) { uinterval_count = ret + 1; }

	const float start = (float)(_elapsed_timer(ticks) * 0.001);

	intervals[ret] = { start, duration };

	return ret;
}

void Time::clear_interval(const uint8 handle) {
	intervals[handle] = {};
}

// Empty functions. These will get overridden later by whatever OS timer functions are available.
uint64 (*Time::  _start_timer)() = NULL;
double (*Time::_elapsed_timer)(uint64 start) = NULL;