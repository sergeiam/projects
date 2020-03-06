#pragma once

#include <vector>

typedef unsigned char uint8;
typedef __int64 int64;
typedef unsigned long uint32;

struct Image
{
	Image();
	~Image();

	bool Load(const wchar_t* filename, HWND hwndParent);
	void Set(int w, int h, int channels, int row_size, const void* bits, int Ridx, int Gidx, int Bidx, bool flip_y);
	void Zoom(int x, int y, float scale);
	void Pan(int dx, int dy);
	void Fit(RECT clientRect);
	void Rotate();
	void Flip();
	void Test();
	void Reset();

	void BeginPaint(HDC hdc, RECT clientRect);
	void PaintImage(HDC hdc, RECT clientRect, RECT* pCustomRect = nullptr);
	void EndPaint(HDC hdc, RECT clientRect);

	int  width() const { return m_width; }
	int	 height() const { return m_height; }
	int  channels() const { return m_channels; }
	RECT rect() const { return m_rect; }
	int	 mips() const { return m_mips; }
	int  faces() const { return m_faces; }

	void GetMipSize(int mip, int& w, int& h) const;

	void SetRect(RECT& rc);

	void SetHoverCoords(int x, int y);
	void GetHoverCoords(int& x, int& y);
	bool GetHoverPixel(int& r, int& g, int& b, int& alpha);

	void EnableRedChannel(bool on);
	void EnableGreenChannel(bool on);
	void EnableBlueChannel(bool on);
	void EnableAlphaChannel(bool on);

	void SelectSurface(int mip, int face);

private:
	struct Surface
	{
		uint8*	data;
		int		width, height;
		int		mip, face;
	};
	std::vector<Surface>	m_surfaces;

	uint8*			m_image;
	int				m_width;
	int				m_height;
	int				m_surface;

	int				m_faces, m_mips;
	int				m_channels;

	RECT			m_rect;
	int				m_hoverX, m_hoverY;

	bool	m_Red, m_Green, m_Blue, m_Alpha;

	HDC				m_hDC;
	HBITMAP			m_hBitmap;
	BITMAPINFO		m_BitmapInfo;
	uint8*			m_BitmapPtr;
};