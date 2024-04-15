#include "pch.h"
#include "Main.h"

#define RES_X 640
#define RES_Y 480
#define FPS_TARGET 20

Buffer buffer;
Game game;
Win32 window;
Time time;
Interval *fps;

uint8 fps_handle;

uint8 frames = 0;
uint16 frame_times[FPS_TARGET] = {};
double frame_time = 0;
bool async_frame = false;

int main(int argc, char* argv[]) {
	init();

	game.buffer = &buffer;
	game.key_press = window.key_press;
	window.bits = (void**)(&buffer.bits);
	window    .draw_callback = draw;
	window.sizemove_callback = sizemove;
	Time::  _start_timer = Win32::start_timer;
	Time::_elapsed_timer = Win32:: stop_timer;

	if (
		   game  .init()
		&& buffer.init(RES_X, RES_Y)
		&& game.resize(RES_X, RES_Y)
		&& window.init(RES_X, RES_Y)
	) {
		time.init();

		fps_handle = time.set_interval(1000.0f / (float)FPS_TARGET);
		fps = &time.intervals[fps_handle];

		run();

		time.clear_interval(fps_handle);
	}

	window.unload();
	buffer.unload();
	game.unload();

	return 0;
}

void init() {
	init_lookups();
}

void init_lookups() {
	const float mul = PI / (SINCOSMAX * 0.5f);

	for (int i = 0; i < SINCOSMAX; ++i) {
		const float val = i * mul;

		const float sin_val = (float)sin(val);
		const float cos_val = (float)cos(val);

		sins[i] = sin_val;
		isins[i] = 1.0f / sin_val;

		coss[i] = cos_val;
		icoss[i] = 1.0f / cos_val;
	}
}

void run() {
	game.running = true;

	do {
		if (!window.update()) { break; }

		if (!async_frame) {
			execute_frame();
		}
	} while (game.running);

	window.close();
}

void sizemove() {
	const uint8 sm_handle = time.set_interval(100.0);
	Interval &sm = time.intervals[sm_handle];

	while (game.running && window.resize_move && !sm.update()) {
		if (async_frame) {
			Sleep(1);
			continue;
		}
		
		async_frame = true;
		execute_frame();
		async_frame = false;
	}

	time.clear_interval(sm_handle);
}

void execute_frame() {
	time.update();

	if (!fps->update()) {
		Sleep(1);
		return;
	}

	const uint8 frame_handle = time.start_timer();

	game.update(fps->delta);

	draw();

	const double frame_elapsed = time.stop_timer(frame_handle);
	frame_times[frames++] = ceil<uint16, double>(frame_elapsed);
	frame_time += frame_elapsed;
	if (frames == FPS_TARGET) {
		printf("Avg Frame Time: %.2f\xE6s (%d", frame_time / frames, frame_times[0]);
		for (uint8 i = 1; i < frames; ++i) {
			printf(" %d", frame_times[i]);
		}
		printf(")\r\n");

		frame_time = 0;
		frames     = 0;
	}
}

inline bool resize(int w, int h) {
	if (!buffer.init(w, h)) { return false; }
	const bool res = game.resize(w, h);
	draw();

	return res;
}

inline void draw() {
	if (game.render()) {
		window.swap_buffers();
	}
}