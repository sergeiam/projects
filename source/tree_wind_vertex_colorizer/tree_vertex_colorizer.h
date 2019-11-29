#pragma once

// Colorize a mesh encoding tree height in the red channel, and tree branch distance to the trunk in the green channel

#include <math.h>
#include <vector>
#include <thread>
#include <algorithm>



struct float3
{
	union
	{
		float v[3];
		struct {
			float x, y, z;
		};
	};

	float3() {}
	float3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
	float3(const float3& rhs) : x(rhs.x), y(rhs.y), z(rhs.z) {}


	friend float3 operator + (float3 a, float3 b)
	{
		return float3(a.x + b.x, a.y + b.y, a.z + b.z);
	}
	friend float3 operator - (float3 a, float3 b)
	{
		return float3(a.x - b.x, a.y - b.y, a.z - b.z);
	}
	friend float3 operator * (float3 a, float k)
	{
		return float3(a.x*k, a.y*k, a.z*k);
	}
	friend float3 operator * (float3 a, float3 b)
	{
		return float3(a.x*b.x, a.y*b.y, a.z*b.z);
	}
	friend float3 operator / (float3 a, float3 b)
	{
		return float3(a.x/b.x, a.y/b.y, a.z/b.z);
	}
	float dot(float3 rhs) const
	{
		return x*rhs.x + y*rhs.y + z*rhs.z;
	}
	float length_sq() const { return dot(*this); }
	float length() const { return sqrtf(length_sq()); }

	float3 cross(float3 rhs) const
	{
		return float3(y*rhs.z - z*rhs.y, z*rhs.x - x*rhs.z, x*rhs.y - y*rhs.x);
	}
	float3 normalize() const
	{
		return *this * (1.0f / length());
	}
	void add_to_box(float3& box_min, float3& box_max) const
	{
		box_min.x = std::min(box_min.x, x);
		box_min.y = std::min(box_min.y, y);
		box_min.z = std::min(box_min.z, z);

		box_max.x = std::max(box_max.x, x);
		box_max.y = std::max(box_max.y, y);
		box_max.z = std::max(box_max.z, z);
	}
	bool inside(const float3& box_min, const float3& box_max) const
	{
		return x >= box_min.x && y >= box_min.y && z >= box_min.z && x <= box_max.x && y <= box_max.y && z <= box_max.z;
	}
};

struct MeshAdapter
{
	int		num_triangles();
	void	triangle(int i, int& v0, int& v1, int& v2);
	void	position(int v, float& x, float& y, float& z);

	void	set_position(int v, float3 value);
	void	set_normal(int v, float3 value);
	void	set_color(int v, float r, float g, float b);
	void	set_triangle(int i, int v0, int v1, int v2);
};

template< class T > struct VOLUME
{
	T* buffer;
	int xr, yr, zr;

	VOLUME() : buffer(nullptr), xr(0), yr(0), zr(0) {}
	VOLUME(int xr, int yr, int zr) : xr(xr), yr(yr), zr(zr) { buffer = new T[xr * yr * zr]; }
	VOLUME(T* ptr, int _xr, int _yr, int _zr) : buffer(ptr), xr(_xr), yr(_yr), zr(_zr) {}

	T& operator()(int x, int y, int z)
	{
		return buffer[x + y * xr + z * xr * yr];
	}
};

bool point_vs_triangle(const float3& point, const float3& v0, const float3& v1, const float3& v2, const float3& N, float radius)
{
	const float Nd = -N.dot(v0);
	const float d = N.dot(point) + Nd;

	if (d < -radius || d > radius) return false;

	float3 pt = point - N*d;

	float3 e0 = v1 - v0;
	float3 e1 = v2 - v0;
	float dot00 = e0.dot(e0);
	float dot01 = e0.dot(e1);
	float dot11 = e1.dot(e1);
	float invDenom = 1.f / (dot00 * dot11 - dot01 * dot01);

	float3 e2 = pt - v0;
	float dot02 = e0.dot(e2);
	float dot12 = e1.dot(e2);

	float v = (dot11 * dot02 - dot01 * dot12) * invDenom;
	float w = (dot00 * dot12 - dot01 * dot02) * invDenom;
	float u = 1.0f - v - w;

	return u < 1.0f && v < 1.0f && w < 1.0f;
}

template< class TMeshAdapter > void mesh_to_voxel(TMeshAdapter& ma, VOLUME<int>& voxels)
{
	const int xr = voxels.xr, yr = voxels.yr, zr = voxels.zr;

	memset(voxels.buffer, 0, xr * yr * zr * sizeof(voxels.buffer[0]));

	float3 box_min(FLT_MAX, FLT_MAX, FLT_MAX), box_max(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	int count = 0;

	for (int i = 0; i < ma.num_triangles(); ++i)
	{
		int v[3];
		ma.triangle(i, v[0], v[1], v[2]);

		if (v[0] > count) count = v[0];
		if (v[1] > count) count = v[1];
		if (v[2] > count) count = v[2];

		float3 pt[3];
		for (int j = 0; j < 3; ++j)
		{
			float3 pos;
			ma.position(v[j], pos.x, pos.y, pos.z);
			pos.add_to_box(box_min, box_max);
		}
	}
	count += 1;

	memset(voxels.buffer, 0, xr * yr * zr * sizeof(int));

	float3 box_size = box_max - box_min;

	box_max = box_max + box_size * 0.05f;
	box_size = box_max - box_min;


	float3 inv_box_size(1.0f / box_size.x, 1.0f / box_size.y, 1.0f / box_size.z);

	float3* npos = new float3[count];
	for (int i = 0; i < count; ++i)
	{
		float3& p = npos[i];
		ma.position(i, p.x, p.y, p.z);
		p = (p - box_min) * inv_box_size;
	}

	auto rasterize_voxels = [&ma, npos, xr, yr, zr, &voxels](int first, int last) -> void
	{
		const float3 inv_res(1.0f / voxels.xr, 1.0f / voxels.yr, 1.0f / voxels.zr);

		const float radius = std::max(inv_res.x, std::max(inv_res.y, inv_res.z)) * 0.8f;

		for (int i = first; i < last; ++i)
		{
			int v[3];
			ma.triangle(i, v[0], v[1], v[2]);

			const float3& p0 = npos[v[0]];
			const float3& p1 = npos[v[1]];
			const float3& p2 = npos[v[2]];

			float3 a = p0 - p1;
			float3 b = p0 - p2;
			float3 N = a.cross(b).normalize();

			float3 t_min = p0, t_max = p0;
			p1.add_to_box(t_min, t_max);
			p2.add_to_box(t_min, t_max);

			voxels(int(p0.x * xr), int(p0.y * yr), int(p0.z * zr)) = 1;
			voxels(int(p1.x * xr), int(p1.y * yr), int(p1.z * zr)) = 1;
			voxels(int(p2.x * xr), int(p2.y * yr), int(p2.z * zr)) = 1;

			for (int z = int(t_min.z * zr), max_z = int(t_max.z * zr); z <= max_z; ++z) {
				for (int y = int(t_min.y * yr), max_y = int(t_max.y * yr); y <= max_y; ++y) {
					for (int x = int(t_min.x * xr), max_x = int(t_max.x * xr); x <= max_x; ++x)
					{
						if (voxels(x, y, z)) continue;

						float3 pt = float3(x, y, z) * inv_res;

						if (point_vs_triangle(pt, p0, p1, p2, N, radius))
						{
							voxels(x, y, z) = 1;
						}
					}
				}
			}
		}
	};

	int num_cores = std::max(2U, std::thread::hardware_concurrency() * 1);
	int num_triangles = ma.num_triangles();
	int partition = num_triangles / num_cores + 1;

	std::vector<std::thread> jobs;
	for (int i = 0; i < num_triangles; i += partition)
	{
		jobs.emplace_back( rasterize_voxels, i, std::min(i + partition, num_triangles));
	}
	for (auto& j : jobs)
	{
		j.join();
	}
}

template< class TMeshAdapter > void voxel_to_mesh(VOLUME<int>& voxels, TMeshAdapter& ma, float3 scale, float3 pivot)
{
	float3 diagonal(1.0f / voxels.xr, 1.0f / voxels.yr, 1.0f / voxels.zr);

	int vi = 0, fi = 0;
	for (int x = 0; x < voxels.xr; ++x)
	{
		for (int y = 0; y < voxels.yr; ++y)
		{
			for (int z = 0; z < voxels.zr; ++z)
			{
				if (!voxels(x, y, z)) continue;

				for (int d = 0; d<2; ++d)
				{
					float direction = d ? 1.0f : -1.0f;

					int xc = x + d, yc = y + d, zc = z + d;

					for (int axis = 0; axis < 3; ++axis)
					{
						float3 pos = float3(xc, yc, zc) * diagonal;

						float3 axis1(0.0f, 0.0f, 0.0f), axis2(0.0f, 0.0f, 0.0f);

						int ax1 = (axis + 1) % 3;
						int ax2 = (axis + 2) % 3;

						axis1.v[ax1] = -diagonal.v[ax1] * direction;
						axis2.v[ax2] = -diagonal.v[ax2] * direction;

						float3 n(0.0f, 0.0f, 0.0f);
						n.v[axis] = direction;

						ma.set_position(vi, pos * scale + pivot);
						ma.set_normal(vi++, n);
						ma.set_position(vi, (pos + axis1) * scale + pivot);
						ma.set_normal(vi++, n);
						ma.set_position(vi, (pos + axis1 + axis2) * scale + pivot);
						ma.set_normal(vi++, n);
						ma.set_position(vi, (pos + axis2) * scale + pivot);
						ma.set_normal(vi++, n);

						ma.set_triangle(fi++, vi - 4, vi - 3, vi - 2);
						ma.set_triangle(fi++, vi - 4, vi - 2, vi - 1);
					}
				}
			}
		}
	}
}