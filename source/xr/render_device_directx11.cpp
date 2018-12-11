#include <xr/render_device.h>
#include <xr/window.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#define SAFE_RELEASE(ptr) if(ptr) ptr->Release()

namespace xr
{
	static IDXGISwapChain*		s_swap_chain;
	static ID3D11DeviceContext*	s_device_context;
	static ID3D11Device*		s_device;

#define LOG_HR(h) // just a stub

	struct RENDER_DEVICE_DX11 : public RENDER_DEVICE
	{


// --- TEXTURE implementation

		struct TEXTURE_DX11 : public TEXTURE
		{
			TEXTURE_DX11(const TEXTURE_PARAMS& params)
				: texture_2d(nullptr)
				, shader_resource_view(nullptr)
				, render_target_view(nullptr)
			{
				m_params = params;
			}
			virtual ~TEXTURE_DX11()
			{
				SAFE_RELEASE(shader_resource_view);
				SAFE_RELEASE(depth_stencil_view);
				SAFE_RELEASE(texture_2d);
			}
			bool allocate()
			{
				HRESULT hr = S_OK;

				if (m_params.flags & TF_BACKBUFFER) {
					hr = s_swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&texture_2d);
					LOG_HR(hr);
					if (hr != S_OK)
						return false;

					m_params.flags |= TF_TEXTURE2D;
					D3D11_TEXTURE2D_DESC desc;
					texture_2d->GetDesc(&desc);
					m_params.width = desc.Width;
					m_params.height = desc.Height;
					m_params.array_size = 1;
					m_params.depth = 1;
					m_params.format = dx11_to_xr_format(desc.Format);
					m_params.mips = 1;
				}
				else if (m_params.flags & (TF_TEXTURE2D | TF_CUBEMAP | TF_DEPTH_STENCIL)) {
					D3D11_TEXTURE2D_DESC td;
					td.Format = xr_to_dx11_format(m_params.format);
					td.ArraySize = m_params.array_size;
					if (m_params.flags & TF_CUBEMAP)
						td.ArraySize *= 6;
					td.BindFlags = 0;
					if (m_params.flags & TF_DEPTH_STENCIL)
						td.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
					else if (m_params.flags & TF_RENDER_TARGET)
						td.BindFlags |= D3D11_BIND_RENDER_TARGET;
					td.MiscFlags = (m_params.flags & TF_CUBEMAP) ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0;
					td.Usage = (m_params.flags & TF_DYNAMIC) ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
					td.CPUAccessFlags = (m_params.flags & TF_DYNAMIC) ? D3D11_CPU_ACCESS_WRITE : 0;
					td.Width = m_params.width;
					td.Height = m_params.height;
					td.MipLevels = m_params.mips;
					td.SampleDesc.Count = 1;	// MSAA here
					td.SampleDesc.Quality = 0;

					hr = s_device->CreateTexture2D(&td, nullptr, &texture_2d);
					LOG_HR(hr);
					if (hr != S_OK) return false;
				}
				else if (m_params.flags & TF_TEXTURE3D) {
					D3D11_TEXTURE3D_DESC td;
					td.Format = xr_to_dx11_format(m_params.format);
					td.BindFlags = 0;
					td.Width = m_params.width;
					td.Height = m_params.height;
					td.Depth = m_params.depth;
					td.MipLevels = m_params.mips;
					td.Usage = (m_params.flags & TF_DYNAMIC) ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
					td.CPUAccessFlags = (m_params.flags & TF_DYNAMIC) ? D3D11_CPU_ACCESS_WRITE : 0;
					td.MiscFlags = 0;

					hr = s_device->CreateTexture3D(&td, nullptr, &texture_3d);
					LOG_HR(hr);
					if (hr != S_OK) return false;
				}
				else {
					return false;
				}
				if (m_params.flags & TF_RENDER_TARGET) {
					D3D11_RENDER_TARGET_VIEW_DESC rtvd;
					rtvd.Format = xr_to_dx11_format(m_params.format);
					rtvd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
					rtvd.Texture2D.MipSlice = 0;
					hr = s_device->CreateRenderTargetView(texture_2d, &rtvd, &render_target_view);
					LOG_HR(hr);
					if (hr != S_OK) return false;
				}
				else if (m_params.flags & TF_DEPTH_STENCIL) {
					D3D11_DEPTH_STENCIL_VIEW_DESC dsvd;
					dsvd.Format = xr_to_dx11_format(m_params.format);
					dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
					dsvd.Flags = 0;
					dsvd.Texture2D.MipSlice = 0;
					hr = s_device->CreateDepthStencilView(texture_2d, &dsvd, &depth_stencil_view);
					LOG_HR(hr);
					if (hr != S_OK) return false;
				}
				if (m_params.flags & TF_SHADER_RESOURCE) {
					D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
					srvd.Format = xr_to_dx11_format(m_params.format);
					if (m_params.flags & TF_TEXTURE3D) {
						srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
						srvd.Texture2D.MipLevels = 0;
						srvd.Texture2D.MostDetailedMip = 0;
						s_device->CreateShaderResourceView(texture_3d, &srvd, &shader_resource_view);
					}
					else {
						srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
						srvd.Texture3D.MipLevels = 0;
						srvd.Texture3D.MostDetailedMip = 0;
						s_device->CreateShaderResourceView(texture_2d, &srvd, &shader_resource_view);
					}
				}
				return true;
			}

			union {
				ID3D11Texture2D * texture_2d;
				ID3D11Texture3D * texture_3d;
			};
			ID3D11ShaderResourceView*	shader_resource_view;
			union {
				ID3D11RenderTargetView*	render_target_view;
				ID3D11DepthStencilView*	depth_stencil_view;
			};
		};

		virtual TEXTURE* create_texture(const TEXTURE_PARAMS& params)
		{
			TEXTURE_DX11* ptr = new TEXTURE_DX11(params);
			if (!ptr->allocate())
			{
				delete ptr;
				return nullptr;
			}
			return ptr;
		}



// --- BUFFER implementation

		struct BUFFER_DX11 : public BUFFER
		{
			BUFFER_DX11(const BUFFER_PARAMS& params)
				: m_buffer(nullptr)
			{
				m_params = params;
			}
			virtual ~BUFFER_DX11()
			{
				SAFE_RELEASE(m_buffer);
			}
			bool allocate()
			{
				D3D11_BUFFER_DESC bd;
				bd.ByteWidth = m_params.size;
				bd.BindFlags = 0;
				if (m_params.flags & BF_VERTEX_BUFFER)
					bd.BindFlags |= D3D11_BIND_VERTEX_BUFFER;
				if (m_params.flags & BF_INDEX_BUFFER)
					bd.BindFlags |= D3D11_BIND_INDEX_BUFFER;
				if (m_params.flags & BF_CONSTANT_BUFFER)
					bd.BindFlags |= D3D11_BIND_CONSTANT_BUFFER;
				if (m_params.flags & BF_SHADER_RESOURCE)
					bd.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
				bd.CPUAccessFlags = (m_params.flags & BF_DYNAMIC) ? D3D11_CPU_ACCESS_WRITE : 0;
				bd.Usage = (m_params.flags & BF_DYNAMIC) ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
				bd.StructureByteStride = 0;
				bd.MiscFlags = 0;

				D3D11_SUBRESOURCE_DATA srd;
				srd.pSysMem = m_params.init_data;
				srd.SysMemPitch = 0;
				srd.SysMemSlicePitch = 0;

				HRESULT hr = s_device->CreateBuffer(&bd, m_params.init_data ? &srd : NULL, &m_buffer);
				LOG_HR(hr);
				return hr != S_OK;
			}
			virtual bool map(MAPPED_DATA& mapped_data, u32 flags)
			{
				if (!m_buffer)
					return false;

				D3D11_MAP map = D3D11_MAP_WRITE;
				if (flags & MAP_DISCARD)
					map = D3D11_MAP_WRITE_DISCARD;
				else if (flags & D3D11_MAP_WRITE_NO_OVERWRITE)
					map = D3D11_MAP_WRITE_NO_OVERWRITE;
				else if (flags & (MAP_READ | MAP_WRITE))
					map = D3D11_MAP_READ_WRITE;
				else if (flags & MAP_READ)
					map = D3D11_MAP_READ;

				D3D11_MAPPED_SUBRESOURCE mapped;
				HRESULT hr = s_device_context->Map(m_buffer, 0, map, 0, &mapped);
				LOG_HR(hr);
				if (hr != S_OK) return false;

				mapped_data.data = mapped.pData;
				mapped_data.row_pitch = mapped.RowPitch;
				mapped_data.depth_pitch = mapped.DepthPitch;

				return true;
			}
			virtual void unmap()
			{
				if (m_buffer) s_device_context->Unmap(m_buffer, 0);
			}
			ID3D11Buffer* m_buffer;
		};

		virtual BUFFER* create_buffer(const BUFFER_PARAMS& params)
		{
			BUFFER_DX11* ptr = new BUFFER_DX11(params);
			if (!ptr->allocate())
			{
				delete ptr;
				return nullptr;
			}
			return ptr;
		}

		virtual void set_buffers(const BUFFER** ptr, u32 count, u32 slot, const u32 * strides, const u32 * offsets, u32 flags)
		{
			if (!ptr) return;

			ASSERT(count > 0 && count <= 8);

			ID3D11Buffer * pb[8];
			for (u32 i = 0; i < count; ++i) {
				pb[i] = ((BUFFER_DX11*)ptr[i])->m_buffer;
			}

			switch ( ptr[0]->params().flags & (BF_VERTEX_BUFFER | BF_INDEX_BUFFER | BF_CONSTANT_BUFFER))
			{
			case BF_VERTEX_BUFFER:
				s_device_context->IASetVertexBuffers(slot, count, pb, (const UINT*)strides, (const UINT*)offsets);
				break;

			case BF_INDEX_BUFFER:
				ASSERT(count == 1);
				s_device_context->IASetIndexBuffer(pb[0], (ptr[0]->params.flags & BF_INDICES_16) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, offsets[0]);
				break;

			case BF_CONSTANT_BUFFER:
				if (flags & SF_VERTEX_SHADER) {
					s_device_context->VSSetConstantBuffers(slot, count, pb);
				}
				if (flags & SF_PIXEL_SHADER) {
					s_device_context->PSSetConstantBuffers(slot, count, pb);
				}
				if (flags & SF_COMPUTE_SHADER) {
					s_device_context->CSSetConstantBuffers(slot, count, pb);
					break;
				}
			}
		}



// --- RENDER STATE

		struct RENDER_STATE_DX11 : public RENDER_STATE
		{
			RENDER_STATE_DX11(const RENDER_STATE_PARAMS& params)
			{
				m_params = params;
			}
			~RENDER_STATE_DX11()
			{
				SAFE_RELEASE(depth_stencil_state);
				SAFE_RELEASE(rasterizer_state);
				SAFE_RELEASE(blend_state);
			}
			bool allocate()
			{
				HRESULT hr = S_OK;

				D3D11_DEPTH_STENCIL_DESC ds;
				ds.DepthFunc = xr_to_dx11_comparison(m_params.depth_func);
				ds.DepthWriteMask = (m_params.flags & RSF_DEPTH_WRITE) ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
				ds.DepthEnable = m_params.depth_func == CMP_NEVER ? FALSE : TRUE;

				ds.StencilEnable = m_params.stencil ? TRUE : FALSE;

				if (ds.StencilEnable)
				{
					ds.StencilReadMask = m_params.StencilReadMask;
					ds.StencilWriteMask = m_params.StencilWriteMask;
					ds.FrontFace.StencilDepthFailOp = GetStencilOp(m_params.FrontStencil.depthFail);
					ds.FrontFace.StencilFailOp = GetStencilOp(m_params.FrontStencil.fail);
					ds.FrontFace.StencilPassOp = GetStencilOp(m_params.FrontStencil.pass);
					ds.FrontFace.StencilFunc = GetComparison(m_params.FrontStencil.cmpFunc);
					ds.BackFace.StencilDepthFailOp = GetStencilOp(m_params.BackStencil.depthFail);
					ds.BackFace.StencilFailOp = GetStencilOp(m_params.BackStencil.fail);
					ds.BackFace.StencilPassOp = GetStencilOp(m_params.BackStencil.pass);
					ds.BackFace.StencilFunc = GetComparison(m_params.BackStencil.cmpFunc);
					ds.StencilEnable = TRUE;
				}

				hr = s_device->CreateDepthStencilState(&ds, &depth_stencil_state);
				LOG_HR(hr);
				if (hr != S_OK) {
					delete ptr;
					return nullptr;
				}
			}

			ID3D11DepthStencilState*	depth_stencil_state = nullptr;
			ID3D11RasterizerState*		rasterizer_state = nullptr;
			ID3D11BlendState*			blend_state = nullptr;
		};

		virtual RENDER_STATE* create_render_state(const RENDER_STATE_PARAMS& params)
		{
			RENDER_STATE_DX11* ptr = new RENDER_STATE_DX11(params);
			if (!ptr->allocate())
			{
				delete ptr;
				return nullptr;
			}
			return ptr;
		}

		virtual void set_render_state(const RENDER_STATE* rs, u8 stencil = 0, const float* blend_factors = nullptr)
		{

		}


// --- RENDER_DEVICE implementation

		virtual ~RENDER_DEVICE_DX11()
		{
			SAFE_RELEASE(s_device_context);
			SAFE_RELEASE(s_swap_chain);
			SAFE_RELEASE(s_device);
		}

		virtual bool create(WINDOW* window, const DEVICE_PARAMS& params)
		{
			HWND window_handle;
			window->get_handle(&window_handle);

			DXGI_SWAP_CHAIN_DESC scd;
			ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

			scd.BufferCount = 1;
			scd.BufferDesc.Format = xr_to_dx11_format(params.back_buffer_format);
			scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			scd.OutputWindow = window_handle;
			scd.SampleDesc.Count = params.msaa;
			scd.Windowed = TRUE;

			HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL,
				D3D_DRIVER_TYPE_HARDWARE,
				NULL,
				NULL,
				NULL,
				NULL,
				D3D11_SDK_VERSION,
				&scd,
				&s_swap_chain,
				&s_device,
				NULL,
				&s_device_context);
			LOG_HR(hr);
			return hr == S_OK;
		}




#define XR_TO_DX_FORMAT_MAPPING	\
		ITEM(FMT_UNKNOWN,				DXGI_FORMAT_UNKNOWN) \
		ITEM(FMT_BYTE4,					DXGI_FORMAT_R8G8B8A8_UNORM)	\
		ITEM(FMT_FLOAT16_1,				DXGI_FORMAT_R16_FLOAT) \
		ITEM(FMT_FLOAT16_2,				DXGI_FORMAT_R16G16_FLOAT) \
		ITEM(FMT_FLOAT16_4,				DXGI_FORMAT_R16G16B16A16_FLOAT) \
		ITEM(FMT_FLOAT32_1,				DXGI_FORMAT_R32_FLOAT) \
		ITEM(FMT_FLOAT32_2,				DXGI_FORMAT_R32G32_FLOAT) \
		ITEM(FMT_FLOAT32_3,				DXGI_FORMAT_R32G32B32_FLOAT) \
		ITEM(FMT_FLOAT32_4,				DXGI_FORMAT_R32G32B32A32_FLOAT) \
		ITEM(FMT_UINT32_1,				DXGI_FORMAT_R32_UINT) \
		ITEM(FMT_D32_FLOAT,				DXGI_FORMAT_D32_FLOAT) \
		ITEM(FMT_D24_S8,				DXGI_FORMAT_D24_UNORM_S8_UINT) \
		ITEM(FMT_D16,					DXGI_FORMAT_D16_UNORM)

		static FORMAT dx11_to_xr_format(DXGI_FORMAT format)
		{
			switch (format) {
				#define ITEM(tf,dxgi) case dxgi: return tf;
					XR_TO_DX_FORMAT_MAPPING
				#undef ITEM
			}
			return FMT_UNKNOWN;
		}
		static DXGI_FORMAT xr_to_dx11_format(RENDER_DEVICE::FORMAT format)
		{
			switch (format) {
				#define ITEM(tf,dxgi) case tf: return dxgi;
					XR_TO_DX_FORMAT_MAPPING
				#undef ITEM
			}
			return DXGI_FORMAT_UNKNOWN;
		}


		static D3D11_COMPARISON_FUNC xr_to_dx11_comparison(CMP cmp)
		{
			switch (cmp)
			{
				case CMP_ALWAYS: return D3D11_COMPARISON_ALWAYS;
				case CMP_LESS: return D3D11_COMPARISON_LESS;
				case CMP_LESS_EQUAL: return D3D11_COMPARISON_LESS_EQUAL;
				case CMP_EQUAL: return D3D11_COMPARISON_EQUAL;
				case CMP_GREATER_EQUAL: return D3D11_COMPARISON_GREATER_EQUAL;
				case CMP_GREATER: return D3D11_COMPARISON_GREATER;
			}
			return D3D11_COMPARISON_NEVER;
		}

		D3D11_STENCIL_OP		GetStencilOp(RDStencilOp cmp);
		D3D11_CULL_MODE			GetCullMode(RDCull cull);
		D3D11_BLEND_OP			GetBlendOp(RDBlendOp op);
		D3D11_BLEND				GetBlend(RDBlend blend);
	};
}