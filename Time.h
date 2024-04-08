#pragma once
#ifndef TIME_H
#define TIME_H

struct Interval {
	float last;
	float dur;
	float elapsed;
	float delta;

	inline const bool update() {
		const float diff = elapsed - last;
		if (diff < dur) { return false; }

		last = elapsed;
		delta = dur / diff;

		return true;
	}
};

class Time {
	uint8    utimer_count;
	uint8 uinterval_count;

	uint64 ticks;
	double freq;
public:
	bool  update_frame;
	float delta;

	float       timers[256] = { };
	Interval intervals[256] = { };

	long long(*_start_timer)(double &);
	double(*_elapsed_timer)(long long, double);

	void init();
	void update();

	const uint8 start_timer();
	const float  stop_timer(const uint8);

	const uint8  set_interval(const float duration);
	const void clear_interval(const uint8 handle);
};

#endif