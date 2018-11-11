#include <xr/mesh.h>

#include <map>
#include <set>

#define VERBOSE 0

namespace xr
{
#if 0
	void MESH::simplify(float degrees_tollerance)
	{
		if (faces.empty()) return;

		if (VERBOSE > 0)
			log("Mesh info: %d vertices, %d faces\n", positions.size(), faces.size());

		//ctx.check_integrity();

		const float epsilon = cosf(degrees_tollerance * 3.141592f / 180.0f);
		float avg_edge_len_sq = 0.0f;
		Vec4 min_pos(FLT_MAX, FLT_MAX, FLT_MAX), max_pos(-FLT_MAX, -FLT_MAX, -FLT_MAX);

		for (int f = 0; f < faces.size(); ++f)
		{
			const int i0 = faces[f].i0, i1 = faces[f].i1, i2 = faces[f].i2;

			ASSERT(i0 < positions.size() && i1 < positions.size() && i2 < positions.size());

			avg_edge_len_sq += length_sq(positions[i0] - positions[i1]) + length_sq(positions[i1] - positions[i2]) + length_sq(positions[i0] - positions[i2]);
			min_pos = Min(min_pos, positions[i0]);
			max_pos = Max(min_pos, positions[i0]);
		}
		avg_edge_len_sq /= faces.size()*3.0f;

		if (VERBOSE > 0)
			log("Bounding box around mesh: %0.2f x %0.2f x %0.2f\nAverage edge length: %0.2f\n",
				max_pos.x - min_pos.x, max_pos.y - min_pos.y, max_pos.z - min_pos.z, sqrtf(avg_edge_len_sq)
			);

		const float max_edge_len_sq = length_sq(max_pos - min_pos) * 0.5f;
		for (float curr_edge_len_sq = avg_edge_len_sq*4.0f; curr_edge_len_sq < max_edge_len_sq; curr_edge_len_sq *= 2.0f)
		{
			if (VERBOSE > 0) printf("Loop with max edge length: %0.2f\n", sqrtf(curr_edge_len_sq));

			if (vertex_normals.empty())
			{
				compute_vertex_normals();
			}

			for (int v = 0; v < positions.size(); ++v)
			{
				if (positions[v].w == VW_FIXED) continue;

				int best_v = find_best_merge(v, epsilon, curr_edge_len_sq);
				if (best_v == -1) continue;

				if (VERBOSE > 1)
					printf("Collapsing %d -> %d, merging %d faces to %d faces\n", v, best_v, vertex_faces[v].size(), vertex_faces[best_v].size());

				ASSERT(v != best_v && best_v >= 0);

				// add all collapsed vertex faces to the vertex we're collapsing to
				m_vert_faces[best_v].append(m_vert_faces[v].begin(), m_vert_faces[v].end());

				// merge the vertex with another one
				int to_update[3], to_update_count = 1;
				to_update[0] = best_v;

				int collapsed = 0;

				VECTOR<FACE> old_faces;
				for (int j = 0; j < m_vert_faces[v].size(); ++j)
				{
					FACE& face = faces[m_vert_faces[v][j]];
					old_faces.push_back(face);
				}

				for (int j = 0; j < m_vert_faces[v].size(); ++j)
				{
					FACE& face = faces[m_vert_faces[v][j]];
					if (face.i0 == -1) continue;

					ASSERT(face.i0 != -1);
					ASSERT(face.i0 != face.i1 && face.i1 != face.i2 && face.i0 != face.i2);

					// if face contains the vertex we're collapsing to, it will be deleted
					if (face.i0 == best_v || face.i1 == best_v || face.i2 == best_v) {
						for (int k = 0; k < 3; ++k) {
							int index = face.i[k];
							if (index != best_v && index != v) {
								ASSERT(index >= 0);
								if (to_update_count == 3) {
									printf("\nCollapsing %d -> %d\n", v, best_v);
									for (int h = 0; h < old_faces.size(); ++h) {
										Triangle& fc = old_faces[h];
										char buf[128];
										int l = sprintf(buf, "Face %d = ", m_vert_faces[v][h]);
										for (int p = 0; p < 3; ++p)
										{
											char c = fc.i[p] == v ? '!' : (fc.i[p] == best_v ? '>' : ' ');
											l += sprintf(buf + l, "%c%d, ", c, fc.i[p]);
										}
										printf("%s\n", buf);
									}
								}
								ASSERT(to_update_count < 3);
								to_update[to_update_count++] = index;
							}
						}
						face.i0 = -1;
						collapsed++;
					}
					else {
						for (int k = 0; k < 3; ++k) {
							ASSERT(face.i[k] >= 0);
							if (face.i[k] == v) {
								face.i[k] = best_v;
								break;
							}
						}
					}
				}
				m_vert_faces[v].clear();

				ASSERT(to_update_count >= 2);
				ASSERT(collapsed >= 1 && collapsed <= 2);

				// update these m_vertices neighbour face info due to some faces being destroyed

				for (int i = 0; i < to_update_count; ++i)
				{
					int u = to_update[i];
					ASSERT(u >= 0);
					size_t vf_size = m_vert_faces[u].size();
					for (size_t j = 0; j < vf_size; )
					{
						int face_index = m_vert_faces[u][j];
						Triangle& face = m_faces[face_index];
						if (face.i0 == -1) {
							m_vert_faces[u][j] = m_vert_faces[u][vf_size - 1];
							vf_size--;
						}
						else {
							//m_face_normals[face_index] = face_normal(face);
							j++;
						}
					}
					m_vert_faces[u].resize(vf_size);
				}

				for (int i = 0; i < m_vert_faces[best_v].size(); ++i)
				{
					Triangle& face = m_faces[m_vert_faces[best_v][i]];
					ASSERT(face.i0 == best_v || face.i1 == best_v || face.i2 == best_v);
				}
			}
			// compact m_indices removing collapsed faces

			compact_indices();

			check_integrity();

			if (VERBOSE > 0) printf("\tpass completed\n");

			//break;
		}
	}

	int find_best_merge(MESH& m, int v, float epsilon, float max_edge_len_sq)
	{
		if (m.vertex_faces[v].size() >= 8) return -1;

		Vec4 vertex = m_vertices[v];
		const int vf_count = m_vert_faces[v].size();
		float vw = m_vertices[v].w;

		Vec4 N;
		Vec4 VN = m_vert_normals[v];

		auto can_merge_with = [&](int v0, int v1) -> bool
		{
			float w0 = m_vertices[v0].w, w1 = m_vertices[v1].w;
			return w0 != VERTEX_FIXED && w0 == w1;
		};

		int vn[8], vn_count = 0;

		// gather neighbour vertices
		for (int f = 0; f < vf_count; ++f)
		{
			Triangle& face = m_faces[m_vert_faces[v][f]];
			ASSERT(face.i0 >= 0);

			float eval = 1.0f;

			if (!f) {
				N = m_face_normals[m_vert_faces[v][0]];
			}
			else {
				const Vec4& fN = m_face_normals[m_vert_faces[v][f]];
				eval = dot(N, fN);
				if (eval < epsilon)
					return -1;
			}

			for (int fi = 0; fi < 3; ++fi)
			{
				int vertex = face.i[fi];
				ASSERT(vertex >= 0);
				if (vertex == v) continue;

				int j;
				for (j = 0; j < vn_count; ++j)
					if (vn[j] == vertex) break;
				ASSERT(j < 8);
				if (j == vn_count) {
					vn[j] = vertex;
					vn_count++;
				}
			}
		}

		auto test_new_face_shape = [&](int e0, int e1, int oldv, int newv) -> bool
		{
			Vec4 edge = m_vertices[e1] - m_vertices[e0];
			Vec4 to_old = m_vertices[oldv] - m_vertices[e0];
			Vec4 to_new = m_vertices[newv] - m_vertices[e0];
			Vec4 c0 = cross(edge, to_old);
			Vec4 c1 = cross(edge, to_new);
			if (dot(normalize(c0), normalize(c1)) < 0.0001f)
				return false;

			float l0 = length(c0), l1 = length(c1);
			return l0*0.1f < l1;
		};

		float best = -1.0f; // FLT_MAX;
		int best_v = -1;

		// try each neighbour vertex
		for (int i = 0; i < vn_count; ++i)
		{
			int next_v = vn[i];
			if (!can_merge_with(v, next_v))
				continue;

			if (dot(VN, m_vert_normals[next_v]) < epsilon)
				return -1;

			// find the length of the maximal edge that this collapse will produce
			float max_dist_sq = 0.0f;
			for (int j = 0; j < vn_count; ++j)
			{
				if (j != i)
					max_dist_sq = Max(max_dist_sq, length_sq(m_vertices[next_v] - m_vertices[vn[j]]));
			}

			if (max_dist_sq > max_edge_len_sq)
				continue;

			// try all non-collapsing faces for keeping their orientation against that collapse
			int f = 0;
			for (; f < vf_count; ++f)
			{
				Triangle& face = m_faces[m_vert_faces[v][f]];
				if (face.i0 == next_v || face.i1 == next_v || face.i2 == next_v) continue; // collapsed face

				int a, b;
				if (face.i0 == v) {
					a = face.i1, b = face.i2;
				}
				else if (face.i1 == v) {
					a = face.i0, b = face.i2;
				}
				else {
					a = face.i0, b = face.i1;
				}

				if (!test_new_face_shape(a, b, v, next_v)) break;
			}

			if (f < vf_count)
				continue;

			Vec4 d = m_vertices[next_v] - m_vertices[v];
			float dist_sq = dot(d, d);
			if (dist_sq > best) {
				best = dist_sq;
				best_v = next_v;
			}
		}

		return best_v;
	}

	struct edge
	{
		int a, b;

		edge() {}
		edge(int _a, int _b)
		{
			if (_a < _b)
				a = _a, b = _b;
			else
				a = _b, b = _a;
		}
		bool operator()(const edge& lhs, const edge& rhs) const
		{
			if (lhs.a != rhs.a)
				return lhs.a < rhs.a;
			return lhs.b < rhs.b;
		}
		bool operator == (const edge& rhs) const
		{
			return a == rhs.a && b == rhs.b;
		}
	};

	void CONTEXT::check_integrity()
	{
		std::map<edge, int, edge> edges;

		printf("Checking mesh integrity...\n");

		edge broken(4, 515);

		const int num_vertices = m_vertices.size();

		for (int i = 0; i < m_num_faces; ++i)
		{
			Triangle& face = m_faces[i];
			if (face.i0 == -1) continue;

			if (face.i0 >= num_vertices || face.i1 >= num_vertices || face.i2 >= num_vertices)
			{
				printf("Face %d has an index out of range: %d, %d, %d\n", i, face.i0, face.i1, face.i2);
				ASSERT(0);
			}

			edges[edge(face.i0, face.i1)]++;
			edges[edge(face.i1, face.i2)]++;
			edges[edge(face.i2, face.i0)]++;
		}

		for (auto it = edges.begin(); it != edges.end(); ++it)
		{
			if (it->second > 2) {
				printf("ERROR: broken edge %d, %d = %d such edges found\n", it->first.a, it->first.b, it->second);
				ASSERT(0);
			}
		}
	}
#endif
}