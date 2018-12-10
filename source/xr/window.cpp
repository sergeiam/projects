#include <xr/window.h>
#include <xr/vector.h>
#include <string.h>
#include <windows.h>

namespace xr
{

LRESULT CALLBACK main_window_procedure(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

class WINDOW_WIN32;
static VECTOR<WINDOW_WIN32*>	s_windows;



class WINDOW_WIN32 : public WINDOW
{
public:
	WINDOW_WIN32() : m_event_handler(nullptr), m_quit(false)
	{
		s_windows.push_back(this);
	}
	virtual ~WINDOW_WIN32()
	{
		for( int i=0; i<s_windows.size(); ++i )
			if (s_windows[i] == this) {
				s_windows[i] = s_windows.back();
				s_windows.resize(s_windows.size() - 1);
				break;
			}
	}
	virtual bool init(int w, int h)
	{
		static char window_class[] = "XR_WINDOW_WIN32_CLASS";

		HINSTANCE instance = ::GetModuleHandle(NULL);
		WNDCLASSEXA wcex = {
			sizeof(WNDCLASSEXA),
			CS_CLASSDC | CS_DBLCLKS,
			main_window_procedure,
			0, 0,
			instance,
			0, 0, 0, 0,
			window_class,
			0
		};
		wcex.hIcon = LoadIcon(instance, MAKEINTRESOURCE(IDI_APPLICATION));
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex.hIconSm = LoadIcon(instance, MAKEINTRESOURCE(IDI_APPLICATION));
		if (!RegisterClassExA(&wcex))
			return false;

		RECT desktop_rect;
		::GetClientRect(::GetDesktopWindow(), &desktop_rect);

		DWORD style = WS_OVERLAPPED | WS_BORDER | WS_VISIBLE | WS_SYSMENU | WS_SIZEBOX | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CAPTION;

		m_window_handle = CreateWindowExA(
			0, window_class, (const char*)L"", style,
			(desktop_rect.right - w) / 2, (desktop_rect.bottom - h) / 2, w, h,
			NULL, NULL, instance, NULL
		);
		if (!m_window_handle)
			return false;

		ShowWindow(m_window_handle, SW_SHOW);
		SetFocus(m_window_handle);
		UpdateWindow(m_window_handle);

		return true;
	}
	virtual bool update()
	{
		MSG msg;
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		return !m_quit;
	}
	virtual void redraw()
	{
		::InvalidateRect(m_window_handle, NULL, TRUE);
		::UpdateWindow(m_window_handle);
	}
	virtual void set_caption(const wchar_t * caption)
	{
		::SetWindowTextA(m_window_handle, (const char*)caption);
	}
	virtual void exit()
	{
		::PostQuitMessage(0);
	}
	virtual int width()
	{
		return m_width;
	}
	virtual int height()
	{
		return m_height;
	}
	virtual void get_handle(void* ptr) const
	{
		*((HWND*)ptr) = m_window_handle;
	}
	virtual void set_event_handler(EVENT_HANDLER* handler)
	{
		m_event_handler = handler;
	}
	virtual bool choose_file(const wchar_t * folder, const wchar_t * filter, const wchar_t * caption, bool load, VECTOR<wchar_t>& filename)
	{
		wchar_t wBuffer[1024] = { 0 };

		OPENFILENAME ofn = { 0 };
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = m_window_handle;
		ofn.lpstrTitle = caption;
		ofn.lpstrFile = wBuffer;
		ofn.nMaxFile = 1024;
		ofn.lpstrInitialDir = folder;
		ofn.lpstrFilter = filter;
		ofn.Flags = OFN_DONTADDTORECENT | OFN_ENABLESIZING;
		if (load)
			ofn.Flags |= OFN_FILEMUSTEXIST;

		BOOL ret = load ? GetOpenFileName(&ofn) : GetSaveFileName(&ofn);
		if (!ret) wBuffer[0] = L'\0';
		filename.resize(wcslen(wBuffer));
		memcpy(&filename[0], wBuffer, filename.size() * sizeof(wchar_t));
		return !filename.empty();
	}


	struct MENU_ITEM
	{
		VECTOR<wchar_t> name;
		HMENU			handle;
	};
	VECTOR<MENU_ITEM> m_menus;

	virtual void add_menu_item(const wchar_t* menu_pathname, int id)
	{
		if (m_menus.empty()) {
			MENU_ITEM m;
			m.handle = ::CreateMenu();
			::SetMenu(m_window_handle, m.handle);
			m_menus.push_back(m);
		}

		HMENU prev = m_menus[0].handle;
		for (size_t i = 0, n = wcslen(menu_pathname); i < n; )
		{
			const wchar_t* next = wcschr(menu_pathname + i, L'/');
			int j = next ? next - menu_pathname : n;

			bool found = false;
			for (int k = 1; k <= m_menus.size(); ++k)
			{
				if (k < m_menus.size() && m_menus[k].name.size() == j+1 && !memcmp(menu_pathname, &m_menus[k].name[0], j*2)) {
					prev = m_menus[k].handle;
					break;
				}
				if (k == m_menus.size())
				{
					MENU_ITEM m;
					m.name.resize(j+1);
					memcpy(&m.name[0], menu_pathname, j * sizeof(wchar_t));
					m.name[j] = L'\0';

					wchar_t item_name[256];
					memcpy(item_name, menu_pathname + i, (j - i) * 2);
					item_name[j - i] = L'\0';

					if (j < n) {
						m.handle = ::CreateMenu();
						::AppendMenu(prev, MF_STRING | MF_POPUP, (UINT_PTR)m.handle, item_name);
						prev = m.handle;
					}
					else {
						m.handle = 0;
						::AppendMenu(prev, MF_STRING, (UINT_PTR)id, item_name);
					}
					m_menus.push_back(m);
					break;
				}
			}
			i = j + 1;
		}
		::DrawMenuBar(m_window_handle);
	}
	void draw_image(int x, int y, int w, int h, const u8 * image, int iw, int ih, int bpp)
	{
		if (!m_paint_handle)
			return;
		static BITMAPINFO bmp = { { sizeof(BITMAPINFOHEADER), 0, 0, 1, bpp, BI_RGB, 0, 0, 0, 0, 0 } };
		bmp.bmiHeader.biWidth = iw;
		bmp.bmiHeader.biHeight = ih;
		StretchDIBits(m_paint_handle, x, y, w, h, 0, 0, iw, ih, image, &bmp, DIB_RGB_COLORS, SRCCOPY);
	}
	void draw_text(int x, int y, const char * text, u32 color)
	{
		if (!m_paint_handle)
			return;
		SetBkMode(m_paint_handle, TRANSPARENT);
		SetTextColor(m_paint_handle, color);
		TextOutA(m_paint_handle, x, y, text, (int)strlen(text));
	}
	virtual bool message_box(const wchar_t * caption, const wchar_t * text, EMESSAGEBOXTYPE type)
	{
		UINT mb_type;
		switch (type)
		{
			case MSGBOX_OK: mb_type = MB_OK; break;
			case MSGBOX_OKCANCEL: mb_type = MB_OKCANCEL; break;
			case MSGBOX_YESNO: mb_type = MB_YESNO; break;
		}
		auto ret = ::MessageBox(m_window_handle, text, caption, mb_type);
		return ret == IDYES || ret == IDOK;
	}

	::HWND			m_window_handle;
	::HDC			m_paint_handle;
	EVENT_HANDLER*	m_event_handler;
	bool			m_quit;
	int				m_width, m_height;
};

WINDOW* WINDOW::create()
{
	return new WINDOW_WIN32();
}

static LRESULT CALLBACK main_window_procedure(HWND handle, UINT message, WPARAM wparam, LPARAM lparam)
{
	int mouseup = 0, mousedown = 0, mousedblclick = 0, mousewheel = 0;

	xr::WINDOW_WIN32* window = nullptr;

	for (int i = 0; i < xr::s_windows.size(); ++i)
	{
		if (xr::s_windows[i]->m_window_handle == handle)
		{
			window = xr::s_windows[i];
			break;
		}
	}
	if (!window)
		return DefWindowProc(handle, message, wparam, lparam);

	xr::WINDOW::EVENT_HANDLER* event_handler = window->m_event_handler;

	switch (message)
	{
	case WM_KEYDOWN:
		if(event_handler) event_handler->on_key_down((int)wparam);
		break;

	case WM_KEYUP:
		if (event_handler) event_handler->on_key_up((int)wparam);
		break;

	case WM_MOUSEMOVE:
		if (event_handler) event_handler->on_mouse_move(LOWORD(lparam), HIWORD(lparam));
		break;

	case WM_LBUTTONDOWN: if (event_handler) event_handler->on_mouse_button(0, false); break;
	case WM_LBUTTONUP: if (event_handler) event_handler->on_mouse_button(0, true); break;
	case WM_RBUTTONDOWN: if (event_handler) event_handler->on_mouse_button(1, false); break;
	case WM_RBUTTONUP: if (event_handler) event_handler->on_mouse_button(1, true); break;
	case WM_MOUSEWHEEL: if (event_handler) event_handler->on_mouse_wheel(GET_WHEEL_DELTA_WPARAM(wparam)); break;

	case WM_PAINT:
	{
		if (event_handler)
		{
			PAINTSTRUCT ps;
			window->m_paint_handle = BeginPaint(handle, &ps);
			event_handler->on_paint();
			EndPaint(handle, &ps);
			window->m_paint_handle = NULL;
		}
		break;
	}
	case WM_ERASEBKGND:
		return 0;

	case WM_SIZE:
	{
		RECT rc;
		GetClientRect(handle, &rc);

		window->m_width = rc.right - rc.left;
		window->m_height = rc.bottom - rc.top;
		if (event_handler) event_handler->on_size(window->m_width, window->m_height);
		return 0;
	}
	case WM_COMMAND:
		if (HIWORD(wparam) == 0 && event_handler) {
			event_handler->on_menu(LOWORD(wparam));
		}
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_QUIT:
	case WM_CLOSE:
		if (event_handler) event_handler->on_quit();
		window->m_quit = true;
		return 0;
	}

	return DefWindowProc(handle, message, wparam, lparam);
}

} // xr