#pragma once

namespace xr
{
	class SPRITE_RENDER_WINDOW
	{
	public:
		virtual ~SPRITE_RENDER_WINDOW() {}

		virtual bool	create_window(int w, int h, const wchar_t* caption) = 0;
		virtual int		load_texture(const char* filename) = 0;
		virtual void	load_text_texture(const char* filename, int columns, int rows, int first_index, int pixels_to_shrink_per_letter) = 0;

		virtual void	set_texture(int texture) = 0;
		virtual void	set_color(float r, float g, float b) = 0;
		virtual void	draw_sprite(float x, float y, float z, float angle, float w, float h, float alpha = 1.0f, float u1 = 0.0f, float v1 = 0.0f, float u2 = 1.0f, float v2 = 1.0f) = 0;
		virtual void	draw_text(const char* text, float x, float y, float z, float cw, float ch, float alpha = 1.0f) = 0;
		virtual void	present_frame() = 0;

		virtual bool	key_pressed(int code) = 0;
		virtual bool	key_held(int code) = 0;
		virtual int		mouse_x() = 0;
		virtual int		mouse_y() = 0;
		virtual bool	mouse_button(int button) = 0;

		virtual float	time_sec() = 0;

		static SPRITE_RENDER_WINDOW* create();
	};
};