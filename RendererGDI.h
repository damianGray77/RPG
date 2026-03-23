#pragma once
#ifndef RENDERERGDI_H
#define RENDERERGDI_H

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <immintrin.h>

#include "Thread.h"

class RendererGDI {
public:
	bool draw();
	bool draw(int32 x, int32 y, int32 w, int32 h);
	bool resize(const uint32 width, const uint32 height);
	bool init(HWND window, ThreadPool *threads, void** bits, const uint32 width, const uint32 height);
	void unload();

	RendererGDI();

	// box filter structs (public for static worker access)
	struct BoxColSpan {
		uint16 sx0;            // first source column
		uint16 sx_inner_end;   // end of interior columns (right edge pixel index when wn_last > 0)
		uint32 wn_first;       // wx_first * hrecip (pre-combined weight+normalize)
		uint32 wn_full;        // 256 * hrecip (interior full-weight pixels)
		uint32 wn_last;        // wx_last * hrecip (0 = no right edge)
	};  // 16 bytes

	struct BoxRowSpan {
		uint16 sy0;
		uint16 sy1;           // exclusive end
		uint16 lerp_w;        // 2-row: blend weight. 3-row: w01 (weight of c1 in c0+c1)
		uint16 lerp_w2;       // 3-row: weight of c2 in final blend (unused for 1-2 row)
	};  // 8 bytes

private:
	void scale_nn_int(int32 dx, int32 dy, int32 dw, int32 dh);
	void scale_box(int32 dx, int32 dy, int32 dw, int32 dh);
	// box_hpass is now a free function (box_hpass_static) for thread access
	void scale_sharp(int32 dx, int32 dy, int32 dw, int32 dh);

	static const uint32 MAX_THREADS = ThreadPool::MAX_WORKERS + 1;

	struct ScaleSharpCtx {
		const uint32*  src;
		uint32*        front_bits;
		const uint32*  sx_lut;
		const uint32*  sx1_lut;
		const uint16*  wx_lut;
		const __m256i* wx_lo_lut;
		const __m256i* wx_hi_lut;
		const uint32*  sy_lut;
		const uint32*  wy_lut;
		uint32*        temp_row_a;
		uint32*        temp_row_b;
		uint32  buffer_width;
		uint32  buffer_height;
		uint32  client_width;
		int32   dx;
		int32   dx2;
		int32   dw;
	};

	static void scale_sharp_worker(void* ctx, int32 row_start, int32 row_end);

	struct ScaleBoxCtx {
		const uint32*     src;
		uint32*           front_bits;
		const BoxColSpan* box_col;
		const BoxRowSpan* box_row;
		uint32  buffer_width;
		uint32  client_width;
		int32   dx;
		int32   dx2;
		int32   dw;
	};

	static void scale_box_worker(void* ctx, int32 row_start, int32 row_end);

	HWND window;
	ThreadPool *threads;
	HDC dc;
	HDC mem_dc;
	HDC front_dc;
	HBITMAP dib;
	HBITMAP front_dib;
	void** bits;
	uint32* front_bits;
	// sharp bilinear (upscale) LUTs
	uint32* sx_lut;
	uint32* sx1_lut;
	uint16* wx_lut;
	__m256i* wx_lo_lut;
	__m256i* wx_hi_lut;
	uint32* sy_lut;
	uint32* wy_lut;
	uint32* temp_rows[MAX_THREADS * 2];

	BoxColSpan *box_col;
	BoxRowSpan *box_row;
	uint64      box_recip;    // 2D reciprocal for normalization
	uint64      box_hrecip;   // horizontal-only reciprocal

	BITMAPINFO info;

	uint16 color_depth;
	uint32 buffer_width;
	uint32 buffer_height;
	uint32 client_width;
	uint32 client_height;
	uint32 scale_x_fp;
	uint32 scale_y_fp;
	uint32 inv_scale_x_fp;
	uint32 inv_scale_y_fp;
	uint32 scale_int;
	uint32 stretch_threshold;
	uint32 sharp_half_w;
	bool   downscaled;
};

#endif