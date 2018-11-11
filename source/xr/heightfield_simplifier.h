#pragma once

#include <xr/vector.h>
#include <xr/mesh.h>

namespace xr
{
	void heightfield_simplify(
		const float* hf, const int* weights, int w, int h, float scale, float hole_value,
		int max_width_per_triangle, float epsilon,
		MESH& mesh
	);
}