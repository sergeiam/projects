#include <stdio.h>
#include <memory.h>
#include <windows.h>

#include <xr/mesh.h>

#include <unordered_map>
#include <vector>

#include "tree_vertex_colorizer.h"



struct XrMeshAdapter : public MeshAdapter
{
	XrMeshAdapter(xr::MESH& m) : m_mesh(m)
	{
		m_mesh.m_colors.resize(m_mesh.m_positions.size());
	}

	int	num_triangles()
	{
		return m_mesh.num_faces();
	}
	void triangle(int f, int& v0, int& v1, int& v2)
	{
		v0 = m_mesh.m_faces[f].i0;
		v1 = m_mesh.m_faces[f].i1;
		v2 = m_mesh.m_faces[f].i2;
	}
	void position(int v, float& x, float& y, float& z)
	{
		auto& pos = m_mesh.m_positions[v];
		x = pos.x;
		y = pos.y;
		z = pos.z;
	}
	template< class T > void ensure_size(T& container, int size)
	{
		if (container.size() <= size) container.resize(size + 1);
	}
	void set_color(int v, float r, float g, float b)
	{
		ensure_size(m_mesh.m_colors, v);
		m_mesh.m_colors[v] = Vec4(r, g, b, 0.0f);
	}
	void set_position(int v, float3 pos)
	{
		ensure_size(m_mesh.m_positions, v);
		m_mesh.m_positions[v] = Vec3(pos.x, pos.y, pos.z);
	}
	void set_normal(int v, float3 value)
	{
		ensure_size(m_mesh.m_normals, v);
		m_mesh.m_normals[v] = Vec3(value.x, value.y, value.z);
	}
	void set_triangle(int i, int v0, int v1, int v2)
	{
		ensure_size(m_mesh.m_faces, i);
		m_mesh.m_faces[i] = xr::MESH::FACE(v0, v1, v2);
	}

	xr::MESH& m_mesh;
};



int main()
{
	xr::MESH mesh;
	XrMeshAdapter xma(mesh);

	VOLUME<int> voxels(128, 128, 128);

	if (mesh.read_obj("TreeTrunk.obj"))
	{
		clock_t t0 = clock();
		mesh_to_voxel(xma, voxels);
		printf("mesh_to_voxel: %d ms\n", (clock() - t0) * 1000 / CLOCKS_PER_SEC);
	}
	else
	{
		for (int i = 0; i < voxels.xr * voxels.yr * voxels.zr; ++i)
			voxels.buffer[i] = (i % 17 == 0) ? 1 : 0;
	}

	mesh.clear();
	voxel_to_mesh(voxels, xma, float3(1.0f, 1.0f, 1.0f), float3(0.0f, 0.0f, 0.0f));
	mesh.write_obj("TreeTrunkVoxel.obj");

	return 0;
}