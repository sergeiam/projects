#include <xr/render_device2.h>
#include <xr/window.h>
#include <d3d11.h>
#include <d3dcompiler.h>


#define SAFE_RELEASE(ptr) if(ptr) ptr->Release()
#define LOG_HR(h) // just a stub

namespace xr
{
	static IDXGISwapChain* s_swap_chain;
	static ID3D11DeviceContext* s_device_context;
	static ID3D11Device* s_device;

	struct rd_texture
	{
		union
		{
			ID3D11Texture2D* texture_2d;
			ID3D11Texture3D* texture_3d;
		};
		ID3D11ShaderResourceView* shader_resource_view;
		union
		{
			ID3D11RenderTargetView* render_target_view;
			ID3D11DepthStencilView* depth_stencil_view;
		};
		rd_create_texture_params	params;
	};

	rd_texture* rd_texture_create(const rd_create_texture_params* params)
	{
		if (!params) return nullptr;

		rd_texture* ptr = new rd_texture();
		ptr->params = *params;

		HRESULT hr = S_OK;

		if (params->flags & RDTF_BACKBUFFER) {
			hr = s_swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)& texture_2d);
			LOG_HR(hr);
			if (hr != S_OK)
				return nullptr;

			ptr->params.flags |= RDTF_TEXTURE2D;
			D3D11_TEXTURE2D_DESC desc;
			texture_2d->GetDesc(&desc);
			ptr->params.width = desc.Width;
			ptr->params.height = desc.Height;
			ptr->params.array_size = 1;
			ptr->params.depth = 1;
			ptr->params.format = dx11_to_xr_format(desc.Format);
			ptr->params.mips = 1;
		}
		else if (params->flags & (RDTF_TEXTURE2D | RDTF_CUBEMAP | RDTF_DEPTH_STENCIL)) {
			D3D11_TEXTURE2D_DESC td;
			td.Format = xr_to_dx11_format(params->format);
			td.ArraySize = params=>array_size;
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
		return ptr;
	}

	void rd_texture_destroy(void* p)
	{
		if (p)
		{
			rd_texture* ptr = (rd_texture*)p;
			SAFE_RELEASE(ptr->shader_resource_view);
			SAFE_RELEASE(ptr->depth_stencil_view);
			SAFE_RELEASE(ptr->texture_2d);
			delete ptr;
		}
	}
		union {
			ID3D11Texture2D* texture_2d;
			ID3D11Texture3D* texture_3d;
		};
		ID3D11ShaderResourceView* shader_resource_view;
		union {
			ID3D11RenderTargetView* render_target_view;
			ID3D11DepthStencilView* depth_stencil_view;
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