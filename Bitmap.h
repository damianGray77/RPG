#pragma once
#ifndef BITMAP_H
#define BITMAP_H

#include <stdio.h>

#pragma pack(push, bitmap_data, 1)

struct BMPFileHeader {
	ushort type;
	ulong  size;
	ushort reserved1;
	ushort reserved2;
	ulong  off_bits;
};

struct BMPInfoHeader {
	ulong  size;
	long   width;
	long   height;
	ushort planes;
	ushort bit_count;
	ulong  compression;
	ulong  size_image;
	long   xppm;
	long   yppm;
	ulong  clr_used;
	ulong  clr_important;
};

struct RGBA {
	ubyte b;
	ubyte g;
	ubyte r;
	ubyte a;
};

#pragma pack(pop, bitmap_data)

//#endif

class Bitmap {
public:
	Bitmap();
	Bitmap(const char*);
	~Bitmap();

	bool init(const char*);
	void unload();

	ulong *data;
	int *yoffs;
	int w;
	int h;
private:
	void from_8bit(const uchar*);
	void from_24bit(const uchar*);

	BMPFileHeader fheader;
	BMPInfoHeader iheader;
	RGBA *palette;
	int size;
	bool inv_height;
};

#endif
