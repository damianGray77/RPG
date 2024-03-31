#include "pch.h"
#include "Point2b.h"

Point2b Point2b::operator+(const Point2b& p) const {
	return { (byte)(p.x + x), (byte)(p.y + y) };
}

Point2b Point2b::operator-(const Point2b& p) const {
	return { (byte)(x - p.x), (byte)(y - p.y) };
}

bool Point2b::operator==(const Point2b& p) const {
	return p.x == x && p.y == y;
}