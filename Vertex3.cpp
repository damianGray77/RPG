#include "pch.h"
#include "Vertex3.h"

Vertex3 Vertex3::operator+(const Vertex3& v) const {
	return { v.x + x, v.y + y, v.z + z };
}

Vertex3 Vertex3::operator-(const Vertex3& v) const {
	return { x - v.x, y - v.y, z - v.z };
}

bool Vertex3::operator==(const Vertex3& v) const {
	return v.x == x && v.y == y && v.z == z;
}