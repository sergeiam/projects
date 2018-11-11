#pragma once

#include <xr/core.h>
#include <xr/math.h>
#include <xr/vector.h>"
#include <xr/mesh.h>



#define VERBOSE 1

#define VERTEX_FREE			0.0f
#define VERTEX_FIXED		-1.0f
#define VERTEX_OUTER_EDGE_1	-2.0f
#define VERTEX_OUTER_EDGE_2	-3.0f

namespace xr
{
	class mesh_simplifier
	{
	public:
		MESH m_mesh;

		void simplify(float degrees_tollerance, float min_res);
		void compact_vertices();
		void compact_indices();

		void debug_show_vertex(int vertex, const char* filename);

		void center(Vec4 v);

	private:
		int find_best_merge(int v, float epsilon, float max_edge_len_sq);

		void check_integrity();
		void compute_face_normals();

		VECTOR<VECTOR<int>,false>	m_vert_faces;
		VECTOR<Vec4>				m_vert_normals;
		VECTOR<Vec4>				m_face_normals;
	};
}