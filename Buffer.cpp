#include "pch.h"
#include "Buffer.h"

Buffer::Buffer() {
	bits = NULL;

	width  = 0;
	height = 0;

	mid_width  = 0;
	mid_height = 0;
	size = 0;
}

bool Buffer::init(const ulong w, const ulong h) {
	width = w;
	mid_width = (ulong)(w * 0.5f);
	height = h;
	mid_height = (ulong)(h * 0.5f);
	size = w * h * sizeof(ulong);

	return true;
}

void Buffer::unload() {
	bits = NULL;

	width = 0;
	height = 0;
}