#pragma once
#ifndef BUFFER_H
#define BUFFER_H

struct Buffer {
	uint32 *bits;
	uint32  size;
	uint16  width, height;
	uint16  mid_width, mid_height;
};

Buffer create_buffer();

void configure_buffer(Buffer &buffer, const uint16 width, const uint16 height);
void     alloc_buffer(Buffer &buffer, const uint16 width, const uint16 height);

void dealloc_buffer(Buffer &buffer);
void   clear_buffer(Buffer &buffer);

void fill_buffer(Buffer &buffer, const uint32 color);

#endif