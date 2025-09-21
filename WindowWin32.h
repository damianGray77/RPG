#pragma once
#ifndef WINDOWSWIN32_H
#define WINDOWSWIN32_H

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <mmsystem.h>

#define COLOR_DEPTH 32

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

class WindowWin32 {
public:
	static WindowWin32* self;

	HWND      window;
	HINSTANCE instance;

	uint32 width;
	uint32 height;

	LPCWSTR app_name;

	bool resize_move;

	bool key_press[256] = {};

	static LRESULT CALLBACK proc(HWND window, uint msg, WPARAM wparam, LPARAM lparam);

	void (*sizemove_callback)();
	void (    *draw_callback)();
	bool (  *resize_callback)(const uint32 width, const uint32 height);

	bool init(const uint32 width, const uint32 height);
	void unload();

	void  close();
	void  show_window();
	void  hide_console();
	void  show_console();
	bool  update();
	bool  full_screen();
	uint8 map_key(const WPARAM wparam);

	WindowWin32(
		  void (    *draw_callback)()
		, bool (  *resize_callback)(const uint32 width, const uint32 height)
		, void (*sizemove_callback)()
	);

	static inline uint32 get_system_ticks() {
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

		return double(li.QuadPart - start) * WindowWin32::query_perf_freq;
	}
private:
	uint32 cores;
	bool fullscreen;
	Bounds<uint16> minmax;

	LPCWSTR class_atom;
	MSG msg;

	static double query_perf_freq;

	LRESULT CALLBACK _proc(HWND window, uint msg, WPARAM wparam, LPARAM lparam);

	bool init_window(const uint32 width, const uint32 height);
	void update_client_size();
	void sizemove();

	const wchar_t* get_unique_classname() {
		static int counter = 0;
		static wchar_t buf[64];

		DWORD pid = GetCurrentProcessId();

		_snwprintf_s(buf, 64, L"__Process_%lu_%d", (uint32)pid, counter++);
		
		return buf;
	}
};

#endif