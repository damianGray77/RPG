#pragma once
#ifndef RENDERER_H
#define RENDERER_H

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

struct IRenderer {
	bool (  *display_buffer_callback)(const void *self, const uint32 width, const uint32 height);
	void (*set_display_mode_callback)(const void *self, const uint32 width, const uint32 height);
	bool (     *init_buffer_callback)(const void *self, HWND window, void** bits, const uint32 width, const uint32 height);
	void (   *unload_buffer_callback)(const void *self);

	const void* self;

	inline bool display_buffer(const uint32 width, const uint32 height) const {
		return display_buffer_callback(self, width, height);
	}

	inline void set_display_mode(const uint32 width, const uint32 height) const {
		set_display_mode_callback(self, width, height);
	}

	inline bool init_buffer(HWND window, void** bits, const uint32 width, const uint32 height) const {
		return init_buffer_callback(self, window, bits, width, height);
	}

	inline void unload_buffer() const {
		unload_buffer_callback(self);
	}
};

#endif