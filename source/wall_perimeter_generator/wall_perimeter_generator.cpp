#include <xr/sprite_render_window.h>
#include <xr/math.h>
#include <xr/vector.h>
#include <math.h>
#include <Windows.h>

#define WIDTH 1200
#define HEIGHT 800

#define PI 3.141528f

class PERIMETER : public xr::VECTOR<Vec2>
{
public:
	PERIMETER() {}

	u8 get_point_available_directions_mask(int index) const
	{
		int next = step(index, 1);
		int prev = step(index, -1);

		Vec2 edge_next = (*this)[next] - (*this)[index];
		Vec2 edge_prev = (*this)[prev] - (*this)[index];

		int dir_next = get_direction(edge_next);
		int dir_prev = get_direction(edge_prev);

		u8 m1 = get_prev_4_dirs(dir_next), m2 = get_next_4_dirs(dir_prev);

		return m1 | m2;
	}

	bool advance_perimeter( float dx, float dy, int i, int j)
	{
		if (i < 0 || j < 0) return false;
		if (i == j) return false;

		u8 mask = get_point_available_directions_mask(i) & get_point_available_directions_mask(j);
		if (!mask) return false;

		int dir = get_direction(Vec2(dx, dy));

		if (!(mask & (1 << dir))) return false;

		Vec2 next = (*this)[step(i, 1)] - (*this)[i];
		Vec2 prev = (*this)[step(i, -1)] - (*this)[i];

		int s1 = (dot(next, Vec2(dx, dy)) > dot(prev, Vec2(dx,dy))) ? +1 : -1;

		//float ex = P[i].x - P[j].x, ey = P[i].y - P[j].y;
		//if (fabsf(ex*dx + ey*dy) < 0.00001f) continue;
		//for (int i = 0, pi = P.size()-1; i < P.size(); ++i)
		//{
		//}

		xr::VECTOR<Vec2> p1, p2;

		for (int k = i; k != j; k = step(k, s1))
			p1.push_back((*this)[k]);
		p1.push_back((*this)[j]);

		for (int k = j; k != i; k = step(k, s1))
			p2.push_back((*this)[k]);
		p2.push_back((*this)[i]);

		for (int k = 0; k < p1.size(); ++k)
			p1[k].x += dx, p1[k].y += dy;

		clear();
		append(p1.begin(), p1.end());
		append(p2.begin(), p2.end());

		return true;
	}

	int step(int index, int offset) const
	{
		return (index + size() + offset) % size();
	}

	static int get_direction(Vec2 v)
	{
		const float dirs[8][2] = { { 0,1 },{ 1,1 },{ 1,0 },{ 1,-1 },{ 0,-1 },{ -1,-1 },{ -1,0 },{ -1,1 } };
		for (int i = 0; i < 8; ++i)
			if ((v.x == dirs[i][0] || v.x*dirs[i][0] > 0) && (v.y == dirs[i][1] || v.y*dirs[i][1] > 0))
				return i;
		return -1;
	}
	static u8 get_next_4_dirs(int dir)
	{
		int mask = 0xF << (dir + 1);
		return (mask & 0xFF) | (mask >> 8);
	}

	static u8 get_prev_4_dirs(int dir)
	{
		int mask = 0xF << dir;
		return ((mask & 0xFF) | (mask >> 8)) ^ 0xFF;  // next 4 counting from our dir - inverted
	}
};



static bool line_intersect(const Vec2& a1, const Vec2& a2, const Vec2& b1, const Vec2& b2)
{
	float det = (a1.x - a2.x)*(b1.y - b2.y) - (a1.y - a2.y)*(b1.x - b2.x);
	if (fabsf(det) < 0.000001f) return false;

	float x = ((a1.x*a2.y - a1.y*a2.x)*(b1.x - b2.x) - (a1.x - a2.x)*(b1.x*b2.y - b1.y*b2.x)) / det;
	float y = ((a1.x*a2.y - a1.y*a2.x)*(b1.y - b2.y) - (a1.y - a2.y)*(b1.x*b2.y - b1.y*b2.x)) / det;

	return
		x >= xr::Min(a1.x, a2.x) && x <= xr::Max(a1.x, a2.x) &&
		x >= xr::Min(b1.x, b2.x) && x <= xr::Max(b1.x, b2.x) &&
		y >= xr::Min(a1.y, a2.y) && y <= xr::Max(a1.y, a2.y) &&
		y >= xr::Min(b1.y, b2.y) && y <= xr::Max(b1.y, b2.y);
}

#define SIDE 30

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	xr::SPRITE_RENDER_WINDOW* device = xr::SPRITE_RENDER_WINDOW::create();
	if (!device)
		return 1;

	if (!device->create_window(1200, 800, L"Random level generation test"))
		return 2;

	const int TEX_LINE = device->load_texture("textures/line.tga");

	device->load_text_texture("textures/ascii_256.tga", 16, 16, 0, 3);

	auto draw_line = [device, TEX_LINE](float x1, float y1, float x2, float y2) -> void
	{
		float x = (x1 + x2)*0.5f, y = (y1 + y2)*0.5f;

		float len = sqrtf((x2 - x1)*(x2 - x1) + (y2 - y1)*(y2 - y1));

		float angle = atan2f(y2 - y1, x1 - x2);

		device->set_texture(TEX_LINE);
		device->draw_sprite(x, y, 0, angle * 180.0f / PI, len, 5);
	};
	auto draw_square = [draw_line](float x, float y, float r) -> void
	{
		draw_line(x - r, y - r, x + r, y - r);
		draw_line(x - r, y - r, x - r, y + r);
		draw_line(x + r, y + r, x + r, y - r);
		draw_line(x + r, y + r, x - r, y + r);
	};

	PERIMETER perimeter;
	perimeter.push_back(Vec2(WIDTH / 2 - SIDE, HEIGHT / 2 - SIDE));
	perimeter.push_back(Vec2(WIDTH / 2 - SIDE, HEIGHT / 2 + SIDE));
	perimeter.push_back(Vec2(WIDTH / 2 + SIDE, HEIGHT / 2 + SIDE));
	perimeter.push_back(Vec2(WIDTH / 2 + SIDE, HEIGHT / 2 - SIDE));

	int s1 = -1, s2 = -2;

	for (;;)
	{
		float angle = device->time_sec();
		float s = sinf(angle), c = cosf(angle);
		draw_line(20, 20, 20 + c*20, 20 + s*20);

		float dx = 0, dy = 0;

		if (device->key_held(38)) dy = -SIDE;
		if (device->key_held(40)) dy = +SIDE;
		if (device->key_held(37)) dx = -SIDE;
		if (device->key_held(39)) dx = +SIDE;

		if (device->key_pressed(32))
		{
			if (perimeter.advance_perimeter(dx, dy, s1, s2))
			{
				s1 = s2 = -1;
			}
		}

		if (device->mouse_button(0))
		{
			Vec2 mouse(device->mouse_x(), device->mouse_y());
			float best = 300.0f;
			for( int i=0; i<perimeter.size(); ++i )
				if (length(mouse - perimeter[i]) < SIDE/2.0f)
				{
					if (s1 == -1) s1 = i; else if (s1 != i) s2 = i;
					break;
				}
		}
		if (device->mouse_button(1))
		{
			s1 = s2 = -1;
		}

		device->set_color(1, 1, 1);
		for (int i = 0, pi = perimeter.size()-1; i < perimeter.size(); ++i)
		{
			draw_line(perimeter[i].x, perimeter[i].y, perimeter[pi].x, perimeter[pi].y);
			pi = i;
		}
		device->set_color(1, 0, 0);
		if( s1 >= 0 )
			draw_square(perimeter[s1].x, perimeter[s1].y, 5);
		if (s2 >= 0)
			draw_square(perimeter[s2].x, perimeter[s2].y, 5);

		device->set_color(1, 1, 1);
		device->draw_text("Use arrows to advance perimeter in a specified direction", 10, 780, 0, 16, 20);
		device->present_frame();
	}
	delete device;
	return 0;
}