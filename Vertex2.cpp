#include "pch.h"
#include "Vertex2.h"

Vertex2 Vertex2::operator+(const Vertex2& v) const {
	return { v.x + x, v.y + y };
}

Vertex2 Vertex2::operator-(const Vertex2& v) const {
	return { x - v.x, y - v.y };
}

bool Vertex2::operator==(const Vertex2& v) const {
	return v.x == x && v.y == y;
}

float Vertex2::dot(const Vertex2& v1, const Vertex2& v2) {
	return v1.x * v2.x + v1.y * v2.y;
}

float Vertex2::cross(const Vertex2& v1, const Vertex2& v2) {
	return v1.x * v2.y - v1.y * v2.x;
}

int Vertex2::left_of(const Vertex2& v1, const Vertex2& v2, const Vertex2& v3) {
	const float x = cross(v2 - v1, v3 - v2);

	return x < 0 ? -1 : x > 0;
}

bool Vertex2::intersects(const Vertex2& x0, const Vertex2& x1, const Vertex2& y0, const Vertex2& y1, const ClipSide s, Vertex2& r) {
	const Vertex2 dx = x1 - x0;
	const Vertex2 dy = y1 - y0;
	const Vertex2 d  = x0 - y0;

	float dyx = cross(dy, dx);
	if (0 == dyx) { return false; }

	dyx = cross(d, dx) / dyx;
	if (dyx <= 0 || dyx >= 1) { return false; }

	const float sx = dyx * dy.x;
	const float sy = dyx * dy.y;

	// deal with float point errors, since we know that the intersection is at
	// the particular bounds we are checking for we can just assign it directly
	// which saves the potential of calculating outside and causing an overflow
	switch (s) {
		case top:
		case bottom:
			r.x = y0.x + sx;
			r.y = x0.y;
			break;
		case left:
		case right:
			r.x = x0.x;
			r.y = y0.y + sy;
			break;
	}

	return true;
}