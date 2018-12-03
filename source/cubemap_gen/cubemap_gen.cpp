#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <windows.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define PI 3.14159265359f

struct float3
{
	union
	{
		struct {
			float r, g, b;
		};
		float v[3];
	};
	float3() { r = g = b = 0.0f; }

	void operator += (float3 rhs) {
		r += rhs.r;
		g += rhs.g;
		b += rhs.b;
	}
	friend float3 operator * (float3& a, float k) {
		float3 x = a;
		x.r *= k;
		x.g *= k;
		x.b *= k;
		return x;
	}
};


typedef unsigned char    u8;
typedef unsigned short    u16;
typedef unsigned long    u32;

template< class T > T Clamp(T x, T a, T b) { return (x < a) ? a : ((x > b) ? b : x); }
template< class T > T Min(T a, T b) { return (a < b) ? a : b; }



struct CUBEMAP_CONVERTOR
{
	const u8*		image;
	const float*	imagef;
	int				width, height, bytes_per_pixel;
	float			min_angle_z, max_angle_z;  // full spherical image would have -1.0..+1.0 range, meaning -PI..+PI angular range

	int				cubemap_resolution;
	float			cone_angle;
	int				num_samples;

	float			threshold;

	CUBEMAP_CONVERTOR() : image(nullptr), imagef(nullptr), min_angle_z(-1.0f), max_angle_z(+1.0f), threshold(20000.0f) {}

	void sample_spherical_map(float x, float y, float z, float3& texel)
	{
		float len = sqrtf(x*x + y*y + z*z);
		float yc = z / len;

		if (yc < min_angle_z || yc > max_angle_z) {
			texel.r = texel.g = texel.b = 0.0f;
			return;
		}

		yc = (yc - min_angle_z) / (max_angle_z - min_angle_z);
		yc = 1.0f - yc;

		int sy = Clamp(int(yc*height), 0, height - 1);
		float angle = atan2f(y, x) + PI;
		int sx = Clamp(int(angle * width / (2.0f*PI)), 0, width - 1);

		const int offset = (sx + sy*width) * 3;

		if (image) {
			texel.r = image[offset + 0] * (1.0f / 255.0f);
			texel.g = image[offset + 1] * (1.0f / 255.0f);
			texel.b = image[offset + 2] * (1.0f / 255.0f);
		} else {
			texel.r = imagef[offset + 0];
			if (texel.r > threshold)
			{
				texel.r = texel.g = texel.b = 0.0f;
				return;
			}
			texel.v[1] = imagef[offset + 1];
			texel.v[2] = imagef[offset + 2];
		}
	}

	void sample_spherical_map_cone(float x, float y, float z, float angle, int samples, float3& result)
	{
		sample_spherical_map(x, y, z, result);
		if (samples == 1)
			return;

		const float len = sqrtf(x*x + y*y + z*z);
		const float ax = x * (1.0f / len), ay = y * (1.0f / len), az = z * (1.0f / len);

		float3 acc = result;

		const float offset_len = tanf(angle * 0.5f * PI / 180.0f);

		float inv_rand = 2.0f / RAND_MAX;
		for (int i = 1; i < samples; ++i)
		{
			float rx = rand() * inv_rand - 1.0f;
			float ry = rand() * inv_rand - 1.0f;
			float rz = rand() * inv_rand - 1.0f;
			float rlen = sqrtf(rx*rx + ry*ry + rz*rz);
			float invlen = offset_len / rlen;

			float vx = ax + rx * invlen;
			float vy = ay + ry * invlen;
			float vz = az + rz * invlen;

			float inv_v = 1.0f / sqrtf(vx*vx + vy*vy + vz*vz);
			vx *= inv_v, vy *= inv_v, vz *= inv_v;

			float weight = vx*ax + vy*ay + vz*az;

			float3 texel;
			sample_spherical_map(vx, vy, vz, texel);

			acc += texel * weight;  // LDR -> HDR ?
		}
		//u32 texel = hdr_to_ldr(acc);
		result = acc * (1.0f / samples);
	}

	void compute_cube_face(float3* dst, int fx, int fy, int zaxis, float zsign, int xaxis, float xsign, int yaxis, float ysign)
	{
		const int dw = cubemap_resolution * 4, dh = cubemap_resolution * 3;

		float v[3];
		v[zaxis] = zsign;

		for (int y = 0; y < cubemap_resolution; ++y)
		{
			v[yaxis] = (-1.0f + y * 2.0f / (cubemap_resolution - 1)) * ysign;
			for (int x = 0; x < cubemap_resolution; ++x)
			{
				v[xaxis] = (-1.0f + x * 2.0f / (cubemap_resolution - 1)) * xsign;
				sample_spherical_map_cone( v[0], v[1], v[2], cone_angle, num_samples, dst[x + fx + (y + fy)*dw]);
			}
		}
	}

	void export_cubemap_cross(const char* input_filename, const char* ext)
	{
		const int cr = cubemap_resolution;
		const int dw = cr * 4, dh = cr * 3;

		float3* dst = new float3[dw * dh];

#define X 0
#define Y 1
#define Z 2

		compute_cube_face(dst, cr, cr * 0, Z, +1.0f, X, +1.0f, Y, +1.0f);

		compute_cube_face(dst, cr * 0, cr, X, -1.0f, Y, +1.0f, Z, -1.0f);
		compute_cube_face(dst, cr * 1, cr, Y, +1.0f, X, +1.0f, Z, -1.0f);
		compute_cube_face(dst, cr * 2, cr, X, +1.0f, Y, -1.0f, Z, -1.0f);
		compute_cube_face(dst, cr * 3, cr, Y, -1.0f, X, -1.0f, Z, -1.0f);

		compute_cube_face(dst, cr, cr * 2, Z, -1.0f, X, +1.0f, Y, -1.0f);

#undef X
#undef Y
#undef Z

		char output_filename[512];
		strcpy(output_filename, input_filename);

		char* last_dot = strrchr(output_filename, '.');
		strcpy(last_dot, ext);
		strcpy(last_dot + strlen(ext), strrchr(input_filename, '.'));

		if (imagef)
			stbi_write_hdr(output_filename, dw, dh, 3, &dst->r );
		else {
			u8* image = new u8[dw * dh * 3];
			for (int i = 0; i < dw*dh; ++i)
			{
				image[i * 3 + 0] = u8(dst[i].r * 255.0f);
				image[i * 3 + 1] = u8(dst[i].g * 255.0f);
				image[i * 3 + 2] = u8(dst[i].b * 255.0f);
			}
			stbi_write_tga(output_filename, dw, dh, 3, image);
		}
	}
};

float ldr_to_hdr_scalar(float value)
{
	return value / (1.000001f - value);
}

float hdr_to_ldr_scalar(float value)
{
	return value / (1.01f + value);
}

float3 ldr_to_hdr(float3 texel)
{
	float3 hdr;
	hdr.r = ldr_to_hdr_scalar(texel.r);
	hdr.g = ldr_to_hdr_scalar(texel.g);
	hdr.b = ldr_to_hdr_scalar(texel.b);
	return hdr;
}

float3 hdr_to_ldr(float3 texel)
{
	float3 ldr;
	ldr.r = hdr_to_ldr_scalar(texel.r);
	ldr.g = hdr_to_ldr_scalar(texel.g);
	ldr.b = hdr_to_ldr_scalar(texel.b);
	return ldr;
}

/*
void export_top_spheremap(const EXPORT_CUBEMAP_PARAMS* ecp, const char* input_filename, const char* new_extention)
{
	const int res = ecp->cubemap_resolution;
	const int bpp = ecp->src_bytes_per_pixel;

	u8* dst = new u8[res * res * bpp];
	memset(dst, 0, res * res * bpp);

	for (int y = 0; y<res; ++y)
	{
		for (int x = 0; x<res; ++x)
		{
			float vx = (x - res / 2) * 2.0f / res;
			float vy = (y - res / 2) * 2.0f / res;

			float vz = -sqrtf(1.0f - Min(vx*vx + vy*vy, 1.0f));

			u32 texel = sample_spherical_map_cone(ecp->src_image, ecp->src_width, ecp->src_height, ecp->src_bytes_per_pixel, vx, vy, vz, ecp->cone_angle, ecp->num_samples);
			write_texel(dst, res, res, bpp, x, y, texel);
		}
	}

	char output_filename[512];
	strcpy(output_filename, input_filename);
	strcpy(strrchr(output_filename, '.'), new_extention);

	save_tga(output_filename, dst, res, res, bpp);
}
*/

int main(int argc, char** argv)
{
	if (strlen(argv[1]) == 0) {
		printf( "Drag and drop source texture to produce cubemap" );
		return 1;
	}

	CUBEMAP_CONVERTOR cc;

	if (strstr(argv[1], ".hdr"))
		cc.imagef = stbi_loadf(argv[1], &cc.width, &cc.height, NULL, 0);
	else if (strstr(argv[1], ".tga"))
		cc.image = stbi_load(argv[1], &cc.width, &cc.height, NULL, 0);

	if (!cc.image && !cc.imagef) {
		MessageBoxA(NULL, "Unable to load source image", "Error", MB_OK);
		return 1;
	}

	cc.cubemap_resolution = 128;
	cc.cone_angle = 0.5f;
	cc.num_samples = 100;
	cc.min_angle_z = 0.0f;

	cc.export_cubemap_cross(argv[1], ".cbm128");

	cc.cubemap_resolution = 8;
	cc.cone_angle = 60.0f;
	cc.num_samples = 16000;
	cc.export_cubemap_cross(argv[1], ".cbm8");

	MessageBoxA(NULL, "We are done computing!", "Success", MB_OK);

	return 0;
}

