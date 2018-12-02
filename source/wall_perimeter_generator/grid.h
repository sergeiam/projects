#pragma once

#include <xr/core.h>
#include <xr/math.h>
#include <memory.h>
#include <math.h>

class GRID
{
public:
	GRID(int w, int h)
	{
		m_width = w, m_height = h;
		m_grid = new int[w*h];
		fill(0);
		set_world_to_grid_transform(0, 0, w, h);
	}
	GRID(const GRID& rhs)
	{
		m_width = rhs.m_width, m_height = rhs.m_height;
		m_ws_x = rhs.m_ws_x, m_ws_y = rhs.m_ws_y, m_ws_width = rhs.m_ws_width, m_ws_height = rhs.m_ws_height;
		m_grid = new int[m_width * m_height];
		memcpy(m_grid, rhs.m_grid, m_width * m_height * 4);
	}
	GRID(GRID&& rhs)
	{
		m_width = rhs.m_width;
		m_height = rhs.m_height;
		m_grid = rhs.m_grid;
		m_ws_x = rhs.m_ws_x, m_ws_y = rhs.m_ws_y, m_ws_width = rhs.m_ws_width, m_ws_height = rhs.m_ws_height;
		rhs.m_grid = nullptr;
	}
	~GRID()
	{
		delete[] m_grid;
	}

	int width() const { return m_width; }
	int height() const { return m_height; }

	void set_world_to_grid_transform(float x, float y, float w, float h)
	{
		m_ws_x = x;
		m_ws_y = y;
		m_ws_width = w;
		m_ws_height = h;
	}

	void reset(int w, int h)
	{
		if (width() != w || height() != h) {
			delete[] m_grid;
			m_width = w, m_height = h;
			m_grid = new int[m_width*m_height];
		}
	}

	void fill(int x)
	{
		for (int i = 0; i < m_width*m_height; m_grid[i++] = x);
	}

	int& operator()(int x, int y)
	{
		return m_grid[x + y*m_width];
	}
	int operator()(int x, int y) const
	{
		return m_grid[x + y*m_width];
	}
	int& operator[](int index)
	{
		return m_grid[index];
	}

	void operator = (const GRID& rhs)
	{
		reset(rhs.width(), rhs.height());
		memcpy(m_grid, rhs.m_grid, m_width*m_height * sizeof(m_grid[0]));
		m_ws_x = rhs.m_ws_x, m_ws_y = rhs.m_ws_y, m_ws_width = rhs.m_ws_width, m_ws_height = rhs.m_ws_height;
	}

	void add(const GRID& rhs)
	{
		if( m_width == rhs.width() && m_height == rhs.height())
			for (int i = 0; i < m_width*m_height; ++i)
				m_grid[i] += rhs.m_grid[i];
	}

	void fill_oriented_rectangle(float x, float y, float w, float h, float angle, int value)
	{
		Vec2 sincos(sinf(deg_to_rad(angle)), cosf(deg_to_rad(angle)));

		x = (x - m_ws_x) * m_width / m_ws_width;
		y = (y - m_ws_y) * m_height / m_ws_height;
		w = w * m_width / m_ws_width;
		h = h * m_height / m_ws_height;

		Vec2 ax = Vec2( sincos.y, sincos.x ) * w * 0.5f;
		Vec2 bx = Vec2( -sincos.x, sincos.y ) * h * 0.5f;

		Vec2 p[4] = {
			Vec2(x,y) - ax - bx,
			Vec2(x,y) - ax + bx,
			Vec2(x,y) + ax + bx,
			Vec2(x,y) + ax - bx
		};

		int* l = new int[m_height];
		int* r = new int[m_height];

		int H = m_height;

		auto rasterize_edge = [H]( Vec2 p1, Vec2 p2, int* ptr, bool left_align) -> void
		{
			//if (p1.y > p2.y) xr::Swap(p1, p2);

			if (p1.y > H || p2.y < 0.5f) return;

			int y = int(floorf(p1.y));
			int y2 = int(floorf(p2.y));

			if (y == y2) return;

			const float dxy = (p2.x - p1.x) / (p2.y - p1.y);

			if( left_align && dxy<0.0f || !left_align && dxy>0.0f )  // go forward the edge or backward to reach the properly conservative x-coordinate for that pixel row
				p1.x += (ceilf(p1.y) - p1.y) * dxy;

			for (; y <= y2; p1.x += dxy )
			{
				int x = floorf(p1.x);
				ptr[y++] = x;
			}
		};

		int r90 = int(floorf(angle / 90.0f));
		int top = r90 & 3;
		int bottom = (r90 + 2) & 3;
		int left = (r90 + 1) & 3;
		int right = (r90 + 3) & 3;

		rasterize_edge(p[top], p[left], l, true);
		rasterize_edge(p[left], p[bottom], l, true);
		rasterize_edge(p[top], p[right], r, false);
		rasterize_edge(p[right], p[bottom], r, false);

		const int min_y = xr::Max(int(p[top].y), 0), max_y = xr::Min(int(p[bottom].y), m_height - 1);
		for (int y = min_y; y <= max_y; ++y)
		{
			int* grid = m_grid + y*m_width;
			const int min_x = xr::Max(int(l[y]), 0), max_x = xr::Min(int(r[y]), m_width - 1);
			for (int x = min_x; x <= max_x; ++x)
				grid[x] = value;
		}
		delete[] l;
		delete[] r;
	}

	void distance_to_image(GRID& result)
	{
		result.reset( m_width, m_height );
		for (int i = 0; i < m_width*m_height; ++i)
			result[i] = m_grid[i] ? 0 : 100000;

#define DOWN  +m_width
#define TOP   -m_width
#define LEFT  -1
#define RIGHT +1

		int* ptr = result.m_grid + 1;

		for (int x = 1; x < m_width; ++x)
			*ptr++ = xr::Min(*ptr, ptr[LEFT] + 5);

		for (int y = 1; y < m_height; ++y)
		{
			*ptr++ = xr::Min(*ptr, xr::Min(ptr[TOP] + 5, ptr[TOP + RIGHT] + 7));
			for (int x = 1; x < m_width - 1; ++x, ++ptr) {
				int n = xr::Min(xr::Min(ptr[TOP + LEFT] + 7, ptr[TOP] + 5), xr::Min(ptr[TOP + RIGHT] + 7, ptr[LEFT] + 5));
				*ptr = xr::Min(*ptr, n);
			}
			*ptr++ = xr::Min(xr::Min(*ptr, ptr[TOP] + 5), xr::Min(ptr[LEFT] + 5, ptr[TOP + LEFT] + 7));
		}

		// second pass, bottom -> up, right -> left

		ptr = result.m_grid + m_width * m_height - 2;
		for (int x = m_width - 2; x >= 0; --x)
			*ptr-- = xr::Min(*ptr, ptr[RIGHT] + 5);

		for (int y = m_height - 2; y >= 0; --y)
		{
			*ptr-- = xr::Min(*ptr, xr::Min(ptr[DOWN + LEFT] + 7, ptr[DOWN] + 5));
			for (int x = m_width - 2; x >= 1; --x, --ptr) {
				int n = xr::Min(xr::Min(ptr[DOWN + LEFT] + 7, ptr[DOWN] + 5), xr::Min(ptr[DOWN + RIGHT] + 7, ptr[RIGHT] + 5));
				*ptr = xr::Min(*ptr, n);
			}
			*ptr-- = xr::Min(xr::Min(*ptr, ptr[DOWN] + 5), xr::Min(ptr[DOWN + RIGHT] + 7, ptr[RIGHT] + 5));
		}

#undef DOWN
#undef TOP
#undef LEFT
#undef RIGHT

		//for (int i = 0; i < m_width*m_height; ++i)
		//	result[i] /= 5;
	}

	void dilate_by_rectangle(int w, int h)
	{

	}

private:
	int		m_width, m_height;
	int*	m_grid;
	float	m_ws_x, m_ws_y, m_ws_width, m_ws_height;
};