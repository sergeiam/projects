#include <Windows.h>
#include <string>
#include <vector>

typedef unsigned char	u8;
typedef unsigned short	u16;
typedef unsigned long	u32;

#pragma warning( disable : 4996 )

template< class T >
void FolderRecursiveIteration(std::string folder, T& callback)
{
	if (folder.back() != '/' && folder.back() != '\\')
		folder += "/";

	WIN32_FIND_DATAA fd;
	for (HANDLE h = FindFirstFileA((folder + "*.*").c_str(), &fd); h != INVALID_HANDLE_VALUE; )
	{
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if( fd.cFileName[0] != '.' )
				FolderRecursiveIteration(folder + std::string(fd.cFileName), callback);
		}
		else
		{
			callback(folder + fd.cFileName);
		}

		if (!FindNextFile(h, &fd))
		{
			FindClose(h);
			break;
		}
	}
}

bool endswith(std::string s, const char* ext)
{
	return strcmp(&s[0] + s.size() - strlen(ext), ext) == 0;
}

struct ConvertXBTtoDDS
{
	bool operator()(std::string filename)
	{
		if (!endswith( filename, ".xbt") && !endswith( filename, ".xbts"))
			return false;

		FILE* fp = fopen(filename.c_str(), "rb");
		if (!fp)
			return false;

		FILE* fpOut = fopen((filename + ".dds").c_str(), "wb");
		if (!fpOut)
			return false;

		char buffer[256];
		fread(buffer, 1, 256, fp);

		bool dds = false;
		for (int i = 0; i < 256-4; ++i)
		{
			if (buffer[i] == 'D' && buffer[i + 1] == 'D' && buffer[i + 2] == 'S')
			{
				dds = true;
				fwrite(buffer + i, 1, 256 - i, fpOut);
				break;
			}
		}
		if (dds)
		{
			for (;;)
			{
				char buffer[4096];
				int len = fread(buffer, 1, 4096, fp);
				fwrite(buffer, 1, len, fpOut);
				if (len < 4096) break;
			}
		}
		fclose(fp);
		fclose(fpOut);
		return true;
	}
};

#pragma pack(push)
#pragma pack( 1 )

struct DDS_PIXELFORMAT
{
	DWORD dwSize;
	DWORD dwFlags;
	DWORD dwFourCC;
	DWORD dwRGBBitCount;
	DWORD dwRBitMask;
	DWORD dwGBitMask;
	DWORD dwBBitMask;
	DWORD dwABitMask;
};

struct DDS_HEADER
{
	DWORD           dwSize;
	DWORD           dwFlags;
	DWORD           dwHeight;
	DWORD           dwWidth;
	DWORD           dwPitchOrLinearSize;
	DWORD           dwDepth;
	DWORD           dwMipMapCount;
	DWORD           dwReserved1[11];
	DDS_PIXELFORMAT ddspf;
	DWORD           dwCaps;
	DWORD           dwCaps2;
	DWORD           dwCaps3;
	DWORD           dwCaps4;
	DWORD           dwReserved2;
};

struct DDS_HEADER_DXT10
{
	DWORD	dxgiFormat;
	DWORD	resourceDimension;
	UINT    miscFlag;
	UINT    arraySize;
	UINT    miscFlags2;
};

struct DDS_BLOCK_BC1
{
	u16 c0, c1;
	u8	cb[4];

	void decode(FILE* fp, u8* pixels, int bpp, int pitch)
	{
		fread(this, 1, 8, fp);

		int r[4], g[4], b[4];
		r[0] = (c0 >> 11) << 3;
		g[0] = ((c0 >> 5) & 63) << 2;
		b[0] = (c0 & 31) << 3;
		r[1] = (c1 >> 11) << 3;
		g[1] = ((c1 >> 5) & 63) << 2;
		b[1] = (c1 & 31) << 3;

		r[2] = (r[0] * 2 + r[1]) / 3;
		r[3] = (r[0] + r[1] * 2) / 3;
		g[2] = (g[0] * 2 + g[1]) / 3;
		g[3] = (g[0] + g[1] * 2) / 3;
		b[2] = (b[0] * 2 + b[1]) / 3;
		b[3] = (b[0] + b[1] * 2) / 3;

		for (int by = 0; by < 4; ++by)
		{
			u8* ptr = pixels + by * pitch;
			for (int bx = 0; bx < 4; ++bx, ptr += bpp/8)
			{
				int cx = bx;
				int cy = by;

				int index = (cb[cy] >> (2 * cx)) & 3;
				ptr[2] = r[index];
				ptr[1] = g[index];
				ptr[0] = b[index];
			}
		}
	}
};

struct DDS_BLOCK_BC3
{
	u8	a0, a1;
	u8	ab[6];
	u16 c0, c1;
	u8	cb[4];

	void decode(FILE* fp, u8* pixels, int pitch)
	{
		fread(this, 1, 16, fp);

		int r[4], g[4], b[4];
		r[0] = (c0 >> 11) << 3;
		g[0] = ((c0 >> 5) & 63) << 2;
		b[0] = (c0 & 31) << 3;
		r[1] = (c1 >> 11) << 3;
		g[1] = ((c1 >> 5) & 63) << 2;
		b[1] = (c1 & 31) << 3;

		r[2] = (r[0] * 2 + r[1]) / 3;
		r[3] = (r[0] + r[1] * 2) / 3;
		g[2] = (g[0] * 2 + g[1]) / 3;
		g[3] = (g[0] + g[1] * 2) / 3;
		b[2] = (b[0] * 2 + b[1]) / 3;
		b[3] = (b[0] + b[1] * 2) / 3;

		int a[8];
		a[0] = a0;
		a[1] = a1;

		if (a0 > a1) {
			a[2] = (6 * a0 + 1 * a1) / 7;
			a[3] = (5 * a0 + 2 * a1) / 7;
			a[4] = (4 * a0 + 3 * a1) / 7;
			a[5] = (3 * a0 + 4 * a1) / 7;
			a[6] = (2 * a0 + 5 * a1) / 7;
			a[7] = (1 * a0 + 6 * a1) / 7;
		} else {
			a[2] = (4 * a0 + 1 * a1) / 5;
			a[3] = (3 * a0 + 2 * a1) / 5;
			a[4] = (2 * a0 + 3 * a1) / 5;
			a[5] = (1 * a0 + 4 * a1) / 5;
			a[6] = 0;
			a[7] = 255;
		}

		for (int by = 0; by < 4; ++by)
		{
			u8* ptr = pixels + by * pitch;

			int alpha_bit_offset = by * 4 * 3;

			u8* aptr = ab + alpha_bit_offset / 8;

			u32 alpha = (aptr[0] + (aptr[1] << 8) + (aptr[2] << 16));
			if (by & 1) alpha >>= 4;

			for (int bx = 0; bx < 4; ++bx, ptr += 4, alpha >>= 3)
			{
				int cx = bx;
				int cy = by;

				int index = (cb[cy] >> (2 * cx)) & 3;
				ptr[2] = r[index];
				ptr[1] = g[index];
				ptr[0] = b[index];

				index = alpha & 7;
				ptr[3] = a[index];
			}
		}
	}
};

struct TGA_HEADER
{
	char  idlength;
	char  colourmaptype;
	char  datatypecode;
	short colourmaporigin;
	short colourmaplength;
	char  colourmapdepth;
	short x_origin;
	short y_origin;
	short width;
	short height;
	char  bitsperpixel;
	char  imagedescriptor;
};

#pragma pack(pop)

struct TGA_FILE
{
	TGA_HEADER	m_header;
	u8*			m_pixels;

	TGA_FILE(int w, int h, int bpp)
	{
		m_header.bitsperpixel = bpp;
		m_header.width = w;
		m_header.height = h;
		m_header.idlength = 0;
		m_header.x_origin = 0;
		m_header.y_origin = 0;
		m_header.datatypecode = 2;
		m_header.colourmapdepth = 0;
		m_header.colourmaplength = 0;
		m_header.colourmaporigin = 0;
		m_header.colourmaptype = 0;
		m_pixels = new u8[w * h * bpp / 8];
		memset(m_pixels, 0, w * h * bpp / 8);
	}
	~TGA_FILE()
	{
		delete[] m_pixels;
	}

	u8& operator()(int x, int y, int channel)
	{
		if (x < 0 || y < 0 || x >= m_header.width || y >= m_header.height)
			__debugbreak();
		return m_pixels[(x + y * m_header.width) * (m_header.bitsperpixel / 8) + channel];
	}

	void save(const char* filename)
	{
		FILE* fp = fopen(filename, "wb");
		if (fp)
		{
			fwrite(&m_header, 1, sizeof(m_header), fp);
			fwrite(m_pixels, 1, m_header.width * m_header.height * m_header.bitsperpixel / 8, fp);
			fclose(fp);
			//printf("%s\n", filename);
		}
	}
};

struct ConvertDiffusemapDDS
{
	bool operator()(std::string filename)
	{
		FILE* fpIn = fopen(filename.c_str(), "rb");
		if (!fpIn)
			return false;

		char stamp[4];
		fread(&stamp, 1, 4, fpIn);
		if (stamp[0] != 'D' || stamp[1] != 'D' || stamp[2] != 'S' || stamp[3] != ' ')
		{
			fclose(fpIn);
			return false;
		}

		DDS_HEADER dds_header;
		DDS_HEADER_DXT10 dxt10_header;

		fread(&dds_header, 1, sizeof(dds_header), fpIn);
		if (dds_header.ddspf.dwFourCC == MAKEFOURCC('D', 'X', '1', '0'))
		{
			fread(&dxt10_header, 1, sizeof(dxt10_header), fpIn);
		}

		printf("%s\n%d x %d, %d mips, fmt: %d\n", strrchr( filename.c_str(), '/')+1, dds_header.dwWidth, dds_header.dwHeight, dds_header.dwMipMapCount, dxt10_header.dxgiFormat);

		DWORD fmt = dxt10_header.dxgiFormat;
		bool bc1 = fmt == 71 || fmt == 72;
		bool bc3 = fmt == 77 || fmt == 78;

		if (!bc1 && !bc3)
			return false;

		if (dds_header.dwWidth < 256 || dds_header.dwHeight < 256)
			return false;

		int w = dds_header.dwWidth, h = dds_header.dwHeight;

		TGA_FILE tga(w, h, 24), tga32(w, h, 32);

		for (int y = 0; y < h; y += 4)
		{
			for (int x = 0; x < w; x += 4)
			{
				if (bc1)
				{
					DDS_BLOCK_BC1 block;
					block.decode(fpIn, &tga(x, y, 0), 24, w * 3);
				}
				else
				{
					DDS_BLOCK_BC3 block;
					block.decode(fpIn, &tga32(x, y, 0), w * 4);
				}
			}
		}
		if(bc1)
			tga.save((filename + ".diffuse.tga").c_str());
		else
			tga32.save((filename + ".diffuse.tga").c_str());

		return true;
	}
};

struct ConvertNormalmapDDS
{
	void operator()(std::string filename)
	{
		FILE* fpIn = fopen(filename.c_str(), "rb");
		if (!fpIn)
			return;

		char stamp[4];
		fread(&stamp, 1, 4, fpIn);
		if (stamp[0] != 'D' || stamp[1] != 'D' || stamp[2] != 'S' || stamp[3] != ' ')
		{
			fclose(fpIn);
			return;
		}

		DDS_HEADER dds_header;
		DDS_HEADER_DXT10 dxt10_header;

		fread(&dds_header, 1, sizeof(dds_header), fpIn);
		if (dds_header.ddspf.dwFourCC == MAKEFOURCC('D', 'X', '1', '0'))
		{
			fread(&dxt10_header, 1, sizeof(dxt10_header), fpIn);
		}

		DWORD fmt = dxt10_header.dxgiFormat;
		bool bc3 = fmt == 77 || fmt == 78;

		if (!bc3 || dds_header.dwWidth < 256 || dds_header.dwHeight < 256)
			return;

		printf("%s\n%d x %d, %d mips, fmt: %d\n", strrchr(filename.c_str(), '/') + 1, dds_header.dwWidth, dds_header.dwHeight, dds_header.dwMipMapCount, dxt10_header.dxgiFormat);

		int w = dds_header.dwWidth, h = dds_header.dwHeight;

		TGA_FILE tga(w, h, 32);
		TGA_FILE tga_normal(w, h, 24);
		TGA_FILE tga_gloss(w, h, 24);

		for (int y = 0; y < h; y += 4)
		{
			for (int x = 0; x < w; x += 4)
			{
				DDS_BLOCK_BC3 block;
				block.decode(fpIn, &tga(x, y, 0), w * 4);
			}
		}

		for (int y = 0; y < h; ++y)
		{
			for (int x = 0; x < w; ++x)
			{
				int nx = tga(x, y, 1);
				int ny = tga(x, y, 3);

				float fx = (nx - 128) / 128.0f;
				float fy = (ny - 128) / 128.0f;
				float fz = sqrtf(1.0f - fx * fx - fy * fy);

				int nz = int(fz * 128.0f + 128.0f);
				if (nz < 0) nz = 0;
				if (nz > 255) nz = 255;

				tga_normal(x,y,0) = u8(nz);
				tga_normal(x, y, 1) = ny;
				tga_normal(x, y, 2) = nx;

				tga_gloss(x, y, 0) = tga_gloss(x, y, 1) = tga_gloss(x, y, 2) = tga(x, y, 2);
			}
		}
		tga_normal.save((filename + ".normal.tga").c_str());
		tga_gloss.save((filename + ".gloss.tga").c_str());
	}
};

struct ProcessDDS
{
	void operator()(std::string filename)
	{
		ConvertXBTtoDDS dds;
		if (endswith(filename, "_d.xbt") || endswith(filename, "_d.xbts"))
		{
			if (dds(filename))
			{
				ConvertDiffusemapDDS _d;
				_d(filename + ".dds");
			}
		}
		else if (endswith(filename, "_n.xbt") || endswith(filename, "_n.xbts"))
		{
			if (dds(filename))
			{
				ConvertNormalmapDDS _n;
				_n(filename + ".dds");
			}
		}
	}
};

struct Files
{
	std::vector<std::string> files;
	void operator()(std::string filename)
	{
		if(endswith(filename, ".xbt") || endswith(filename, ".xbts"))
			files.push_back(filename);
	}
};

int main(int argc, char* argv[])
{
	if (argc == 2)
	{
		Files files;
		FolderRecursiveIteration(argv[1], files);

		for (auto it = files.files.begin(); it != files.files.end(); ++it)
		{
			ProcessDDS converter;
			converter(*it);
		}
	}
	return 0;
}