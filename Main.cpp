#include "pch.h"
#include "Main.h"

Game game;
Win32 window;
FPS fps;

uint frames = 0;
double frame_time = 0;

int main(int argc, char* argv[]) {
	init();

	game.buffer = &buffer;
	game.key_press = window.key_press;
	window.bits = (void**)(&buffer.bits);
	window  .resize_callback = resize;
	window    .draw_callback = draw;
	fps.get_system_ticks = Win32::get_system_ticks;
	fps._start_timer = Win32::start_timer;
	fps. _stop_timer = Win32:: stop_timer;

	if (
		   game  .init()
		&& buffer.init(window.width, window.height)
		&& window.init()
	) {
		fps.init();
		
		run();
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
	int fps_target = 60;

	fps.set_rate(fps_target);

	do {
		if (!window.update()) { break; }

		fps.update();
		if (fps.update_frame) {
			const int fps_handle = fps.start_timer();

			game.update(fps.delta);

			draw();

			frame_time += fps.stop_timer(fps_handle);
			++frames;

			if (frames == fps_target) {
				//printf("Avg Frame Time: %.2f\xE6s\n", frame_time / frames);
				frame_time = 0;
				frames     = 0;
			}
		} else {
			Sleep(1);
		}
	} while (game.running);

	window.close();
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