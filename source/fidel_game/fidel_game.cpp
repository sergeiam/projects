#include <xr/sprite_render_window.h>
#include <math.h>
#include <vector>
#include <Windows.h>

#define WIDTH	800
#define HEIGHT	600
#define MAP_WIDTH	10
#define	MAP_HEIGHT	10
#define INVALID_PATH_VALUE 10000

#define Max(a,b) ((a)>(b)?(a):(b))

enum MAP_OBJECT
{
	MO_EMPTY,
	MO_WALL,
	MO_DOOR,
	MO_MONSTER,
	MO_FIRST_AID,
	MO_PATH
};

using namespace std;

static int rand_range(int a, int b) {
	return a + rand() % (b - a + 1);
}

static bool rand_bool() {
	return (rand() & 1) ? true : false;
}

struct point {
	int x, y;
	point() {}
	point(int _x, int _y) :x(_x), y(_y) {}

	friend point operator + (point a, point b) {
		return point(a.x + b.x, a.y + b.y);
	}
	bool operator == (const point& rhs) const {
		return x == rhs.x && y == rhs.y;
	}
	bool operator != (const point& rhs) const {
		return !( *this == rhs );
	}
};

struct FIELD
{
	int v[MAP_WIDTH][MAP_HEIGHT];

	FIELD() { fill(0); }

	void fill(int value) {
		for (int i = 0; i < MAP_HEIGHT; ++i)
			for (int j = 0; j < MAP_WIDTH; ++j)
				v[i][j] = value;
	}
	void fill_path(std::vector<point>& path, int value, int step = 0) {
		for (size_t i = 0; i < path.size(); ++i, value += step)
			this->operator()(path[i]) = value;
	};
	int& operator()(point pt) {
		return v[pt.x][pt.y];
	}
	int operator()(point pt) const {
		return v[pt.x][pt.y];
	}
	int& operator()(int x, int y) {
		return v[x][y];
	}
	int operator()(int x, int y) const {
		return v[x][y];
	}
	void breadth_first_search(point start, const FIELD& mask)
	{
		fill(INVALID_PATH_VALUE);

		std::vector<point> wave;
		wave.push_back(start);
		this->operator()(start) = 0;

		for (size_t i = 0; i<wave.size(); i++) {
			point pt = wave[i];
			int value = this->operator()(pt);

			point dir[] = { { 0,1 },{ 0,-1 },{ 1,0 },{ -1,0 } };
			for (int j = 0; j < 4; ++j) {
				point pn = pt + dir[j];
				if (this->operator()(pn) > value + 1 && mask(pn) == 0) {
					this->operator()(pn) = value + 1;
					wave.push_back(pn);
				}
			}
		}
	}
	bool find_masked_path(point start, point end, std::vector<point>& path)
	{
		FIELD result;
		result.breadth_first_search(start, *this);
		if (result(end) == INVALID_PATH_VALUE) return false;

		path.clear();
		path.push_back(end);
		for (int value = result(end)-1; start != end; --value) {
			const point dir[] = { { 0,1 },{ 0,-1 },{ 1,0 },{ -1,0 } };
			for (int i = 0; i < 4; i++) {
				point pn = end + dir[i];
				if (result(pn) != value) continue;
				path.push_back(pn);
				end = pn;
			}
		}
		for (int i = 0; i < path.size() / 2; ++i) {
			point sw = path[i];
			path[i] = path[path.size() - i - 1];
			path[path.size() - i - 1] = sw;
		}
		return true;
	};
};

static bool extend_path(const FIELD& objects, vector<point>& path)
{
	FIELD f;
	f = objects;
	f.fill_path(path, 1, 0);

	struct FOUND_ITEM
	{
		point p1, p2;
		int	path_index;
	};

	vector<FOUND_ITEM> found;
	for (size_t i = 0; i+1 < path.size(); ++i)
	{
		point p1 = path[i], p2 = path[i + 1];
		int dx = p2.x - p1.x, dy = p2.y - p1.y;

		point ortho1(dy, -dx), ortho2(-dy, dx);

		if (f(p1 + ortho1) == 0 && f(p2 + ortho1) == 0) {
			FOUND_ITEM fi;
			fi.p1 = p1 + ortho1;
			fi.p2 = p2 + ortho1;
			fi.path_index = i;
			found.push_back(fi);
		}
		if (f(p1 + ortho2) == 0 && f(p2 + ortho2) == 0) {
			FOUND_ITEM fi;
			fi.p1 = p1 + ortho2;
			fi.p2 = p2 + ortho2;
			fi.path_index = i;
			found.push_back(fi);
		}
	}
	if (found.empty()) return false;

	int dice = rand_range(0, found.size() - 1);
	path.insert(path.begin() + found[dice].path_index + 1, found[dice].p2);
	path.insert(path.begin() + found[dice].path_index + 1, found[dice].p1);
	return true;
}

int CALLBACK WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
	xr::SPRITE_RENDER_WINDOW* device = xr::SPRITE_RENDER_WINDOW::create();
	if (!device)
		return 1;

	if (!device->create_window(800, 600, L"Random level generation test"))
		return 2;

	const int TEX_DOGGY = device->load_texture("textures/doggy.tga");
	const int TEX_EMPTY_TILE = device->load_texture("textures/empty_tile.tga");
	const int TEX_WALL_TILE = device->load_texture("textures/wall_tile.tga");
	const int TEX_DOOR_TILE = device->load_texture("textures/door.tga");
	const int TEX_MONSTER = device->load_texture("textures/monster.tga");
	const int TEX_DEAD_MONSTER = device->load_texture("textures/dead_monster.tga");
	const int TEX_HP = device->load_texture("textures/hp.tga");
	const int TEX_FIRSTAID = device->load_texture("textures/first_aid.tga");
	const int TEX_ROUTE_HOR = device->load_texture("textures/route_horizontal.tga");
	const int TEX_ROUTE_HOR_HALF = device->load_texture("textures/route_horizontal_half.tga");
	const int TEX_ROUTE_COR = device->load_texture("textures/route_corner.tga");
	const int TEX_WIN = device->load_texture("textures/win.tga");
	const int TEX_BLACK = device->load_texture("textures/black.tga");

	device->load_text_texture("textures/ascii_256.tga", 16, 16, 0, 3);

	const float map_x0 = (WIDTH - HEIGHT)*0.5f;
	const float tile_size = HEIGHT / float(MAP_HEIGHT);

	FIELD objects, dog_route, gen_route;

	point dog_pos, door_pos;
	float dog_fx, dog_fy;
	vector<int> dog_hp;
	float dog_hurt_t0 = -FLT_MAX;
	int   monsters_to_kill;

	auto generate_random_map = [&]() -> void
	{
		dog_pos = point(1, MAP_HEIGHT / 2);
		dog_fx = (float)dog_pos.x;
		dog_fy = (float)dog_pos.y;
		dog_route.fill(0);
		dog_route(dog_pos) = 1;
		dog_hp.resize(1);
		dog_hp[0] = 2;

		for (int i = 0; i < MAP_WIDTH; ++i)
			for (int j = 0; j < MAP_HEIGHT; ++j)
				objects(i,j) = (i == 0 || i + 1 == MAP_WIDTH || j == 0 || j + 1 == MAP_HEIGHT) ? MO_WALL : MO_EMPTY;

		door_pos = point(rand_range(MAP_WIDTH / 2, MAP_WIDTH - 2), rand_range(1, MAP_HEIGHT - 2));
		objects(door_pos) = MO_DOOR;

		int num_walls = rand_range(3, 6);
		for (int i = 0; i < num_walls; ) {
			point pt(rand_range(2, MAP_WIDTH - 3), rand_range(2, MAP_HEIGHT - 3));
			if (pt == dog_pos || objects(pt) != MO_EMPTY) continue;
			objects(pt) = MO_WALL;
			i++;
		}

		gen_route = objects;
		gen_route(door_pos) = 0;

		vector<point> path;
		gen_route.find_masked_path(dog_pos, door_pos, path);

		int prev_left = 0;
		for(;;)
		{
			if (!extend_path(objects, path)) break;
			int left = (MAP_WIDTH - 2)*(MAP_HEIGHT - 2) - path.size();
			if ( left < 8) break;
		}

		gen_route.fill(0);
		gen_route.fill_path(path, 1, 1);

		int hp = 2, monsters = 0;
		for (size_t i = 1; i+1 < path.size(); ) {

			int r2 = i + 3 < path.size() - 2 ? i + 3 : path.size() - 2;
			int r = rand_range(i, r2 );
			if (hp > 0) {
				objects(path[r]) = MO_MONSTER;
				monsters++;
				hp--;
			}
			else {
				objects(path[r]) = MO_FIRST_AID;
				hp = 2;
			}
			i = r + 1;
		}

		monsters_to_kill = monsters - 3;
	};

	generate_random_map();


	auto move_dog = [&](int dx, int dy) -> void
	{
		point moved = dog_pos + point(dx, dy);
		if (dog_route(moved) != 0 && dog_route(moved) != dog_route(dog_pos) - 1 || objects(moved) == MO_WALL) return;
		if (objects(moved) == MO_DOOR && monsters_to_kill > 0) return;
		if (objects(moved) == MO_MONSTER && dog_hp.back() == 0 && dog_route(moved) == 0) return;

		if (dog_route(moved) == 0)
		{
			dog_route(moved) = dog_route(dog_pos) + 1;
			switch (objects(moved))
			{
				case MO_FIRST_AID:
					dog_hp.push_back(2);
					dog_hurt_t0 = -FLT_MAX;
					break;
				case MO_MONSTER:
					dog_hp.push_back(dog_hp.back() - 1);
					dog_hurt_t0 = device->time_sec();
					monsters_to_kill--;
					break;
			}
		}
		else
		{
			dog_route(dog_pos) = 0;
			dog_hurt_t0 = -FLT_MAX;
			switch (objects(dog_pos))
			{
				case MO_MONSTER:
					monsters_to_kill++;
				case MO_FIRST_AID:
					dog_hp.pop_back();
					break;
			}
		}
		dog_pos = moved;
	};

	float win_t0 = FLT_MAX, win_period = 2.0f, level_t0 = 0, level_fadein = 1.0f;

	for (;;)
	{
		const float time = device->time_sec();

		if (dog_pos == door_pos && win_t0 > time) win_t0 = time;

	// level fadein
		if (time < level_t0 + 1.0f) {
			device->set_texture(TEX_BLACK);
			device->draw_sprite(WIDTH/2.0f, HEIGHT/2.0f, 20, 0, WIDTH, HEIGHT, level_t0+1.0f - time);
		}

	// handle input
		if (time < win_t0) {
			if (device->key_pressed(27) && ::MessageBoxA(NULL, "You sure you want to quit?", "Really?", MB_OKCANCEL) == IDOK) break;
			if (device->key_pressed(38)) move_dog(0, -1);
			if (device->key_pressed(40)) move_dog(0, +1);
			if (device->key_pressed(37)) move_dog(-1, 0);
			if (device->key_pressed(39)) move_dog(+1, 0);
			if (device->key_pressed(9)) generate_random_map();
		} else {
	// you win and level fadeout
			float len = (time - win_t0) / win_period;
			float anim = pow(len, 0.2f);
			device->set_texture(TEX_WIN);

			device->set_color(0, 0, 0);
			device->draw_sprite(WIDTH / 2.0f + 10.0f, HEIGHT / 2.0f + 10.0f, 15, 0, WIDTH / 2.0f*anim, WIDTH / 8.0f*anim, anim);

			device->set_color(1, 1, 1);
			device->draw_sprite(WIDTH / 2.0f, HEIGHT / 2.0f, 16, 0, WIDTH/2.0f*anim, WIDTH /8.0f*anim, anim);

			float fade_out = win_t0 + win_period - time;
			if (fade_out < 1.0f) {
				device->set_texture(TEX_BLACK);
				device->draw_sprite(WIDTH / 2.0f, HEIGHT / 2.0f, 20, 0, WIDTH, HEIGHT, 1.0f - fade_out);
			}
			if (len > 1.0f) {
				generate_random_map();
				win_t0 = FLT_MAX;
				level_t0 = time;
			}
		}

	// render the map

		for (int y = 0; y < MAP_HEIGHT; ++y) {
			for (int x = 0; x < MAP_WIDTH; ++x) {
				device->set_texture(TEX_EMPTY_TILE);
				device->draw_sprite(map_x0 + (x + 0.5f) * tile_size, (y + 0.5f) * tile_size, 0, 0, tile_size, tile_size);

				int tex = TEX_EMPTY_TILE;

				switch (objects(x,y))
				{
				case MO_FIRST_AID:
					if(dog_route(x,y) == 0)
						tex = TEX_FIRSTAID;
					break;
				case MO_MONSTER:
					tex = dog_route(x,y) ? TEX_DEAD_MONSTER : TEX_MONSTER;
					break;

				case MO_WALL:
					tex = TEX_WALL_TILE;
					break;

				case MO_DOOR:
					tex = TEX_DOOR_TILE;
					break;

				default:
					break;
				}

				if (tex != TEX_EMPTY_TILE) {
					device->set_texture(tex);
					device->draw_sprite(map_x0 + (x + 0.5f) * tile_size, (y + 0.5f) * tile_size, 1, 0, tile_size, tile_size);
				}

				FIELD& route = device->key_held(32) ? gen_route : dog_route;

				if (route(x,y) == 0) continue;

			// render route
				int mask = 0;

				#define iabs(x) (((x)>0) ? (x) : -(x))
				if (iabs(route(x - 1,y) - route(x,y)) == 1) mask |= 1;
				if (iabs(route(x + 1,y) - route(x,y)) == 1) mask |= 2;
				if (iabs(route(x,y - 1) - route(x,y)) == 1) mask |= 4;
				if (iabs(route(x,y + 1) - route(x,y)) == 1) mask |= 8;

				tex = TEX_EMPTY_TILE;
				float angle = 0.0f;

				switch (mask)
				{
					case 0: break;
					case 1: tex = TEX_ROUTE_HOR_HALF; angle = 0; break;
					case 2: tex = TEX_ROUTE_HOR_HALF; angle = 180; break;
					case 4: tex = TEX_ROUTE_HOR_HALF; angle = 270; break;
					case 8: tex = TEX_ROUTE_HOR_HALF; angle = 90; break;
					case 3: tex = TEX_ROUTE_HOR; angle = 0; break;
					case 12: tex = TEX_ROUTE_HOR; angle = 90; break;
					case 5: tex = TEX_ROUTE_COR; angle = 90; break;
					case 9: tex = TEX_ROUTE_COR; angle = 180; break;
					case 6: tex = TEX_ROUTE_COR; angle = 0; break;
					case 10: tex = TEX_ROUTE_COR; angle = 270; break;
				}

				if (tex != TEX_EMPTY_TILE) {
					device->set_texture(tex);
					device->draw_sprite(map_x0 + (x + 0.5f) * tile_size, (y + 0.5f) * tile_size, 2, angle, tile_size, tile_size);
				}
			}
		}

		if (fabsf(dog_pos.x - dog_fx) > 0.001f) dog_fx += (dog_pos.x - dog_fx)*0.5f;
		if (fabsf(dog_pos.y - dog_fy) > 0.001f) dog_fy += (dog_pos.y - dog_fy)*0.5f;

	// dog rendering
		device->set_texture(TEX_DOGGY);
		float len = (time - dog_hurt_t0) / 4.0f;
		if (len < 1.0f) {
			float pulse = cosf(len*36.0f);
			float fade_in = len*len*len;
			float c = 0.6f + (pulse*(1.0f - fade_in) + fade_in)*0.4f;
			device->set_color(1.0f, c, c);
		}
		device->draw_sprite(map_x0 + (dog_fx+0.5f) * tile_size, (dog_fy+0.5f)*tile_size, 10, 0, tile_size, tile_size);
		device->set_color(1.0f, 1.0f, 1.0f);

		device->set_texture(TEX_HP);
		for( int i=0; i < dog_hp.back(); ++i )
			device->draw_sprite(map_x0 + (dog_fx + 0.5f) * tile_size - tile_size*0.15f + i*tile_size/4.0f, (dog_fy + 0.5f)*tile_size - tile_size*0.4f, 10, 0, tile_size * 0.35f, tile_size * 0.35f);

	// monsters left number rendering
		if (dog_pos != door_pos) {
			char buf[32];
			_itoa_s(monsters_to_kill >= 0 ? monsters_to_kill : 0, buf, 10);
			device->set_color(0, 1, 0);
			device->draw_text(buf, map_x0 + door_pos.x*tile_size + tile_size*0.25f, door_pos.y*tile_size + tile_size*0.25f, 10, tile_size*0.5f, tile_size*0.5f);
			device->set_color(1, 1, 1);
		}

		device->draw_text("Hold SPACE to see the generated solution", 0, 0, 10, 16, 16);
		device->draw_text("Press TAB to generate new map", 0, 18, 10, 16, 16);
		device->draw_text("Press ESC to quit", 0, HEIGHT - 18, 10, 16, 16);
		device->present_frame();
	}
	delete device;

	return 0;
}