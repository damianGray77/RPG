#include "pch.h"
#include "Point2l.h"

Point2l Point2l::operator+(const Point2l& p) const {
	return { p.x + x, p.y + y };
}

Point2l Point2l::operator-(const Point2l& p) const {
	return { x - p.x, y - p.y };
}

bool Point2l::operator==(const Point2l& p) const {
	return p.x == x && p.y == y;
}