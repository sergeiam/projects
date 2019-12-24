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
	float3 operator -()
	{
		return float3(-x, -y, -z);
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

struct int3
{
	int v[3];

	int3() {}
	int3(int i0, int i1, int i2) { v[0] = i0; v[1] = i1; v[2] = i2; }
	int3(const int* ptr) { v[0] = ptr[0]; v[1] = ptr[1]; v[2] = ptr[2]; }

	int& operator[](int i) { return v[i]; }
	int operator[](int i) const { return v[i]; }
};

struct MeshAdapter
{
	int		num_triangles();
	int3	get_triangle(int i);
	float3	get_position(int i);

	void	set_position(int v, float3 value);
	void	set_normal(int v, float3 value);
	void	set_color(int v, float3 value);
	void	set_triangle(int i, int3 indices);
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

template< class TMeshAdapter > void mesh_to_voxel(TMeshAdapter& ma, VOLUME<int>& voxels, float3& scale, float3& pivot)
{
	const int xr = voxels.xr, yr = voxels.yr, zr = voxels.zr;

	memset(voxels.buffer, 0, xr * yr * zr * sizeof(voxels.buffer[0]));

	float3 box_min(FLT_MAX, FLT_MAX, FLT_MAX), box_max(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	int count = 0;
	for (int i = 0; i < ma.num_triangles(); ++i)
	{
		int3 v = ma.get_triangle(i);
		for (int j = 0; j < 3; ++j)
		{
			cound = std::max( count, v[j]);
			ma.get_position(v[j]).add_to_box(box_min, box_max);
		}
	}
	count += 1;

	memset(voxels.buffer, 0, xr * yr * zr * sizeof(int));

	float3 box_size = box_max - box_min;

	box_max = box_max + box_size * 0.05f;
	box_size = box_max - box_min;

	scale = box_size;
	pivot = box_min;

	float3 inv_box_size(1.0f / box_size.x, 1.0f / box_size.y, 1.0f / box_size.z);

	float3* npos = new float3[count];
	for (int i = 0; i < count; ++i)
	{
		float3& p = npos[i];
		p = ma.get_position(i);
		p = (p - box_min) * inv_box_size;
	}

	auto rasterize_voxels = [&ma, npos, xr, yr, zr, &voxels](int first, int last) -> void
	{
		const float3 inv_res(1.0f / voxels.xr, 1.0f / voxels.yr, 1.0f / voxels.zr);

		const float radius = std::max(inv_res.x, std::max(inv_res.y, inv_res.z)) * 0.8f;

		for (int i = first; i < last; ++i)
		{
			int3 v = ma.get_triangle(i);

			const float3& p0 = npos[v[0]];
			const float3& p1 = npos[v[1]];
			const float3& p2 = npos[v[2]];

			float3 a = p0 - p1;
			float3 b = p0 - p2;
			float3 N = a.cross(b).normalize();

			float3 t_min = p0, t_max = p0;
			p1.add_to_box(t_min, t_max);
			p2.add_to_box(t_min, t_max);

			//voxels(int(p0.x * xr), int(p0.y * yr), int(p0.z * zr)) = 1;
			//voxels(int(p1.x * xr), int(p1.y * yr), int(p1.z * zr)) = 1;
			//voxels(int(p2.x * xr), int(p2.y * yr), int(p2.z * zr)) = 1;

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

template< class TMeshAdapter > void mesh_skeleton(TMeshAdapter& ma, TMeshAdapter& skeleton)
{
	int num_vertices = 0;
	for (int i = 0; i < ma.num_triangles(); ++i)
	{
		int3 v = ma.get_triangle(i);
		num_vertices = std::max(std::max(num_vertices, v[0]), std::max(v[1], v[2]));
	}
	num_vertices++;

// find max vertex index

	std::vector<int> vertex_remap(num_vertices);
	for (int i = 0; i < num_vertices; ++i)
	{
		vertex_remap[i] = i;
	}

// find max edge length

	float max_edge_len_sq = 0.0f;
	for (int i = 0; i < ma.num_triangles(); ++i)
	{
		int3 v = ma.get_triangle(i);
		float3 pos[3] = { ma.get_position(v[0]), ma.get_position(v[1]), ma.get_position(v[2]) };
		max_edge_len_sq = std::max(
			std::max(max_edge_len_sq, (pos[0] - pos[1]).length_sq()),
			std::max((pos[1] - pos[2]).length_sq(), (pos[2] - pos[0]).length_sq())
		);
	}

// clamp small edges

	std::vector<int> next(num_vertices);
	std::fill(next.begin(), next.end(), -1);

	const float max_edge_len = sqrtf(max_edge_len_sq);
	for (int i = 0; i < ma.num_triangles(); ++i)
	{
		int3 v = ma.get_triangle(i);
		v = int3(vertex_remap[v[0]], vertex_remap[v[1]], vertex_remap[v[2]]);
		float3 pos[3] = { ma.get_position(v[0]), ma.get_position(v[1]), ma.get_position(v[2]) };

		int prev = 2;
		for (int j = 0; j < 3; prev = j, ++j )
		{
			float len_sq = (pos[j] - pos[prev]).length_sq();
			if (len_sq > 0.0f && len_sq < max_edge_len * 0.3f * 0.3f)
			{
				float3 avg = (pos[j] + pos[prev]) * 0.5f;
				pos[j] = pos[prev] = avg;

				// a < b
				int a = v[j], b = v[prev];
				if (a > b) std::swap(a, b);

				// let the whole 'B' island point to 'A' and join them both
				for (int index = b; b >= 0; b = next[b])
				{
					vertex_remap[b] = a;
				}
				for (int index = a; next[a] >= 0; a = next[a]);
				next[a] = b;
			}
		}
	}

// compact all vertex islands into continious index range [0.. number of islands]

	int curr_index = 0;
	for (int i = 0; i < num_vertices; ++i)
	{
		if (vertex_remap[i] < curr_index)
		{
			skeleton.set_position(curr_index, ma.get_position(vertex_remap[i]));

			for (int j = i; j >= 0; j = next[j])
			{
				vertex_remap[j] = curr_index;
			}
			curr_index++;
		}
	}

// create only faces that contain at least two different indices

	int num_triangles = 0;
	for (int i = 0; i < ma.num_triangles(); ++i)
	{
		int3 face = ma.get_triangle(i);
		face = int3(vertex_remap[face[0]], vertex_remap[face[1]], vertex_remap[face[2]]);
		if (face[0] != face[1] || face[1] != face[2] || face[2] != face[0])
		{
			skeleton.set_triangle(num_triangles++, face);
		}
	}
}