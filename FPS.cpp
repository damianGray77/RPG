#include "pch.h"
#include "FPS.h"


void FPS::init() {
	ulong ticks = get_system_ticks();

	frames_since    = 0;
	next_calc_ticks = ticks + 1000;
	last_ticks      = ticks;
	frame_ticks     = 0;
	current_ticks   = 0;
	total_frames    = 0;
	seconds         = 0;
	target          = 0;
	utimer_count    = 0;
}

void FPS::set_rate(const int rate) {
	target = 1000.0f / rate;
}

void FPS::update() {
	current_ticks = get_system_ticks();

	frame_ticks += (float)(current_ticks - last_ticks);
	last_ticks = current_ticks;

	if (frame_ticks >= target) {
		++frames_since;
		delta = target / frame_ticks;

		frame_ticks -= target;

		update_frame = true;
	} else {
		update_frame = false;
	}

	if (current_ticks >= next_calc_ticks) {
		++seconds;
		next_calc_ticks = current_ticks + 1000;
		total_frames += frames_since;

		//printf("FPS: %d Avg.: %d Ms/F: %f\n", frames_since, ceil(total_frames / (float)seconds), 1000.0f / frames_since);

		frames_since = 0;
	}
}

// BUG: if there is a long running timer, and more than 255 other timers in 
// that time the running one will get overridden while it is still running.
ubyte FPS::start_timer() {
	ubyte ret = utimer_count++;

	double freq;
	ulonglong ticks = _start_timer(freq);
	timers[ret] = { ticks, freq };

	return ret;
}

double FPS::stop_timer(const ubyte handle) {
	timer t = timers[handle];

	double elapsed = _stop_timer(t.ticks, t.freq);

	timers[handle] = {};

	return elapsed;
}