#include <stdio.h>
#include <vector>
#include <algorithm>

#define RES 2048
#define SEARCH_DISTANCE 20

#pragma warning( disable: 4996 )

typedef unsigned char u8;

template< class T > T& texel(T* ptr, int x, int y)
{
	return ptr[x + y * RES];
}

struct Vec3i
{
	int x, y, z;

	Vec3i() {}
	Vec3i(int _x, int _y, int _z) : x(_x), y(_y), z(_z) {}
};

template< class T1, class T2 > T1 min(T1 a, T2 b) { return (a < b) ? a : b; }
template< class T1, class T2 > T1 max(T1 a, T2 b) { return (a > b) ? a : b; }

int main()
{
	u8* mask = new u8[RES * RES];
	FILE* fp = fopen("greece.raw", "rb");
	if (!fp) return 1;
	fread(mask, 1, RES * RES, fp);
	fclose(fp);

	float* df = new float[RES * RES];
	for (int i = 0; i < RES * RES; ++i)
		df[i] = mask[i] ? SEARCH_DISTANCE : 0.0f;

// compute signed distance field
#if 1
	for (int y = 0; y < 2048; ++y)
	{
		for (int x = 0; x < 2048; ++x)
		{
			float v = texel(df, x, y);
			if (!v) continue;

			if (x) v = min(v, texel(df, x - 1, y) + 1.0f);
			if( x && y ) v = min(v, texel(df, x - 1, y - 1) + 1.414213f);
			if( y ) v = min(v, texel(df, x, y - 1) + 1.0f);
			if (x + 1 < RES && y) v = min(v, texel(df, x + 1, y - 1) + 1.414213f);

			texel(df, x, y) = v;
			texel(mask, x, y) = u8(v);
		}
	}
	for (int y = 2047; y >= 0; --y)
	{
		for (int x = 2047; x >= 0; --x)
		{
			float v = texel(df, x, y);
			if (!v) continue;

			if (x+1<RES) v = min(v, texel(df, x + 1, y) + 1.0f);
			if (x+1<RES && y+1<RES) v = min(v, texel(df, x + 1, y + 1) + 1.414213f);
			if (y+1<RES) v = min(v, texel(df, x, y + 1) + 1.0f);
			if (x && y+1<RES) v = min(v, texel(df, x - 1, y + 1) + 1.414213f);

			texel(df, x, y) = v;
			texel(mask, x, y) = u8(v);
		}
	}

	fp = fopen("sdf.raw", "wb");
	fwrite(mask, 1, RES * RES, fp);
	fclose(fp);
#endif

// compute brute-force distance field
#if 0
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

	float* df2 = new float[RES * RES];
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
			if (texel(df2, x, y) < 1.0f) continue;

			for (int i = 0, n = sk.size(); i < n; ++i)
			{
				int xc = x + psk[i].x;
				int yc = y + psk[i].y;
				if (xc < 0 || yc < 0 || xc >= RES || yc >= RES) continue;
				if (texel(df2, xc, yc) >= 1.0f) continue;

				texel(df2, x, y) = psk[i].z;
				texel(mask, x, y) = psk[i].z;
				break;
			}
		}
	}
	fp = fopen("sdf2.raw", "wb");
	fwrite(mask, 1, RES*RES, fp);
	fclose(fp);
#endif

// Outline
#if 1
	u8* outline = new u8[RES*RES];
	for (int y = 0; y < RES-1; ++y)
	{
		for (int x = 0; x < RES-1; ++x)
		{
			u8 quad[4] = { texel(mask, x, y), texel(mask, x + 1,y), texel(mask, x,y + 1), texel(mask,x + 1,y + 1) };
			u8 vmin = min(min(quad[0], quad[1]), min(quad[2], quad[3]));
			u8 vmax = max(max(quad[0], quad[1]), max(quad[2], quad[3]));

			texel(outline, x, y) = (vmin == 0 && vmax) ? 255 : 0;
		}
	}
	fp = fopen("outline.raw", "wb");
	fwrite(outline, 1, RES*RES, fp);
	fclose(fp);
#endif

// Clean-up outline from excessive pixels
#if 1
	for (int y = 0; y < RES; ++y)
	{
		for (int x = 0; x < RES; ++x)
		{
			int ofs[8][2] = {{-1,-1},{-1,0},{-1,1},{0,1},{1,1},{1,0},{1,-1},{0,-1}};
			u8 mask = 0;
			for (int i = 0; i < 8; ++i, mask <<= 1)
			{
				int nx = x + ofs[i][0], ny = y + ofs[i][1];
				if (nx < 0 || ny < 0 || nx == RES || ny == RES) continue;
				if (texel(outline, nx, ny)) mask |= 1;
			}

			if (mask == 0xFF)
			{
				texel(outline, x, y) = 0;
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
					texel(outline, x, y) = 0;
				}
			}
		}
	}
	fp = fopen("outline.raw", "wb");
	fwrite(outline, 1, RES* RES, fp);
	fclose(fp);
#endif

// walk the outline
#if 1
	u8* used = new u8[RES * RES];
	memset(used, 0, RES* RES);

	for (int y = 0; y < RES; ++y)
	{
		for (int x = 0; x < RES; ++x)
		{
			if (texel(outline, x, y) == 0 || texel(used, x, y)) continue;

			texel(outline, x, y) = 0;
			texel(used, x, y) = 1;

			int xc = x, yc = y;

			for (u8 value = 1;; ++value)
			{
				int ofs[4][2] = { {-1,0},{0,1},{1,0},{0,-1} };
				int xn = -1, yn = -1;

				for (int i = 0; i < 4; ++i)
				{
					int nx = xc + ofs[i][0], ny = yc + ofs[i][1];
					if (nx < 0 || ny < 0 || nx == RES || ny == RES) continue;
					if (texel(used, nx, ny)) continue;
					if (texel(outline, nx, ny) == 0) continue;

					texel(outline, nx, ny) = value;
					texel(used, nx, ny) = 1;

					xn = nx;
					yn = ny;
				}
				if (xn == -1) break;

				xc = xn;
				yc = yn;
			}
		}
	}
	fp = fopen("uv.raw", "wb");
	fwrite(outline, 1, RES* RES, fp);
	fclose(fp);
#endif

// blur outline
#if 1
	for (int pass = 0; pass < 10; ++pass)
	{
		u8* dest_outline = new u8[RES * RES];
		memcpy(dest_outline, outline, RES* RES);
		u8* dest_used = new u8[RES * RES];
		memcpy(dest_used, used, RES* RES);

		for (int y = 0; y < RES; ++y)
		{
			for (int x = 0; x < RES; ++x)
			{
				if (texel(mask, x, y) == 0 || texel(mask, x, y) == SEARCH_DISTANCE || texel(used, x, y)) continue;

				const int ofs[8][2] = { {-1,0},{0,1},{1,0},{0,-1}, {1,1}, {-1,1}, {1,-1}, {-1,-1} };

				int sum = 0, count = 0;

				for (int i = 0; i < 8; ++i)
				{
					int nx = x + ofs[i][0], ny = y + ofs[i][1];
					if (nx < 0 || ny < 0 || nx == RES || ny == RES) continue;
					if (!texel(used, nx, ny)) continue;

					sum += texel(outline, nx, ny);
					count++;
				}
				if (count)
				{
					texel(dest_outline, x, y) = sum / count;
					texel(dest_used, x, y) = 1;
				}
			}
		}
		delete[] outline;
		outline = dest_outline;
		delete[] used;
		used = dest_used;
	}
	fp = fopen("uv_blurred.raw", "wb");
	fwrite(outline, 1, RES* RES, fp);
	fclose(fp);
#endif

	return 0;
}