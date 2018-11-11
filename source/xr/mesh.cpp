#include "mesh.h"
#include <stdio.h>

namespace xr
{
	void MESH::compact_vertices()
	{
		VECTOR<int> remap(positions.size());
		remap.fill(0);

		// mark references vertices
		for (int i = 0, n = faces.size(); i < n; ++i) {
			remap[faces[i].i0] = 1;
			remap[faces[i].i1] = 1;
			remap[faces[i].i2] = 1;
		}

		// compact referenced vertices and compute remap
		size_t vertex_count = 0;
		for (size_t i = 0, n = remap.size(); i < n; ++i) {
			if (!remap[i]) continue;
			remap[i] = vertex_count;
			positions[vertex_count] = positions[i];
			vertex_count++;
		}
		positions.resize(vertex_count);

		// remap indices to the compacted vertices
		for (size_t i = 0, n = faces.size(); i < n; ++i) {
			for (int j = 0; j < 3; ++j)
			{
				faces[i].i[j] = remap[faces[i].i[j]];
			}
		}
	}

	void MESH::compact_indices()
	{
		if (faces.empty()) return;

		size_t dst = 0;
		for (int src = 0, n = faces.size(); src < n; ++src)
		{
			if (faces[src].i0 == -1) continue;

			if (dst != src) {
				faces[dst] = faces[src];
			}
			dst++;
		}
		faces.resize(dst);
	}

	void MESH::center(Vec4 v)
	{
		for (int i = 0; i < positions.size(); ++i)
		{
			positions[i] = positions[i] - v;
		}
	}

	void MESH::compute_face_normals()
	{
		int n = faces.size();
		face_normals.resize(n);
		for (int f = 0; f < n; ++f)
		{
			const MESH::FACE& face = faces[f];
			Vec4 v0 = positions[face.i0] - positions[face.i1];
			Vec4 v1 = positions[face.i0] - positions[face.i2];
			face_normals[f] = normalize(cross(v0, v1));
		}
	}

	void MESH::compute_vertex_faces()
	{
		int n = positions.size();
		vertex_faces.resize(n);
		for (int f = 0; f < n; ++f)
		{
			const int i0 = faces[f].i0, i1 = faces[f].i1, i2 = faces[f].i2;
			vertex_faces[i0].push_back(f);
			vertex_faces[i1].push_back(f);
			vertex_faces[i2].push_back(f);
		}
	}

	void MESH::compute_vertex_normals()
	{
		if (face_normals.size() != faces.size())
			compute_face_normals();

		if (vertex_faces.size() != positions.size())
			compute_vertex_faces();

		int n = positions.size();
		vertex_normals.resize(n);
		for (int i = 0; i < n; ++i)
		{
			const VECTOR<int>& vf = vertex_faces[i];
			if (vf.empty())
			{
				vertex_normals[i] = Vec4(0, 0, 0, 0);
				continue;
			}

			Vec4 sum(0, 0, 0);
			for (int j = 0, jn = vf.size(); j < jn; ++j)
				sum = sum + vertex_normals[vf[j]];
			vertex_normals[i] = sum * (1.0f / vf.size());
		}
	}

	bool MESH::write_obj(const char* filename)
	{
		FILE* fp = FILE::open(filename, "wb");
		if (!fp) {
			log("ERROR: can not create '%s' file!\n", filename);
			return false;
		}
		for (size_t i = 0, n = positions.size(); i < n; ++i)
		{
			fast_printf(fp, "v %.5f %.5f %.5f\n", positions[i].x, positions[i].y, positions[i].z);
		}
		for (size_t i = 0, n = faces.size(); i < n; ++i)
		{
			fast_printf(fp, "f %d %d %d\n", faces[i].i0 + 1, faces[i].i1 + 1, faces[i].i2 + 1);
		}
		delete fp;
		return true;
	}

	void MESH::clear()
	{
		positions.clear();
		faces.clear();
		face_normals.clear();
		vertex_normals.clear();
		vertex_faces.clear();
	}
}