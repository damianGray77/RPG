#include "pch.h"
#include "RendererGDI.h"

RendererGDI::RendererGDI() {
	info   = {};
	window = NULL;

	bits = NULL;
	dib  = NULL;
	dc   = NULL;

	buffer_width  = 0;
	buffer_height = 0;
	client_width  = 0;
	client_height = 0;
	color_depth   = 32;
}

bool RendererGDI::draw() {
	if (client_width == buffer_width && client_height == buffer_height) {
		return SetDIBitsToDevice(dc
			, 0, 0, client_width, client_height
			, 0, 0, 0, client_height
			, *bits
			, &info
			, DIB_RGB_COLORS
		);
	} else {
		return 0 == StretchDIBits(dc
			, 0, 0, client_width, client_height
			, 0, 0, buffer_width, buffer_height
			, *bits
			, &info
			, DIB_RGB_COLORS
			, SRCCOPY
		);
	}
}

bool RendererGDI::resize(const uint32 width, const uint32 height) {
	if (0 == width || 0 == height) { return true; }

	const  int new_mode = (width < buffer_width || height < buffer_height) ? HALFTONE : COLORONCOLOR;
	static int cur_mode = (width < buffer_width || height < buffer_height) ? HALFTONE : COLORONCOLOR;

	if (new_mode != cur_mode) {
		SetStretchBltMode(dc, new_mode);
		cur_mode = new_mode;
	}

	client_width  = width;
	client_height = height;

	return true;
}

void RendererGDI::unload() {
	if (dc) {
		ReleaseDC(window, dc);
		dc = NULL;
	}

	if (dib) {
		DeleteObject(dib);
		dib = NULL;
	}
}

bool RendererGDI::init(HWND window, void **bits, const uint32 width, const uint32 height) {
	this->window = window;
	this->bits   = bits;

	dc = GetDC(window);

	info = {};
	info.bmiHeader = {};
	info.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
	info.bmiHeader.biPlanes      = 1;
	info.bmiHeader.biBitCount    = color_depth;
	info.bmiHeader.biCompression = BI_RGB;
	info.bmiHeader.biWidth       = width;
	info.bmiHeader.biHeight      = -(int32)height; // this is inverted to allow top-down per win32 documentation

	dib = CreateDIBSection(dc, &info, DIB_RGB_COLORS, this->bits, NULL, 0);
	if (NULL == dib) { return false; }

	SelectObject(dc, dib);

	buffer_width  = width;
	buffer_height = height;
	client_width  = width;
	client_height = height;

	return true;
}