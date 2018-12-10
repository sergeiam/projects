#pragma once

#include <xr/core.h>
#include <xr/vector.h>

namespace xr
{
	class WINDOW
	{
	protected:
		WINDOW() {}
	public:
		class EVENT_HANDLER
		{
		public:
			~EVENT_HANDLER() {}

			virtual void on_paint() {}
			virtual void on_key_down(int key) {}
			virtual void on_key_up(int key) {}
			virtual void on_mouse_move(int x, int y) {}
			virtual void on_mouse_button(int button, bool up) {}
			virtual void on_mouse_wheel(int wheel) {}
			virtual void on_size(int w, int h) {}
			virtual void on_menu(int id) {}
			virtual void on_quit() {}
		};

		virtual ~WINDOW() {}

		virtual bool init(int w, int h) = 0;
		virtual bool update() = 0;
		virtual void redraw() = 0;
		virtual void set_caption(const wchar_t * caption) = 0;
		virtual void exit() = 0;
		virtual void get_handle(void* ptr) const = 0;

		virtual int width() = 0;
		virtual int height() = 0;

		virtual void set_event_handler(EVENT_HANDLER* handler) = 0;

		virtual bool choose_file(const wchar_t * folder, const wchar_t * filter, const wchar_t * caption, bool load, VECTOR<wchar_t>& filename) = 0;

		virtual void add_menu_item(const wchar_t* menu_pathname, int id) = 0;
		virtual void draw_image(int x, int y, int w, int h, const u8 * image, int iw, int ih, int bpp) = 0;
		virtual void draw_text(int x, int y, const char * text, u32 color) = 0;

		enum EMESSAGEBOXTYPE
		{
			MSGBOX_OK,
			MSGBOX_OKCANCEL,
			MSGBOX_YESNO
		};
		virtual bool message_box(const wchar_t * caption, const wchar_t * text, EMESSAGEBOXTYPE type) = 0;

		static WINDOW* create();

	};
}