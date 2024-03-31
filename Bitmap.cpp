#include "pch.h"
#include "Bitmap.h"

Bitmap::Bitmap() {
	data    = NULL;
	yoffs   = NULL;
	palette = NULL;
}

Bitmap::Bitmap(const char* path) {
	data    = NULL;
	yoffs   = NULL;
	palette = NULL;

	init(path);
}

Bitmap::~Bitmap() {
	unload();
}

void Bitmap::unload() {
	if (NULL != data) {
		delete[] data;
		data = NULL;
	}

	if (NULL != palette) {
		delete[] palette;
		palette = NULL;
	}

	if (NULL != yoffs) {
		delete[] yoffs;
		yoffs = NULL;
	}
}

bool Bitmap::init(const char* path) {
	unload();

	if (NULL == path) { return false; }

	FILE *file;
	errno_t e = fopen_s(&file, path, "rb");
	if (0 != e) { return false; }

	fread(&fheader, sizeof(BMPFileHeader), 1, file);
	if (0x4D42 != fheader.type) {
		fclose(file);

		return false;
	}

	fread(&iheader, sizeof(BMPInfoHeader), 1, file);
	if (iheader.bit_count <= 8) {
		ushort num_colors = 1 << iheader.bit_count;
		palette = new RGBA[num_colors];
		fread(palette, sizeof(RGBA), num_colors, file);
	}

	fseek(file, fheader.off_bits, SEEK_SET);
	size = fheader.size - fheader.off_bits;

	w = iheader.width;
	if (iheader.height < 0) {
		h = -iheader.height;
		inv_height = true;
	}
	else {
		h = iheader.height;
		inv_height = false;
	}

	yoffs = new int[h];
	for (int i = 0; i < h; ++i) {
		yoffs[i] = w * i;
	}

	uchar *temp = new uchar[size];

	fread(temp, sizeof(uchar), size, file);

	fclose(file);

	if (NULL == temp) {
		if (NULL != palette) {
			delete[] palette;
			palette = NULL;
		}

		delete[] temp;
		temp = NULL;

		return false;
	}

	data = new ulong[h * w];

	switch (iheader.bit_count) {
	case 24: from_24bit(temp); break;
	case 8:  from_8bit(temp);  break;
	}

	if (NULL != palette) {
		delete[] palette;
		palette = NULL;
	}

	delete[] temp;
	temp = NULL;

	return true;
}

void Bitmap::from_8bit(const uchar* temp) {
	int c;

	if (inv_height) {
		for (int y = 0; y < h; ++y) {
			int yoff = yoffs[y];
			for (int x = 0; x < w; ++x) {
				c = *(temp + yoff + x);

				*(data + yoff + x) = RGBb(palette[c].r, palette[c].g, palette[c].b);
			}
		}
	}
	else {
		for (int y = 0; y < h; ++y) {
			int yoff1 = yoffs[y];
			int yoff2 = yoffs[h - y - 1];
			for (int x = 0; x < w; ++x) {
				c = *(temp + yoff1 + x);

				*(data + yoff2 + x) = RGBb(palette[c].r, palette[c].g, palette[c].b);
			}
		}
	}
}

void Bitmap::from_24bit(const uchar* temp) {
	const uchar *c;

	if (inv_height) {
		for (int y = 0; y < h; ++y) {
			int yoff = yoffs[y];
			for (int x = 0; x < w; ++x) {
				c = temp + (yoff + x) * 3;

				*(data + yoff + x) = RGBb(c[2], c[1], c[0]);
			}
		}
	}
	else {
		for (int y = 0; y < h; ++y) {
			int yoff1 = yoffs[y];
			int yoff2 = yoffs[h - y - 1];
			for (int x = 0; x < w; ++x) {
				c = temp + (yoff1 + x) * 3;

				*(data + yoff2 + x) = RGBb(c[2], c[1], c[0]);
			}
		}
	}
}