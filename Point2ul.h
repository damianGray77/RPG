#pragma once
#ifndef POINT2UL_H
#define POINT2UL_H

struct Point2ul {
	ulong x;
	ulong y;

	Point2ul operator+ (const Point2ul&) const;
	Point2ul operator- (const Point2ul&) const;
	bool     operator==(const Point2ul&) const;
};

#endif
