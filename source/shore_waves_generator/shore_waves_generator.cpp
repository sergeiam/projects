#include <stdio.h>
#include <vector>
#include <algorithm>

#define RES				2048
#define SEARCH_DISTANCE	20
#define DILATE_PASSES	8



#pragma warning( disable: 4996 )

typedef unsigned char u8;

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

	T& operator()(int x, int y)
	{
		return m_data[x + y * RES];
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
};

template< class T1, class T2 > T1 min(T1 a, T2 b) { return (a < b) ? a : b; }
template< class T1, class T2 > T1 max(T1 a, T2 b) { return (a > b) ? a : b; }

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

	for (int i = 0; i < DILATE_PASSES*2; ++i)
	{
		Image<u8> dest_mask(mask);

		const u8 dilation_mask = (i < DILATE_PASSES) ? 0 : 255;

		for (int y = 0; y < RES; ++y)
		{
			for (int x = 0; x < RES; ++x)
			{
				if (mask(x, y) == dilation_mask) continue;

				const int ofs[8][2] = { {-1,0},{0,-1},{0,1},{1,0},{1,1},{-1,1},{-1,-1},{1,-1} };
				for (int i = 0; i < 4; ++i)
				{
					int nx = x + ofs[i][0], ny = y + ofs[i][1];
					if (!inside(nx, ny)) continue;
					if (mask(nx, ny) == dilation_mask)
					{
						dest_mask(x, y) = dilation_mask;
						break;
					}
				}
			}
		}
		mask = dest_mask;
	}
	mask.save("dilated_mask.raw");


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
	std::vector<Vec3i> sk;
	for (int y = -SEARCH_DISTANCE; y <= SEARCH_DISTANCE; ++y)
	{
		for (int x = -SEARCH_DISTANCE; x <= SEARCH_DISTANCE; ++x)
		{
			int dist = (int)sqrtf(x * x + y * y);
			if (dist <= SEARCH_DISTANCE && (x || y))
				sk.push_back(Vec3i(x, y, dist));
		}
	}
	std::sort(sk.begin(), sk.end(), [](const Vec3i & a, const Vec3i & b) -> bool { return a.z < b.z; });

	Image<float> df2;
	for (int i = 0; i < RES * RES; ++i)
	{
		df2[i] = mask[i] ? SEARCH_DISTANCE : 0.0f;
		mask[i] = mask[i] ? SEARCH_DISTANCE : 0;
	}

	const Vec3i* psk = &sk[0];
	for (int y = 0; y < RES; ++y)
	{
		for (int x = 0; x < RES; ++x)
		{
			if (df2(x, y) < 1.0f) continue;

			for (int i = 0, n = sk.size(); i < n; ++i)
			{
				int xc = x + psk[i].x, yc = y + psk[i].y;

				if (!inside(xc, yc)) continue;

				if (df2(xc, yc) >= 1.0f) continue;

				df2(x, y) = psk[i].z;
				mask(x, y) = psk[i].z;

				float len = sqrtf(psk[i].x * psk[i].x + psk[i].y * psk[i].y);

				norm_x(x, y) = psk[i].x * 127.0f / len + 128;
				norm_y(x, y) = psk[i].y * 127.0f / len + 128;

				break;
			}
		}
	}
	mask.save("sdf2.raw");
	norm_x.save("norm_x.raw");
	norm_y.save("norm_y.raw");
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