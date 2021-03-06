#include <stdio.h>
#include <memory.h>
#include <windows.h>

#include <xr/mesh_simplifier.h>
#include <xr/heightfield_simplifier.h>
#include <xr/mesh.h>
#include <xr/time.h>

//#define SIMPLIFICATION_RATIO	0.15f    // how much from the original mesh we would like to keep as face count
#define SIMPLIFICATION_EPSILON 0.0136f

//#define HOLE_TEST  // circular hole in the center of the terrain

#define INPUT_FILENAME "hf42k.raw"
#define OUTPUT_FILENAME "mesh_hf.obj"
#define W 4096
#define H 2048
#define XY_SCALE 0.005f
#define HOLE_VALUE -1000.0f

struct TEST
{
	int a : 2;
	int b = 1;

	TEST() : a(0) {}
};

bool load_heightfield(const char* file, float max_height, xr::VECTOR<float>& hf)
{
	FILE* fp = fopen(file, "rb");
	if (!fp) return false;
	fseek(fp, 0, SEEK_END);
	hf.resize(ftell(fp));
	fseek(fp, 0, SEEK_SET);

	const float scale = max_height / 255.0f;

	unsigned char* buf = new unsigned char[hf.size()];
	fread(buf, 1, hf.size(), fp);
	fclose(fp);
	for (int i = 0; i < hf.size(); ++i)
		hf[i] = float(buf[i]) * scale;
	delete[] buf;
	return true;
}

int main()
{
	xr::VECTOR<float> heightfield;

	load_heightfield(INPUT_FILENAME, 1.0f, heightfield);

	xr::MESH mesh_hf;

	xr::VECTOR<int> mask;
	mask.resize(W*H);
	mask.fill(0);

	// draw a circle to test the weight/hole functionality

#ifdef HOLE_TEST
	for (int i = 0; i < W*H; ++i ) {
		int x = (i%W) - (W / 2), y = (i / W) - (H / 2);
		int xysq = x*x + y*y;
		//mask[i] = (xysq >= (H*H / 16) && xysq <= ((H+1)*(H+1)/16)) ? 1 : 0;
		if( xysq >= (H*H / 16) && xysq <= ((H + 10)*(H + 10) / 16) )
			heightfield[i] = HOLE_VALUE;
	}
#endif

	xr::TIME_SCOPE time;

#ifdef SIMPLIFICATION_RATIO

	float epsilon_l = 0.0f, epsilon_r = 0.0f;

	for (int y = 0; y < H - 1; ++y) {
		const float* ptr = &heightfield[y*W];
		for (int x = 0; x < W - 1; ++x) {
			float dx = fabsf(ptr[0] - ptr[1]), dy = fabsf(ptr[0] - ptr[W]);
			if (dx > epsilon_r) epsilon_r = dx;
			if (dy > epsilon_r) epsilon_r = dy;
		}
	}

	const int target_vertices = W * H * SIMPLIFICATION_RATIO;
	for (int i = 0; i < 6; ++i)
	{
		const float epsilon = (epsilon_l + epsilon_r)*0.5f;

		mesh_hf.clear();

		xr::heightfield_simplify(&heightfield[0], &mask[0], W, H, XY_SCALE, HOLE_VALUE, xr::Max(W / 16, H / 16), epsilon, mesh_hf);
		if (mesh_hf.positions.size() > target_vertices )
			epsilon_l = epsilon;
		else
			epsilon_r = epsilon;
	}
#elif defined(SIMPLIFICATION_EPSILON)

	xr::heightfield_simplify(&heightfield[0], &mask[0], W, H, XY_SCALE, HOLE_VALUE, xr::Max(W / 16, H / 16), SIMPLIFICATION_EPSILON, mesh_hf);

#endif

	char pc[256];
	sprintf(pc, "Simplify took %d ms\n", time.measure_duration_ms());
	OutputDebugStringA(pc);

	time.reset();

	mesh_hf.write_obj(OUTPUT_FILENAME);

	sprintf(pc, "WriteObj took %d ms\n", time.measure_duration_ms());
	OutputDebugStringA(pc);

	return 0;
}