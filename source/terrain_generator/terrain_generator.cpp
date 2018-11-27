#include <stdlib.h>
#include <xr/core.h>
#include <xr/heightfield_simplifier.h>
#include "heightfield.h"

#define RESOLUTION 512



int main()
{
	HEIGHTFIELD hf(RESOLUTION);
	hf.sine_wave(135.0f, 0.3f, [](float s1, float s2) -> float {
		return (s1 + s2);
	});

	HEIGHTFIELD hf1(RESOLUTION);
	hf1.sine_wave(79.0f, 0.1f, [](float s1, float s2) -> float {
		return (s1 + s2);
	});

	HEIGHTFIELD hf2(RESOLUTION);
	hf2.sine_wave(125.0f, 0.3f, [](float s1, float s2) -> float {
		return (s1 + s2);
	});

	HEIGHTFIELD hf3(RESOLUTION);
	hf3.sine_wave(175.0f, 0.13f, [](float s1, float s2) -> float {
		return (s1 + s2);
	});

	HEIGHTFIELD hf5(RESOLUTION);
	hf5.sine_wave(15.0f, 0.3f, [](float s1, float s2) -> float {
		return (s1 + s2);
	});

	hf.combine(hf1,[](float a, float b) -> float { return a + b; });
	hf.combine(hf2, [](float a, float b) -> float { return a + b; });
	hf.combine(hf3, [](float a, float b) -> float { return a + b; });
	hf.combine(hf5, [](float a, float b) -> float { return a + b; });

	hf.bilinear_noise(16);
	hf.scale(20.0f);

	//hf.scale(0.0f);
	hf.facing_circle(0.25f, 0.25f, 0.3f, 1.0f, 0.0f, [](float dist) -> float
	{
		float k = dist / 0.3f;
		float k2 = 1 - 2 * k + k*k;
		return ((k > 0.0f) ? k2*k2*k2*k2*k2*k2 : 1.0f - k*k) * 20.0f;
	});
	hf.facing_circle(0.35f, 0.75f, 0.3f, 0.77f, -0.77f, [](float dist) -> float
	{
		float k = dist / 0.3f;
		float k2 = 1 - 2 * k + k*k;
		return ((k > 0.0f) ? k2*k2*k2*k2*k2*k2 : 1.0f - k*k*k*k) * 20.0f;
	});

	struct point { short x, y; };

	xr::VECTOR<point> pts;
	pts.resize(RESOLUTION*RESOLUTION);
	for (int i = 0; i < RESOLUTION*RESOLUTION; ++i) {
		pts[i].x = i % RESOLUTION;
		pts[i].y = i / RESOLUTION;
	}
	for (int i = 0; i < RESOLUTION*RESOLUTION; ++i) {
		int a1 = rand() % (RESOLUTION*RESOLUTION);
		xr::Swap(pts[i], pts[a1]);
	}

	for (int i = 0; i < 30; ++i)
	{
		for( int j=0; j<RESOLUTION*RESOLUTION; ++j )
			hf.water_drop(pts[j].x, pts[j].y);
	}

	xr::MESH mesh;
	xr::heightfield_simplify(hf.m_hf, nullptr, RESOLUTION, RESOLUTION, 256.0f / RESOLUTION, -10000.0f, 128, 0.1f, mesh);
	mesh.write_obj("terrain.obj");

	return 0;
}