#include <stdio.h>
#include <string.h>
#include <xr/core.h>
#include <math.h>
#include <stdlib.h>

#define PI 3.14159265359f

struct EXPORT_CUBEMAP_PARAMS
{
	const u32*	src_image;
	int			src_width, src_height;
	int			cubemap_resolution;
	float		cone_angle;
	int			num_samples;
};

u32 sample_spherical_map(const u32* map, int w, int h, float x, float y, float z)
{
	float len = sqrtf(x*x + y*y + z*z);

	int sy = xr::Clamp( int(((z / len)*0.5f + 0.5f) * h), 0, h-1 );

	float angle = atan2f(y, x) + PI;

	int sx = xr::Clamp( int(angle * w / (2.0f*PI)), 0, w-1 );

	return map[sx + sy*w];
}

void compute_cube_face(const EXPORT_CUBEMAP_PARAMS* ecp, u32* dst, int fx, int fy, int zaxis, float zsign, int xaxis, float xsign, int yaxis, float ysign)
{
	const u32* src = ecp->src_image;
	const int w = ecp->src_width;
	const int h = ecp->src_height;
	const int cube_res = ecp->cubemap_resolution;
	const float cone_angle = ecp->cone_angle;
	const int samples = ecp->num_samples;

	const float offset_len = tanf(cone_angle * 0.5f * PI / 180.0f);

	float v[3];
	v[zaxis] = zsign;

	for (int y = 0; y < cube_res; ++y)
	{
		v[yaxis] = (-1.0f + y * 2.0f / (cube_res - 1)) * ysign;
		for (int x = 0; x < cube_res; ++x)
		{
			v[xaxis] = (-1.0f + x * 2.0f / (cube_res - 1)) * xsign;

			if (samples == 1)
			{
				dst[x + fx + (y + fy)*(cube_res * 4)] = sample_spherical_map(src, w, h, v[0], v[1], v[2]);
			}
			else {
				const float len = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
				const float ax = v[0] * (1.0f / len), ay = v[1] * (1.0f / len), az = v[2] * (1.0f / len);

				float acc[4] = { 0.0f };
				float sum_w = 0.0f;

				float inv_rand = 2.0f / RAND_MAX;
				for (int i = 0; i < samples; ++i)
				{
					float rx = rand() * inv_rand - 1.0f;
					float ry = rand() * inv_rand - 1.0f;
					float rz = rand() * inv_rand - 1.0f;
					float rlen = sqrtf(rx*rx + ry*ry + rz*rz);
					float invlen = offset_len / rlen;

					float vx = ax + rx * invlen, vy = ay + ry * invlen, vz = az + rz * invlen;

					float inv_v = 1.0f / sqrtf(vx*vx + vy*vy + vz*vz);
					vx *= inv_v, vy *= inv_v, vz *= inv_v;

					float w = vx*ax + vy*ay + vz*az;

					sum_w += w;

					u32 s = sample_spherical_map(src, w, h, vx, vy, vz);

					acc[0] += (s >> 24) * w;
					acc[1] += ((s >> 16) & 0xFF) * w;
					acc[2] += ((s >> 8) & 0xFF) * w;
					acc[3] += ((s >> 0) & 0xFF) * w;
				}
				u32 dval = (int(acc[0] / sum_w) << 24) | (int(acc[1] / sum_w) << 16) | (int(acc[2] / sum_w) << 8) | (int(acc[3] / sum_w));
				dst[x + fx + (y + fy)*(cube_res * 4)] = dval;
			}
		}
	}
}

void export_cubemap(const EXPORT_CUBEMAP_PARAMS* ecp, const char* input_filename, const char* new_extention )
{
	const u32* src = ecp->src_image;
	const int w = ecp->src_width;
	const int h = ecp->src_height;
	const int cube_res = ecp->cubemap_resolution;

	u32* dst = new u32[cube_res*4 * cube_res*3];
	memset(dst, 0, cube_res * 4 * cube_res * 3 * 4);

	compute_cube_face(ecp, dst, cube_res * 0, cube_res, 0, -1.0f, 1, +1.0f, 2, 1.0f);
	compute_cube_face(ecp, dst, cube_res * 1, cube_res, 1, +1.0f, 0, +1.0f, 2, 1.0f);
	compute_cube_face(ecp, dst, cube_res * 2, cube_res, 0, +1.0f, 1, -1.0f, 2, 1.0f);
	compute_cube_face(ecp, dst, cube_res * 3, cube_res, 1, -1.0f, 0, -1.0f, 2, 1.0f);
	compute_cube_face(ecp, dst, cube_res, cube_res * 0, 2, -1.0f, 0, 1.0f, 1, +1.0f);
	compute_cube_face(ecp, dst, cube_res, cube_res * 2, 2, +1.0f, 0, 1.0f, 1, -1.0f);

	char output_filename[512];
	strcpy(output_filename, input_filename);
	strcpy(strrchr(output_filename, '.'), new_extention);

	FILE* fp_dst = fopen(output_filename, "wb");
	for (int y = 0; y<cube_res*3; ++y)
		fwrite(dst + y*cube_res*4, 1, cube_res * 16, fp_dst);
	fclose(fp_dst);
}

int main(int argc, char** argv)
{
	if (argc != 2) {
		printf("Usage: %s <input-file>\n", argv[0]);
		return 1;
	}

	FILE* fp_src = fopen(argv[1], "rb");
	if (!fp_src) {
		printf("Error: failed to open file '%s'\n", argv[1]);
		return 2;
	}

	fseek(fp_src, 0, SEEK_END);
	int size = ftell(fp_src);
	fseek(fp_src, 0, SEEK_SET);

	if (size % 8 != 0) {
		printf("Error: input file '%s' size can not be divided by 8", argv[1]);
		return 3;
	}

	size /= 8;
	int sq = sqrtf(size);
	if (sq*sq < size)
		sq++;
	else if (sq*sq > size)
		sq--;

	if (sq*sq != size) {
		printf("Error: it looks like the input image has different aspect than 2:1");
		return 4;
	}

	int width = sq * 2;
	int height = sq;

	u32* src = new u32[width * height];
	for( int y=0; y<height; ++y )
		fread(src + y*width, 1, width * 4, fp_src);
	fclose(fp_src);

	EXPORT_CUBEMAP_PARAMS params;
	params.src_image = src;
	params.src_width = width;
	params.src_height = height;
	params.cubemap_resolution = 1024;
	params.cone_angle = 0.0f;
	params.num_samples = 1;

	export_cubemap(&params, argv[1], ".1024.cbm.raw" );

	params.cubemap_resolution = 32;
	params.cone_angle = 60.0f;
	params.num_samples = 512;
	export_cubemap(&params, argv[1], ".32.cbm.raw");

	return 0;
}