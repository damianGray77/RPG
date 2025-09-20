#include "pch.h"
#include "Buffer.h"

Buffer create_buffer() {
	Buffer buffer = {};
	return buffer;
}

void configure_buffer(Buffer &buffer, const uint16 width, const uint16 height) {
	buffer.width  = width;
	buffer.height = height;

	buffer.mid_width  = width  / 2;
	buffer.mid_height = height / 2;

	buffer.size = (uint32)width * (uint32)height;
}

void alloc_buffer(Buffer& buffer, const uint16 width, const uint16 height) {
	configure_buffer(buffer, width, height);

	buffer.bits = new uint32[buffer.width * buffer.height];
}

void fill_buffer(Buffer& buffer, const uint32 color) {
	const uint32  len = buffer.size;
	      uint32 *ptr = buffer.bits;

	for (uint32 i = 0; i < len; ++i) {
		ptr[i] = color;
	}
}

void dealloc_buffer(Buffer& buffer) {
	delete[](buffer.bits);

	clear_buffer(buffer);
}

void clear_buffer(Buffer &buffer) {
	buffer = {};
}