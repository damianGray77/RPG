#pragma once
#ifndef POINT2B_H
#define POINT2B_H

struct Point2b {
	byte x;
	byte y;

	Point2b operator+ (const Point2b&) const;
	Point2b operator- (const Point2b&) const;
	bool    operator==(const Point2b&) const;
};

#endif
