#pragma once
#ifndef RENDERERGDI_H
#define RENDERERGDI_H

#define WIN32_LEAN_AND_MEAN

#include "Renderer.h"

#include <windows.h>

class RendererGDI {
public:
	HWND window;
	HDC dc;
	HBITMAP dib;
	void** bits;

	BITMAPINFO info;

	uint16 color_depth;
	uint32 buffer_width;
	uint32 buffer_height;

	bool display_buffer(const uint32 width, const uint32 height);
	void set_display_mode(const uint32 width, const uint32 height);
	bool init_buffer(HWND window, void** bits, const uint32 width, const uint32 height);
	void unload_buffer();

	RendererGDI();

	inline static IRenderer create_renderer(RendererGDI *gdi_renderer) {
		IRenderer renderer;
		renderer  .display_buffer_callback = RendererGDI::  display_buffer_callback;
		renderer.set_display_mode_callback = RendererGDI::set_display_mode_callback;
		renderer     .init_buffer_callback = RendererGDI::     init_buffer_callback;
		renderer   .unload_buffer_callback = RendererGDI::   unload_buffer_callback;
		renderer.self = gdi_renderer;

		return renderer;
	}
private:
	inline static bool display_buffer_callback(const void* self, const uint32 width, const uint32 height) {
		return ((RendererGDI*)self)->display_buffer(width, height);
	}

	inline static void set_display_mode_callback(const void* self, const uint32 width, const uint32 height) {
		((RendererGDI*)self)->set_display_mode(width, height);
	}

	inline static bool init_buffer_callback(const void* self, HWND window, void** bits, const uint32 width, const uint32 height) {
		return ((RendererGDI*)self)->init_buffer(window, bits, width, height);
	}

	inline static void unload_buffer_callback(const void* self) {
		((RendererGDI*)self)->unload_buffer();
	}
};

#endif