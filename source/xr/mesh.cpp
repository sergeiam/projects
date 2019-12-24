#include <xr/mesh.h>
#include <xr/file.h>
#include <xr/hash.h>
#include <algorithm>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <functional>

namespace xr
{
	enum
	{
		COMPONENT_POS = 1,
		COMPONENT_UV = 2,
		COMPONENT_NORMAL = 4
	};


	void MESH::compact_vertices()
	{
		VECTOR<int> remap(m_positions.size());
		remap.fill(0);

		// mark references vertices
		for (int i = 0, n = m_faces.size(); i < n; ++i) {
			remap[m_faces[i].i0] = 1;
			remap[m_faces[i].i1] = 1;
			remap[m_faces[i].i2] = 1;
		}

		// compact referenced vertices and compute remap
		size_t vertex_count = 0;
		for (size_t i = 0, n = remap.size(); i < n; ++i) {
			if (!remap[i]) continue;
			remap[i] = vertex_count;
			m_positions[vertex_count] = m_positions[i];
			vertex_count++;
		}
		m_positions.resize(vertex_count);

		// remap indices to the compacted vertices
		for (size_t i = 0, n = m_faces.size(); i < n; ++i) {
			for (int j = 0; j < 3; ++j)
			{
				m_faces[i].i[j] = remap[m_faces[i].i[j]];
			}
		}
	}

	void MESH::compact_indices()
	{
		if (m_faces.empty()) return;

		size_t dst = 0;
		for (int src = 0, n = m_faces.size(); src < n; ++src)
		{
			if (m_faces[src].i0 == -1) continue;

			if (dst != src) {
				m_faces[dst] = m_faces[src];
			}
			dst++;
		}
		m_faces.resize(dst);
	}

	void MESH::center(Vec3 v)
	{
		for (int i = 0; i < m_positions.size(); ++i)
		{
			m_positions[i] = m_positions[i] - v;
		}
	}

	void MESH::compute_face_normals()
	{
		int n = m_faces.size();
		m_face_normals.resize(n);
		for (int f = 0; f < n; ++f)
		{
			const MESH::FACE& face = m_faces[f];
			Vec3 v0 = m_positions[face.i0] - m_positions[face.i1];
			Vec3 v1 = m_positions[face.i0] - m_positions[face.i2];
			m_face_normals[f] = normalize(cross(v0, v1));
		}
	}

	void MESH::compute_vertex_normals()
	{
		if (m_face_normals.size() != m_faces.size())
			compute_face_normals();

		if (m_vertex_faces.size() != m_positions.size())
			compute_vertex_faces();

		int n = m_positions.size();
		m_normals.resize(n);
		for (int i = 0; i < n; ++i)
		{
			const VECTOR<int>& vf = m_vertex_faces[i];
			if (vf.empty())
			{
				m_normals[i] = Vec3(0, 0, 0);
				continue;
			}

			Vec3 sum(0, 0, 0);
			for (int j = 0, jn = vf.size(); j < jn; ++j)
				sum = sum + m_normals[vf[j]];
			m_normals[i] = sum * (1.0f / vf.size());
		}
	}

	void MESH::clear()
	{
		m_positions.clear();
		m_normals.clear();
		m_uv1.clear();
		m_uv2.clear();
		m_colors.clear();

		m_faces.clear();

		m_face_normals.clear();
		m_vertex_faces.clear();
		m_materials.clear();
	}

	bool MESH::write_obj(const char* filename, const char* material_file)
	{
		FILE* fp = FILE::open(filename, FILE::WRITE | FILE::TRUNC);
		if (!fp) {
			log("ERROR: can not create '%s' file!\n", filename);
			return false;
		}
		FILE* fp_mtl = nullptr;
		if (material_file && !m_materials.empty())
		{
			fp_mtl = FILE::open(material_file, FILE::WRITE | FILE::TRUNC);
			if (!fp_mtl)
			{
				log("ERROR: can not create '%s' file!\n", material_file);

			}
		}

		int face_component = COMPONENT_POS;

		for (size_t i = 0; i < m_positions.size(); ++i)
		{
			fp->printf( "v %.5f %.5f %.5f\n", m_positions[i].x, m_positions[i].y, m_positions[i].z);
		}
		if (m_uv1.size())
		{
			for (size_t i = 0; i < m_uv1.size(); ++i)
			{
				fp->printf("vt %.5f %.5f\n", m_uv1[i].x, m_uv1[i].y);
			}
			face_component |= COMPONENT_UV;
		}
		if (m_normals.size())
		{
			for (size_t i = 0; i < m_normals.size(); ++i)
			{
				fp->printf("vn %.5f %.5f %.5f\n", m_normals[i].x, m_normals[i].y, m_normals[i].z);
			}
			face_component |= COMPONENT_NORMAL;
		}
		int prev_material = -1;

		for (size_t i = 0; i < m_faces.size(); ++i)
		{
			FACE f = m_faces[i];
			f.i0++;
			f.i1++;
			f.i2++;

			if (fp_mtl && f.id != prev_material && material_file)
			{
				if (m_materials[f.id].name.empty())
					fp->printf("usemtl %d\n", f.id);
				else
					fp->printf("usemtl %s\n", m_materials[f.id].name.c_str());
			}

			switch (face_component)
			{
				case COMPONENT_POS:
					fp->printf("f %d %d %d\n", f.i0, f.i1, f.i2);
					break;
				case COMPONENT_POS | COMPONENT_UV:
					fp->printf("f %d/%d %d/%d %d/%d\n", f.i0, f.i0, f.i1, f.i1, f.i2, f.i2);
					break;
				case COMPONENT_POS | COMPONENT_NORMAL:
					fp->printf("f %d//%d %d//%d %d//%d\n", f.i0, f.i0, f.i1, f.i1, f.i2, f.i2);
					break;
				case COMPONENT_POS | COMPONENT_UV | COMPONENT_NORMAL:
					fp->printf("f %d/%d/%d %d/%d/%d %d/%d/%d\n", f.i0, f.i0, f.i0, f.i1, f.i1, f.i1, f.i2, f.i2, f.i2);
					break;
			}
		}
		delete fp;

		if (fp_mtl)
		{
			for (int i = 0; i < m_materials.size(); ++i)
			{
				const MATERIAL& m = m_materials[i];
				if (m.name.empty())
					fp->printf("newmtl %d\n", i);
				else
					fp->printf("newmtl %s\n", m.name.c_str());
				if (m.ambient != Vec3(1, 1, 1)) fp->printf("Ka %0.2f %0.2f %0.2f\n", m.ambient.x, m.ambient.y, m.ambient.z);
				if (m.diffuse != Vec3(1, 1, 1)) fp->printf("Kd %0.2f %0.2f %0.2f\n", m.diffuse.x, m.diffuse.y, m.diffuse.z);
				if (m.specular != Vec3(1, 1, 1)) fp->printf("Ks %0.2f %0.2f %0.2f\n", m.specular.x, m.specular.y, m.specular.z);
			}
			delete fp_mtl;
		}
		return true;
	}

	struct FAT_VERTEX
	{
		Vec3 pos, normal;
		Vec2 uv;

		bool operator == (const FAT_VERTEX& rhs) const
		{
			return
				pos.x == rhs.pos.x && pos.y == rhs.pos.y && pos.z == rhs.pos.z &&
				uv.x == rhs.uv.x && uv.y == rhs.uv.y &&
				normal.x == rhs.normal.x && normal.y == rhs.normal.y && normal.z == rhs.normal.z;
		}
	};

	u32 hash(const FAT_VERTEX& v)
	{
		u32 h = hash(v.pos.x);
		h = h * 1337 + hash(v.pos.y);
		h = h * 1337 + hash(v.pos.z);

		h = h * 1337 + hash(v.uv.x);
		h = h * 1337 + hash(v.uv.y);

		h = h * 1337 + hash(v.normal.x);
		h = h * 1337 + hash(v.normal.y);
		h = h * 1337 + hash(v.normal.z);

		return h;
	}

	bool MESH::read_obj(const char* filename)
	{
		clear();

		FILE* fp = FILE::open(filename, FILE::READ );
		if (!fp) {
			log("ERROR: can not open '%s' file!\n", filename);
			return false;
		}

		VECTOR<Vec3> positions;
		VECTOR<Vec2> uvs;
		VECTOR<Vec3> normals;

		HASH_MAP<FAT_VERTEX, int>	v2i;

		int face_component_mask = 0;  // what compinents we should read from a face indices

		while (!fp->eof())
		{
			char line[1024];
			fp->read_line(line, 1024);

			switch (line[0])
			{
				case 'v':
					face_component_mask = 0;
					if (line[1] == 't') {
						float u, v;
						sscanf_s(line + 3, "%f %f", &u, &v);
						uvs.push_back(Vec2(u, 1.0f - v));
					}
					else if (line[1] == 'n') {
						Vec3 n;
						sscanf_s(line + 3, "%f %f %f", &n.x, &n.y, &n.z);
						normals.push_back(n);
					}
					else if (line[1] == ' ') {
						Vec3 pos;
						sscanf_s(line + 2, "%f %f %f", &pos.x, &pos.y, &pos.z);
						positions.push_back(pos);
					}
					break;
				case 'f':
				{
					FACE pf, tf, nf;
					const char* p = line + 2;

					if (!face_component_mask)
					{
						face_component_mask = COMPONENT_POS;
						const char* s1 = strchr(p, '/');
						const char* next_space = strchr(p, ' ');
						if (s1 && s1 < next_space)
						{
							const char* s2 = strchr(s1 + 1, '/');
							if (s2 && s2 < next_space)
							{
								if (s2 > s1 + 1)
									face_component_mask |= COMPONENT_UV;
								const char* s3 = strchr(s2 + 1, '/');
								if (s3 > s2 + 1 && s3 < next_space)
									face_component_mask |= COMPONENT_NORMAL;
							}
						}
					}

					FACE face;

					for (int index = 0; p; ++index )
					{
						int pi, ni, ui;

						switch (face_component_mask)
						{
							case COMPONENT_POS:
								sscanf_s(p, "%d", &pi); break;
							case COMPONENT_POS + COMPONENT_UV:
								sscanf_s(p, "%d/%d", &pi, &ui); break;
							case COMPONENT_POS + COMPONENT_UV + COMPONENT_NORMAL:
								sscanf_s(p, "%d/%d/%d", &pi, &ui, &ni); break;
							case COMPONENT_POS + COMPONENT_NORMAL:
								sscanf_s(p, "%d//%d", &pi, &ni); break;
						}

						pi += (pi > 0) ? -1 : positions.size();
						if (face_component_mask & COMPONENT_UV) ui += (ui > 0) ? -1 : uvs.size();
						if (face_component_mask & COMPONENT_NORMAL) ni += (ni > 0) ? -1 : normals.size();

						FAT_VERTEX fv;
						fv.pos = positions[pi];
						fv.normal = normals.empty() ? Vec3(0,0,0) : normals[ni];
						fv.uv = uvs.empty() ? Vec2(0,0) : uvs[ui];

						int vi;
						if (!v2i.find(fv, vi))
						{
							vi = m_positions.size();
							m_positions.push_back(fv.pos);
							if(normals.size()) m_normals.push_back(fv.normal);
							if(uvs.size()) m_uv1.push_back(fv.uv);
							v2i.insert(fv, vi);
						}

						// produce triangles from a fan - cycle 2 -> 1 and add to the 2nd index
						if (index <= 2)
						{
							face.i[index] = vi;
						}
						else
						{
							face.i[1] = face.i[2];
							face.i[2] = vi;
						}
						if (index >= 2) m_faces.push_back(face);

						p = strchr(p + 1, ' ');
						if (!p)
							break;
					}
					break;
				}
			}
		}

		delete fp;
		return true;
	}

	bool MESH::write_ply(const char* file)
	{
		FILE* fp = FILE::open(file, FILE::WRITE);
		if (!fp) {
			log("ERROR: can not open '%s' file!\n", file);
			return false;
		}

		fp->printf("ply\nformat ascii 1.0\n");
		fp->printf("element vertex %d\n", m_positions.size());
		fp->printf("property float32 x\nproperty float32 y\nproperty float32 z\n");
		if (m_normals.size())
			fp->printf("property float32 normalx\nproperty float32 normaly\nproperty float32 normalz\n");
		if(m_uv1.size())
			fp->printf("property float32 u\nproperty float32 v\n");

		bool colors_alpha = false;
		if (m_colors.size())
		{
			float prev = m_colors[0].w;
			fp->printf("property uchar red\nproperty uchar green\nproperty uchar blue\n");
			for( int i=1; i<m_colors.size(); ++i )
				if (m_colors[i].w != prev) {
					colors_alpha = true;
					break;
				}
			if (prev != 0.0f && prev != 1.0f)
				colors_alpha = true;
		}
		fp->printf("element face %d\n", m_faces.size());
		fp->printf("property list uint8 int32 vertex_indices\n");
		fp->printf("end_header\n");

		for (int i = 0; i < m_positions.size(); ++i)
		{
			fp->printf("%.6f %.6f %.6f", m_positions[i].x, m_positions[i].y, m_positions[i].z);
			if (m_normals.size())
				fp->printf(" %.6f %.6f %.6f", m_normals[i].x, m_normals[i].y, m_normals[i].z);
			if (m_uv1.size())
				fp->printf(" %.4f %.4f", m_uv1[i].x, m_uv1[i].y);
			if (m_colors.size())
			{
				fp->printf(" %d %d %d", u32(m_colors[i].x * 255.0f), u32(m_colors[i].y*255.0f), u32(m_colors[i].z*255.0f));
				//if (colors_alpha) fp->printf(" %d", u32(m_colors[i].w * 255.0f));
			}
			fp->printf("\n");
		}
		for (int i = 0; i < m_faces.size(); ++i)
		{
			fp->printf("3 %d %d %d\n", m_faces[i].i0, m_faces[i].i1, m_faces[i].i2);
		}
		delete fp;

		return true;
	}

	bool MESH::read_ply(const char* file)
	{
		clear();

		FILE* fp = FILE::open(file, FILE::READ);
		if (!fp) {
			log("ERROR: can not open '%s' file!\n", file);
			return false;
		}

		enum EPROPERTY
		{
			PROP_POS_X,
			PROP_POS_Y,
			PROP_POS_Z,
			PROP_NORM_X,
			PROP_NORM_Y,
			PROP_NORM_Z,
			PROP_COLOR_R,
			PROP_COLOR_G,
			PROP_COLOR_B,
			PROP_COLOR_A
		};

		auto read_float = [](const char* str, float& f) -> const char*
		{
			f = atof(str);
			str = strchr(str, ' ');
			if (str) str++;
			return str;
		};

		auto read_uchar = [](const char* str, float& u) -> const char*
		{
			u8 uc = atoi(str);
			str = strchr(str, ' ');
			if (str) str++;
			u = float(uc) / 255.0f;
			return str;
		};

		auto read_int = [](const char* str, int& i) -> const char*
		{
			i = atoi(str);
			str = strchr(str, ' ');
			if (str) str++;
			return str;
		};

		struct {
			const char* key;
			EPROPERTY prop;
		} rules[] = {
			{ "float32 x", PROP_POS_X },
			{ "float32 y", PROP_POS_Y },
			{ "float32 z", PROP_POS_Z },

			{ "uchar red", PROP_COLOR_R},
			{ "uchar green", PROP_COLOR_G},
			{ "uchar blue", PROP_COLOR_B},
			{ "uchar alpha", PROP_COLOR_A},
		};

		EPROPERTY props[32];
		int props_count = 0;

		auto check_prefix = [](const char* &str, const char* prefix) -> bool
		{
			int len = strlen(prefix);
			if (strncmp(str, prefix, len)) return false;

			str += len;
			return true;
		};

		int num_vertices = 0;

		while (!fp->eof())
		{
			char line[1024];
			fp->read_line(line, 1024);

			const char* ptr = line;

			if (check_prefix(ptr, "element vertex "))
			{
				num_vertices = atoi(ptr);
				m_positions.resize(num_vertices);
			}
			else if (check_prefix(ptr, "element face "))
			{
				m_faces.resize(atoi(ptr));
			}
			else if (check_prefix(ptr, "property "))
			{
				for (int i = 0; i < sizeof(rules) / sizeof(rules[0]); ++i)
				{
					if (!strncmp(ptr, rules[i].key, strlen(rules[i].key)))
					{
						props[props_count++] = rules[i].prop;

						switch (rules[i].prop)
						{
						case PROP_POS_X:
						case PROP_POS_Y:
						case PROP_POS_Z:
							m_positions.resize(num_vertices);
							break;

						case PROP_COLOR_R:
						case PROP_COLOR_G:
						case PROP_COLOR_B:
						case PROP_COLOR_A:
							m_colors.resize(num_vertices);
							break;

						case PROP_NORM_X:
						case PROP_NORM_Y:
						case PROP_NORM_Z:
							m_normals.resize(num_vertices);
							break;
						}
						break;
					}
				}
			}
			else if (check_prefix(ptr, "end_header"))
			{
				for (int i = 0; i < num_vertices; ++i)
				{
					fp->read_line(line, 1024);
					const char* ptr = line;

					for (int j = 0; j < props_count; ++j)
					{
						switch (props[j])
						{
							case PROP_POS_X:
								ptr = read_float(ptr, m_positions[i].x);
								break;
							case PROP_POS_Y:
								ptr = read_float(ptr, m_positions[i].y);
								break;
							case PROP_POS_Z:
								ptr = read_float(ptr, m_positions[i].z);
								break;

							case PROP_COLOR_R:
								ptr = read_uchar(ptr, m_colors[i].x);
								break;
							case PROP_COLOR_G:
								ptr = read_uchar(ptr, m_colors[i].y);
								break;
							case PROP_COLOR_B:
								ptr = read_uchar(ptr, m_colors[i].z);
								break;
							case PROP_COLOR_A:
								ptr = read_uchar(ptr, m_colors[i].w);
								break;
						}
					}
				}
				for (int i = 0, num_faces = m_faces.size(); i < num_faces; ++i)
				{
					fp->read_line(line, 1024);
					const char* ptr = line;

					int count;
					ptr = read_int(ptr, count);

					ptr = read_int(ptr, m_faces[i].i0);
					ptr = read_int(ptr, m_faces[i].i1);
					ptr = read_int(ptr, m_faces[i].i2);
				}
			}
		}
	}

	void MESH::compute_vertex_faces()
	{
		m_vertex_faces.clear();
		m_vertex_faces.resize(m_positions.size());

		for (int i = 0, n = m_faces.size(); i < n; ++i)
		{
			for (int j = 0; j < 3; ++j)
			{
				int vertex = m_faces[i].i[j];

				auto& vec = m_vertex_faces[vertex];

				int k = 0;
				for (int kn = vec.size(); k < kn; ++k)
					if (vec[k] == i) break;

				if (k == vec.size())
					vec.push_back(i);
			}
		}
	}

	void MESH::compute_vertex_neighbors()
	{
		m_vertex_neighbors.clear();
		m_vertex_neighbors.resize(m_positions.size());

		for (int i = 0, in = m_faces.size(); i < in; ++i)
		{
			for (int j = 0; j < 3; ++j)
			{
				int vertex = m_faces[i].i[j];

				auto& vec = m_vertex_neighbors[vertex];

				for (int n = 1; n <= 2; ++n)  // iterate both vertex neihbors
				{
					int neighbor = m_faces[i].i[(j + n) % 3];

					int k = 0;
					for (int kn = vec.size(); k < kn; ++k)
						if (vec[k] == neighbor) break;

					if (k == vec.size())
						vec.push_back(neighbor);
				}
			}
		}
	}

	void MESH::add_cube(Vec3 pos, float size, Vec4 color)
	{
		const bool add_colors = !m_colors.empty() || m_positions.empty();
		for (int i = 0; i < 8; ++i)
		{
			Vec3 sign((i & 1) ? +1.0f : -1.0f, (i & 2) ? +1.0f : -1.0f, (i & 4) ? +1.0f : -1.0f);
			m_positions.push_back(pos + sign * size * 0.5f);
			if (add_colors)
			{
				m_colors.push_back(color);
			}
		}

		auto add_quad = [this](int a, int b, int c, int d) -> void
		{
			m_faces.push_back(FACE(a, b, c));
			m_faces.push_back(FACE(b, d, c));
		};
		add_quad(0, 1, 2, 3);
		add_quad(5, 4, 7, 6);
		add_quad(1, 5, 3, 7);
		add_quad(4, 0, 6, 2);
		add_quad(0, 1, 4, 5);
		add_quad(2, 3, 6, 7);
	}

	Box3 MESH::compute_box_around_faces()
	{
		Box3 box = Box3::empty();
		for (int i = 0; i < m_faces.size(); ++i)
		{
			for (int j = 0; j < 3; ++j)
				box += m_positions[m_faces[i].i[j]];
		}
		return box;
	}

	Box3 MESH::compute_box_around_vertices()
	{
		Box3 box = Box3::empty();
		for (int i = 0; i < m_positions.size(); ++i)
		{
			box += m_positions[i];
		}
		return box;
	}

	void MESH::copy_vertices(const MESH& rhs)
	{
		m_positions = rhs.m_positions;
		m_normals = rhs.m_normals;
		m_uv1 = rhs.m_uv1;
		m_uv2 = rhs.m_uv2;
		m_colors = rhs.m_colors;
	}

	void MESH::quantize_vertices(float pos_eps)
	{
		Box3 box = compute_box_around_vertices();

		VECTOR<int>	verts;
		verts.resize(m_positions.size());
		for (int i = 0; i < verts.size(); verts[i] = i++);

		auto& positions = m_positions;

		std::sort(verts.begin(), verts.end(), [&positions](int a, int b) -> bool
		{
			return positions[a].x < positions[b].x;
		});

		const float epsilon = box.radius() * pos_eps;
		const float eps_sq = epsilon * epsilon;

		VECTOR<int> mapping;
		mapping.resize(m_positions.size());
		for (int i = 0; i < verts.size(); ++i)
			verts[i] = i;

		for (int i = 0; i < verts.size(); ++i)
		{
			int j;
			for (j = i + 1; j < verts.size(); ++j)
			{
				int vi = verts[i];
				int vj = verts[j];

				if (positions[vj].x - positions[vi].x > epsilon) break;

				if (length_sq(m_positions[vi] - m_positions[vj]) < eps_sq)
				{
					mapping[vj] = vi;
				}
			}
		}

	}

	Vec3 MESH::barycentric_coords(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& p)
	{
		Vec3 v0 = b - a, v1 = c - a, v2 = p - a;
		float d00 = dot(v0, v0);
		float d01 = dot(v0, v1);
		float d11 = dot(v1, v1);
		float d20 = dot(v2, v0);
		float d21 = dot(v2, v1);
		float denom = d00 * d11 - d01 * d01;

		float v = (d11 * d20 - d01 * d21) / denom;
		float w = (d00 * d21 - d01 * d20) / denom;

		return Vec3(1.0f - v - w, v, w);
	}
}