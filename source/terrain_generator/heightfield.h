#pragma once

#include <stdlib.h>
#include <math.h>

struct HEIGHTFIELD
{
	float*	m_hf;
	int		m_res;

	HEIGHTFIELD(int resolution)
	{
		m_res = resolution;
		m_hf = new float[resolution * resolution];
		for (int i = 0; i < m_res*m_res; m_hf[i++] = 0.0f);
	}
	~HEIGHTFIELD() { delete[] m_hf; }

	float operator[](int index) const { return m_hf[index]; }
	float operator()(int x, int y) const { return m_hf[x + y*m_res]; }

	float at(int x, int y) const { return m_hf[x + y*m_res]; }
	float& at(int x, int y) { return m_hf[x + y*m_res]; }

	template< class T > void combine(const HEIGHTFIELD& a, T func)
	{
		for (int i = 0, n = m_res*m_res; i < n; ++i)
			m_hf[i] = func(m_hf[i], a[i]);
	}

	template< class T > void apply(T func)
	{
		for (int y = 0, index = 0; y < m_res; ++y)
			for (int x = 0; x < m_res; ++x, ++index)
				func(x, y, index, m_hf[index]);
	}

	void scale(float k)
	{
		for (int i = 0; i < m_res*m_res; m_hf[i++] *= k);
	}

	template< class T > void apply_circular(float cx, float cy, T func)
	{
		apply([this,cx, cy, func](int x, int y, int index, float& val) -> void {
			float dx = float(x)/m_res - cx, dy = float(y)/m_res - cy;
			float dist = sqrtf(dx*dx + dy*dy);
			func(dist, val);
		});
	}

	template< class T > void sine_wave(float period, float offset, T func)
	{
		float* sn = new float[m_res];
		for (int i = 0; i < m_res; ++i)
			sn[i] = sinf(float(i) / m_res * period + offset);

		for (int y = 0, index = 0; y < m_res; ++y)
			for (int x = 0; x < m_res; ++x, ++index)
				m_hf[index] = func(sn[x], sn[y]);
	}

	void bilinear_noise(int noise_res)
	{
		HEIGHTFIELD noise(noise_res+1);
		noise.apply([](int x, int y, int index, float& val) -> void
		{
			val = float(rand()) / RAND_MAX;
		});

		apply([&noise, this, noise_res](int x, int y, int index, float& val) -> void
		{
			int side = m_res / noise_res;
			int nx = x / side, ny = y / side;
			float xw = (x % side) / float(side), yw = (y % side) / float(side);

			float x1 = noise(nx, ny) * (1.0f - xw) + noise(nx + 1, ny) * xw;
			float x2 = noise(nx, ny + 1) * (1.0f - xw) + noise(nx + 1, ny + 1) * xw;

			val = x1*(1.0f - yw) + x2*yw;
		});
	}

	template< class T >
	void facing_circle(float cx, float cy, float r, float dirx, float diry, T func)
	{
		int x1 = xr::Max(int((cx - r)*m_res), 0);
		int y1 = xr::Max(int((cy - r)*m_res), 0);
		int x2 = xr::Min(int((cx + r)*m_res), m_res);
		int y2 = xr::Min(int((cy + r)*m_res), m_res);

		for( int y=0; y<m_res; ++y )
			for (int x = 0; x < m_res; ++x)
			{
				float d = (x - cx*m_res)*dirx + (y - cy*m_res)*diry;
				float dx = float(x) / m_res - cx, dy = float(y) / m_res - cy;
				float cd = sqrtf(dx*dx + dy*dy) / r;
				cd = xr::Max(1.0f - cd*cd, 0.0f);
				m_hf[x + y*m_res] = func(d / m_res)*cd + m_hf[x + y*m_res]*(1.0f - cd);
			}
	}

	void water_drop(unsigned int x, unsigned int y)
	{
		float deposit = 0.0f;
		const float Ks = 0.003f;
		const float Kss = 0.2f;

		unsigned int R = unsigned int(m_res-1);
		for (int count = 0; x+1 < R && y+1 < R && x > 0 && y > 0 && count < 20; ++count )
		{
			float h10 = at(x - 1, y);
			float h20 = at(x + 1, y);
			float h01 = at(x, y - 1);
			float h02 = at(x, y + 1);
			float dx = h10 - h20 + (rand()%3 - 1);
			float dy = h01 - h02 + (rand()% 3 - 1);

			float h00 = at(x,y);

			if (h00 < h10 && h00 < h20) dx = 0;
			if (h00 < h01 && h00 < h02) dy = 0;

			float len = sqrtf(dx*dx + dy*dy);

			if (len < 0.001f)
			{
				at(x,y) = (h10 + h20 + h01 + h02)*0.25f;
				return;
			}

			if (h00 < 0.0f || len < 0.1f )
			{
				at(x,y) += deposit;
				break;
			}

			dx /= len;
			dy /= len;

			float sediment = len * Ks;
			if (h00 > h10 && h00 > h20 && h00 > h01 && h00 > h02)
				sediment = (h00 - h10)*Kss;

			at(x,y) -= sediment;
			deposit += sediment;

			x = (unsigned int)( x + dx );
			y = (unsigned int)( y + dy );
		}
	}

};