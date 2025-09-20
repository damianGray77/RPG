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

		last  = elapsed;
		delta = diff * 0.001f;

		return true;
	}
};

class Time {
private:
	uint8    utimer_count;
	uint8 uinterval_count;
	uint8 current_timer;

public:
	bool  update_frame;
	float delta;
private:
	uint64 ticks;
public:
	float       timers[256] = { };
	Interval intervals[256] = { };

	static uint64(*  _start_timer)();
	static double(*_elapsed_timer)(uint64 start);

	void init();
	void update();

	uint8 start_timer();
	float  stop_timer(const uint8);

	uint8  set_interval(const float duration);
	void clear_interval(const uint8 handle);
};

#endif