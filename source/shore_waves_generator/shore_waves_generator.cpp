#include <stdio.h>
#include <vector>
#include <algorithm>

#define RES				2048
#define SEARCH_DISTANCE	20
#define DILATE_PASSES	8



#pragma warning( disable: 4996 )

typedef unsigned char u8;

template< class T1, class T2 > T1 min(T1 a, T2 b) { return (a < b) ? a : b; }
template< class T1, class T2 > T1 max(T1 a, T2 b) { return (a > b) ? a : b; }
template< class T> u8 u8_clamp(T x) { return x<T(0) ? 0 : (x>T(255) ? 255 : u8(x)); }

bool inside(int x, int y) { return x >= 0 && y >= 0 && x < RES && y < RES; }

struct Vec3i
{
	int x, y, z;

	Vec3i() {}
	Vec3i(int _x, int _y, int _z) : x(_x), y(_y), z(_z) {}
};

template< class T > struct Image
{
	T* m_data;

	Image() : m_data(new T[RES * RES]) { clear(); }
	Image(const Image& rhs)
	{
		m_data = new T[RES * RES];
		memcpy(m_data, rhs.m_data, RES * RES * sizeof(T));
	}
	~Image()
	{
		delete[] m_data;
	}
	void operator = (const Image& rhs)
	{
		memcpy(m_data, rhs.m_data, RES * RES * sizeof(T));
	}
	T& at(int x, int y)
	{
		return m_data[x + y * RES];
	}
	T& operator()(int x, int y)
	{
		return at(x, y);
	}
	T& operator [] (int index)
	{
		return m_data[index];
	}
	T* ptr()
	{
		return m_data;
	}
	void clear()
	{
		memset(m_data, 0, RES * RES * sizeof(T));
	}
	template< class T2 > void convert_to(Image<T2>& image, T base)
	{
		for (int i = 0; i < RES * RES; ++i)
			image[i] = T2(m_data[i] * base);
	}
	bool save(const char* filename)
	{
		FILE* fp = fopen(filename, "wb");
		if (!fp) return false;

		long bytes_written = fwrite(m_data, 1, RES * RES * sizeof(T), fp);
		if (bytes_written != RES * RES * sizeof(T))
			return false;
		fclose(fp);
		return true;
	}
	bool load(const char* filename)
	{
		FILE* fp = fopen(filename, "rb");
		if (!fp) return false;

		long bytes_read = fread(m_data, 1, RES * RES * sizeof(T), fp);
		if (bytes_read != RES * RES * sizeof(T))
			return false;
		fclose(fp);
		return true;
	}
	void multiply_add(T mul, T add)
	{
		for (int i = 0; i < RES * RES; ++i)
			m_data[i] = m_data[i] * mul + add;
	}
};

void compute_distance_and_normals(Image<u8>& mask, int radius, Image<u8>& distance, Image<u8>* normal_x, Image<u8>* normal_y)
{
	std::vector<Vec3i> sk;
	for (int y = -radius; y <= radius; ++y)
	{
		for (int x = -radius; x <= radius; ++x)
		{
			int dist = (int)sqrtf(x * x + y * y);
			if (dist <= radius && (x || y))
				sk.push_back(Vec3i(x, y, dist));
		}
	}
	std::sort(sk.begin(), sk.end(), [](const Vec3i & a, const Vec3i & b) -> bool { return a.z < b.z; });

	for (int i = 0; i < RES * RES; ++i)
	{
		distance[i] = mask[i] ? radius : 0;
	}

	const Vec3i* psk = &sk[0];
	for (int y = 0; y < RES; ++y)
	{
		for (int x = 0; x < RES; ++x)
		{
			if (mask(x, y) == 0) continue;

			for (int i = 0, n = sk.size(); i < n; ++i)
			{
				int xc = x + psk[i].x, yc = y + psk[i].y;

				if (!inside(xc, yc)) continue;

				if (mask(xc, yc) > 0) continue;

				distance(x, y) = psk[i].z;

				if (normal_x && normal_y)
				{
					int dx = psk[i].x, dy = psk[i].y, count = 1;
					for (int found_distance = psk[i].z; i < n && psk[i].z <= found_distance+1; ++i)
					{
						int xc = x + psk[i].x, yc = y + psk[i].y;
						if (!inside(xc, yc)) continue;
						if (mask(xc, yc) > 0) continue;

						dx += psk[i].x;
						dy += psk[i].y;
						count++;
					}

					float nx = dx / float(count);
					float ny = dy / float(count);
					float len = sqrtf(nx*nx + ny*ny);
					float mul = 127.0f / len;

					normal_x->at(x, y) = u8_clamp(nx * mul + 128.0f);
					normal_y->at(x,y) = u8_clamp(ny * mul + 128.0f);
				}
				break;
			}
		}
	}
}



int main()
{
	Image<u8> mask;
	if (!mask.load("map.raw"))
	{
		printf("ERROR: failed to load map.raw file!\n");
		return 1;
	}

	Image<u8> norm_x, norm_y;

	Image<float> df;
	for (int i = 0; i < RES * RES; ++i)
		df[i] = mask[i] ? SEARCH_DISTANCE : 0.0f;
	
// compute signed distance field
#if 1
	for (int y = 0; y < RES; ++y)
	{
		for (int x = 0; x < RES; ++x)
		{
			float v = df(x, y);
			if (!v) continue;

			if (x) v = min(v, df(x - 1, y) + 1.0f);
			if( x && y ) v = min(v, df(x - 1, y - 1) + 1.414213f);
			if( y ) v = min(v, df(x, y - 1) + 1.0f);
			if (x + 1 < RES && y) v = min(v, df(x + 1, y - 1) + 1.414213f);

			df(x, y) = v;
			mask(x, y) = u8(v);
		}
	}
	for (int y = RES-1; y >= 0; --y)
	{
		for (int x = RES-1; x >= 0; --x)
		{
			float v = df(x, y);
			if (!v) continue;

			if (x+1<RES) v = min(v, df(x + 1, y) + 1.0f);
			if (x+1<RES && y+1<RES) v = min(v, df(x + 1, y + 1) + 1.414213f);
			if (y+1<RES) v = min(v, df(x, y + 1) + 1.0f);
			if (x && y+1<RES) v = min(v, df(x - 1, y + 1) + 1.414213f);

			df(x, y) = v;
			mask(x, y) = u8(v);
		}
	}
	mask.save("sdf.raw");
#endif

// compute brute-force distance field
#if 1
	compute_distance_and_normals(mask, SEARCH_DISTANCE, mask, &norm_x, &norm_y);
	mask.save("sdf2.raw");
	norm_x.save("norm_x.raw");
	norm_y.save("norm_y.raw");
#endif

// grow & dilate to smooth smaller coast features
#if 1
	Image<u8> grow(mask);
	for (int i = 0; i < RES * RES; ++i)
		grow[i] = (grow[i] < 8) ? 255 : 0;
	grow.save("grow.raw");
	compute_distance_and_normals(grow, 16, grow, nullptr, nullptr);

	for (int i = 0; i < RES * RES; ++i)
		grow[i] = (grow[i] < 12) ? 255 : 0;
	grow.save("shrink.raw");

#endif



// Outline
#if 1
	Image<u8> outline;
	for (int y = 0; y < RES-1; ++y)
	{
		for (int x = 0; x < RES-1; ++x)
		{
			u8 quad[4] = { mask(x, y), mask(x + 1,y), mask(x,y + 1), mask(x + 1,y + 1) };
			u8 vmin = min(min(quad[0], quad[1]), min(quad[2], quad[3]));
			u8 vmax = max(max(quad[0], quad[1]), max(quad[2], quad[3]));

			outline(x, y) = (vmin == 0 && vmax) ? 255 : 0;
		}
	}
	outline.save("outline.raw");
#endif

// Clean-up outline from excessive pixels
#if 1
	for (int pass = 0; pass < 2; ++pass)
	{
		for (int y = 0; y < RES; ++y)
		{
			for (int x = 0; x < RES; ++x)
			{
				int ofs[8][2] = { {-1,-1},{-1,0},{-1,1},{0,1},{1,1},{1,0},{1,-1},{0,-1} };
				u8 mask = 0;
				for (int i = 0; i < 8; ++i, mask <<= 1)
				{
					int nx = x + ofs[i][0], ny = y + ofs[i][1];
					if (!inside(nx,ny)) continue;
					if (outline(nx, ny)) mask |= 1;
				}

				if (mask == 0xFF)
				{
					outline(x, y) = 0;
				}
				else
				{
					while (mask & 128) mask = (mask >> 1) | ((mask & 1) << 7);  // rotate right
					while (mask && (mask & 1) == 0) mask >>= 1;

					bool continuous = true;
					for (; mask; mask >>= 1)
					{
						if ((mask & 1) == 0) continuous = false;
					}
					if (continuous)
					{
						outline(x, y) = 0;
					}
				}
			}
		}
	}
	outline.save("outline.raw");
#endif

// walk the outline
#if 1
	Image<u8> used;
	Image<float> outlinef;

	for (int y = 0; y < RES; ++y)
	{
		for (int x = 0; x < RES; ++x)
		{
			if (outline(x, y) == 0 || used(x, y)) continue;

			outline(x, y) = 0;
			used(x, y) = 1;

			int xc = x, yc = y;

			for (int value = 1;; ++value)
			{
				int ofs[4][2] = { {-1,0},{0,1},{1,0},{0,-1} };
				int xn = -1, yn = -1;

				for (int i = 0; i < 4; ++i)
				{
					int nx = xc + ofs[i][0], ny = yc + ofs[i][1];

					if ( ! inside(nx,ny )) continue;

					if (used(nx, ny)) continue;
					if (outline(nx, ny) == 0) continue;

					outline(nx, ny) = u8(value & 0xFF);
					outlinef(nx, ny) = value;
					used(nx, ny) = 1;

					xn = nx;
					yn = ny;
				}
				if (xn == -1) break;

				xc = xn;
				yc = yn;
			}
		}
	}
	outline.save("uv.raw");
#endif

// blur outline
#if 1
	mask.save("debug_mask.raw");
	used.save("debug_used.raw");
	for (int pass = 0; pass < 1; ++pass)
	{
		Image<u8> dest_outline;
		Image<u8> dest_used;
		Image<float> dest_outlinef;

		for (int y = 0; y < RES; ++y)
		{
			for (int x = 0; x < RES; ++x)
			{
				if (mask(x, y) == 0 || mask(x, y) == SEARCH_DISTANCE || used(x, y)) continue;

				const int ofs[8][2] = { {-1,0},{0,1},{1,0},{0,-1}, {1,1}, {-1,1}, {1,-1}, {-1,-1} };
				const float w[8] = { 1.0f, 1.0f, 1.0f, 1.0f, 0.77f, 0.77f, 0.77f, 0.77f };

				int sum = 0, count = 0;
				float wsum = 0.0f, wweight = 0.0f;

				for (int i = 0; i < 8; ++i)
				{
					int nx = x + ofs[i][0];
					int ny = y + ofs[i][1];

					if (!inside(nx,ny)) continue;

					if (!used(nx, ny)) continue;

					sum += outline(nx, ny);
					count++;

					wsum += outlinef(nx, ny);
					wweight += w[i];
				}
				if (count)
				{
					dest_outline(x, y) = sum / count;
					dest_outlinef(x, y) = wsum / wweight;
					dest_used(x, y) = 1;
				}
			}
		}
		outline = dest_outline;
		outlinef = dest_outlinef;
		used = dest_used;
	}
	outline.save("uv_blurred.raw");

	Image<u8> ofsave;
	outlinef.convert_to(ofsave, 1.0f);
	ofsave.save("uvf_blurred.raw");
#endif

	return 0;
}