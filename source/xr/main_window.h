#pragma once

namespace xr
{
	class Window {
	public:
		virtual ~Window() {}
		virtual void SendCommand(int idx);

	protected:
		void * m_pHandle;
	};

	class MAIN_WINDOW
	{
	public:
		MAIN_WINDOW();
		virtual ~MAIN_WINDOW();

		bool init(int w, int h);
		bool update();
		void set_caption(const char * caption);
		void exit();

		virtual void on_key_down(int key) {}
		virtual void on_key_up(int key) {}
		virtual void on_mouse(int x, int y) {}
		virtual void on_mouse_button(int button, bool up) {}
		virtual void on_mouse_wheel(int wheel) {}
		virtual void on_size(int w, int h) {}
		virtual void on_menu(int id) {}
		virtual void on_quit() {}

		int width();
		int height();

		void add_menu_item(const char* menu, int menu_command_id);

		std::wstring ChooseFile(const wchar_t * pwFolder, const wchar_t * pwFilter, const wchar_t * pwCaption, bool bLoad);
		//void  DrawImage(int x, int y, int w, int h, const uint8 * pImage, int iw, int ih, int bpp);
		//void  DrawText(int x, int y, const char * pcText, uint32 color);

		int CreateMenu();
		void AddSubmenu(int menu, const wchar_t * pcName, int submenu);
		void AddMenuItem(int menu, const wchar_t * pcName, int id);
		void ClearMenu(int menu);

		enum EMESSAGEBOXTYPE
		{
			MSGBOX_OK,
			MSGBOX_OKCANCEL,
			MSGBOX_YESNO
		};
		bool message_box(const wchar_t * caption, const wchar_t * text, EMESSAGEBOXTYPE type);
	};
}