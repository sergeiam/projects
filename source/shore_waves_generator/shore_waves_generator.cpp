#include <stdio.h>
#include <vector>
#include <algorithm>

#include <xr/job_manager.h>
#include <xr/time.h>

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

template< class T > class Image
{
	T*	m_data;
	int m_res;

public:

	Image(int res) : m_data(new T[res * res]), m_res(res) { clear(); }
	Image(const Image& rhs)
	{
		m_res = rhs.m_res;
		m_data = new T[num_texels()];
		memcpy(m_data, rhs.m_data, size());
	}
	~Image()
	{
		delete[] m_data;
	}
	void operator = (const Image& rhs)
	{
		memcpy(m_data, rhs.m_data, size());
	}
	size_t resolution() const { return m_res; }
	size_t size() const { return m_res * m_res * sizeof(T); }
	size_t num_texels() const { return m_res * m_res; }

	T& at(int x, int y)
	{
		return m_data[x + y * m_res];
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
		memset(m_data, 0, size());
	}
	template< class T2 > void convert_to(Image<T2>& image, T base)
	{
		if (m_res == image.m_res)
		{
			for (size_t i = 0, n = num_texels(); i < n; ++i)
				image[i] = T2(m_data[i] * base);
		}
	}
	bool save(const char* filename)
	{
		FILE* fp = fopen(filename, "wb");
		if (!fp) return false;

		long bytes_written = fwrite(m_data, 1, size(), fp);
		if (bytes_written != size())
			return false;
		fclose(fp);
		return true;
	}
	bool load(const char* filename)
	{
		FILE* fp = fopen(filename, "rb");
		if (!fp) return false;

		long bytes_read = fread(m_data, 1, size(), fp);
		if (bytes_read != size())
			return false;
		fclose(fp);
		return true;
	}
};

#define THREADS

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

	for (int i = 0, n = mask.num_texels(); i < n; ++i)
	{
		distance[i] = mask[i] ? radius : 0;
	}

	xr::TIME_SCOPE ts;

	const size_t res = mask.resolution();

	const Vec3i* psk = &sk[0];
#ifndef THREADS
	for (size_t y = 0; y < res; ++y)
	{
#else
	for (size_t yt = 0; yt < res; yt += 32)
	{
		xr::jobs_add([res, yt, &mask, psk, &sk, &distance, normal_x, normal_y]() -> void
			{
				for (size_t y = yt; y < yt + 32; ++y) {
#endif
					for (size_t x = 0; x < res; ++x)
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
								for (int found_distance = psk[i].z; i < n && psk[i].z <= found_distance + 1; ++i)
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
								float len = sqrtf(nx * nx + ny * ny);
								float mul = 127.0f / len;

								normal_x->at(x, y) = u8_clamp(nx * mul + 128.0f);
								normal_y->at(x, y) = u8_clamp(ny * mul + 128.0f);
							}
							break;
						}
					}

#ifdef THREADS
				}}, 0, 0);
#endif
	}

#ifdef THREADS
	xr::jobs_wait_all();
#endif
	printf("%d ms\n", ts.measure_duration_ms());
}

void smooth_coast(Image<u8>& mask, int radius)
{
	compute_distance_and_normals(mask, radius, mask, nullptr, nullptr);

	for (size_t i = 0, n = mask.num_texels(); i < n; ++i)
		mask[i] = (mask[i] < radius) ? 255 : 0;

	compute_distance_and_normals(mask, radius, mask, nullptr, nullptr);

	for (int i = 0, n = mask.num_texels(); i < n; ++i)
		mask[i] = (mask[i] < radius) ? 255 : 0;
}

int main()
{
	xr::jobs_init(4);

	Image<u8> mask(2048), norm_x(2048), norm_y(2048);
	if (!mask.load("map.raw"))
	{
		printf("ERROR: failed to load map.raw file!\n");
		return 1;
	}

	smooth_coast(mask, 32);

	mask.save("map_smooth.raw");

// compute brute-force distance field
#if 1
	compute_distance_and_normals(mask, SEARCH_DISTANCE, mask, &norm_x, &norm_y);
	mask.save("sdf2.raw");
	norm_x.save("norm_x.raw");
	norm_y.save("norm_y.raw");
#endif



	return 0;
}