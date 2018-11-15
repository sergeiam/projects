#include <stdio.h>
#include <string.h>

#define RES			2304
#define INPUT_FILE	"blend.raw"

typedef unsigned char u8;

template< class T > void swap(T& a, T& b) { T temp = a; a = b; b = temp; }
template< class T > T max(T a, T b) { return a > b ? a : b; }

// SW - source weight type, SC - source channels count
template< class SW, int SC, class GET, class SET > void filter_terrain_blend(
	int x1, int y1, int x2, int y2,
	GET& source_terrain,
	SET& store_indices
)
{
	for (int y = y1; y < y2; ++y)
	{
		for (int x = x1; x < x2; ++x)
		{
			SW	weights[SC];
			int	indices[SC];

			for (int i = 0; i < SC; ++i) {
				indices[i] = i;
				weights[i] = max(
					max(source_terrain(x, y + 0, i), source_terrain(x + 1, y + 0, i)),
					max(source_terrain(x, y + 1, i), source_terrain(x + 1, y + 1, i))
				);
			}

			for( int i = 0; i < SC-1; ++i )
				for( int j = i + 1; j < SC; ++j )
					if (weights[i] < weights[j]) {
						swap(weights[i], weights[j]);
						swap(indices[i], indices[j]);
					}

			store_indices(x, y, indices);
		}
	}
}

int main()
{
	auto count_layers = [](const u8* row, FILE*out) -> void
	{
		u8 dst[RES] = { 0 };

		for (int x = 0; x < RES * 8; x += 8)
		{
			u8 count = 0;
			for (int i = 0; i < 8; ++i)
				if (row[x + i]) count += 16;
			dst[x / 8] = count;
		}
		fwrite(dst, 1, RES, out);
	};

	FILE* fp = fopen(INPUT_FILE, "rb");
	if (!fp) return 1;

	u8** src = new u8*[RES];
	for (int y = 0; y < RES; ++y)
	{
		src[y] = new u8[RES * 8];
		fread(src[y], 1, RES * 8, fp);
	}
	fclose(fp);

	auto fetch_terrain = [src](int x, int y, int channel) -> u8
	{
		return src[y][x * 8 + channel];
	};

	auto store_terrain = [src](int x, int y, int* indices) -> void
	{
		for (int i = 3; i < 8; ++i)
		{
			int channel = indices[i];
			src[y][x * 8 + channel] = 0;
			src[y][x * 8 + 8 + channel] = 0;
			src[y + 1][x * 8 + channel] = 0;
			src[y + 1][x * 8 + 8 + channel] = 0;
		}
	};

	// 2k x 2k terrain
	// 2 x 4M for the current tech - 8 textures
	// 2k 16 bit - 8M + 4k DXT1 - 8M

	filter_terrain_blend<u8, 8>(0, 0, RES-1, RES-1, fetch_terrain, store_terrain);

	fp = fopen("filtered.raw", "wb");
	if (!fp) return 1;

	for (int y = 0; y < RES; ++y)
	{
		fwrite(src[y], 1, RES * 8, fp);
	}
	fclose(fp);
	
	return 0;
}