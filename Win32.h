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

	Bounds<uint16> minmax;

	ushort color_depth;
	bool fullscreen;
	LPCWSTR cname;
	LPCWSTR wname;
	int cores;

	bool resize_move;

	bool key_press[256] = {};

	void (    *draw_callback)();
	void (*sizemove_callback)();

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

	static inline uint64 start_timer() {
		LARGE_INTEGER li;
		QueryPerformanceCounter(&li);
		
		return li.QuadPart;
	}

	static inline double stop_timer(uint64 start) {
		LARGE_INTEGER li;
		QueryPerformanceCounter(&li);

		return double(li.QuadPart - start) * Win32::query_perf_freq;
	}
private:
	static double query_perf_freq;

	LRESULT CALLBACK _proc(HWND, uint, WPARAM, LPARAM);
};

#endif
