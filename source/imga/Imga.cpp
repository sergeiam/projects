#include <Windows.h>
#include <windowsx.h>
#include <CommCtrl.h>
#include "Shellapi.h"
#include "resource.h"
#include "Image.h"
#include <stdio.h>


const wchar_t APP_NAME[] = L"Imga";



static HWND		g_hwndMainWindow, g_hwndViewport, g_hwndStaticText;
static Image	g_Image;
static int		g_ToolbarHeight = 26;
static int		g_StaticTextHeight = 24;
static int		g_MarginHeight = 4;
static int		g_ComboHeight = 200;

enum eToolbarButton
{
	ID_STATIC_TEXT = 1000,

	ID_OPEN = 1001,

	ID_FLIP = 1002,
	ID_ROTATE = 1003,

	ID_ZOOM_TO_FIT = 1004,
	ID_ZOOM_TO_ORIGINAL = 1005,

	ID_TILE = 1006,

	ID_RED_CHANNEL = 1007,
	ID_GREEN_CHANNEL = 1008,
	ID_BLUE_CHANNEL = 1009,
	ID_ALPHA_CHANNEL = 1010,

	ID_MIP_COMBO = 1011,
	ID_FACE_COMBO = 1012,
};

enum eToolbarElementType
{
	ePushButton,
	eToggleButton,
	eCombo
};

struct ToolbarElement
{
	const wchar_t* name;
	int	id, width;
	eToolbarElementType type;
	HWND hwnd;
};

enum eToolbarElement
{
	eToolbarOpen,
	eToolbarFlip,
	eToolbarRotate,
	eToolbarZoomToFit,
	eToolbar100,
	eToolbarTile,
	eToolbarRed,
	eToolbarGreen,
	eToolbarBlue,
	eToolbarAlpha,
	eToolbarMip,
	eToolbarFace
};

static ToolbarElement g_Toolbar[] =
{
	{L"Open", ID_OPEN, 50, ePushButton, NULL},
	{L"Flip", ID_FLIP, 50, ePushButton, NULL},
	{L"Rotate", ID_ROTATE, 60, ePushButton, NULL},
	{L"Zoom to Fit", ID_ZOOM_TO_FIT, 90, ePushButton, NULL},
	{L"100%", ID_ZOOM_TO_ORIGINAL, 60, ePushButton, NULL},
	{L"Tile", ID_TILE, 60, eToggleButton, NULL},
	{L"R", ID_RED_CHANNEL, 30, eToggleButton, NULL},
	{L"G", ID_GREEN_CHANNEL, 30, eToggleButton, NULL},
	{L"B", ID_BLUE_CHANNEL, 30, eToggleButton, NULL},
	{L"A", ID_ALPHA_CHANNEL, 30, eToggleButton, NULL},
	{L"Mip", ID_MIP_COMBO, 150, eCombo, NULL},
	{L"Face", ID_FACE_COMBO, 90, eCombo, NULL},
};

static void UpdateStaticText()
{
	wchar_t buf[256] = L"";

	if (g_Image.width())
	{
		int l = wsprintf(buf, L"%d x %d, %d %%", g_Image.width(), g_Image.height(), (g_Image.rect().right - g_Image.rect().left) * 100 / g_Image.width());

		int r, g, b, a;
		if (g_Image.GetHoverPixel(r, g, b, a))
		{
			int x, y;
			g_Image.GetHoverCoords(x, y);
			l += wsprintf(buf + l, L" | Cursor: (%d, %d) %d, %d, %d, %d  #%X", x, y, r, g, b, a, (a << 24U) + (r << 16U) + (g << 8U) + b);
		}
		else
		{
			wsprintf(buf + l, L" | Press Ctrl+click to select pixel color");
		}
	}
	SetDlgItemText(g_hwndMainWindow, ID_STATIC_TEXT, buf);
	InvalidateRect(g_hwndStaticText, NULL, TRUE);
}

static void UpdateToolbar(bool new_image_loaded)
{
	SendMessage(g_Toolbar[eToolbarMip].hwnd, CB_RESETCONTENT, 0, 0);
	for (int i = 0; i < g_Image.mips(); ++i)
	{
		wchar_t buf[64];
		int w, h;
		g_Image.GetMipSize(i, w, h);
		wsprintf(buf, L"Mip %d (%dx%d)", i, w, h);
		SendMessage(g_Toolbar[eToolbarMip].hwnd, CB_ADDSTRING, 0, (LPARAM)buf);
	}
	SendMessage(g_Toolbar[eToolbarMip].hwnd, CB_SETCURSEL, 0, 0);
	EnableWindow(g_Toolbar[eToolbarMip].hwnd, g_Image.mips() > 1);

	SendMessage(g_Toolbar[eToolbarFace].hwnd, CB_RESETCONTENT, 0, 0);
	if (g_Image.faces() == 1)
	{
		SendMessage(g_Toolbar[eToolbarFace].hwnd, CB_ADDSTRING, 0, (LPARAM)L"Image 2D");
	}
	else
	{
		for (int i = 0; i < g_Image.faces(); ++i)
		{
			wchar_t buf[16];
			wsprintf(buf, L"%d", i);
			SendMessage(g_Toolbar[eToolbarFace].hwnd, CB_ADDSTRING, 0, (LPARAM)buf);
		}
	}
	SendMessage(g_Toolbar[eToolbarFace].hwnd, CB_SETCURSEL, 0, 0);
	EnableWindow(g_Toolbar[eToolbarFace].hwnd, g_Image.faces() > 1);

	if (new_image_loaded)
	{
		Button_SetCheck(g_Toolbar[eToolbarRed].hwnd, BST_CHECKED);
		Button_SetCheck(g_Toolbar[eToolbarGreen].hwnd, BST_CHECKED);
		Button_SetCheck(g_Toolbar[eToolbarBlue].hwnd, BST_CHECKED);
		Button_SetCheck(g_Toolbar[eToolbarAlpha].hwnd, BST_UNCHECKED);
	}
}

static LRESULT CALLBACK ImageViewportMessageHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static bool mouse_drag = false;
	static int	lastMouseX, lastMouseY;
	static RECT lastRect;

	switch (msg)
	{
		case WM_CREATE:
			GetClientRect(hWnd, &lastRect);
			break;

		case WM_MOUSEMOVE:
		{
			int x = ((short*)&lParam)[0];
			int y = ((short*)&lParam)[1];

			if (mouse_drag)
			{
				int dx = x - lastMouseX;
				int dy = y - lastMouseY;

				if (dx || dy)
				{
					g_Image.Pan(dx, -dy);
					InvalidateRect(hWnd, NULL, TRUE);
				}
			}
			lastMouseX = x;
			lastMouseY = y;
			break;
		}

		case WM_LBUTTONDOWN:
			if (GetKeyState(VK_CONTROL) >> 15)
			{
				RECT rc;
				GetWindowRect(g_hwndViewport, &rc);
				g_Image.SetHoverCoords(lastMouseX - rc.left, lastMouseY - rc.top);
				UpdateStaticText();
			}
			else
			{
				mouse_drag = true;
				SetCapture(hWnd);
			}
			break;

		case WM_LBUTTONUP:
			mouse_drag = false;
			ReleaseCapture();
			break;

		case WM_SIZE:
		{
			int old_width = lastRect.right - lastRect.left;
			int old_height = lastRect.bottom - lastRect.top;
			GetClientRect(g_hwndViewport, &lastRect);

			int new_width = LOWORD(lParam);
			int new_height = HIWORD(lParam);

			RECT old_rc = g_Image.rect();

			RECT new_rc;

			new_rc.left = old_rc.left * new_width / old_width;
			new_rc.right = old_rc.right * new_width / old_width;
			new_rc.top = old_rc.top * new_height / old_height;
			new_rc.bottom = new_rc.top + (new_rc.right - new_rc.left) * g_Image.height() / g_Image.width();

			g_Image.SetRect(new_rc);
			break;
		}

		case WM_ERASEBKGND:
			return 0;

		case WM_PAINT:
		{
			PAINTSTRUCT	ps;
			RECT		rc;
			HDC			hdc = BeginPaint(hWnd, &ps);

			GetClientRect(hWnd, &rc);
			g_Image.BeginPaint(hdc, rc);

			RECT vr = g_Image.rect();
			LONG vx = vr.right - vr.left;
			LONG vy = vr.bottom - vr.top;

			bool bTile = Button_GetCheck(g_Toolbar[eToolbarTile].hwnd) == BST_CHECKED;
			int tileRadius = bTile ? 1 : 0;

			for (int y = -tileRadius; y <= tileRadius; ++y)
			{
				for (int x = -tileRadius; x <= tileRadius; ++x)
				{
					RECT r;
					r = vr;
					OffsetRect(&r, vx * x, vy * y);
					g_Image.PaintImage(hdc, rc, &r);
				}
			}

			g_Image.EndPaint(hdc, rc);
			EndPaint(hWnd, &ps);
			return 0;
		}
	}
	return DefWindowProcA(hWnd, msg, wParam, lParam);
}

static void UpdateWindowSizes()
{
	int m = g_MarginHeight;
	int th = g_ToolbarHeight;
	int sh = g_StaticTextHeight;

	RECT rect;
	GetClientRect(g_hwndMainWindow, &rect);
	MoveWindow(g_hwndViewport, m, th + 2*m, rect.right - rect.left - 2*m, rect.bottom - rect.top - th - sh - 4*m, TRUE);
	MoveWindow(g_hwndStaticText, m, rect.bottom - sh - m, rect.right - rect.left - 2*m, sh, TRUE);
	InvalidateRect(g_hwndViewport, NULL, TRUE);
	InvalidateRect(g_hwndStaticText, NULL, TRUE);
}

static void RedrawViewport(bool process)
{
	if (process)
	{
		bool red = (SendMessage(g_Toolbar[eToolbarRed].hwnd, BM_GETSTATE, 0, 0) & BST_CHECKED) != 0;
		bool green = (SendMessage(g_Toolbar[eToolbarGreen].hwnd, BM_GETSTATE, 0, 0) & BST_CHECKED) != 0;
		bool blue = (SendMessage(g_Toolbar[eToolbarBlue].hwnd, BM_GETSTATE, 0, 0) & BST_CHECKED) != 0;
		bool alpha = (SendMessage(g_Toolbar[eToolbarAlpha].hwnd, BM_GETSTATE, 0, 0) & BST_CHECKED) != 0;

		if (!red && !green && !blue && !alpha)
		{
			UpdateToolbar(true);
			RedrawViewport(true);
			return;
		}

		g_Image.EnableRedChannel(red);
		g_Image.EnableGreenChannel(green);
		g_Image.EnableBlueChannel(blue);
		g_Image.EnableAlphaChannel(alpha);

		int mip = SendMessage(g_Toolbar[eToolbarMip].hwnd, CB_GETCURSEL, 0, 0);
		int face = SendMessage(g_Toolbar[eToolbarFace].hwnd, CB_GETCURSEL, 0, 0);

		g_Image.SelectSurface(mip, face);
	}
	InvalidateRect(g_hwndViewport, NULL, TRUE);
}

static void OpenImageFile(const wchar_t* filename, HWND hParent)
{
	if (g_Image.Load(filename, hParent))
	{
		RECT rc;
		GetClientRect(g_hwndViewport, &rc);
		g_Image.Fit(rc);

		UpdateStaticText();
		UpdateToolbar(true);
		RedrawViewport(true);

		wchar_t caption[512];
		wsprintf(caption, L"%ls | %ls", APP_NAME, filename);
		SetWindowTextW(hParent, caption);
	}
}

static void OpenImageFileDialog(HWND hParent)
{
	OPENFILENAMEW ofn;       // common dialog box structure
	wchar_t szFile[260] = { 0 };       // if using TCHAR macros

	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hParent;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = L"All (*.*)\0*.*\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn) == TRUE)
	{
		OpenImageFile(ofn.lpstrFile, hParent);
	}
}

static LRESULT CALLBACK MainWindowMessageHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static RECT lastSize;

	switch (msg)
	{
		case WM_CREATE:
			GetClientRect(g_hwndViewport, &lastSize);
			SetWindowText(hWnd, APP_NAME);
			break;

		case WM_SIZE:
		{
			UpdateWindowSizes();
			UpdateStaticText();
			break;
		}

		case WM_KEYDOWN:
			if (wParam == 0x56 && GetKeyState(VK_CONTROL))
			{
				SendMessage(hWnd, WM_PASTE, 0, 0);
			}
			break;

		case WM_PASTE:
			if (OpenClipboard(hWnd))
			{
				HANDLE h = GetClipboardData(CF_DIB);
				if (h != INVALID_HANDLE_VALUE)
				{
					LPBITMAPINFO lpBI = (LPBITMAPINFO)GlobalLock(h);
					if (lpBI)
					{
						uint8* ptr = (unsigned char*)(lpBI + 1);

						int pitch = lpBI->bmiHeader.biWidth * lpBI->bmiHeader.biBitCount / 8;
						if (pitch % 4) pitch += 4 - pitch % 4;

						g_Image.Set(lpBI->bmiHeader.biWidth, lpBI->bmiHeader.biHeight, lpBI->bmiHeader.biBitCount / 8, pitch, ptr, 1, 0, 2, true);

						GlobalUnlock(h);

						RECT rc;
						GetClientRect(g_hwndViewport, &rc);
						g_Image.Fit(rc);

						UpdateToolbar(true);
						UpdateStaticText();
						RedrawViewport(true);
					}
				}

#if 0
				UINT fmt = 0;
				do
				{
					fmt = EnumClipboardFormats(fmt);
					char name[512];
					GetClipboardFormatNameA(fmt, name, 512);
					char buf[128];
					sprintf_s(buf, "Clipboard fmt = %d(%X) (%s)\n", fmt, fmt, name);
					OutputDebugStringA(buf);
				}
				while (fmt);
#endif
				CloseClipboard();
			}
			break;

		case WM_MOUSEWHEEL:
		{
			RECT rc;
			GetWindowRect(g_hwndViewport, &rc);
			float scale = GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? 1.2f : 1.0f / 1.2f;
			g_Image.Zoom(LOWORD(lParam) - rc.left, HIWORD(lParam) - rc.top, scale);
			UpdateStaticText();
			RedrawViewport(false);
			break;
		}

		case WM_COMMAND:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				WORD toolbar_id = LOWORD(wParam);
				switch (toolbar_id)
				{
					case ID_OPEN:
						OpenImageFileDialog(hWnd);
						return 0;

					case ID_FLIP:
						g_Image.Flip();
						UpdateStaticText();
						RedrawViewport(false);
						return 0;

					case ID_ROTATE:
						g_Image.Rotate();
						UpdateStaticText();
						RedrawViewport(false);
						return 0;

					case ID_ZOOM_TO_FIT:
					{
						RECT rc;
						GetClientRect(g_hwndViewport, &rc);
						g_Image.Fit(rc);
						UpdateStaticText();
						RedrawViewport(false);
						break;
					};
					case ID_ZOOM_TO_ORIGINAL:
					{
						RECT rc;
						GetClientRect(g_hwndViewport, &rc);
						g_Image.Fit(rc);

						LONG cx = (rc.left + rc.right) / 2;
						LONG cy = (rc.top + rc.bottom) / 2;

						RECT new_rc;
						new_rc.left = cx - g_Image.width() / 2;
						new_rc.right = g_Image.rect().left + g_Image.width();
						new_rc.top = cy - g_Image.height() / 2;
						new_rc.bottom = g_Image.rect().top + g_Image.height();

						g_Image.SetRect(new_rc);

						UpdateStaticText();
						RedrawViewport(false);
						break;
					};
					case ID_TILE:
						RedrawViewport(false);
						break;

					case ID_RED_CHANNEL:
					case ID_GREEN_CHANNEL:
					case ID_BLUE_CHANNEL:
					case ID_ALPHA_CHANNEL:
						if (GetKeyState(VK_CONTROL) >> 15)
						{
							for (int i = eToolbarRed; i <= eToolbarAlpha; ++i)
							{
								Button_SetCheck(g_Toolbar[i].hwnd, g_Toolbar[i].id == toolbar_id ? BST_CHECKED : BST_UNCHECKED);
							}
						}
						RedrawViewport(true);
						break;
				}
			}
			break;

		case WM_DROPFILES:
		{
			wchar_t wFilename[MAX_PATH];
			if (DragQueryFileW((HDROP)wParam, 0, wFilename, MAX_PATH))
			{
				DragFinish((HDROP)wParam);
				OpenImageFile(wFilename, hWnd);
			}
			break;
		}

		case WM_CLOSE:
		case WM_QUIT:
			DestroyWindow(hWnd);
			PostQuitMessage(0);
			return 0;
		
		case WM_DESTROY:
			break;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

static void CreateToolbar(HINSTANCE hInstance)
{
	for (int i = 0, bx = g_MarginHeight; i < sizeof(g_Toolbar) / sizeof(g_Toolbar[0]); ++i)
	{
		int elementWidth = g_Toolbar[i].width;
		switch (g_Toolbar[i].type)
		{
		case ePushButton:
			g_Toolbar[i].hwnd = CreateWindow(
				L"BUTTON", g_Toolbar[i].name,
				WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
				bx, g_MarginHeight, elementWidth, g_ToolbarHeight, g_hwndMainWindow,
				(HMENU)g_Toolbar[i].id, hInstance, NULL
			);
			break;
		case eToggleButton:
			g_Toolbar[i].hwnd = CreateWindow(
				L"BUTTON", g_Toolbar[i].name,
				WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | BS_PUSHLIKE,
				bx, g_MarginHeight, elementWidth, g_ToolbarHeight, g_hwndMainWindow,
				(HMENU)g_Toolbar[i].id, hInstance, NULL
			);
			break;
		case eCombo:
			g_Toolbar[i].hwnd = CreateWindow(
				WC_COMBOBOX, L"Bla",
				CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE,
				bx, g_MarginHeight, elementWidth, g_ComboHeight, g_hwndMainWindow,
				(HMENU)g_Toolbar[i].id, hInstance, NULL
			);
			break;
		}
		bx += elementWidth + g_MarginHeight;
	}
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
// --- create main window
	{
		const wchar_t* className = L"Imagka class";
		WNDCLASSEX wc = {
			sizeof(WNDCLASSEX),
			CS_CLASSDC | CS_DBLCLKS,
			MainWindowMessageHandler,
			0L, 0L,
			GetModuleHandle(0),
			LoadIcon(hInstance,IDI_APPLICATION),
			LoadCursor(NULL, IDC_ARROW),
			(HBRUSH)COLOR_WINDOW,
			NULL,
			className,
			NULL
		};
		RegisterClassEx(&wc);

		DWORD dwStyle = WS_THICKFRAME | WS_VISIBLE | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CLIPCHILDREN;
		DWORD dwExStyle = 0;

		RECT desktop_rect;
		::GetClientRect(::GetDesktopWindow(), &desktop_rect);

		LONG w = (desktop_rect.right - desktop_rect.left) / 2;
		LONG h = (desktop_rect.bottom - desktop_rect.top) / 2;
		LONG x = w / 2, y = h / 2;

		g_hwndMainWindow = CreateWindowEx(dwExStyle, className, NULL, dwStyle, x, y, w, h, NULL, NULL, GetModuleHandle(NULL), NULL);

		CreateToolbar(hInstance);

// --- create bottom static text
		g_hwndStaticText = CreateWindow(
			L"EDIT", L"",
			WS_VISIBLE | WS_CHILD | ES_LEFT | ES_READONLY | WS_BORDER,
			0, 0, 0, 0,
			g_hwndMainWindow,
			(HMENU)ID_STATIC_TEXT,
			hInstance,
			NULL
		);
	}
// --- create viewport
	{
		const char* className = "Imga viewport class";
		WNDCLASSEXA wc = {
			sizeof(WNDCLASSEX),
			CS_CLASSDC | CS_DBLCLKS,
			ImageViewportMessageHandler,
			0L, 0L,
			GetModuleHandle(0),
			NULL,
			LoadCursor(NULL, IDC_ARROW),
			(HBRUSH)COLOR_WINDOW,
			NULL,
			className,
			NULL
		};
		RegisterClassExA(&wc);

		DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
		DWORD dwExStyle = 0;

		g_hwndViewport = CreateWindowExA(dwExStyle, className, NULL, dwStyle, 0, 0, 0, 0, g_hwndMainWindow, NULL, GetModuleHandle(NULL), NULL);
	}

// --- initialization

	ShowWindow(g_hwndMainWindow, nCmdShow);
	SetFocus(g_hwndMainWindow);
	UpdateWindowSizes();
	UpdateToolbar(true);
	UpdateStaticText();

	DragAcceptFiles(g_hwndMainWindow, TRUE);

// --- message pump

	MSG msg;
	while (GetMessageA(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}
	return 0;
}