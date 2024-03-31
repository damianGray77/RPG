#pragma once
#ifndef BUFFER_H
#define BUFFER_H

class Buffer {
public:
	Buffer();

	bool init(const ulong, const ulong);
	void unload();

	ulong *bits;
	ulong width, height;
	ulong mid_width, mid_height;
	size_t size;

	inline void clear(const ulong c) {
		memset(bits, c, size);
	}
};

#endif
