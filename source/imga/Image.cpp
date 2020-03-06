#include <Windows.h>
#include "Image.h"
#include "ImageDxtc.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

template< class T > void swap(T& a, T& b)
{
	T temp = a;
	a = b;
	b = temp;
}


Image::Image()
{
	m_image = nullptr;
	m_hDC = NULL;
	m_hBitmap = NULL;
	m_BitmapInfo.bmiHeader.biWidth = m_BitmapInfo.bmiHeader.biHeight = -1;
	m_hoverX = m_hoverY = -100000;
}

Image::~Image()
{
	DeleteObject(m_hBitmap);
	DeleteObject(m_hDC);
	delete[] m_image;
}

bool Image::Load(const wchar_t* image, HWND hwndParent)
{
	FILE* fp = nullptr;
	if(_wfopen_s(&fp, image, L"rb"))
	{
		return false;
	}

	int w, h, channels;
	uint8* pixels = nullptr;

	if (!wcscmp(image + wcslen(image) - 4, L".dds"))
	{
		pixels = load_dxtc(fp, w, h);
	}
	else
	{
		pixels = stbi_load_from_file(fp, &w, &h, &channels, 0);
	}

	fclose(fp);
	if (!pixels)
	{
		return false;
	}

	Set(w, h, channels, 0, pixels, 0, 1, 2, false);

	free(pixels);
	return true;
}

static void CopyImageToImageRGBA(int w, int h, int src_channels, const void* src, const void* dst, int ri, int gi, int bi, int ai)
{
	if (ri >= src_channels) ri = -1;
	if (gi >= src_channels) gi = -1;
	if (bi >= src_channels) bi = -1;
	if (ai >= src_channels) ai = -1;

	for (int y = 0; y < h; ++y)
	{
		const uint8* psrc = (const uint8*)src + y * w * src_channels;
		uint8* pdst = (uint8*)dst + y * w * 4;

		for (int x = 0; x < w; ++x, pdst += 4, psrc += src_channels )
		{
			pdst[0] = (ri >= 0) ? psrc[bi] : 0;
			pdst[1] = (gi >= 0) ? psrc[gi] : 0;
			pdst[2] = (ri >= 0) ? psrc[ri] : 0;
			pdst[3] = (ai >= 0) ? psrc[ai] : 0xFF;
		}
	}
}

void Image::SelectSurface(int mip, int face)
{
	m_surface = 0;

	if (mip < 0) mip = 0;
	if (face < 0) face = 0;

	int channels_enabled = 0, single_channel = -1;
	if (m_Red) { channels_enabled++; single_channel = 2; }
	if (m_Green) { channels_enabled++; single_channel = 1; }
	if (m_Blue) { channels_enabled++; single_channel = 0; }
	if (m_Alpha) { channels_enabled++; single_channel = 3; }

	for (Surface& s : m_surfaces)
	{
		if (s.mip == mip && s.face == face)
		{
			m_width = s.width;
			m_height = s.height;

			delete[] m_image;
			m_image = new uint8[m_width * m_height * 4];
			memset(m_image, 0, m_width * m_height * 4);

			int r = m_Red ? 2 : (channels_enabled == 1 ? single_channel : -1);
			int g = m_Green ? 1 : (channels_enabled == 1 ? single_channel : -1);
			int b = m_Blue ? 0 : (channels_enabled == 1 ? single_channel : -1);
			int a = m_Alpha ? 3 : (channels_enabled == 1 ? single_channel : -1);

			CopyImageToImageRGBA(m_width, m_height, m_channels, s.data, m_image, r, g, b, a);
			return;
		}
		m_surface++;
	}

	m_surface = 0;
}

void Image::Set(int w, int h, int channels, int row_size, const void* bits, int Ridx, int Gidx, int Bidx, bool flip_y)
{
	Reset();

	Surface surf;
	surf.data = new uint8[w * h * channels];

	uint8* dst = surf.data;

	if (!row_size) row_size = w * channels;

	for (int y = 0; y < h; ++y)
	{
		const uint8* src = (const uint8*)bits + (flip_y ? h - y - 1 : y)*row_size;
		for (int x = 0; x < w; ++x, dst += channels, src += channels )
		{
			dst[0] = src[Bidx];
			if (channels > 1)
			{
				dst[1] = src[Gidx];
				if (channels > 2)
				{
					dst[2] = src[Ridx];
					if (channels > 3)
					{
						dst[3] = src[3];
					}
				}
			}
		}
	}

	surf.width = w;
	surf.height = h;
	surf.mip = 0;
	surf.face = 0;
	m_surfaces.push_back(surf);

	m_channels = channels;
	m_faces = 1;
	m_mips = 1;

	SelectSurface(0, 0);
}

void Image::Zoom(int x, int y, float scale)
{
	if (m_image)
	{
		RECT rc = m_rect;
		m_rect.left = x + (rc.left - x) * scale;
		m_rect.right = x + (rc.right - x) * scale;
		m_rect.top = y + (rc.top - y) * scale;
		// keep the aspect, so compute the last rectangle coordinate explicitly
		m_rect.bottom = m_rect.top + m_height * (m_rect.right - m_rect.left) / m_width;
	}
}

void Image::Pan(int dx, int dy)
{
	m_rect.left += dx;
	m_rect.right += dx;
	m_rect.top -= dy;
	m_rect.bottom -= dy;
}

void Image::Fit(RECT clientRect)
{
	int cw = clientRect.right - clientRect.left;
	int ch = clientRect.bottom - clientRect.top;
	int w = m_width;
	int h = m_height;

	if (cw / ch > w / h)
	{
		int nw = w * ch / h;
		m_rect.left = (clientRect.left + clientRect.right) / 2 - nw/2;
		m_rect.right = m_rect.left + nw;
		m_rect.top = 0;
		m_rect.bottom = ch;
	}
	else
	{
		int nh = h * cw / w;
		m_rect.top = (clientRect.top + clientRect.bottom) / 2 - nh / 2;
		m_rect.bottom = m_rect.top + nh;
		m_rect.left = 0;
		m_rect.right = cw;
	}
}

void Image::BeginPaint(HDC hdc, RECT clientRect)
{
	if (!m_hDC)
	{
		m_hDC = CreateCompatibleDC(hdc);
	}

	LONG cw = clientRect.right - clientRect.left;
	LONG ch = clientRect.bottom - clientRect.top;

	if (!m_hBitmap || m_BitmapInfo.bmiHeader.biWidth != cw || m_BitmapInfo.bmiHeader.biHeight != ch)
	{
		if (m_hBitmap) DeleteObject(m_hBitmap);

		BITMAPINFOHEADER& bih = m_BitmapInfo.bmiHeader;

		bih.biBitCount = 32;
		bih.biCompression = BI_RGB;
		bih.biPlanes = 1;
		bih.biSize = sizeof(BITMAPINFOHEADER);
		bih.biSizeImage = 0;
		bih.biClrImportant = bih.biClrUsed = 0;
		bih.biWidth = cw;
		bih.biHeight = -ch;

		m_hBitmap = CreateDIBSection(m_hDC, &m_BitmapInfo, DIB_RGB_COLORS, (void**)&m_BitmapPtr, NULL, 0);
	}

	memset(m_BitmapPtr, 0, cw * ch * 4);
}

void Image::PaintImage(HDC hdc, RECT clientRect, RECT* pCustomRect)  // (0,0,w,h)
{
	if (!m_image)
	{
		return;
	}

	LONG cw = clientRect.right - clientRect.left;
	LONG ch = clientRect.bottom - clientRect.top;

	RECT vrect = pCustomRect ? *pCustomRect : m_rect;

	RECT clip;
	clip.left = max(clientRect.left, vrect.left - clientRect.left);
	clip.top = max(clientRect.top, vrect.top - clientRect.top);
	clip.right = min(clientRect.right, vrect.right - clientRect.left);
	clip.bottom = min(clientRect.bottom, vrect.bottom - clientRect.top);

	int rw = vrect.right - vrect.left;
	int rh = vrect.bottom - vrect.top;

	LONG x0 = LONG(int64(clip.left - vrect.left) * m_width * 65536 / rw);
	LONG xstep = LONG(m_width * 65536 / rw);

	for (int dy = clip.top; dy < clip.bottom; ++dy)
	{
		unsigned char* dest = m_BitmapPtr + (dy * cw + clip.left) * 4;
		unsigned char* src = m_image + ((dy - vrect.top) * m_height / rh) * m_width * 4;

		LONG x = x0;
		for (int dx = clip.left; dx < clip.right; ++dx, dest += 4)
		{
			unsigned char* s = src + (x >> 16) * 4;

			dest[0] = s[0];
			dest[1] = s[1];
			dest[2] = s[2];

			x += xstep;
		}
	}
}

void Image::EndPaint(HDC hdc, RECT clientRect)
{
	SelectObject(m_hDC, m_hBitmap);
	BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, m_hDC, 0, 0, SRCCOPY);
}

void Image::Rotate()
{
	uint32* temp = new uint32[m_width * m_height];

	int w = m_width, h = m_height;
	for (int y = 0; y < h; ++y)
	{
		uint32* row = (uint32*)m_image + y * w;
		uint32* col = temp + y + (m_width-1)*m_height;

		for (int x = 0; x < w; ++x, ++row, col -= h)
		{
			*col = *row;
		}
	}

	delete[] m_image;
	m_image = (unsigned char*)temp;

	swap(m_width, m_height);

	LONG rw = m_rect.right - m_rect.left;
	LONG rh = m_rect.bottom - m_rect.top;

	m_rect.right = m_rect.left + rh;
	m_rect.bottom = m_rect.top + rw;
}

void Image::Flip()
{
	for (int y = 0; y < m_height; ++y)
	{
		uint32* row1 = (uint32*)m_image + y * m_width;
		uint32* row2 = row1 + m_width - 1;

		for (int x = 0; x < m_width / 2; ++x, ++row1, --row2)
		{
			swap(*row1, *row2);
		}
	}
}

void Image::Test()
{
	uint8* new_image = new uint8[m_width * m_height * 4];
	memset(new_image, 0, m_width * m_height * 4);

	uint8* ptr = new_image;
	for (int y = 0; y < m_height; ++y)
	{
		for (int x = 0; x < m_width; ++x, ptr += 4)
		{
			float dx = x - m_width / 2;
			float dy = y - m_height / 2;

			if (dx * dx + dy * dy > m_width* m_width / 4.0f) continue;

			float a1 = atan2(dy, dx);

			float a2 = a1 * 37.0f / 38.0f;

			float s = sinf(a2 - a1), c = cosf(a2 - a1);

			float nx = m_width / 2 + dx * c - dy * s;
			float ny = m_height / 2 + dx * s + dy * c;

			const uint8* src = m_image + (int(nx) + int(ny) * m_width) * 4;

			ptr[0] = src[0];
			ptr[1] = src[1];
			ptr[2] = src[2];
		}
	}

	delete[] m_image;
	m_image = new_image;
}

void Image::SetRect(RECT& rc)
{
	m_rect = rc;
}

void Image::SetHoverCoords(int x, int y)
{
	if (m_image)
	{
		m_hoverX = (x - m_rect.left) * width() / (m_rect.right - m_rect.left);
		m_hoverY = (y - m_rect.top) * height() / (m_rect.bottom - m_rect.top);
	}
}

void Image::GetHoverCoords(int& x, int& y)
{
	x = m_hoverX;
	y = m_hoverY;
}

bool Image::GetHoverPixel(int& r, int& g, int& b, int& alpha)
{
	if (m_hoverX < 0 || m_hoverY < 0 || m_hoverX >= m_width || m_hoverY >= m_height)
	{
		return false;
	}

	const uint8* ptr = m_image + (m_hoverX + m_hoverY * m_width) * 4;

	r = g = b = 0;
	alpha = 255;

	b = ptr[0];
	if (m_channels > 1)
	{
		g = ptr[1];
		if (m_channels > 2)
		{
			r = ptr[2];
			if (m_channels > 3)
			{
				alpha = ptr[3];
			}
		}
	}
	return true;
}

void Image::Reset()
{
	for (Surface& s : m_surfaces)
	{
		delete[] s.data;
	}
	m_surfaces.clear();
	delete[] m_image;
	m_image = nullptr;
	m_width = m_height = 0;
}

void Image::EnableRedChannel(bool on)
{
	m_Red = on;
}

void Image::EnableGreenChannel(bool on)
{
	m_Green = on;
}

void Image::EnableBlueChannel(bool on)
{
	m_Blue = on;
}

void Image::EnableAlphaChannel(bool on)
{
	m_Alpha = on;
}

void Image::GetMipSize(int mip, int& w, int& h) const
{
	w = h = 0;
	for (const Surface& s : m_surfaces)
	{
		if (s.mip == mip)
		{
			w = s.width;
			h = s.height;
			return;
		}
	}
}