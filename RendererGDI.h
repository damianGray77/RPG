#pragma once
#ifndef RENDERERGDI_H
#define RENDERERGDI_H

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

class RendererGDI {
public:
	bool draw();
	bool resize(const uint32 width, const uint32 height);
	bool init(HWND window, void** bits, const uint32 width, const uint32 height);
	void unload();

	RendererGDI();
private:
	HWND window;
	HDC dc;
	HBITMAP dib;
	void** bits;

	BITMAPINFO info;

	uint16 color_depth;
	uint32 buffer_width;
	uint32 buffer_height;
	uint32 client_width;
	uint32 client_height;
};

#endif