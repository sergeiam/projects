#pragma once

#include <xr/core.h>
#include <xr/math.h>
#include <xr/vector.h>

#include <string>


namespace xr
{
	class MESH
	{
	public:
		const float VW_FREE = 0.0f;
		const float VW_FIXED = -1.0f;
		const float VW_OUTER_EDGE_1 = -2.0f;
		const float VW_OUTER_EDGE_2 = -3.0f;

		struct FACE
		{
			union {
				int i[3];
				struct {
					int	i0, i1, i2;
				};
			};
			int id;

			FACE() {}
			FACE(int _i0, int _i1, int _i2) : i0(_i0), i1(_i1), i2(_i2) {}
		};

	// vertex data
		VECTOR<Vec3>	m_positions;
		VECTOR<Vec3>	m_normals;
		VECTOR<Vec2>	m_uv1, m_uv2;
		VECTOR<Vec4>	m_colors;

		VECTOR<VECTOR<int>, false>	m_vertex_faces;
		VECTOR<VECTOR<int>, false>	m_vertex_neighbors;

	// face data
		VECTOR<FACE>	m_faces;
		VECTOR<Vec3>	m_face_normals;

		struct MATERIAL
		{
			std::string		name;
			Vec3			diffuse, specular, ambient;
		};
		VECTOR<MATERIAL>	m_materials;

		int num_vertices() const { return m_positions.size(); }
		int num_faces() const { return m_faces.size(); }

		void copy_vertices(const MESH& rhs);


		struct face_iterator
		{
			MESH& mesh;
			int fi;

			face_iterator(MESH& _mesh, int _fi) : mesh(_mesh), fi(_fi) {}

			int& index(int idx)
			{
				return mesh.m_faces[fi].i[idx];
			}
			Vec3& position(int idx)
			{
				return mesh.m_positions[mesh.m_faces[fi].i[idx]];
			}
			Vec4& color(int idx)
			{
				return mesh.m_colors[mesh.m_faces[fi].i[idx]];
			}
			void operator++()
			{
				fi++;
			}
			bool operator != (const face_iterator& rhs) const
			{
				return fi != rhs.fi;
			}
			FACE& operator*()
			{
				return mesh.m_faces[fi];
			}
		};
		face_iterator begin_face()
		{
			return face_iterator(*this, 0);
		}
		face_iterator end_face()
		{
			return face_iterator(*this, m_faces.size());
		}

		struct vertex_iterator
		{
			MESH&	mesh;
			int		vi;

			vertex_iterator(MESH& _mesh, int _vi) : mesh(_mesh), vi(_vi) {}

			Vec3& position() { return mesh.m_positions[vi]; }
			Vec4& color() { return mesh.m_colors[vi]; }

			void operator++()
			{
				vi++;
			}
			bool operator != (const vertex_iterator& rhs) const
			{
				return vi != rhs.vi;
			}
		};
		vertex_iterator begin_vertex()
		{
			return vertex_iterator(*this, 0);
		}
		vertex_iterator end_vertex()
		{
			return vertex_iterator(*this, m_positions.size());
		}

		void	simplify(float degrees_tollerance);

		void	compact_vertices();
		void	compact_indices();

		void	compute_face_normals();
		void	compute_vertex_faces();
		void	compute_vertex_normals();
		void	compute_vertex_neighbors();

		Box3	compute_box_around_faces();
		Box3	compute_box_around_vertices();

		void	quantize_vertices(float pos_eps);

		void	center(Vec3 v);

		void	clear();

		bool	write_obj(const char* file, const char* material_file = nullptr);
		bool	read_obj(const char* file);

		bool	write_ply(const char* file);
		bool	read_ply(const char* file);

		void	add_cube(Vec3 pos, float size, Vec4 color);

		void	set_material_name(int index, const char* name);
		void	set_material_diffuse(int index, Vec3 color);

		static Vec3 barycentric_coords(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& p);
	};
}