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

// BUG: Timers / intervals are working correctly from a timing standpoint, but
// storing them in a 256 index array and removing them or adding too many will
// cause issues with running over empty slots or overwriting in-use indices.
const uint8 Time::start_timer() {
	const uint8 ret = utimer_count++;

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
	const uint8 ret = uinterval_count++;

	const float start = (float)(_elapsed_timer(ticks, freq) * 0.001);

	intervals[ret] = { start, duration };

	return ret;
}

const void Time::clear_interval(const uint8 handle) {
	intervals[handle] = {};
}