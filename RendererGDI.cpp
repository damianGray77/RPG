#include "pch.h"
#include "RendererGDI.h"

RendererGDI::RendererGDI() {
	info   = {};
	window = NULL;

	bits       = NULL;
	front_bits = NULL;
	dib        = NULL;
	front_dib  = NULL;
	dc         = NULL;
	mem_dc     = NULL;
	front_dc   = NULL;

	threads          = NULL;
	sx_lut           = NULL;
	sx1_lut          = NULL;
	wx_lut           = NULL;
	wx_lo_lut        = NULL;
	wx_hi_lut        = NULL;
	sy_lut           = NULL;
	wy_lut           = NULL;
	memset(temp_rows, 0, sizeof(temp_rows));
	box_col          = NULL;
	box_row          = NULL;
	box_recip        = 0;
	box_hrecip       = 0;
	buffer_width     = 0;
	buffer_height    = 0;
	client_width     = 0;
	client_height    = 0;
	scale_x_fp       = 0;
	scale_y_fp       = 0;
	inv_scale_x_fp   = 0;
	inv_scale_y_fp   = 0;
	scale_int          = 0;
	stretch_threshold  = 0;
	sharp_half_w       = 256;
	downscaled         = false;
	color_depth        = 32;
}

bool RendererGDI::draw() {
	if (client_width == buffer_width && client_height == buffer_height) {
		return BitBlt(dc, 0, 0, client_width, client_height, mem_dc, 0, 0, SRCCOPY);
	}
	if (downscaled) {
		if (NULL == front_bits) { return false; }
		scale_box(0, 0, client_width, client_height);
		return BitBlt(dc, 0, 0, client_width, client_height, front_dc, 0, 0, SRCCOPY);
	}
	// upscaled integer: StretchBlt
	if (scale_int) {
		return StretchBlt(dc, 0, 0, client_width, client_height, mem_dc, 0, 0, buffer_width, buffer_height, SRCCOPY);
	}
	// upscaled non-integer: sharp bilinear
	if (NULL == front_bits) { return false; }
	scale_sharp(0, 0, client_width, client_height);
	return BitBlt(dc, 0, 0, client_width, client_height, front_dc, 0, 0, SRCCOPY);
}

bool RendererGDI::draw(int32 x, int32 y, int32 w, int32 h) {
	if (client_width == buffer_width && client_height == buffer_height) {
		return BitBlt(dc, x, y, w, h, mem_dc, x, y, SRCCOPY);
	}

	const int32 dx =       (x  * scale_x_fp) >> 16;
	const int32 dy =       (y  * scale_y_fp) >> 16;
	const int32 dx2 = MIN((int32)((((w + x) * scale_x_fp) + 0xFFFF) >> 16), (int32)client_width);
	const int32 dy2 = MIN((int32)((((h + y) * scale_y_fp) + 0xFFFF) >> 16), (int32)client_height);
	const int32 dw = dx2 - dx;
	const int32 dh = dy2 - dy;

	if (dw <= 0 || dh <= 0) { return false; }

	if (downscaled) {
		if (NULL == front_bits) { return false; }
		scale_box(dx, dy, dw, dh);
		return BitBlt(dc, dx, dy, dw, dh, front_dc, dx, dy, SRCCOPY);
	}

	// upscaled integer: StretchBlt full screen if >= 75% dirty, else partial NN
	if (scale_int) {
		if ((uint32)(w * h) >= stretch_threshold) {
			return StretchBlt(dc, 0, 0, client_width, client_height, mem_dc, 0, 0, buffer_width, buffer_height, SRCCOPY);
		}
		if (NULL == front_bits) { return false; }
		scale_nn_int(dx, dy, dw, dh);
		return BitBlt(dc, dx, dy, dw, dh, front_dc, dx, dy, SRCCOPY);
	}

	// upscaled non-integer: sharp bilinear (no StretchBlt to preserve filtering)
	if (NULL == front_bits) { return false; }
	scale_sharp(dx, dy, dw, dh);
	return BitBlt(dc, dx, dy, dw, dh, front_dc, dx, dy, SRCCOPY);
}

bool RendererGDI::resize(const uint32 width, const uint32 height) {
	if (0 == width || 0 == height) { return true; }

	client_width     = width;
	client_height    = height;
	scale_x_fp       = (client_width  << 16) / buffer_width;
	scale_y_fp       = (client_height << 16) / buffer_height;
	inv_scale_x_fp   = (buffer_width  << 16) / client_width;
	inv_scale_y_fp   = (buffer_height << 16) / client_height;
	downscaled       = (width < buffer_width || height < buffer_height);

	// detect integer scale (same factor in both axes, exact multiple)
	scale_int = 0;
	if (
		   width  > buffer_width
		&& height > buffer_height
		&& width  % buffer_width  == 0
		&& height % buffer_height == 0
		&& width / buffer_width == height / buffer_height
	) { scale_int = width / buffer_width; }

	if (front_dc)  { DeleteDC    (front_dc);  front_dc   = NULL; }
	if (front_dib) { DeleteObject(front_dib); front_dib  = NULL; front_bits = NULL; }
	if (   sx_lut) { _aligned_free(   sx_lut);    sx_lut = NULL; }
	if (  sx1_lut) { _aligned_free(  sx1_lut);   sx1_lut = NULL; }
	if (   wx_lut) { _aligned_free(   wx_lut);    wx_lut = NULL; }
	if (wx_lo_lut) { _aligned_free(wx_lo_lut); wx_lo_lut = NULL; }
	if (wx_hi_lut) { _aligned_free(wx_hi_lut); wx_hi_lut = NULL; }
	if (   sy_lut) {          free(   sy_lut);    sy_lut = NULL; }
	if (   wy_lut) {          free(   wy_lut);    wy_lut = NULL; }
	for (uint32 i = 0; i < MAX_THREADS * 2; ++i) {
		if (temp_rows[i]) { _aligned_free(temp_rows[i]); temp_rows[i] = NULL; }
	}
	if (box_col) { free(box_col); box_col = NULL; }
	if (box_row) { free(box_row); box_row = NULL; }

	if (client_width != buffer_width || client_height != buffer_height) {
		BITMAPINFO fi              = {};
		fi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
		fi.bmiHeader.biPlanes      = 1;
		fi.bmiHeader.biBitCount    = color_depth;
		fi.bmiHeader.biCompression = BI_RGB;
		fi.bmiHeader.biWidth       = client_width;
		fi.bmiHeader.biHeight      = -(int32)client_height;

		front_dib = CreateDIBSection(dc, &fi, DIB_RGB_COLORS, (void**)&front_bits, NULL, 0);
		if (NULL == front_dib) { return false; }

		front_dc = CreateCompatibleDC(dc);
		SelectObject(front_dc, front_dib);

		// precompute LUTs for non-integer upscale (sharp bilinear)
		if (0 == scale_int) {
			 sx_lut = (uint32*)_aligned_malloc(client_width * sizeof(uint32), 32);
			sx1_lut = (uint32*)_aligned_malloc(client_width * sizeof(uint32), 32);
			 wx_lut = (uint16*)_aligned_malloc(client_width * sizeof(uint16), 32);
			 sy_lut = (uint32*)malloc(client_height * sizeof(uint32));
			 wy_lut = (uint32*)malloc(client_height * sizeof(uint32));
			const uint32 num_threads = threads ? threads->worker_count + 1 : 1;
			for (uint32 i = 0; i < num_threads * 2; ++i) {
				temp_rows[i] = (uint32*)_aligned_malloc(client_width * sizeof(uint32), 32);
			}

			// adaptive transition: inversely proportional to scale
			// so the blend zone stays ~1 destination pixel wide at any scale
			// sharp_half_w is a multiplier (256 = 1.0 dest pixel, 128 = sharper, 512 = softer)
			const uint32 base_hw = (32768u << 16) / scale_x_fp;
			const uint32 half_w  = MIN((base_hw * sharp_half_w) >> 8, 32768u);
			const uint32 lo = 32768 - half_w;
			const uint32 hi = 32768 + half_w;

			{
				const uint32 sx_max = buffer_width - 1;
				for (uint32 col = 0; col < client_width; ++col) {
					const uint32 fp   = col * inv_scale_x_fp;
					const uint32 frac = fp & 0xFFFF;
					const uint32 sx   = fp >> 16;
					const uint32 sx1  = (sx < sx_max) ? sx + 1 : sx;
					uint32 wx;

					if      (frac <= lo) { wx = 0;   }
					else if (frac >= hi) { wx = 256; }
					else {
						const uint32 t = ((frac - lo) << 8) / (hi - lo);
						wx = (t * t * (768 - 2 * t)) >> 16;
					}

					// SoA for AVX2 gather
					sx_lut[col]  = sx;
					sx1_lut[col] = sx1;
					wx_lut[col]  = (uint16)wx;
				}
			}

			// precompute AVX2 weight vectors: one pair per 8-pixel block
			{
				const uint32 block_count = (client_width + 7) >> 3;
				wx_lo_lut = (__m256i*)_aligned_malloc(block_count * sizeof(__m256i), 32);
				wx_hi_lut = (__m256i*)_aligned_malloc(block_count * sizeof(__m256i), 32);

				for (uint32 b = 0; b < block_count; ++b) {
					const uint32 base = b << 3;
					// safe reads: wx_lut is client_width entries, pad with 0 for tail
					short w[8];
					for (int i = 0; i < 8; ++i)
						w[i] = (base + i < client_width) ? (short)wx_lut[base + i] : 0;

					// unpacklo layout: [P0,P1|P4,P5]  unpackhi layout: [P2,P3|P6,P7]
					const __m128i w01 = _mm_unpacklo_epi64(_mm_set1_epi16(w[0]), _mm_set1_epi16(w[1]));
					const __m128i w45 = _mm_unpacklo_epi64(_mm_set1_epi16(w[4]), _mm_set1_epi16(w[5]));
					wx_lo_lut[b] = _mm256_set_m128i(w45, w01);

					const __m128i w23 = _mm_unpacklo_epi64(_mm_set1_epi16(w[2]), _mm_set1_epi16(w[3]));
					const __m128i w67 = _mm_unpacklo_epi64(_mm_set1_epi16(w[6]), _mm_set1_epi16(w[7]));
					wx_hi_lut[b] = _mm256_set_m128i(w67, w23);
				}
			}

			for (uint32 row = 0; row < client_height; ++row) {
				const uint32 fp   = row * inv_scale_y_fp;
				const uint32 frac = fp & 0xFFFF;
				sy_lut[row] = fp >> 16;

				if      (frac <= lo) { wy_lut[row] = 0;   }
				else if (frac >= hi) { wy_lut[row] = 256; }
				else {
					const uint32 t = ((frac - lo) << 8) / (hi - lo);
					wy_lut[row] = (t * t * (768 - 2 * t)) >> 16;
				}
			}
		}

		// precompute LUTs for box downscale
		if (downscaled) {
			const uint32 inv_x = inv_scale_x_fp;
			const uint32 inv_y = inv_scale_y_fp;

			box_col = (BoxColSpan*)malloc(client_width * sizeof(BoxColSpan));
			for (uint32 col = 0; col < client_width; ++col) {
				const uint32  left_fp =  col      * inv_x;
				const uint32 right_fp = (col + 1) * inv_x;
				uint32 sx0 =  left_fp >> 16;
				uint32 sx1 = right_fp >> 16;

				const uint32 wx_first = 256 - ((left_fp >> 8) & 0xFF);
				const uint32 wx_last  =       (right_fp >> 8) & 0xFF;

				if (wx_last > 0 && sx1 < buffer_width)  { ++sx1; }
				if (sx1 <= sx0)                         {   sx1 = sx0 + 1; }
				if (sx1 > buffer_width)                 {   sx1 = buffer_width; }

				const uint32 sx_inner_end = wx_last > 0 ? (sx1 - 1) : sx1;
				// compute horizontal weight sum and reciprocal
				uint32 wx_sum = wx_first;
				for (uint32 sx = sx0 + 1; sx < sx_inner_end; ++sx) {
					wx_sum += 256;
				}

				if (sx_inner_end < sx1 && wx_last > 0) { wx_sum += wx_last; }

				const uint32 hrecip   = wx_sum > 0 ? ((1u << 22) + wx_sum - 1) / wx_sum : 0;
				const uint32 wn_first = wx_first * hrecip;
				const uint32 wn_last  = wx_last  * hrecip;
				const uint32 wn_full  = 256 * hrecip;
				box_col[col] = { (uint16)sx0, (uint16)sx_inner_end, wn_first, wn_full, wn_last };
			}

			box_row = (BoxRowSpan*)malloc(client_height * sizeof(BoxRowSpan));
			for (uint32 row = 0; row < client_height; ++row) {
				const uint32 top_fp =  row      * inv_y;
				const uint32 bot_fp = (row + 1) * inv_y;
				uint32 sy0 = top_fp >> 16;
				uint32 sy1 = bot_fp >> 16;

				const uint32 wy_first = 256 - ((top_fp >> 8) & 0xFF);
				const uint32 wy_last  =        (bot_fp >> 8) & 0xFF;

				if (wy_last > 0 && sy1 < buffer_height) { ++sy1; }
				if (sy1 <= sy0)                         {   sy1 = sy0 + 1; }
				if (sy1 > buffer_height)                {   sy1 = buffer_height; }

				// compute vertical weight sum
				uint32 wy_sum = wy_first;
				const uint32 num_rows = sy1 - sy0;
				for (uint32 i = 1; i < num_rows; ++i) {
					const uint32 sy = sy0 + i;
					wy_sum += (sy == sy1 - 1 && wy_last > 0) ? wy_last : 256;
				}

				// precompute vertical blend weights
				uint32 lerp_w = 0, lerp_w2 = 0;
				switch(num_rows) {
					case 2: {
						const uint32 top_w = wy_first;
						const uint32 bot_w = wy_last > 0 ? wy_last : 256;
						lerp_w = (bot_w << 8) / (top_w + bot_w);
						break;
					}
					case 3: {
						const uint32 wy0 = wy_first;
						const uint32 wy1 = 256;
						const uint32 wy2 = wy_last > 0 ? wy_last : 256;
						lerp_w  = (wy1 << 8) / (wy0 + wy1);
						lerp_w2 = (wy2 << 8) / (wy0 + wy1 + wy2);

						break;
					}
				}

				box_row[row] = { (uint16)sy0, (uint16)sy1, (uint16)lerp_w, (uint16)lerp_w2 };
			}

			// precompute reciprocal for normalization
			const uint32 total_w = (inv_x >> 8) * (inv_y >> 8);
			box_recip = total_w > 0 ? (((uint64)1 << 32) + total_w - 1) / total_w : 0;

			// horizontal-only reciprocal for packed hpass normalization
			const uint32 h_total_w = inv_x >> 8;
			box_hrecip = h_total_w > 0 ? (((uint64)1 << 32) + h_total_w - 1) / h_total_w : 0;
		}
	}

	return true;
}

void RendererGDI::unload() {
	if (sx_lut)    { _aligned_free(sx_lut);  sx_lut = NULL; }
	if (sx1_lut)   { _aligned_free(sx1_lut); sx1_lut = NULL; }
	if (wx_lut)    { _aligned_free(wx_lut);    wx_lut = NULL; }
	if (wx_lo_lut) { _aligned_free(wx_lo_lut); wx_lo_lut = NULL; }
	if (wx_hi_lut) { _aligned_free(wx_hi_lut); wx_hi_lut = NULL; }
	if (sy_lut)    { free(sy_lut);    sy_lut = NULL; }
	if (wy_lut)    { free(wy_lut);    wy_lut = NULL; }
	for (uint32 i = 0; i < MAX_THREADS * 2; ++i) {
		if (temp_rows[i]) { _aligned_free(temp_rows[i]); temp_rows[i] = NULL; }
	}
	if (box_col) { free(box_col); box_col = NULL; }
	if (box_row) { free(box_row); box_row = NULL; }

	if (front_dc) {
		DeleteDC(front_dc);
		front_dc = NULL;
	}

	if (front_dib) {
		DeleteObject(front_dib);
		front_dib  = NULL;
		front_bits = NULL;
	}

	if (mem_dc) {
		DeleteDC(mem_dc);
		mem_dc = NULL;
	}

	if (dc) {
		ReleaseDC(window, dc);
		dc = NULL;
	}

	if (dib) {
		DeleteObject(dib);
		dib = NULL;
	}
}

bool RendererGDI::init(HWND window, ThreadPool *threads, void **bits, const uint32 width, const uint32 height) {
	this->window  = window;
	this->threads = threads;
	this->bits    = bits;

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

	mem_dc = CreateCompatibleDC(dc);
	SelectObject(mem_dc, dib);

	buffer_width      = width;
	buffer_height     = height;
	stretch_threshold = width * height * 3 / 4;
	client_width      = width;
	client_height     = height;
	scale_x_fp       = 1 << 16;
	scale_y_fp       = 1 << 16;
	inv_scale_x_fp   = 1 << 16;
	inv_scale_y_fp   = 1 << 16;
	downscaled       = false;

	return true;
}

// lerp two packed pixels: blends R+B together, G separately
// w is 0..256, avoids per-channel unpacking
static inline uint32 lerp_px(uint32 a, uint32 b, uint32 w) {
	const uint32 t = ((b & 0xFF00FF) - (a & 0xFF00FF)) * w;
	const uint32 u = ((b & 0x00FF00) - (a & 0x00FF00)) * w;

	return
		  ((a & 0xFF00FF) + ((t >> 8) & 0xFF00FF))
		| ((a & 0x00FF00) + ((u >> 8) & 0x00FF00))
	;
}

// AVX2 bilinear lerp: 8 pixels at a time in 16-bit lanes
// a, b are 8 packed XRGB pixels (256-bit)
// w_lo/w_hi have per-pixel weights broadcast to all 4 channels
// lane layout after unpacklo: [P0,P1|P4,P5], unpackhi: [P2,P3|P6,P7]
static inline __m256i lerp_px_avx2(__m256i a, __m256i b, __m256i w_lo, __m256i w_hi) {
	const __m256i zero = _mm256_setzero_si256();

	__m256i a_lo = _mm256_unpacklo_epi8(a, zero);
	__m256i a_hi = _mm256_unpackhi_epi8(a, zero);
	__m256i b_lo = _mm256_unpacklo_epi8(b, zero);
	__m256i b_hi = _mm256_unpackhi_epi8(b, zero);

	__m256i d_lo = _mm256_sub_epi16(b_lo, a_lo);
	__m256i d_hi = _mm256_sub_epi16(b_hi, a_hi);

	// full-precision multiply to avoid int16 overflow
	__m256i ml_lo = _mm256_mullo_epi16(d_lo, w_lo);
	__m256i mh_lo = _mm256_mulhi_epi16(d_lo, w_lo);
	__m256i ml_hi = _mm256_mullo_epi16(d_hi, w_hi);
	__m256i mh_hi = _mm256_mulhi_epi16(d_hi, w_hi);

	d_lo = _mm256_or_si256(_mm256_slli_epi16(mh_lo, 8), _mm256_srli_epi16(ml_lo, 8));
	d_hi = _mm256_or_si256(_mm256_slli_epi16(mh_hi, 8), _mm256_srli_epi16(ml_hi, 8));

	return _mm256_packus_epi16(_mm256_add_epi16(a_lo, d_lo), _mm256_add_epi16(a_hi, d_hi));
}

// SSE2 bilinear lerp: 4 pixels at a time (used by hlerp_row)
static inline __m128i lerp_px_sse2(__m128i a, __m128i b, __m128i w_lo, __m128i w_hi) {
	const __m128i zero = _mm_setzero_si128();

	__m128i a_lo = _mm_unpacklo_epi8(a, zero);
	__m128i a_hi = _mm_unpackhi_epi8(a, zero);
	__m128i b_lo = _mm_unpacklo_epi8(b, zero);
	__m128i b_hi = _mm_unpackhi_epi8(b, zero);
	__m128i d_lo = _mm_sub_epi16(b_lo, a_lo);
	__m128i d_hi = _mm_sub_epi16(b_hi, a_hi);

	__m128i ml_lo = _mm_mullo_epi16(d_lo, w_lo);
	__m128i mh_lo = _mm_mulhi_epi16(d_lo, w_lo);
	__m128i ml_hi = _mm_mullo_epi16(d_hi, w_hi);
	__m128i mh_hi = _mm_mulhi_epi16(d_hi, w_hi);

	d_lo = _mm_or_si128(_mm_slli_epi16(mh_lo, 8), _mm_srli_epi16(ml_lo, 8));
	d_hi = _mm_or_si128(_mm_slli_epi16(mh_hi, 8), _mm_srli_epi16(ml_hi, 8));

	return _mm_packus_epi16(_mm_add_epi16(a_lo, d_lo), _mm_add_epi16(a_hi, d_hi));
}

// horizontal lerp pass: AVX2 gather + precomputed weight blocks
static void hlerp_row(uint32 *dst, const uint32 *src, const uint32 *sx_arr, const uint32 *sx1_arr, const uint16 *wx_arr, const __m256i *wlo_arr, const __m256i *whi_arr, int32 dx, int32 dx2) {
	int32 col = dx;

	// scalar head: align to 8-pixel boundary for correct weight block indexing
	const int32 col_aligned = (dx + 7) & ~7;
	for (; col < col_aligned && col < dx2; ++col) {
		dst[col] = lerp_px(src[sx_arr[col]], src[sx1_arr[col]], wx_arr[col]);
	}

	// AVX2 loop: col is now 8-aligned, so col/8 maps to correct weight block
	const int32 dx2_8 = dx2 - 7;
	for (; col < dx2_8; col += 8) {
		const __m256i sx_idx = _mm256_loadu_si256((const __m256i*)(sx_arr + col));
		const __m256i left   = _mm256_i32gather_epi32((const int*)src, sx_idx, 4);

		const __m128i wx_packed = _mm_loadu_si128((const __m128i*)(wx_arr + col));
		if (_mm_testz_si128(wx_packed, wx_packed)) {
			_mm256_storeu_si256((__m256i*)(dst + col), left);
		} else {
			const __m256i sx1_idx = _mm256_loadu_si256((const __m256i*)(sx1_arr + col));
			const __m256i right   = _mm256_i32gather_epi32((const int*)src, sx1_idx, 4);

			const uint32 block = col / 8;
			_mm256_storeu_si256((__m256i*)(dst + col), lerp_px_avx2(left, right, wlo_arr[block], whi_arr[block]));
		}
	}

	// scalar tail
	for (; col < dx2; ++col) {
		dst[col] = lerp_px(src[sx_arr[col]], src[sx1_arr[col]], wx_arr[col]);
	}
}

// AVX2 row copy: replaces memcpy for row-width copies
static inline void copy_row_avx2(uint32 *dst, const uint32 *src, int32 dx, int32 count) {
	int32 i = 0;
	const int32 count_8 = count - 7;
	for (; i < count_8; i += 8) {
		_mm256_storeu_si256((__m256i*)(dst + dx + i), _mm256_loadu_si256((const __m256i*)(src + dx + i)));
	}

	for (; i < count; ++i) {
		dst[dx + i] = src[dx + i];
	}
}

void RendererGDI::scale_sharp_worker(void *ctx, int32 row_start, int32 row_end) {
	const ScaleSharpCtx *c = (const ScaleSharpCtx*)ctx;
	const uint32 sy_max = c->buffer_height - 1;
	const int32  dx     = c->dx;
	const int32  dx2    = c->dx2;
	const int32  dw     = c->dw;

	uint32 *h_cache    [2] = { c->temp_row_a, c->temp_row_b };
	int32   h_cached_sy[2] = { -1, -1 };

	for (int32 row = row_start; row < row_end; ++row) {
		const uint32  sy  = c->sy_lut[row];
		const uint32  wy  = c->wy_lut[row];
		      uint32 *dst = c->front_bits + row * c->client_width;

		// row dedup — skip for first row of chunk (can't reference previous thread's output)
		if (row > row_start && sy == c->sy_lut[row - 1] && wy == c->wy_lut[row - 1]) {
			copy_row_avx2(dst, dst - c->client_width, dx, dw);
			continue;
		}

		int32 slot0;
		if      (h_cached_sy[0] == (int32)sy) { slot0 = 0; }
		else if (h_cached_sy[1] == (int32)sy) { slot0 = 1; }
		else {
			slot0 = (h_cached_sy[0] == (int32)MIN(sy + 1, sy_max)) ? 1 : 0;
			hlerp_row(h_cache[slot0], c->src + sy * c->buffer_width, c->sx_lut, c->sx1_lut, c->wx_lut, c->wx_lo_lut, c->wx_hi_lut, dx, dx2);
			h_cached_sy[slot0] = (int32)sy;
		}

		if (0 == wy) {
			copy_row_avx2(dst, h_cache[slot0], dx, dw);
		} else {
			const uint32 sy1 = MIN(sy + 1, sy_max);
			int32 slot1;
			if      (h_cached_sy[0] == (int32)sy1) { slot1 = 0; }
			else if (h_cached_sy[1] == (int32)sy1) { slot1 = 1; }
			else {
				slot1 = (0 == slot0) ? 1 : 0;
				hlerp_row(h_cache[slot1], c->src + sy1 * c->buffer_width, c->sx_lut, c->sx1_lut, c->wx_lut, c->wx_lo_lut, c->wx_hi_lut, dx, dx2);
				h_cached_sy[slot1] = (int32)sy1;
			}

			const __m256i wy256 = _mm256_set1_epi16((short)wy);
			int32 col = dx;
			const int32 dx2_8 = dx2 - 7;
			for (; col < dx2_8; col += 8) {
				const __m256i t0 = _mm256_loadu_si256((const __m256i*)(h_cache[slot0] + col));
				const __m256i t1 = _mm256_loadu_si256((const __m256i*)(h_cache[slot1] + col));
				_mm256_storeu_si256((__m256i*)(dst + col), lerp_px_avx2(t0, t1, wy256, wy256));
			}

			for (; col < dx2; ++col) {
				dst[col] = lerp_px(h_cache[slot0][col], h_cache[slot1][col], wy);
			}
		}
	}
}

void RendererGDI::scale_sharp(int32 dx, int32 dy, int32 dw, int32 dh) {
	const int32  dx2 = dx + dw;
	const int32  dy2 = dy + dh;
	const uint32 num_workers = (threads && dh >= 64) ? MIN(4u, threads->worker_count) : 0;

	ScaleSharpCtx ctxs    [MAX_THREADS];
	void         *ctx_ptrs[MAX_THREADS];

	// num_workers + 1 contexts (workers + main thread as participant)
	const uint32 num_ctxs = num_workers > 0 ? num_workers + 1 : 1;
	for (uint32 i = 0; i < num_ctxs; ++i) {
		ctxs[i].src           = (const uint32*)*bits;
		ctxs[i].front_bits    = front_bits;
		ctxs[i].sx_lut        = sx_lut;
		ctxs[i].sx1_lut       = sx1_lut;
		ctxs[i].wx_lut        = wx_lut;
		ctxs[i].wx_lo_lut     = wx_lo_lut;
		ctxs[i].wx_hi_lut     = wx_hi_lut;
		ctxs[i].sy_lut        = sy_lut;
		ctxs[i].wy_lut        = wy_lut;
		ctxs[i].temp_row_a    = temp_rows[i * 2];
		ctxs[i].temp_row_b    = temp_rows[i * 2 + 1];
		ctxs[i].buffer_width  = buffer_width;
		ctxs[i].buffer_height = buffer_height;
		ctxs[i].client_width  = client_width;
		ctxs[i].dx            = dx;
		ctxs[i].dx2           = dx2;
		ctxs[i].dw            = dw;
		ctx_ptrs[i]           = &ctxs[i];
	}

	if (0 == num_workers) {
		scale_sharp_worker(ctx_ptrs[0], dy, dy2);
	} else {
		threads->batch(scale_sharp_worker, ctx_ptrs, dy, dy2, num_workers, true);
	}
}

void RendererGDI::scale_nn_int(int32 dx, int32 dy, int32 dw, int32 dh) {
	const uint32 * src        = (const uint32*)*bits;
	const uint32   scale      = scale_int;
	const uint32   inv_x      = inv_scale_x_fp;
	const int32    dx2        = dx + dw;
	const int32    dy2        = dy + dh;
	const uint32   copy_bytes = dw * sizeof(uint32);

	for (int32 row = dy; row < dy2; ) {
		const uint32   sy        = (row * inv_scale_y_fp) >> 16;
		const int32    group_end = MIN((int32)((sy + 1) * scale), dy2);
		const uint32 * src_row   = src + sy * buffer_width;
		      uint32 * dst_row   = front_bits + row * client_width;

		for (int32 col = dx; col < dx2; ++col) {
			dst_row[col] = src_row[(col * inv_x) >> 16];
		}

		for (int32 r = row + 1; r < group_end; ++r) {
			memcpy(front_bits + r * client_width + dx, dst_row + dx, copy_bytes);
		}

		row = group_end;
	}
}

// horizontal box average: one source row → packed 8-bit pixels
// SSE per-column: unpack pixel to 32-bit lanes, multiply all channels at once
static void box_hpass_static(uint32 *dst, const uint32 *src_row, const RendererGDI::BoxColSpan *box_col, int32 dx, int32 dx2) {
	const __m128i zero  = _mm_setzero_si128();
	const __m128i round = _mm_set1_epi32(1u << 21);

	for (int32 col = dx; col < dx2; ++col) {
		const RendererGDI::BoxColSpan &cs = box_col[col];

		// unpack left edge pixel to 32-bit lanes [B, G, R, A]
		__m128i px    = _mm_unpacklo_epi8(_mm_cvtsi32_si128(src_row[cs.sx0]), zero);
		        px    = _mm_unpacklo_epi16(px, zero);  // 8-bit → 16-bit → 32-bit
		__m128i accum = _mm_mullo_epi32(px, _mm_set1_epi32(cs.wn_first));

		// interior (full weight)
		for (uint32 sx = cs.sx0 + 1; sx < cs.sx_inner_end; ++sx) {
			px    = _mm_unpacklo_epi8(_mm_cvtsi32_si128(src_row[sx]), zero);
			px    = _mm_unpacklo_epi16(px, zero);
			accum = _mm_add_epi32(accum, _mm_mullo_epi32(px, _mm_set1_epi32(cs.wn_full)));
		}

		// right edge
		if (cs.wn_last) {
			px = _mm_unpacklo_epi8(_mm_cvtsi32_si128(src_row[cs.sx_inner_end]), zero);
			px = _mm_unpacklo_epi16(px, zero);
			accum = _mm_add_epi32(accum, _mm_mullo_epi32(px, _mm_set1_epi32(cs.wn_last)));
		}

		// normalize: >> 22 with rounding, then pack to 8-bit pixel
		accum = _mm_srli_epi32(_mm_add_epi32(accum, round), 22);
		// accum = [B, G, R, 0] each 0-255 in 32-bit lanes
		// pack: 32→16→8
		accum = _mm_packus_epi32(accum, zero);   // 32→16: [B,G,R,0, 0,0,0,0]
		accum = _mm_packus_epi16(accum, zero);   // 16→8:  [B,G,R,0, 0,0,0,0, ...]
		dst[col] = (uint32)_mm_cvtsi128_si32(accum);
	}
}

void RendererGDI::scale_box_worker(void *ctx, int32 row_start, int32 row_end) {
	const ScaleBoxCtx *c = (const ScaleBoxCtx*)ctx;
	const  int32 dx  = c->dx;
	const  int32 dx2 = c->dx2;
	const  int32 dw  = c->dw;
	const uint32 bw  = c->buffer_width;
	const uint32 cw  = c->client_width;

	static const uint32 HCACHE_SLOTS = 3;
	uint32 cache   [HCACHE_SLOTS][1920];
	int32  cache_sy[HCACHE_SLOTS] = { -1, -1, -1 };

	for (int32 row = row_start; row < row_end; ++row) {
		const BoxRowSpan &rs = c->box_row[row];
		const uint32 num_src_rows = rs.sy1 - rs.sy0;

		uint32 slot_map[3];
		for (uint32 i = 0; i < num_src_rows; ++i) {
			const int32 sy = (int32)(rs.sy0 + i);

			uint32 slot = HCACHE_SLOTS;
			for (uint32 s = 0; s < HCACHE_SLOTS; ++s) {
				if (cache_sy[s] == sy) { slot = s; break; }
			}

			if (slot == HCACHE_SLOTS) {
				for (uint32 s = 0; s < HCACHE_SLOTS; ++s) {
					bool in_use = false;
					for (uint32 j = 0; j < i; ++j) {
						if (slot_map[j] == s) { in_use = true; break; }
					}

					if (!in_use) { slot = s; break; }
				}

				box_hpass_static(cache[slot], c->src + sy * bw, c->box_col, dx, dx2);
				cache_sy[slot] = sy;
			}

			slot_map[i] = slot;
		}

		uint32 *dst_row = c->front_bits + row * cw;

		switch (num_src_rows) {
			case 1: {
				copy_row_avx2(dst_row, cache[slot_map[0]], dx, dw);

				break;
			}
			case 2: {
				const uint32* c0 = cache[slot_map[0]];
				const uint32* c1 = cache[slot_map[1]];
				const __m256i w256 = _mm256_set1_epi16((short)rs.lerp_w);

				int32 col = dx;
				const int32 dx2_8 = dx2 - 7;
				for (; col < dx2_8; col += 8) {
					const __m256i a = _mm256_loadu_si256((const __m256i*)(c0 + col));
					const __m256i b = _mm256_loadu_si256((const __m256i*)(c1 + col));
					_mm256_storeu_si256((__m256i*)(dst_row + col), lerp_px_avx2(a, b, w256, w256));
				}

				for (; col < dx2; ++col) {
					dst_row[col] = lerp_px(c0[col], c1[col], rs.lerp_w);
				}

				break;
			}
			default: {
				const uint32* c0 = cache[slot_map[0]];
				const uint32* c1 = cache[slot_map[1]];
				const uint32* c2 = cache[slot_map[2]];
				const __m256i w01_256 = _mm256_set1_epi16((short)rs.lerp_w);
				const __m256i  w2_256 = _mm256_set1_epi16((short)rs.lerp_w2);

				int32 col = dx;
				const int32 dx2_8 = dx2 - 7;
				for (; col < dx2_8; col += 8) {
					const __m256i a = _mm256_loadu_si256((const __m256i*)(c0 + col));
					const __m256i b = _mm256_loadu_si256((const __m256i*)(c1 + col));
					const __m256i cv = _mm256_loadu_si256((const __m256i*)(c2 + col));
					const __m256i mid = lerp_px_avx2(a, b, w01_256, w01_256);
					_mm256_storeu_si256((__m256i*)(dst_row + col), lerp_px_avx2(mid, cv, w2_256, w2_256));
				}

				for (; col < dx2; ++col) {
					const uint32 mid = lerp_px(c0[col], c1[col], rs.lerp_w);
					dst_row[col] = lerp_px(mid, c2[col], rs.lerp_w2);
				}

				break;
			}
		}
	}
}

void RendererGDI::scale_box(int32 dx, int32 dy, int32 dw, int32 dh) {
	const int32  dx2 = dx + dw;
	const int32  dy2 = dy + dh;
	const uint32 num_workers = (threads && dh >= 64) ? MIN(4u, threads->worker_count) : 0;

	ScaleBoxCtx ctxs    [MAX_THREADS];
	void       *ctx_ptrs[MAX_THREADS];

	const uint32 num_ctxs = num_workers > 0 ? num_workers + 1 : 1;
	for (uint32 i = 0; i < num_ctxs; ++i) {
		ctxs[i].src          = (const uint32*)*bits;
		ctxs[i].front_bits   = front_bits;
		ctxs[i].box_col      = box_col;
		ctxs[i].box_row      = box_row;
		ctxs[i].buffer_width = buffer_width;
		ctxs[i].client_width = client_width;
		ctxs[i].dx           = dx;
		ctxs[i].dx2          = dx2;
		ctxs[i].dw           = dw;
		ctx_ptrs[i]          = &ctxs[i];
	}

	if (0 == num_workers) {
		scale_box_worker(ctx_ptrs[0], dy, dy2);
	} else {
		threads->batch(scale_box_worker, ctx_ptrs, dy, dy2, num_workers, true);
	}
}