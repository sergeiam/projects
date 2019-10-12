#include <xr/sprite_render_window.h>
#include <math.h>
#include <vector>
#include <Windows.h>

float randf(float a, float b)
{
	return a + (b - a) * (rand() * (1.0f / RAND_MAX));
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	xr::SPRITE_RENDER_WINDOW* device = xr::SPRITE_RENDER_WINDOW::create();
	if (!device)
		return 1;

	if (!device->create_window(800, 600, L"Random level generation test"))
		return 2;

	const int TEX_DOGGY = device->load_texture("textures/doggy.tga");

	float cx = 400.0f, cy = 400.0f;
	float px = cx, py = cy;
	float vx = 0.0f, vy = 0.0f;

	for (;;)
	{
		device->set_texture(TEX_DOGGY);
		device->draw_sprite(cx, cy, 0.0f, 0.0f, 50, 50, 1.0f);
		device->draw_sprite(px, py, 0.0f, 0.0f, 50, 50, 1.0f);

		px += vx;
		py += vy;

		vx += (cx - px) * 0.03f;
		vy += (cy - py) * 0.03f;

		vx *= 0.97f;
		vy *= 0.97f;

		if (device->key_pressed(32))
		{
			px = cx + randf(-130.0f, 130.0f);
			py = cy + randf(-130.0f, 130.0f);
		}
		device->present_frame();
	}
	delete device;

	return 0;
}