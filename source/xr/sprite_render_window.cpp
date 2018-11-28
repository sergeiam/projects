#include <xr/sprite_render_window.h>

#include <stdio.h>
#include <stdarg.h>
#include <Windows.h>
#include <d3d9.h>
#include <d3dx9shader.h>
#include <vector>
#include <algorithm>



#define MAX_SPRITES_PER_BATCH 512

LRESULT CALLBACK window_message_handler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

namespace xr
{
	const static char	s_class_name[] = "XR sprite render window";

	static struct SPRITE_RENDER_WINDOW_DX9*	s_sprite_render_window;

	struct SPRITE_RENDER_WINDOW_DX9 : public SPRITE_RENDER_WINDOW
	{
		HWND								m_window_handle;
		int									m_width, m_height;
		IDirect3D9*							m_direct3D;
		IDirect3DDevice9*					m_device;
		D3DPRESENT_PARAMETERS				m_present_params;
		std::vector<IDirect3DTexture9*>		m_textures;
		IDirect3DVertexDeclaration9*		m_vertex_declaration;
		IDirect3DVertexBuffer9*				m_vertex_buffer;
		IDirect3DIndexBuffer9*				m_index_buffer;
		IDirect3DVertexShader9*				m_vertex_shader;
		IDirect3DPixelShader9*				m_pixel_shader;

		struct SPRITE_SETUP
		{
			float	x, y, z, angle, w, h, r, g, b, a, u1, v1, u2, v2;
			int		texture;
		};
		std::vector<SPRITE_SETUP>			m_sprites;
		float								m_color_r, m_color_g, m_color_b;
		int									m_texture;
		LARGE_INTEGER						m_time_start, m_time_freq;
		int									m_text_texture, m_text_columns, m_text_rows, m_text_first_index;
		float								m_text_letter_shrink;

		enum KEY_STATE {
			KS_NONE,
			KS_UP,
			KS_DOWN,
			KS_CONSUMED
		};
		KEY_STATE	m_keys[256];
		bool		m_key_held[256];
		bool		m_mouse_left, m_mouse_right;
		int			m_mouse_x, m_mouse_y;

		struct VERTEX
		{
			float	x, y, w, h;
			float	r, g, b, a;
			float	u, v, angle;
		};

		SPRITE_RENDER_WINDOW_DX9()
			: m_window_handle(0)
			, m_direct3D(NULL)
			, m_device(NULL)
			, m_vertex_declaration(NULL)
			, m_vertex_buffer(NULL)
			, m_index_buffer(NULL)
			, m_vertex_shader(NULL)
			, m_pixel_shader(NULL)
		{
			s_sprite_render_window = this;
			reset_keys();
			m_color_r = m_color_g = m_color_b = 1.0f;
			m_texture = -1;
			m_text_texture = -1;

			memset(m_key_held, false, sizeof(m_key_held));
			m_mouse_left = m_mouse_right = false;

			::QueryPerformanceCounter(&m_time_start);
			::QueryPerformanceFrequency(&m_time_freq);
		}

		virtual ~SPRITE_RENDER_WINDOW_DX9()
		{
			for (size_t i = 0; i < m_textures.size(); ++i)
				m_textures[i]->Release();
			if (m_vertex_declaration) m_vertex_declaration->Release();
			if (m_vertex_buffer) m_vertex_buffer->Release();
			if (m_index_buffer) m_index_buffer->Release();
			if (m_vertex_shader) m_vertex_shader->Release();
			if (m_pixel_shader) m_pixel_shader->Release();
			if (m_device) m_device->Release();
			if (m_direct3D) m_direct3D->Release();
			if (m_window_handle) DestroyWindow(m_window_handle);
			s_sprite_render_window = NULL;
		}

		void reset_keys()
		{
			for (int i = 0; i < sizeof(m_keys) / sizeof(m_keys[0]); ++i )
				m_keys[i] = KS_NONE;
			m_mouse_left = m_mouse_right = false;
		}

		void error(const char* fmt, ...)
		{
			char buf[2048];
			va_list args;
			va_start(args, fmt);
			vsprintf_s(buf, fmt, args);
			va_end(args);
			::OutputDebugStringA(buf);
			::MessageBoxA(m_window_handle, buf, "Error", MB_OK);
		}

		virtual bool create_window(int w, int h, const wchar_t* caption)
		{
			WNDCLASSEXA wc = {
				sizeof(WNDCLASSEX),
				CS_CLASSDC | CS_DBLCLKS,
				window_message_handler,
				0L, 0L,
				GetModuleHandle(0),
				LoadIcon(NULL, IDI_APPLICATION),
				NULL, NULL, NULL,
				s_class_name,
				NULL
			};
			if (!RegisterClassExA(&wc)) {
				error("Failed to register main window class");
				return false;
			}

			DWORD dwStyle = WS_OVERLAPPED | WS_BORDER | WS_VISIBLE | WS_SYSMENU | WS_SIZEBOX | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CAPTION;
			DWORD dwExStyle = 0;

			HWND parent = NULL;

			RECT desktop_rect;
			::GetClientRect(::GetDesktopWindow(), &desktop_rect);

			m_width = w;
			m_height = h;

			m_window_handle = CreateWindowExA(dwExStyle, s_class_name, (const char*)caption, dwStyle,
				(desktop_rect.right - w) / 2, (desktop_rect.bottom - h)/2, w, h,
				parent, NULL, GetModuleHandle(NULL), NULL
			);
			DWORD err = GetLastError();
			ShowWindow(m_window_handle, SW_SHOW);
			SetFocus(m_window_handle);

			m_direct3D = Direct3DCreate9(D3D_SDK_VERSION);
			if (!m_direct3D) {
				error("Failed to initialize DirectX 9 interface!");
				return false;
			}

			m_present_params.BackBufferFormat = D3DFMT_UNKNOWN;
			m_present_params.BackBufferCount = 0;
			m_present_params.BackBufferWidth = w;
			m_present_params.BackBufferHeight = h;
			m_present_params.MultiSampleType = D3DMULTISAMPLE_NONE;
			m_present_params.MultiSampleQuality = 0;
			m_present_params.SwapEffect = D3DSWAPEFFECT_DISCARD;
			m_present_params.hDeviceWindow = m_window_handle;
			m_present_params.Windowed = TRUE;
			m_present_params.EnableAutoDepthStencil = FALSE;
			m_present_params.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
			m_present_params.Flags = 0;
			m_present_params.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
			m_present_params.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;

			HRESULT hr = m_direct3D->CreateDevice(
				D3DADAPTER_DEFAULT,
				D3DDEVTYPE_HAL,
				m_window_handle,
				D3DCREATE_HARDWARE_VERTEXPROCESSING,
				&m_present_params,
				&m_device
			);
			if (hr != S_OK) {
				error("Failed to create DirectX 9 device!");
				return false;
			}

			D3DVERTEXELEMENT9 decl[] = {
				{0,	0,D3DDECLTYPE_FLOAT4,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION,0},
				{0,16,D3DDECLTYPE_FLOAT4,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_COLOR,0},
				{0,32,D3DDECLTYPE_FLOAT3,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_TEXCOORD,0},
				D3DDECL_END()
			};
			if (S_OK != m_device->CreateVertexDeclaration(decl, &m_vertex_declaration))
			{
				return false;
			}
			if (S_OK != m_device->CreateVertexBuffer(MAX_SPRITES_PER_BATCH * 4 * sizeof(VERTEX),
				D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &m_vertex_buffer, NULL))
			{
				return false;
			}
			if (S_OK != m_device->CreateIndexBuffer(MAX_SPRITES_PER_BATCH * 12,
				D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &m_index_buffer, NULL))
			{
				return false;
			}

			const char shader[] =
				"float4		params	: register(c0);"\
				"sampler2D	tex0	: register(s0);"\
				""\
				"struct app2vs {"\
				"	float4	xywh : POSITION0;"\
				"	float4	rgba : COLOR0;"\
				"	float3	uv_angle : TEXCOORD0;"\
				"};"\
				""\
				"struct vs2ps {"\
				"	float4	pos : POSITION0;"\
				"	float4	color : COLOR0;"\
				"	float2	uv : TEXCOORD0;"\
				"};"\
				""\
				"vs2ps vertex_shader(app2vs IN) {"\
				"	vs2ps OUT = (vs2ps)0;"\
				"	OUT.color = IN.rgba;"\
				"	OUT.uv = IN.uv_angle.xy;"\
				"	float2 sc;"\
				"	sincos((IN.uv_angle.z + 90.0) * 3.1415 / 180.0, sc.x, sc.y);"\
				"	sc *= 0.5;"\
				"	OUT.pos.x = IN.xywh.x + IN.xywh.z*sc.x - IN.xywh.w*sc.y;"\
				"	OUT.pos.y = IN.xywh.y + IN.xywh.z*sc.y + IN.xywh.w*sc.x;"\
				"	OUT.pos.x = OUT.pos.x * params.x * 2.0 - 1.0;"\
				"	OUT.pos.y = 1.0 - OUT.pos.y * params.y * 2.0;"\
				"	OUT.pos.zw = float2(0.0,1.0);"\
				"	return OUT;"\
				"}"\
				""\
				"float4 pixel_shader(vs2ps IN) : COLOR0 {"\
				"	float4 t0 = tex2D(tex0,IN.uv);"\
				"	float4 color = IN.color;"\
				"	return t0 * color;"\
				"}";

			ID3DXBuffer *shader_buf, *error_buf;
			hr = D3DXCompileShader(shader, strlen(shader), NULL, NULL, "vertex_shader", "vs_3_0", 0, &shader_buf, &error_buf, NULL);
			if (hr != S_OK) {
				error("Failed to compile vertex shader!");
				return false;
			}
			hr = m_device->CreateVertexShader((DWORD*)shader_buf->GetBufferPointer(), &m_vertex_shader);
			if (hr != S_OK) {
				error("Failed to create vertex shader!");
				return false;
			}
			shader_buf->Release();

			hr = D3DXCompileShader(shader, strlen(shader), NULL, NULL, "pixel_shader", "ps_3_0", 0, &shader_buf, &error_buf, NULL);
			if (hr != S_OK) {
				error("Failed to compile pixel shader:\n%s", error_buf->GetBufferPointer());
				return false;
			}
			hr = m_device->CreatePixelShader((DWORD*)shader_buf->GetBufferPointer(), &m_pixel_shader);
			if (hr != S_OK) {
				error("Failed to create pixel shader!");
				return false;
			}
			return true;
		}

		virtual int	load_texture(const char* filename)
		{
			IDirect3DTexture9* tex;
			HRESULT hr = D3DXCreateTextureFromFileA(m_device, filename, &tex);
			if (hr != S_OK) {
				error("Failed to load texture '%s'!", filename);
				return -1;
			}
			m_textures.push_back(tex);
			return m_textures.size() - 1;
		}

		virtual void load_text_texture(const char* filename, int columns, int rows, int first_index, int pixels_to_shrink_per_letter)
		{
			m_text_texture = load_texture(filename);
			m_text_columns = columns;
			m_text_rows = rows;
			m_text_first_index = first_index;

			D3DSURFACE_DESC desc;
			m_textures[m_text_texture]->GetLevelDesc(0, &desc);
			m_text_letter_shrink = float(pixels_to_shrink_per_letter) / desc.Width;
		}

		virtual void set_texture(int texture)
		{
			m_texture = texture;
		}

		virtual void set_color(float r, float g, float b)
		{
			m_color_r = r;
			m_color_g = g;
			m_color_b = b;
		}

		virtual void draw_sprite(float x, float y, float z, float angle, float w, float h, float alpha, float u1, float v1, float u2, float v2)
		{
			m_sprites.resize(m_sprites.size() + 1);
			SPRITE_SETUP& s = m_sprites.back();
			s.x = x;
			s.y = y;
			s.z = z;
			s.angle = angle;
			s.w = w;
			s.h = h;
			s.r = m_color_r;
			s.g = m_color_g;
			s.b = m_color_b;
			s.a = alpha;
			s.u1 = u1;
			s.v1 = v1;
			s.u2 = u2;
			s.v2 = v2;
			s.texture = m_texture;
		}

		virtual void draw_text(const char* text, float x, float y, float z, float cw, float ch, float alpha = 1.0f)
		{
			if (m_text_texture < 0) return;

			const int saved_texture = m_texture;
			m_texture = m_text_texture;

			x += cw*0.5f;
			y += ch*0.5f;

			const float tu = 1.0f / float(m_text_columns), tv = 1.0f / float(m_text_rows);

			for (; *text; ++text)
			{
				char chr = *text;
				if (chr < m_text_first_index || chr >= m_text_first_index + m_text_columns*m_text_rows) chr = 32;
				if (chr < m_text_first_index) continue;

				int r = (chr - m_text_first_index) / m_text_columns;
				int c = (chr - m_text_first_index) % m_text_columns;

				draw_sprite(x, y, z, 0.0f, cw, ch, alpha, c*tu + m_text_letter_shrink, r*tv, c*tu + tu - m_text_letter_shrink, r*tv + tv);
				x += cw;
			}
			m_texture = saved_texture;
		}

		struct SPRITE_COMP
		{
			bool operator()(const SPRITE_SETUP& a, const SPRITE_SETUP& b) const
			{
				if( a.z != b.z )
					return a.z < b.z;
				return a.texture < b.texture;
			}
		};

		virtual void present_frame()
		{
			SPRITE_COMP comp;
			std::sort(m_sprites.begin(), m_sprites.end(), comp);

			m_device->BeginScene();
			m_device->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);

			m_device->SetStreamSource(0, m_vertex_buffer, 0, sizeof(VERTEX));
			m_device->SetIndices(m_index_buffer);
			m_device->SetVertexDeclaration(m_vertex_declaration);
			m_device->SetVertexShader(m_vertex_shader);
			m_device->SetPixelShader(m_pixel_shader);
			m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			m_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
			m_device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
			m_device->SetRenderState(D3DRS_ZENABLE, FALSE);
			m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

			float vsconst[4] = { 1.0f / m_width, 1.0f / m_height, 0.0f, 0.0f };
			m_device->SetVertexShaderConstantF(0, vsconst, 1);

			for (size_t i = 0; i < m_sprites.size(); i += MAX_SPRITES_PER_BATCH)
			{
				int num_sprites = MAX_SPRITES_PER_BATCH;
				if (i + num_sprites > m_sprites.size())
					num_sprites = m_sprites.size() - i;

				VERTEX* pv = NULL;
				HRESULT hr = m_vertex_buffer->Lock(0, num_sprites * sizeof(VERTEX) * 4, (void**)&pv, D3DLOCK_DISCARD );
				if (hr != S_OK) break;

				SPRITE_SETUP* sprite = &m_sprites[i];
				for (int j = 0; j < num_sprites; ++j, ++sprite, pv += 4)
				{
					pv[0].x = pv[1].x = pv[2].x = pv[3].x = sprite->x;
					pv[0].y = pv[1].y = pv[2].y = pv[3].y = sprite->y;

					float w = sprite->w, h = sprite->h;
					pv[0].w = -w; pv[1].w = w; pv[2].w = w; pv[3].w = -w;
					pv[0].h = -h; pv[1].h = -h; pv[2].h = h; pv[3].h = h;

					pv[0].r = pv[1].r = pv[2].r = pv[3].r = sprite->r;
					pv[0].g = pv[1].g = pv[2].g = pv[3].g = sprite->g;
					pv[0].b = pv[1].b = pv[2].b = pv[3].b = sprite->b;
					pv[0].a = pv[1].a = pv[2].a = pv[3].a = sprite->a;

					pv[0].u = sprite->u1; pv[0].v = sprite->v1;
					pv[1].u = sprite->u2; pv[1].v = sprite->v1;
					pv[2].u = sprite->u2; pv[2].v = sprite->v2;
					pv[3].u = sprite->u1; pv[3].v = sprite->v2;

					pv[0].angle = pv[1].angle = pv[2].angle = pv[3].angle = sprite->angle;
				}
				m_vertex_buffer->Unlock();

				unsigned short* pi = NULL;
				hr = m_index_buffer->Lock(0, num_sprites * 12, (void**)&pi, D3DLOCK_DISCARD );
				if (hr != S_OK) break;

				unsigned short index = 0;
				for (int j = 0; j < num_sprites; ++j, index += 4, pi += 6)
				{
					pi[0] = index;
					pi[1] = index + 1;
					pi[2] = index + 3;
					pi[3] = index + 1;
					pi[4] = index + 2;
					pi[5] = index + 3;
				}
				m_index_buffer->Unlock();

				int texture = m_sprites[i].texture;
				int start = 0;
				for (int j = 0; j <= num_sprites; ++j)
				{
					if (texture >= 0 && (j == num_sprites || texture != m_sprites[i + j].texture))
					{
						m_device->SetTexture(0, m_textures[texture]);
						m_device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 0, start * 6, (j - start) * 2);

						if (j == num_sprites) break;

						texture = m_sprites[i + j].texture;
						start = j;
					}
				}
			}

			m_sprites.clear();
			m_color_r = m_color_g = m_color_b = 1.0f;

			m_device->EndScene();

			reset_keys();

			MSG msg;
			while (::PeekMessageA(&msg, m_window_handle, 0, 0, PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessageA(&msg);
			}
			m_device->Present(NULL, NULL, NULL, NULL);
			::Sleep(1);
		}

		virtual bool key_pressed(int code)
		{
			bool pressed = m_keys[code] == KS_DOWN || m_keys[code] == KS_UP;
			m_keys[code] = KS_CONSUMED;
			return pressed;
		}

		virtual bool key_held(int code)
		{
			return m_key_held[code];
		}
		virtual int mouse_x()
		{
			return m_mouse_x;
		}
		virtual int	mouse_y()
		{
			return m_mouse_y;
		}
		virtual bool mouse_button(int button)
		{
			switch (button) {
				case 0: return m_mouse_left;
				case 1: return m_mouse_right;
			}
			return false;
		}
		virtual float time_sec()
		{
			LARGE_INTEGER t;
			::QueryPerformanceCounter(&t);
			return float(double(t.QuadPart - m_time_start.QuadPart) / double(m_time_freq.QuadPart));
		}
	};

	SPRITE_RENDER_WINDOW* SPRITE_RENDER_WINDOW::create()
	{
		if (s_sprite_render_window) return NULL;
		return new SPRITE_RENDER_WINDOW_DX9();
	}
}

static LRESULT CALLBACK window_message_handler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	xr::SPRITE_RENDER_WINDOW_DX9* ptr = xr::s_sprite_render_window;

	switch (msg)
	{
		case WM_LBUTTONDOWN: ptr->m_mouse_left = true; break;
		case WM_RBUTTONDOWN: ptr->m_mouse_right = true; break;

		case WM_MOUSEMOVE: ptr->m_mouse_x = LOWORD(lparam); ptr->m_mouse_y = HIWORD(lparam - 20); break;

		case WM_CLOSE:
		case WM_QUIT: {
			::PostQuitMessage(0);
			return 0;
		}
		case WM_DESTROY: {
			return 0;
		}
		case WM_KEYDOWN:
			if (ptr) {
				ptr->m_keys[wparam] = xr::SPRITE_RENDER_WINDOW_DX9::KS_DOWN;
				ptr->m_key_held[wparam] = true;
			}
			break;
		case WM_KEYUP:
			if (ptr) {
				if (ptr->m_keys[wparam] == xr::SPRITE_RENDER_WINDOW_DX9::KS_DOWN)
					ptr->m_keys[wparam] = xr::SPRITE_RENDER_WINDOW_DX9::KS_UP;
				ptr->m_key_held[wparam] = false;
			}
			break;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}