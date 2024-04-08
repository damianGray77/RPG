#include "pch.h"
#include "Main.h"

#define RES_X 640
#define RES_Y 480

Buffer buffer;
Game game;
Win32 window;
Time time;
Interval *fps;

uint fps_target = 60;
uint8 fps_handle;

uint frames = 0;
double frame_time = 0;

int main(int argc, char* argv[]) {
	init();

	game.buffer = &buffer;
	game.key_press = window.key_press;
	window.bits = (void**)(&buffer.bits);
	window.draw_callback = draw;
	window.move_callback = move;
	time  ._start_timer = Win32::start_timer;
	time._elapsed_timer = Win32:: stop_timer;

	if (
		   game  .init()
		&& buffer.init(RES_X, RES_Y)
		&& game.resize(RES_X, RES_Y)
		&& window.init(RES_X, RES_Y)
	) {
		time.init();

		fps_handle = time.set_interval((float)(1000.0 / (double)fps_target));
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
	do {
		if (!window.update()) { break; }

		execute_frame();
	} while (game.running);

	window.close();
}

void move() {
	const uint8 move_handle = time.set_interval(100.0);
	Interval &move = time.intervals[move_handle];

	while (game.running && window.resize_move && !move.update()) {
		execute_frame();
	}

	time.clear_interval(move_handle);
}

void execute_frame() {
	time.update();

	if(fps->update()) {
		const uint8 frame_handle = time.start_timer();

		game.update(fps->delta);

		draw();

		frame_time += time.stop_timer(frame_handle);
		++frames;

		if (frames == fps_target) {
			printf("Avg Frame Time: %.2f\xE6s\n", frame_time / frames);
			frame_time = 0;
			frames     = 0;
		}
	} else {
		Sleep(1);
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