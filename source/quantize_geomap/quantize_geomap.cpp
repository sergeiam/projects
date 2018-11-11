/*
	Geographic map image quantizator, by Sergei Miloykov (sergei.m@gmail.com)
	Process an image to find continuous single colored regions and spread them over non-background, neighbor colours
*/

#include <stdio.h>
#include <set>

#define W 2048
#define H 2048
#define INPUT_FILE_NAME "geo_map.raw"
#define OUTPUT_FILE_NAME "geo_map2.raw"

#define AREA_SPAN 20			// single-color span to generate a valid palette color
#define GRAY_TOLLERANCE 16		// RGB component distance limit for 'gray' color


using namespace std;

typedef unsigned char u8;
typedef unsigned long u32;

u8* image, *mask;



u32 get_color(int offset)
{
	return ((u32*)(image + offset))[0] & 0xFFFFFFU;
}

void set_color(int offset, u32 color)
{
	image[offset + 0] = color & 0xFF;
	image[offset + 1] = (color >> 8) & 0xFF;
	image[offset + 2] = (color >> 16) & 0xFF;
}

u8 Abs(u8 a, u8 b) {
	return a < b ? b - a : a - b;
}

bool is_border(u32 color) {
	u8 r = color & 0xFF, g = (color >> 8) & 0xFF, b = (color >> 16);
	return Abs(r, g) < GRAY_TOLLERANCE && Abs(g, b) < GRAY_TOLLERANCE;
}

bool is_background(u32 color) {
	return color == 0xFFFFFFU;
}

int main()
{
	FILE* fp;
	if(fopen_s(&fp, INPUT_FILE_NAME, "rb"))
		return 1;
	image = new u8[W*H*3];
	fread(image, 1, W*H * 3, fp);
	fclose(fp);

	set<u32> palette;

	u32 prev_color;
	int count = 0;

	for (int y = 0; y < H; y += 2)
	{
		int offset = y*W * 3;
		for (int x = 0; x < W; ++x, offset += 3)
		{
			u32 rgb1 = get_color(offset);
			u32 rgb2 = get_color(offset + W*3);

			if (rgb1 == rgb2) {
				if (count <= 0) {
					count = 1;
					prev_color = rgb1;
				}
				else {
					count += (rgb1 != prev_color) ? -1 : 1;
				}

				if (count == AREA_SPAN) {
					if( !is_background(rgb1) && !is_border(rgb1))
						palette.insert(rgb1);
					count = 1;
				}
			}
			else {
				count--;
			}
		}
	}

	size_t size = palette.size();
	printf("Palette size == %d\n", size);

	u8* mask = new u8[W*H];
	memset(mask, 0, W*H);

	for (auto it = palette.begin(); it != palette.end(); ++it) {
		u32 pcol = *it;
		for (int i = 0; i < W*H; ++i) {
			u32 col = get_color(i * 3);
			if (is_background(col)) continue;
			if (col == pcol) mask[i] = 1;
		}
	}

	for (u8 mask_val = 1; mask_val < 255; ++mask_val)
	{
		bool changed = false;
		for (int y = 1; y < H - 1; ++y) {
			u8* mask_ptr = mask + y*W + 1;
			int image_offset = (y*W + 1) * 3;
			for (int x = 1; x < W - 1; ++x, ++mask_ptr, image_offset += 3) {
				if (*mask_ptr != 0) continue;
				if (is_background(get_color(image_offset))) continue;

				const int neighb[] = { 1, -1, -W, +W };
				for (int i = 0; i < 4; ++i) {
					int ofs = neighb[i];
					if ( *(mask_ptr + ofs) == mask_val) {
						*mask_ptr = mask_val+1;  // continue wave, so the next iteration will starts there
						u32 color = get_color(image_offset + ofs*3);
						set_color(image_offset, color);
						changed = true;
						break;
					}
				}
			}
		}
		if (!changed) break;
		printf(".");
	}

	if (!fopen_s(&fp, OUTPUT_FILE_NAME, "wb")) {
		fwrite(image, 1, W*H * 3, fp);
		fclose(fp);
	}
	return 0;
}