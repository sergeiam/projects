#pragma once

#include <math.h>
#include "core.h"

struct Vec2
{
	float x, y;

	Vec2() {}
	Vec2(float _x, float _y) : x(_x), y(_y) {}
};


struct Vec4
{
	float x, y, z, w;

	Vec4() {}
	Vec4(const Vec4& rhs) : x(rhs.x), y(rhs.y), z(rhs.z), w(rhs.w) {}
	Vec4(const Vec4&& rhs) : x(rhs.x), y(rhs.y), z(rhs.z), w(rhs.w) {}

	Vec4(float _x, float _y, float _z) : x(_x), y(_y), z(_z), w(1.0f) {}
	Vec4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}

	Vec4& operator = (Vec4 rhs) {
		x = rhs.x; y = rhs.y; z = rhs.z; w = rhs.w;
		return *this;
	}
	friend Vec4 operator + (Vec4 a, Vec4 b) {
		return Vec4(a.x + b.x, a.y + b.y, a.z + b.z);
	}
	friend Vec4 operator - (Vec4 a, Vec4 b) {
		return Vec4(a.x - b.x, a.y - b.y, a.z - b.z);
	}
	friend Vec4 operator - (Vec4 a) {
		return Vec4(-a.x, -a.y, -a.z);
	}
	friend float dot(Vec4 a, Vec4 b) {
		return a.x*b.x + a.y*b.y + a.z*b.z;
	}
	friend Vec4 cross(Vec4 a, Vec4 b) {
		return Vec4(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x);
	}
	friend float length_sq(Vec4 a) {
		return dot(a, a);
	}
	friend float length(Vec4 a) {
		return sqrtf(dot(a, a));
	}
	friend Vec4 normalize(Vec4 a) {
		float inv_len = 1.0f / length(a);
		return a * inv_len;
	}
	friend Vec4 operator * (Vec4 a, Vec4 b) {
		return Vec4(a.x*b.x, a.y*b.y, a.z*b.z);
	}
	friend Vec4 operator * (Vec4 a, float b) {
		return Vec4(a.x*b, a.y*b, a.z*b);
	}
	friend Vec4 Min(Vec4 a, Vec4 b) {
		return Vec4(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z, 1.0f);
	}
	friend Vec4 Max(Vec4 a, Vec4 b) {
		return Vec4(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z, 1.0f);
	}
};