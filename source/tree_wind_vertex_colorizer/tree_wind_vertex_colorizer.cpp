#include <stdio.h>
#include <memory.h>
#include <windows.h>

#include <xr/mesh.h>

#include <unordered_map>
#include <vector>

#define EPSILON 0.01f

typedef Vec3 float3;
typedef Vec4 float4;

struct MESH_ADAPTER
{
	virtual int		num_vertices() = 0;
	virtual int		num_faces() = 0;

	virtual Vec3& pos(int i) = 0;
	virtual Vec3& norm(int i) = 0;
	virtual Vec4& color(int i) = 0;

	virtual int& face(int i, int coord) = 0;

	virtual ~MESH_ADAPTER() {}
};

struct XR_MESH_ADAPTER : public MESH_ADAPTER
{
	XR_MESH_ADAPTER(xr::MESH& m) : m_mesh(m) {}

	virtual int		num_vertices() { return m_mesh.m_positions.size(); }
	virtual int		num_faces() { return m_mesh.m_faces.size(); }

	virtual float3& pos(int i) { return m_mesh.m_positions[i]; }
	virtual float3& norm(int i) { return m_mesh.m_normals[i]; }
	virtual Vec4& color(int i) { return m_mesh.m_colors[i]; }

	virtual int& face(int i, int coord) { return m_mesh.m_faces[i].i[coord]; }

	xr::MESH& m_mesh;
};


size_t float_hash(float x)
{
	return *(unsigned long*)&x;
}

struct float3_hash
{
	size_t operator()(const float3& rhs) const
	{
		return 1337 * float_hash(rhs.x) + 13 * float_hash(rhs.y) + 3 * float_hash(rhs.z);
	}
};

struct float3_equal
{
	bool operator()(const float3& lhs, const float3& rhs) const
	{
		return fabsf(lhs.x - rhs.x) + fabsf(lhs.y - rhs.y) + fabsf(lhs.z - rhs.z) < EPSILON;
	}
};

void compute_mesh_adjacency(MESH_ADAPTER& ma)
{
	std::unordered_map<float3, int, float3_hash, float3_equal> pos_map;
	std::vector<int>	uvi(ma.num_vertices());	// unique vertex indices

	int pos_min_y = 0;

	const int numv = ma.num_vertices();

	for (int i = 0; i < numv; ++i)
	{
		const float3& pos = ma.pos(i);
		auto it = pos_map.find(pos);
		if (it == pos_map.end())
		{
			pos_map.insert(std::make_pair(pos, i));
			uvi[i] = i;
		}
		else
		{
			uvi[i] = it->second;
		}
		if (pos.y < ma.pos(pos_min_y).y)
			pos_min_y = i;
	}
	pos_min_y = uvi[pos_min_y];

	std::vector<std::vector<int>>	adj;

	for (int i = 0; i < ma.num_faces(); ++i)
	{
		int v[3] = { ma.face(i,0), ma.face(i,1), ma.face(i,2) };

		for (int j = 0; j < 3; ++j)
		{
			v[j] = uvi[v[j]];
		}

		adj[v[0]].push_back(v[1]);
		adj[v[0]].push_back(v[2]);
		adj[v[1]].push_back(v[0]);
		adj[v[1]].push_back(v[2]);
		adj[v[2]].push_back(v[0]);
		adj[v[2]].push_back(v[1]);
	}

	std::vector<bool> visit(numv);
	std::fill(visit.begin(), visit.end(), false);

	std::vector<int> wave;
	wave.push_back(pos_min_y);

	while (!wave.empty())
	{
		int v = wave.back();
		wave.pop_back();

		for (int i = 0; i < adj[v].size(); ++i)
		{
			int nv = adj[v][i];
			if (!visit[nv])
			{
				wave.push_back(nv);
				visit[nv] = true;
			}
		}
	}

	for (int i = 0; i < numv; ++i)
	{
		float fv = visit[i] ? 1.0f : 0.0f;
		Vec4& color = ma.color(i);
		color.x = color.y = color.z = fv;
	}
}

void colorize_elongated_faces(xr::MESH& mesh)
{
	mesh.m_colors.resize(mesh.m_positions.size());

	for (auto it = mesh.begin_vertex(); it != mesh.end_vertex(); ++it)
	{
		it.color() = Vec4(0, 0, 0, 0);
	}

	for (auto face = mesh.begin_face(); face != mesh.end_face(); ++face)
	{
		const Vec3& v0 = face.position(0);
		const Vec3& v1 = face.position(1);
		const Vec3& v2 = face.position(2);

		float l0 = length(v0 - v1);
		float l1 = length(v1 - v2);
		float l2 = length(v2 - v0);

		float min_len = min(l0, min(l1, l2));
		float max_len = max(l0, max(l1, l2));

		float size = max_len / min_len / 5.0f;

		face.color(0).x = max(face.color(0).x, size);
		face.color(1).x = max(face.color(1).x, size);
		face.color(2).x = max(face.color(2).x, size);
	}

	for (int i = 0; i < mesh.m_faces.size(); ++i)
	{
		int i0 = mesh.m_faces[i].i0;
		int i1 = mesh.m_faces[i].i1;
		int i2 = mesh.m_faces[i].i2;

		const Vec3& v0 = mesh.m_positions[i0];
		const Vec3& v1 = mesh.m_positions[i1];
		const Vec3& v2 = mesh.m_positions[i2];

		float l0 = length(v0 - v1);
		float l1 = length(v1 - v2);
		float l2 = length(v2 - v0);

		float min_len = min(l0, min(l1, l2));
		float max_len = max(l0, max(l1, l2));

		float size = max_len / min_len / 5.0f;

		mesh.m_colors[i0].x = max(mesh.m_colors[i0].x, size);
		mesh.m_colors[i1].x = max(mesh.m_colors[i1].x, size);
		mesh.m_colors[i2].x = max(mesh.m_colors[i2].x, size);
	}
}

void find_tree_trunk(xr::MESH& mesh, xr::MESH& trunk, xr::MESH& leaves)
{
	trunk.clear();
	leaves.clear();


}

int main()
{
	xr::MESH mesh;

	mesh.read_obj("MapleTree.obj");

// separate trunk from the leaves

	xr::MESH trunk, leaves;

	trunk.m_positions = mesh.m_positions;
	leaves.m_positions = mesh.m_positions;
	for (auto it = mesh.begin_face(); it != mesh.end_face(); ++it)
	{
		const Vec3& v0 = it.position(0);
		const Vec3& v1 = it.position(1);
		const Vec3& v2 = it.position(2);

		float l0 = length(v0 - v1);
		float l1 = length(v1 - v2);
		float l2 = length(v2 - v0);

		float min_len = min(l0, min(l1, l2));
		float max_len = max(l0, max(l1, l2));

		float size = max_len / min_len / 5.0f;

		if (size > 2.0f)
			trunk.m_faces.push_back(*it);
		else
			leaves.m_faces.push_back(*it);
	}

// find tree base

	float min_y = FLT_MAX;
	float epsilon = 0.1f;
	Box3 base;

	for (auto it = trunk.begin_face(); it != trunk.end_face(); ++it)
	{
		for (int i = 0; i < 3; ++i)
		{
			const Vec3& pos = it.position(i);
			if (pos.y < min_y - epsilon)
			{
				min_y = pos.y;
				base = Box3(pos);
			}
			else if (pos.y < min_y + epsilon)
			{
				base += pos;
			}
		}
	}







	mesh.write_ply("MapleTree.ply");

	return 0;
}