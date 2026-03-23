#include "pch.h"
#include "Main.h"
#include "Thread.h"

#define RES_X 640
#define RES_Y 480
#define FPS_TARGET 60

Buffer buffer;
Game   game;

WINDOW     *window;
RENDERER   *renderer;
ThreadPool  threads;

Time time;
Interval *fps;

uint8 fps_handle;

uint8 frames = 0;
uint16 frame_times[FPS_TARGET] = {};
double frame_time = 0;

int main(int argc, char* argv[]) {
#ifdef DEBUG_OUT
	window->show_console();
#else
	window->hide_console();
#endif

	init();

	alloc_buffer(buffer, RES_X, RES_Y);

	window   = new WINDOW(draw, resize, sizemove);
	renderer = new RENDERER();

	window->app_name = APP_NAME;
	game.buffer    = &buffer;
	game.key_press = window->key_press;
	Time::  _start_timer = WINDOW::start_timer;
	Time::_elapsed_timer = WINDOW:: stop_timer;

	configure_buffer(buffer, RES_X, RES_Y);

	if (
		   game.init()
		&& game.resize(RES_X, RES_Y)
		&& window->init(RES_X, RES_Y)
		&& threads.init(window, ThreadPool::MAX_WORKERS)
		&& renderer->init(window->window, &threads, (void**)&buffer.bits, RES_X, RES_Y)
	) {
		time.init();

		fps_handle = time.set_interval(1000.0f / (float)FPS_TARGET);
		fps = &time.intervals[fps_handle];

		window->show_window();

		run();

		time.clear_interval(fps_handle);
	}

	threads.shutdown();
	window->  unload();
	renderer->unload();
	clear_buffer(buffer);
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
		if (!window->update()) { break; }

		execute_frame();
	} while (game.running);

	window->close();
}

void sizemove() {
	execute_frame();
}

void execute_frame() {
	time.update();

	if (!fps->update()) { return; }

#ifdef DEBUG_OUT
	const uint8 frame_handle = time.start_timer();
#endif

	game.update(fps->delta);

	draw();

#ifdef DEBUG_OUT
	const double frame_elapsed = time.stop_timer(frame_handle);
	frame_times[frames++] = ceil<uint16, double>(frame_elapsed);
	frame_time += frame_elapsed;
	if (frames == FPS_TARGET) {
		char buf[512];
		int len = sprintf_s(buf, sizeof(buf), "Avg Frame Time: %.2f\xE6s (%d", frame_time / frames, frame_times[0]);
		for (uint8 i = 1; i < frames; ++i) {
			len += sprintf_s(buf + len, sizeof(buf) - len, " %d", frame_times[i]);
		}
		len += sprintf_s(buf + len, sizeof(buf) - len, ")\r\n");
		fwrite(buf, 1, len, stdout);

		frame_time = 0;
		frames     = 0;
	}
#endif
}

inline bool resize(const uint32 width, const uint32 height) {
	if (!renderer->resize(width, height)) { return false; }

	if (window->resize_move) {
		game.refresh = true;
	} else {
		renderer->draw();
	}

	return true;
}

inline void draw() {
	if (game.render()) {
		const Bounds<int32>& b = game.dirty_px_bounds;
		renderer->draw(b.min.x, b.min.y, b.max.x - b.min.x, b.max.y - b.min.y);
	}
}