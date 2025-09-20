#pragma once
#ifndef VERTEX3_H
#define VERTEX3_H

struct Vertex3 {
	float x, y, z;

	Vertex3 operator+ (const Vertex3&) const;
	Vertex3 operator- (const Vertex3&) const;
	bool    operator==(const Vertex3&) const;
};

#endif