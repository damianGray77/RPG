#pragma once
#ifndef WINDOWS_H
#define WINDOWS_H

#include <windows.h>
#include <mmsystem.h>

#include "resource.h"
#include "Buffer.h"

#define VK_0 0x30
#define VK_1 0x31
#define VK_2 0x32
#define VK_3 0x33
#define VK_4 0x34
#define VK_5 0x35
#define VK_6 0x36
#define VK_7 0x37
#define VK_8 0x38
#define VK_9 0x39

#define VK_A 0x41
#define VK_B 0x42
#define VK_C 0x43
#define VK_D 0x44
#define VK_E 0x45
#define VK_F 0x46
#define VK_G 0x47
#define VK_H 0x48
#define VK_I 0x49
#define VK_J 0x4A
#define VK_K 0x4B
#define VK_L 0x4C
#define VK_M 0x4D
#define VK_N 0x4E
#define VK_O 0x4F
#define VK_P 0x50
#define VK_Q 0x51
#define VK_R 0x52
#define VK_S 0x53
#define VK_T 0x54
#define VK_U 0x55
#define VK_V 0x56
#define VK_W 0x57
#define VK_X 0x58
#define VK_Y 0x59
#define VK_Z 0x60

struct ClientDimensions {
	uint32 width;
	uint32 height;
};

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

	ClientDimensions client_dims;

	uint32 buffer_width;
	uint32 buffer_height;

	Bounds<uint16> minmax;

	uint16 color_depth;
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

	bool  init(const uint32 width, const uint32 height);
	bool  init_window(const int32 width, const int32 height);
	bool  init_buffer(const uint32 width, const uint32 height);
	void  unload();
	void  unload_buffer();
	bool  full_screen();
	bool  display_buffer();
	void  sizemove();
	void  hide_console();
	void  show_console();
	ClientDimensions get_client_dimensions();
	bool  update();
	uint8 map_key(WPARAM);
	void  close();
	void  set_title(wchar_t *);

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
