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
	int3 get_triangle(int f)
	{
		return int3(m_mesh.m_faces[f].i0, m_mesh.m_faces[f].i1, m_mesh.m_faces[f].i2);
	}
	float3 get_position(int v)
	{
		auto& pos = m_mesh.m_positions[v];
		return float3(pos.x, pos.y, pos.z);
	}
	template< class T > void ensure_size(T& container, int size)
	{
		if (container.size() <= size) container.resize(size + 1);
	}
	void set_color(int v, float3 rgb)
	{
		ensure_size(m_mesh.m_colors, v);
		m_mesh.m_colors[v] = Vec4(rgb.x, rgb.y, rgb.z, 0.0f);
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
	void set_triangle(int i, int3 v)
	{
		ensure_size(m_mesh.m_faces, i);
		m_mesh.m_faces[i] = xr::MESH::FACE(v[0], v[1], v[2]);
	}

	xr::MESH& m_mesh;
};



int main()
{
	xr::MESH mesh;
	XrMeshAdapter xma(mesh);

	VOLUME<int> voxels(128, 128, 128);
	float3		mesh_scale, mesh_pivot;

	if (mesh.read_obj("TreeTrunk.obj"))
	{
#if 1// skeletonize mesh
		xr::MESH skeleton;
		XrMeshAdapter xma_skel(skeleton);
		mesh_skeleton(xma, xma_skel);

		skeleton.write_obj("TreeTrunkSkeleton.obj");
#endif
	
#if 0	// voxelize mesh
		clock_t t0 = clock();
		mesh_to_voxel(xma, voxels, mesh_scale, mesh_pivot);
		printf("mesh_to_voxel: %d ms\n", (clock() - t0) * 1000 / CLOCKS_PER_SEC);

	#if 0 // meshify voxels
			xr::MESH voxel;
			XrMeshAdapter vox_mesh(voxel);
			voxel_to_mesh(voxels, vox_mesh, mesh_scale, mesh_pivot);
			vox_mesh.write_obj("TreeTrunkVoxel.obj");
	#endif
#endif
	}
	else
	{
		for (int i = 0; i < voxels.xr * voxels.yr * voxels.zr; ++i)
			voxels.buffer[i] = (i % 17 == 0) ? 1 : 0;
	}

	return 0;
}