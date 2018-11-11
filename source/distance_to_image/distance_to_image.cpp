#include <stdio.h>
#include <string.h>
#include <stdlib.h>



int Min(int a, int b) {
	return a < b ? a : b;
}

int main(int argc, char ** argv)
{
	int W = 0, H = 0, src_bits = 8, dst_bits = 0, test_value = 0;
	const char* src_filename = NULL;
	const char* dst_filename = NULL;
	bool unknown_option = false;

	for (int i = 1; i < argc; ++i) {
		if (i+2 < argc && !strcmp("-size", argv[i])) {
			W = atoi(argv[i + 1]);
			H = atoi(argv[i + 2]);
			i += 2;
		}
		else if (i+1 < argc && !strcmp("-sb", argv[i])) {
			src_bits = atoi(argv[i + 1]);
			i += 1;
		}
		else if (i+1 < argc && !strcmp("-db", argv[i])) {
			dst_bits = atoi(argv[i + 1]);
			i += 1;
		}
		else if (i+1 < argc && !strcmp("-tv", argv[i])) {
			test_value = atoi(argv[i + 1]);
			i += 1;
		}
		else if (argv[i][0] == '-') {
			unknown_option = true;
		}
		else if (!src_filename)
			src_filename = argv[i];
		else if (!dst_filename)
			dst_filename = argv[i];
	}
	if (!dst_bits) dst_bits = src_bits;

	if (unknown_option || !src_filename || !dst_filename || !W || !H || (src_bits != 8 && src_bits != 16 ) || (dst_bits != 8 && dst_bits != 16)) {
		printf("Usage: %s <options> src_image dest_image\n"\
			"	-size W H	- specify image resolution\n"\
			"	-sb Bits	- specify source image bits per pixel - 8 or 16\n"\
			"	-db Bits	- specify destination image bits per pixel - 8 or 16\n"\
			"	-tv Value	- threshold value to consider source image pixels as solid\n"\
			"", argv[0]);
		return 0;
	}

	FILE* fp;
	if (fopen_s(&fp, src_filename, "rb")) {
		printf("ERROR: failed to open source image '%s'\n", src_filename);
		return 1;
	}

	const int src_bytes = src_bits / 8;

	const int size = W*H*src_bytes;
	char* image = new char[W*H*src_bits / 8];
	if (fread(image, 1, size, fp) != size) {
		printf("ERROR: failed to read the specified number of pixels, check your arguments!\n");
		return 1;
	}

	// convert the source image to the initial distance mask

	int* dist = new int[W*H];
	for (int i = 0; i < W*H; ++i ) {
		int value = 0;
		switch (src_bytes) {
			case 1: value = *(const unsigned char*)(image + i); break;
			case 2: value = *(const unsigned short*)(image + i*2); break;
		}
		dist[i] = value > test_value ? 0 : 100000;
	}

	// first pass, top -> down, left -> right

#define DOWN  +W
#define TOP   -W
#define LEFT  -1
#define RIGHT +1

	int* ptr = dist + 1;

	for (int x = 1; x < W; ++x)
		*ptr++ = Min(*ptr, ptr[LEFT] + 5);

	for (int y = 1; y < H; ++y)
	{
		*ptr++ = Min( *ptr, Min( ptr[TOP]+5, ptr[TOP+RIGHT] + 7 ));
		for (int x = 1; x < W-1; ++x, ++ptr) {
			int n = Min( Min( ptr[TOP+LEFT] + 7, ptr[TOP] + 5), Min(ptr[TOP+RIGHT] + 7, ptr[LEFT] + 5));
			*ptr = Min(*ptr, n);
		}
		*ptr++ = Min( Min( *ptr, ptr[TOP] + 5), Min(ptr[LEFT] + 5, ptr[TOP+LEFT] + 7));
	}

	// second pass, bottom -> up, right -> left

	ptr = dist + W*H - 2;
	for (int x = W - 2; x >= 0; --x)
		*ptr-- = Min(*ptr, ptr[RIGHT] + 5);

	for (int y = H-2; y >= 0; --y)
	{
		*ptr-- = Min(*ptr, Min( ptr[DOWN+LEFT] + 7, ptr[DOWN] + 5));
		for (int x = W-2; x >= 1; --x, --ptr) {
			int n = Min(Min(ptr[DOWN+LEFT] + 7, ptr[DOWN] + 5), Min(ptr[DOWN+RIGHT] + 7, ptr[RIGHT] + 5));
			*ptr = Min(*ptr, n);
		}
		*ptr-- = Min(Min(*ptr, ptr[DOWN] + 5), Min(ptr[DOWN+RIGHT] + 7, ptr[RIGHT] + 5));
	}

	// convert to destination image

	const int dst_size = W*H*dst_bits / 8;
	char* dst = new char[dst_size];
	for (int i = 0; i < W*H; ++i) {
		switch (dst_bits) {
			case 8: *(((unsigned char*)dst) + i) = Min(dist[i]/5, 255); break;
			case 16: *(((unsigned short*)dst) + i) = (unsigned short)dist[i] / 5; break;
		}
	}

	if (fopen_s(&fp, dst_filename, "wb")) {
		printf("ERROR: can not create destination image '%s'\n", dst_filename);
		return 1;
	}
	fwrite(dst, 1, dst_size, fp);
	fclose(fp);

	return 0;
}