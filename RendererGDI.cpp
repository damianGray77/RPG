#include "pch.h"
#include "RendererGDI.h"

RendererGDI::RendererGDI() {
	bits = NULL;
	dib  = NULL;
	dc   = NULL;

	color_depth = 32;
}

bool RendererGDI::display_buffer(const uint32 width, const uint32 height) {
	if (width == buffer_width && height == buffer_height) {
		return SetDIBitsToDevice(dc
			, 0, 0, buffer_width, buffer_height
			, 0, 0, 0, buffer_height
			, *bits
			, &info
			, DIB_RGB_COLORS
		);
	}
	else {
		return 0 == StretchDIBits(dc
			, 0, 0, width, height
			, 0, 0, buffer_width, buffer_height
			, *bits
			, &info
			, DIB_RGB_COLORS
			, SRCCOPY
		);
	}
}

bool RendererGDI::resize(const uint32 width, const uint32 height) {
	const  int new_mode = (width < buffer_width || height < buffer_height) ? HALFTONE : COLORONCOLOR;
	static int cur_mode = (width < buffer_width || height < buffer_height) ? HALFTONE : COLORONCOLOR;

	if (new_mode != cur_mode) {
		SetStretchBltMode(dc, new_mode);
		cur_mode = new_mode;
	}

	return true;
}

void RendererGDI::unload_buffer() {
	if (dc) {
		ReleaseDC(window, dc);
		dc = NULL;
	}

	if (dib) {
		DeleteObject(dib);
		dib = NULL;
	}
}

bool RendererGDI::init_buffer(HWND window, void **bits, const uint32 width, const uint32 height) {
	this->window = window;
	this->bits   = bits;

	dc = GetDC(window);

	info = {};
	info.bmiHeader = {};
	info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	info.bmiHeader.biPlanes = 1;
	info.bmiHeader.biBitCount = color_depth;
	info.bmiHeader.biCompression = BI_RGB;
	info.bmiHeader.biWidth = width;
	info.bmiHeader.biHeight = -(int32)height; // this is inverted to allow top-down per win32 documentation

	dib = CreateDIBSection(dc, &info, DIB_RGB_COLORS, this->bits, NULL, 0);
	if (NULL == dib) { return false; }

	SelectObject(dc, dib);

	buffer_width  = width;
	buffer_height = height;

	return true;
}