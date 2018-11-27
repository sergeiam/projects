#include <xr/window.h>
#include <Windows.h>

enum EMENU_IDS
{
	MENU_FILE_OPEN,
	MENU_FILE_SAVE,
	MENU_VIEW_WIREFRAME,
	MENU_VIEW_SOLID
};

struct MODEL_VIEWER : public xr::WINDOW::EVENT_HANDLER
{
	MODEL_VIEWER() : m_main_window(nullptr) {}
	~MODEL_VIEWER()
	{
		delete m_main_window;
	}

	bool init()
	{
		m_main_window = xr::WINDOW::create();
		if (!m_main_window)
			return false;

		m_main_window->init(800, 600);
		m_main_window->set_event_handler(this);
		m_main_window->set_caption(L"Model viewer v0.1 (c) Sergey Miloykov");

		m_main_window->add_menu_item(L"File/Open", MENU_FILE_OPEN);
		m_main_window->add_menu_item(L"File/Save", MENU_FILE_SAVE);
		m_main_window->add_menu_item(L"View/Wireframe", MENU_VIEW_WIREFRAME);
		m_main_window->add_menu_item(L"View/Solid shaded", MENU_VIEW_SOLID);
		return true;
	}

	virtual void on_paint() {}
	virtual void on_key_down(int key) {}
	virtual void on_key_up(int key) {}
	virtual void on_mouse_move(int x, int y) {}
	virtual void on_mouse_button(int button, bool up) {}
	virtual void on_mouse_wheel(int wheel) {}
	virtual void on_size(int w, int h) {}

	virtual void on_menu(int id)
	{
		switch (id)
		{
		case MENU_FILE_OPEN:
			file_open();
			break;
		case MENU_FILE_SAVE:
			break;
		case MENU_VIEW_WIREFRAME:
			break;
		case MENU_VIEW_SOLID:
			break;
		}
	}

	virtual void on_quit() {}

	bool update()
	{
		return m_main_window->update();
	}

	void file_open()
	{
		xr::VECTOR<wchar_t> filename;
		if (!m_main_window->choose_file(L"", L"*.obj", L"Choose model file", true, filename))
			return;


	}

	xr::WINDOW* m_main_window;
};

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, int cmd_show)
{
	MODEL_VIEWER * mv = new MODEL_VIEWER();
	if (!mv->init())
		return -1;

	while(mv->update());

	delete mv;
	return 0;
}