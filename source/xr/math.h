#pragma once

#include <math.h>
#include <float.h>
#include "core.h"

#undef min
#undef max

const float PI = 3.14159265359f;

inline float rad_to_deg(float rad) { return rad * 180.0f / PI; }
inline float deg_to_rad(float deg) { return deg * PI / 180.0f; }

struct Vec2
{
	float x, y;

	Vec2() {}
	Vec2(float _x, float _y) : x(_x), y(_y) {}

	friend float dot(Vec2 a, Vec2 b)
	{
		return a.x*b.x + a.y*b.y;
	}

	friend Vec2 operator - (Vec2 a, Vec2 b) { return Vec2(a.x - b.x, a.y - b.y); }
	friend Vec2 operator + (Vec2 a, Vec2 b) { return Vec2(a.x + b.x, a.y + b.y); }
	friend Vec2 operator * (Vec2 a, float b) { return Vec2(a.x*b, a.y*b); }

	friend Vec2 normalize(Vec2 a)
	{
		float inv_len = 1.0f / sqrtf(a.x*a.x + a.y*a.y);
		return Vec2(a.x * inv_len, a.y * inv_len);
	}
	friend float length(Vec2 v)
	{
		return sqrtf(v.x*v.x + v.y*v.y);
	}
};

struct Vec3
{
	float x, y, z;

	Vec3() {}
	Vec3(const Vec3& rhs) : x(rhs.x), y(rhs.y), z(rhs.z) {}
	Vec3(const Vec3&& rhs) : x(rhs.x), y(rhs.y), z(rhs.z) {}

	Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

	Vec3& operator = (Vec3 rhs) {
		x = rhs.x; y = rhs.y; z = rhs.z;
		return *this;
	}
	friend Vec3 operator + (Vec3 a, Vec3 b) {
		return Vec3(a.x + b.x, a.y + b.y, a.z + b.z);
	}
	friend Vec3 operator - (Vec3 a, Vec3 b) {
		return Vec3(a.x - b.x, a.y - b.y, a.z - b.z);
	}
	friend Vec3 operator - (Vec3 a) {
		return Vec3(-a.x, -a.y, -a.z);
	}
	friend float dot(Vec3 a, Vec3 b) {
		return a.x * b.x + a.y * b.y + a.z * b.z;
	}
	friend Vec3 cross(Vec3 a, Vec3 b) {
		return Vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
	}
	friend float length_sq(Vec3 a) {
		return dot(a, a);
	}
	friend float length(Vec3 a) {
		return sqrtf(dot(a, a));
	}
	friend Vec3 normalize(Vec3 a) {
		float inv_len = 1.0f / length(a);
		return a * inv_len;
	}
	friend Vec3 operator * (Vec3 a, Vec3 b) {
		return Vec3(a.x * b.x, a.y * b.y, a.z * b.z);
	}
	friend Vec3 operator * (Vec3 a, float b) {
		return Vec3(a.x * b, a.y * b, a.z * b);
	}
	friend Vec3 Min(Vec3 a, Vec3 b) {
		return Vec3(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z);
	}
	friend Vec3 Max(Vec3 a, Vec3 b) {
		return Vec3(a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z);
	}
	friend bool operator != (Vec3 a, Vec3 b)
	{
		return a.x != b.x || a.y != b.y || a.z != b.z;
	}
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

struct Box3
{
	Box3() {}
	Box3(Vec3 v) : m_min(v), m_max(v) {}
	Box3(Vec3 min, Vec3 max) : m_min(min), m_max(max) {}
	Box3(const Box3& rhs) : m_min(rhs.m_min), m_max(rhs.m_max) {}

	Vec3& min() { return m_min; }
	Vec3& max() { return m_max; }

	Vec3	center() const { return (m_min + m_max) * 0.5f; }
	Vec3	diag() const { return (m_max - m_min) * 0.5f; }
	float	radius() const { return length(diag()); }

	void operator += (Vec3 v)
	{
		if (v.x < m_min.x) m_min.x = v.x;
		if (v.y < m_min.y) m_min.y = v.y;
		if (v.z < m_min.z) m_min.z = v.z;

		if (v.x > m_max.x) m_max.x = v.x;
		if (v.y > m_max.y) m_max.y = v.y;
		if (v.z > m_max.z) m_max.z = v.z;
	}
	void operator += (Box3 b)
	{
		if (b.m_min.x < m_min.x) m_min.x = b.m_min.x;
		if (b.m_min.y < m_min.y) m_min.y = b.m_min.y;
		if (b.m_min.z < m_min.z) m_min.z = b.m_min.z;

		if (b.m_max.x > m_max.x) m_max.x = b.m_max.x;
		if (b.m_max.y > m_max.y) m_max.y = b.m_max.y;
		if (b.m_max.z > m_max.z) m_max.z = b.m_max.z;
	}

	static Box3 empty()
	{
		return Box3(Vec3(FLT_MAX, FLT_MAX, FLT_MAX), Vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX));
	}

private:
	Vec3 m_min, m_max;
};