#pragma once
#ifndef POINT2L_H
#define POINT2L_H

struct Point2l {
	long x;
	long y;

	Point2l operator+ (const Point2l&) const;
	Point2l operator- (const Point2l&) const;
	bool    operator==(const Point2l&) const;
};

#endif
