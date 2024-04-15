#pragma once
#ifndef CORE_H
#define CORE_H

#include "Point2.h"

#define EPSILON 0 //1e-4
#define PI 3.14159265358f
#define SINCOSMAX 32767

typedef int fixed8;
typedef int fixed12;
typedef int fixed16;
typedef int fixed20;
typedef int fixed24;
typedef int fixed28;

/*typedef int64_t int64;
typedef int32_t int32;
typedef int16_t int16;
typedef int8_t  int8;

typedef uint64_t uint64;
typedef uint32_t uint32;
typedef uint16_t uint16;
typedef uint8_t  uint8;*/

typedef long long int64;
typedef long      int32;
typedef short     int16;
typedef char      int8;

typedef unsigned long long uint64;
typedef unsigned long      uint32;
typedef unsigned short     uint16;
typedef unsigned char      uint8;

typedef unsigned long long ulonglong;
typedef unsigned long      ulong;
typedef unsigned int       uint;
typedef unsigned short     ushort;
typedef unsigned char      uchar;
typedef unsigned char      ubyte;
typedef char               byte;

template <class T>
inline void SWAP(T* a, T* b) {
	T temp = *a;
	*a = *b;
	*b = temp;
}

//#define SWAP(type, i, j) { type _temp_var = i; i = j; j = _temp_var; }
#define ABS(x) ((x) < 0 ? -(x) : (x))
#define MIN(x, y) (x <= y ? x : y)
#define MAX(x, y) (x >= y ? x : y)
#define CLAMP(x, min, max) (x <= min ? min : x >= max ? max : x)
#define WRAP(x, min, max) (x <= min ? x + max : x >= max ? x - max : x)

template <typename TOut, typename TIn> inline TOut floor(const TIn x) {
	TOut xint = (TOut)x;
	return xint - (x < xint);
};

template <typename TOut, typename TIn> inline TOut ceil(const TIn x) {
	TOut xint = (TOut)x;
	return xint + (x > xint);
};

template <typename T> inline T clamp(const T x, const T min, const T max) {
	return x <= min ? min : x >= max ? max : x;
};

template <typename T> struct Bounds {
	Point2<T> min;
	Point2<T> max;
};


const float  fixed8_divisor = 1.0f / (1 <<  8);
const float fixed16_divisor = 1.0f / (1 << 16);
const float fixed20_divisor = 1.0f / (1 << 20);
const float fixed24_divisor = 1.0f / (1 << 24);

inline fixed8 fl_f8(const float f) {
	return (fixed8)(f * (float)(1 << 8));
}

inline fixed8 int_f8(const int i) {
	return (fixed8)(i << 8);
}

inline fixed8 f8_div(const fixed8 a, const fixed8 b) {
	return (a << 8) / b;
}

inline fixed8 f8_mul(const fixed8 a, const fixed8 b) {
	return (a * b) >> 8;
}

inline int f8_int(const fixed8 f) {
	return f >> 8;
}

inline float f8_fl(const fixed8 f) {
	return f * fixed8_divisor;
}



inline fixed16 fl_f16(const float f) {
	return (fixed16)(f * (float)(1 << 16));
}

inline fixed16 f16_div(const fixed16 a, const fixed16 b) {
	return (((long long)a) << 16) / b;
}

inline fixed16 f16_mul(const fixed16 a, const fixed16 b) {
	return (fixed16)((((long long)a) * ((long long)b)) >> 16);
}

inline int f16_int(const fixed16 f) {
	return f >> 16;
}

inline float f16_fl(const fixed16 f) {
	return f * fixed16_divisor;
}


inline fixed20 fl_f20(const float f) {
	return (fixed20)(f * (float)(1 << 20));
}

inline fixed20 f20_div(const fixed20 a, const fixed20 b) {
	return (((long long)a) << 20) / b;
}

inline fixed20 f20_mul(const fixed20 a, const fixed20 b) {
	return (fixed20)((((long long)a) * ((long long)b)) >> 20);
}

inline int f20_int(const fixed20 f) {
	return f >> 20;
}

inline float f20_fl(const fixed20 f) {
	return f * fixed20_divisor;
}



inline fixed28 fl_f28(const float f) {
	return (fixed28)(f * 268435456.0f);
}

inline fixed28 f28_div(const fixed28 a, const fixed28 b) {
	return (((long long)a) << 28) / b;
}

inline fixed8 f28_mul(const fixed28 a, const fixed28 b) {
	return (a * b) >> 28;
}

inline int f28_int(const fixed28 f) {
	return f >> 28;
}

inline float f28_fl(const fixed28 f) {
	return f * fixed24_divisor;
}



#define RGBAb(r, g, b, a) (((ulong)a) << 24 | ((ulong)r) << 16 | ((ulong)g) << 8 | ((ulong)b))
#define RGBAf(r, g, b, a) (((ulong)(a * 255)) << 24 | ((ulong)(r * 255)) << 16 | ((ulong)(g * 255)) << 8 | ((ulong)(b * 255)))

#define RGBb(r, g, b) (4278190080 | ((ulong)r) << 16 | ((ulong)g) << 8 | ((ulong)b))
#define RGBf(r, g, b) (4278190080 | ((ulong)(r * 255)) << 16 | ((ulong)(g * 255)) << 8 | ((ulong)(b * 255)))

#define SIN(x) (sins[x & SINCOSMAX])
#define COS(x) (coss[x & SINCOSMAX])
#define ISIN(x) (isins[x & SINCOSMAX])
#define ICOS(x) (icoss[x & SINCOSMAX])

extern float sins[SINCOSMAX];
extern float coss[SINCOSMAX];
extern float isins[SINCOSMAX];
extern float icoss[SINCOSMAX];

extern const char* texture_files[];

float isqrt(float);

#endif