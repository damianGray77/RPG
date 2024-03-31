#pragma once
#ifndef VERTEX2_H
#define VERTEX2_H

struct Vertex2 {
	float x, y;

	Vertex2 operator+(const Vertex2&) const;
	Vertex2 operator-(const Vertex2&) const;
	bool operator==(const Vertex2&) const;

	static float dot(const Vertex2&, const Vertex2&);
	static float cross(const Vertex2&, const Vertex2&);
	static int left_of(const Vertex2&, const Vertex2&, const Vertex2&);
	static bool intersects(const Vertex2&, const Vertex2&, const Vertex2&, const Vertex2&, const ClipSide, Vertex2&);
};

#endif