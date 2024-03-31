#include "pch.h"
#include "Point2ul.h"

Point2ul Point2ul::operator+(const Point2ul& p) const {
	return { p.x + x, p.y + y };
}

Point2ul Point2ul::operator-(const Point2ul& p) const {
	return { x - p.x, y - p.y };
}

bool Point2ul::operator==(const Point2ul& p) const {
	return p.x == x && p.y == y;
}