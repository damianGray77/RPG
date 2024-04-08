#pragma once
#ifndef WINDOWS_H
#define WINDOWS_H

#include <windows.h>
#include <mmsystem.h>

#include "resource.h"
#include "Buffer.h"

class Win32 {
public:
	HWND window;
	HINSTANCE instance;
	HDC front_dc;
	HDC back_dc;
	HBITMAP dib;
	void **bits;

	BITMAPINFO info;
	MSG msg;
	RECT rect;

	uint16 buffer_width;
	uint16 buffer_height;

	uint16 window_width;
	uint16 window_height;

	ushort color_depth;
	bool fullscreen;
	LPCWSTR cname;
	LPCWSTR wname;
	int cores;

	bool resize_move;

	bool key_press[256] = {};

	void (  *draw_callback)();
	void (  *move_callback)();

	static Win32* self;

	Win32();
	~Win32() { }

	static LRESULT CALLBACK proc(HWND, uint, WPARAM, LPARAM);

	bool init(const uint16 width, const uint16 height);
	bool init_window();
	bool init_buffer();
	void unload();
	void unload_buffer();
	bool full_screen();
	bool swap_buffers();
	void resize(const uint16 width, const uint16 height);
	void sizemove();
	bool update();
	uint map_key(WPARAM);
	void close();
	void set_title(wchar_t*);

	static inline ulong get_system_ticks() {
		return timeGetTime();
	}

	static inline long long start_timer(double &freq) {
		LARGE_INTEGER lif, lic;
		if (!QueryPerformanceFrequency(&lif)) { return NULL; }
		
		QueryPerformanceCounter(&lic);
		freq = double(lif.QuadPart) / 1000000.0;
		
		return lic.QuadPart;
	}

	static inline double stop_timer(long long start, double freq) {
		LARGE_INTEGER li;
		QueryPerformanceCounter(&li);

		return double(li.QuadPart - start) / freq;
	}
private:
	LRESULT CALLBACK _proc(HWND, uint, WPARAM, LPARAM);
};

#endif
