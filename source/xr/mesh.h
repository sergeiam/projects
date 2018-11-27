#pragma once

#include <xr/core.h>
#include <xr/math.h>
#include <xr/vector.h>


namespace xr
{
	class MESH
	{
	public:
		const float VW_FREE = 0.0f;
		const float VW_FIXED = -1.0f;
		const float VW_OUTER_EDGE_1 = -2.0f;
		const float VW_OUTER_EDGE_2 = -3.0f;

		struct FACE {
			union {
				int i[3];
				struct {
					int	i0, i1, i2;
				};
			};

			FACE() {}
			FACE(int _i0, int _i1, int _i2) : i0(_i0), i1(_i1), i2(_i2) {}
		};

		VECTOR<Vec4>	positions;
		VECTOR<FACE>	faces;
		VECTOR<Vec4>	face_normals;
		VECTOR<Vec4>	vertex_normals;
		VECTOR<VECTOR<int>, false>	vertex_faces;

		VECTOR<Vec2>	tex_coords1, tex_coords2;
		VECTOR<FACE>	t_faces;

		VECTOR<Vec4>	normals;
		VECTOR<FACE>	n_faces;

		void	simplify(float degrees_tollerance);

		void	compact_vertices();
		void	compact_indices();

		void	compute_face_normals();
		void	compute_vertex_faces();
		void	compute_vertex_normals();

		void	center(Vec4 v);

		void	clear();

		bool	write_obj(const char* file);
		bool	read_obj(const char* file);
	};
}