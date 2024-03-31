#pragma once
#ifndef FPS_H
#define FPS_H

struct timer {
	ulonglong ticks;
	double    freq;
};

class FPS {
	ulong frames_since;
	ulong next_calc_ticks;
	ulong last_ticks;
	float frame_ticks;
	ulong current_ticks;
	ulong total_frames;
	ulong seconds;

	float target;

	ubyte utimer_count;
	timer timers[256] = { };
public:
	bool  update_frame;
	float delta;

	ulong(*get_system_ticks)();
	long long(*_start_timer)(double &);
	double(*_stop_timer)(long long, double);

	void init();
	void set_rate(const int);
	void update();

	ubyte start_timer();
	double stop_timer(const ubyte);
};

#endif