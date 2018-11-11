#include <xr/core.h>
#include <xr/heightfield_simplifier.h>

namespace xr
{
	struct hf_simplifier
	{
		hf_simplifier(const float* hf, const int* weights, int w, int h, float scale, float hole_value)
			: m_HF(hf), m_width(w), m_height(h), m_scale(scale), m_hole_value(hole_value), m_WF(weights)
		{
		}

		void simplify(int max_width_per_triangle, float epsilon, MESH* mesh);

	private:
		void simplify_recursive(int x1, int y1, int x2, int y2);
		void add_triangle(int i0, int i1, int i2);
		void process_quads();

		const float*	m_HF;
		const int*		m_WF;
		int				m_width, m_height, m_max_width_per_triangle;
		float			m_scale, m_hole_value, m_epsilon;
		MESH*			m_mesh;
		VECTOR<int>		m_VI;

		struct QUAD {
			int x, y, xsize, ysize;

			QUAD() {}
			QUAD(int _x, int _y, int _xsize, int _ysize) { x = _x; y = _y; xsize = _xsize; ysize = _ysize; }
		};
		VECTOR<QUAD>	m_quads;
	};

	void heightfield_simplify(const float* hf, const int* weights, int w, int h, float scale, float hole_value, int max_width_per_triangle, float epsilon, MESH& mesh)
	{
		hf_simplifier hs(hf, weights, w, h, scale, hole_value);
		hs.simplify(max_width_per_triangle, epsilon, &mesh);
	}

	void hf_simplifier::simplify(int max_width_per_triangle, float epsilon, MESH* mesh)
	{
		m_max_width_per_triangle = max_width_per_triangle;
		m_epsilon = epsilon;
		m_mesh = mesh;

		m_VI.resize(m_width * m_height);
		m_VI.fill(-1);

		m_quads.clear();

		simplify_recursive(0, 0, m_width-1, m_height-1);

		process_quads();

		const int src_verts = m_width * m_height;
		const int src_faces = (m_width - 1) * (m_height - 1) * 2;
		const int dst_verts = m_mesh->positions.size();
		const int dst_faces = m_mesh->faces.size();

		log("HF simplify: %d -> %d verts  %d -> %d tris, eps: %0.4f (%0.3f percent)\n",
			src_verts, m_mesh->positions.size(), src_faces, m_mesh->faces.size(), m_epsilon, float(dst_faces) * 100.0f / src_faces
		);
	}

	void hf_simplifier::simplify_recursive(int x1, int y1, int x2, int y2)
	{
		if (x1 >= x2 || y1 >= y2) return;

		auto vertex = [this](int x, int y) -> Vec4
		{
			float h = m_HF[x + y*m_width];
			return Vec4(x*m_scale, h, y*m_scale, m_WF ? m_WF[x + y*m_width] : 0.0f);
		};
		auto vertex_index = [this](int x, int y, const Vec4& value) -> int
		{
			if (value.y == m_hole_value) return -1;
			int i = x + y*m_width;
			if (m_VI[i] == -1) {
				m_VI[i] = m_mesh->positions.size();
				m_mesh->positions.push_back(value);
			}
			return m_VI[i];
		};

		if (x2 - x1 < m_max_width_per_triangle && y2 - y1 < m_max_width_per_triangle)
		{
			//ASSERT(x1 != 0 || y1 != 31);	// [HOWTO] track specific quad
			Vec4 v11 = vertex(x1, y1);
			Vec4 v21 = vertex(x2, y1);
			Vec4 v12 = vertex(x1, y2);
			Vec4 v22 = vertex(x2, y2);

			Vec4 e1_x = v21 - v11;
			Vec4 e1_y = v12 - v11;
			Vec4 e2_x = v21 - v22;
			Vec4 e2_y = v12 - v22;

			Vec4 n11 = normalize(cross(e1_x, e1_y));
			Vec4 n22 = normalize(cross(e2_x, e2_y));

			const float dist11 = -dot(n11, v11);
			const float dist22 = -dot(n22, v22);

			bool flat = true;
			if (x2 - x1 > 1 || y2 - y1 > 1)
			{
				int dx = x2 - x1, dy = y2 - y1;
				for (int y = y1; y <= y2 && flat; ++y) {

					int xdiag = x2 - dx * (y - y1) / dy;

					for (int x = x1; x <= x2; ++x)
					{
						if (m_WF && m_WF[x + y*m_width]) {
							flat = false;
							break;
						}

						Vec4 v = vertex(x, y);

						if (v.y == m_hole_value) {
							flat = false;
							break;
						}

						float d = (x <= xdiag) ? dot(n11, v) + dist11 : dot(n22, v) + dist22;
						if (fabsf(d) > m_epsilon) {
							flat = false;
							break;
						}
					}
				}
			}

			int i11 = vertex_index(x1, y1, v11);
			int i12 = vertex_index(x1, y2, v12);
			int i21 = vertex_index(x2, y1, v21);
			int i22 = vertex_index(x2, y2, v22);

			if (x2 - x1 == 1 && y2 - y1 == 1)
			{
				add_triangle(i11, i12, i21);
				add_triangle(i21, i12, i22);
				return;
			}

			if (flat)
			{
				m_quads.push_back(QUAD(x1, y1, x2 - x1, y2 - y1));
				return;
			}
		}

		const int mx = (x1 + x2) / 2, my = (y1 + y2) / 2;

		simplify_recursive(x1, y1, mx, my);
		simplify_recursive(x1, my, mx, y2);
		simplify_recursive(mx, y1, x2, my);
		simplify_recursive(mx, my, x2, y2);
	}

	void hf_simplifier::add_triangle(int i0, int i1, int i2)
	{
		if( i0 >= 0 && i1 >= 0 && i2 >= 0 )
			m_mesh->faces.push_back(MESH::FACE(i0,i1,i2));
	}

	void hf_simplifier::process_quads()
	{
		auto index_at_position = [this](int x, int y) -> int
		{
			ASSERT(x >= 0 && y >= 0 && x < m_width && y < m_height);
			return m_VI[x + y*m_width];
		};
		auto add_tri_fan = [this,index_at_position](int i_base, int v1, int v2, int axis, bool horizontal, bool include_last) -> int
		{
			int step = (v2 > v1) ? 1 : -1;
			if (include_last) v2 += step;

			int i_prev = horizontal ? index_at_position(v1, axis) : index_at_position(axis, v1);
			ASSERT(i_prev >= 0);
			for (v1 += step; v1 != v2; v1 += step)
			{
				int index = horizontal ? index_at_position(v1, axis) : index_at_position(axis, v1);
				if (index >= 0) {
					if (horizontal)
						add_triangle(i_base, i_prev, index);
					else
						add_triangle(i_base, index, i_prev);
					i_prev = index;
				}
			}
			return i_prev;
		};
		auto add_triangle_fans_tjunctions = [this,index_at_position,add_tri_fan](int x, int y, int xsize, int ysize) -> void	// triangle: (x,y) - (x+xsize,y) - (x,y+ysize)
		{
			ASSERT(xsize*ysize > 0);
			int xc = 0, yc = 0;
			const int step = (xsize > 0) ? 1 : -1;

			for (int i = 0; i != xsize + step; i += step)
				if (index_at_position(x + i, y) != -1) xc++;

			for (int i = 0; i != ysize + step; i += step)
				if (index_at_position(x, y + i) != -1) yc++;

			ASSERT(xc > 1 || yc > 1 || xc < -1 || yc < -1);

			if (xc == 2 && yc == 2)
			{
				add_triangle(index_at_position(x, y), index_at_position(x, y + ysize), index_at_position(x + xsize, y));
			}
			else if (xc >= yc)
			{
				int i_last = add_tri_fan(index_at_position(x, y + ysize), x + xsize, x, y, true, false);
				add_tri_fan(i_last, y + ysize, y, x, false, true);
				int i = 0;
				i;
			}
			else
			{
				int i_last = add_tri_fan(index_at_position(x + xsize, y), y + ysize, y, x, false, false);
				add_tri_fan(i_last, x + xsize, x, y, true, true);
			}
		};

		for (int i = 0; i < m_quads.size(); ++i)
		{
			const QUAD& q = m_quads[i];
			{
				add_triangle_fans_tjunctions(q.x, q.y, q.xsize, q.ysize);
				add_triangle_fans_tjunctions(q.x + q.xsize, q.y + q.ysize, -q.xsize, -q.ysize);
			}
		}
		m_quads.clear();
	}
}

