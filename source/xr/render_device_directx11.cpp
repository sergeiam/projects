#include <xr/render_device.h>
#include <xr/window.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3d11shader.h>

#define SAFE_RELEASE(ptr) if(ptr) ptr->Release()

//D3D11_SHADER_TYPE_DESC
//D3D11_SHADER_BUFFER_DESC

namespace xr
{
	static IDXGISwapChain*		s_swap_chain;
	static ID3D11DeviceContext*	s_device_context;
	static ID3D11Device*		s_device;

#define LOG_HR(h) // just a stub

	struct RENDER_DEVICE_DX11 : public RENDER_DEVICE
	{
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
					hr = s_swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)& texture_2d);
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
				ID3D11Texture2D* texture_2d;
				ID3D11Texture3D* texture_3d;
			};
			ID3D11ShaderResourceView* shader_resource_view;
			union {
				ID3D11RenderTargetView* render_target_view;
				ID3D11DepthStencilView* depth_stencil_view;
			};
		};

		// --- TEXTURE implementation

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
				m_size = params.size;
				m_flags = params.flags;
				m_init_data = params.init_data;
			}
			virtual ~BUFFER_DX11()
			{
				SAFE_RELEASE(m_buffer);
			}
			bool allocate()
			{
				D3D11_BUFFER_DESC bd;
				bd.ByteWidth = size();
				bd.BindFlags = 0;
				if (m_flags & BF_VERTEX_BUFFER)
					bd.BindFlags |= D3D11_BIND_VERTEX_BUFFER;
				if (m_flags & BF_INDEX_BUFFER)
					bd.BindFlags |= D3D11_BIND_INDEX_BUFFER;
				if (m_flags & BF_CONSTANT_BUFFER)
					bd.BindFlags |= D3D11_BIND_CONSTANT_BUFFER;
				if (m_flags & BF_SHADER_RESOURCE)
					bd.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
				bd.CPUAccessFlags = (m_flags & BF_DYNAMIC) ? D3D11_CPU_ACCESS_WRITE : 0;
				bd.Usage = (m_flags & BF_DYNAMIC) ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
				bd.StructureByteStride = 0;
				bd.MiscFlags = 0;

				D3D11_SUBRESOURCE_DATA srd;
				srd.pSysMem = m_init_data;
				srd.SysMemPitch = 0;
				srd.SysMemSlicePitch = 0;

				HRESULT hr = s_device->CreateBuffer(&bd, m_init_data ? &srd : NULL, &m_buffer);
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
			ID3D11Buffer*	m_buffer;
			const void*		m_init_data;
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

		virtual void set_buffers(const BUFFER** ptr, u32 count, u32 slot, const u32* strides, const u32* offsets, u32 flags)
		{
			if (!ptr) return;

			ASSERT(count > 0 && count <= 8);

			ID3D11Buffer* pb[8];
			for (u32 i = 0; i < count; ++i) {
				pb[i] = ((BUFFER_DX11*)ptr[i])->m_buffer;
			}

			switch (ptr[0]->flags() & (BF_VERTEX_BUFFER | BF_INDEX_BUFFER | BF_CONSTANT_BUFFER))
			{
			case BF_VERTEX_BUFFER:
				s_device_context->IASetVertexBuffers(slot, count, pb, (const UINT*)strides, (const UINT*)offsets);
				break;

			case BF_INDEX_BUFFER:
				ASSERT(count == 1);
				s_device_context->IASetIndexBuffer(pb[0], (ptr[0]->flags() & BF_INDICES_16) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, offsets[0]);
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
					ds.StencilReadMask = m_params.stencil->read_mask;
					ds.StencilWriteMask = m_params.stencil->write_mask;
					ds.FrontFace.StencilDepthFailOp = xr_to_dx11_stencilop(m_params.stencil->front.depth_fail);
					ds.FrontFace.StencilFailOp = xr_to_dx11_stencilop(m_params.stencil->front.fail);
					ds.FrontFace.StencilPassOp = xr_to_dx11_stencilop(m_params.stencil->front.pass);
					ds.FrontFace.StencilFunc = xr_to_dx11_comparison(m_params.stencil->front.cmp_func);
					ds.BackFace.StencilDepthFailOp = xr_to_dx11_stencilop(m_params.stencil->back.depth_fail);
					ds.BackFace.StencilFailOp = xr_to_dx11_stencilop(m_params.stencil->back.fail);
					ds.BackFace.StencilPassOp = xr_to_dx11_stencilop(m_params.stencil->back.pass);
					ds.BackFace.StencilFunc = xr_to_dx11_comparison(m_params.stencil->back.cmp_func);
				}

				hr = s_device->CreateDepthStencilState(&ds, &depth_stencil_state);
				LOG_HR(hr);
				if (hr != S_OK)
					return false;

				D3D11_RASTERIZER_DESC rd;
				rd.AntialiasedLineEnable = FALSE;
				rd.CullMode = xr_to_dx11_cull_mode(m_params.cull_mode);
				rd.DepthBias = m_params.depth_bias;
				rd.DepthBiasClamp = 0.0f;
				rd.DepthClipEnable = (m_params.flags & RSF_DEPTH_CLIP) ? TRUE : FALSE;
				rd.FillMode = (m_params.flags & RSF_WIREFRAME) ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
				rd.FrontCounterClockwise = FALSE;
				rd.MultisampleEnable = FALSE;	// MSAA
				rd.ScissorEnable = (m_params.flags & RSF_SCISSOR) ? TRUE : FALSE;

				hr = s_device->CreateRasterizerState(&rd, &rasterizer_state);
				LOG_HR(hr);
				if (hr != S_OK)
					return false;

				D3D11_BLEND_DESC bd;
				bd.AlphaToCoverageEnable = (m_params.flags & RSF_ALPHA_TO_COVERAGE) ? TRUE : FALSE;
				bd.IndependentBlendEnable = (m_params.flags & RSF_INDEPENDENT_BLEND) ? TRUE : FALSE;
				for (int i = 0; i < 8; ++i) {
					if (i < m_params.blend_states.size())
					{
						bd.RenderTarget[i].BlendOp = xr_to_dx11_blendop(m_params.blend_states[i].blendop);
						bd.RenderTarget[i].BlendOpAlpha = xr_to_dx11_blendop(m_params.blend_states[i].blendop_alpha);
						bd.RenderTarget[i].SrcBlend = xr_to_dx11_blend(m_params.blend_states[i].src_blend);
						bd.RenderTarget[i].DestBlend = xr_to_dx11_blend(m_params.blend_states[i].dest_blend);
						bd.RenderTarget[i].SrcBlendAlpha = xr_to_dx11_blend(m_params.blend_states[i].src_blend_alpha);
						bd.RenderTarget[i].DestBlendAlpha = xr_to_dx11_blend(m_params.blend_states[i].dest_blend_alpha);
						bd.RenderTarget[i].RenderTargetWriteMask = m_params.blend_states[i].write_mask;
					}
					else
					{
						bd.RenderTarget[i].BlendOp = D3D11_BLEND_OP_ADD;
						bd.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_ADD;
						bd.RenderTarget[i].SrcBlend = D3D11_BLEND_ONE;
						bd.RenderTarget[i].DestBlend = D3D11_BLEND_ZERO;
						bd.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE;
						bd.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_ZERO;
						bd.RenderTarget[i].RenderTargetWriteMask = 0xFF;
					}
				}
				hr = s_device->CreateBlendState(&bd, &blend_state);
				LOG_HR(hr);
				if (hr != S_OK)
					return false;

				return true;
			}

			ID3D11DepthStencilState* depth_stencil_state = nullptr;
			ID3D11RasterizerState* rasterizer_state = nullptr;
			ID3D11BlendState* blend_state = nullptr;
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
			RENDER_STATE_DX11* ptr = (RENDER_STATE_DX11*)rs;
			s_device_context->RSSetState(ptr->rasterizer_state);
			s_device_context->OMSetDepthStencilState(ptr->depth_stencil_state, stencil);
			s_device_context->OMSetBlendState(ptr->blend_state, blend_factors, 0);
		}



		// --- INPUT LAYOUT

		struct INPUT_LAYOUT_DX11 : public INPUT_LAYOUT
		{
			INPUT_LAYOUT_DX11(const IL_PARAMS& params) : input_layout(nullptr)
			{
				m_params = params;
			}
			virtual ~INPUT_LAYOUT_DX11()
			{
				SAFE_RELEASE(input_layout);
			}
			bool allocate()
			{
				D3D11_INPUT_ELEMENT_DESC desc[16];
				ASSERT(m_params.elements.size() < 16);
				for (int i = 0; i < m_params.elements.size(); ++i)
				{
					const IL_ELEMENT& el = m_params.elements[i];
					switch (el.semantic)
					{
						case IL_SEMANTIC_POSITION:	desc[i].SemanticName = "Position"; break;
						case IL_SEMANTIC_TEXCOORD:	desc[i].SemanticName = "TexCoord"; break;
						case IL_SEMANTIC_NORMAL:	desc[i].SemanticName = "Normal"; break;
						case IL_SEMANTIC_COLOR:		desc[i].SemanticName = "Color"; break;
					}
					desc[i].SemanticIndex = el.semantic_index;
					desc[i].AlignedByteOffset = el.offset;
					desc[i].InstanceDataStepRate = el.instance_step_rate;
					desc[i].InputSlot = el.slot;
					desc[i].InputSlotClass = el.instance_step_rate ? D3D11_INPUT_PER_INSTANCE_DATA : D3D11_INPUT_PER_VERTEX_DATA;
					desc[i].Format = xr_to_dx11_format(el.format);
				}
				HRESULT hr = s_device->CreateInputLayout(desc, m_params.elements.size(), &m_params.vs_bytecode->bytecode[0], m_params.vs_bytecode->bytecode.size(), &input_layout);
				LOG_HR(hr);
				if (hr != S_OK)
					return false;
				return true;
			}
			ID3D11InputLayout* input_layout;
			IL_PARAMS			m_params;
		};
		virtual INPUT_LAYOUT* create_input_layout(const IL_PARAMS& params)
		{
			INPUT_LAYOUT_DX11* p = new INPUT_LAYOUT_DX11(params);
			if (!p->allocate())
			{
				delete p;
				return nullptr;
			}
			return p;
		}
		virtual void set_input_layout(const INPUT_LAYOUT* ptr)
		{
			INPUT_LAYOUT_DX11* il = (INPUT_LAYOUT_DX11*)ptr;
			s_device_context->IASetInputLayout(il->input_layout);
		}



		// --- SHADER

		virtual SHADER_BYTECODE* compile_shader(const SHADER_SOURCE& source)
		{
			D3D_SHADER_MACRO* definitions = nullptr;
			if (!source.macro_definitions) {
				const int count = source.macro_definitions->size() / 2;
				definitions = new D3D_SHADER_MACRO[count];
				for (int i = 0; i < count; ++i) {
					definitions[i].Name = (*source.macro_definitions)[i * 2].c_str();
					definitions[i].Definition = (*source.macro_definitions)[i * 2 + 1].c_str();
				}
			}

			UINT flags = 0;
			ID3DBlob* compiled_blob, * errors_blob;

			HRESULT hr = D3DCompile(
				source.source,
				strlen(source.source),
				source.filename.c_str(),
				definitions,
				D3D_COMPILE_STANDARD_FILE_INCLUDE,
				source.entry_function_name.c_str(),
				source.profile.c_str(),
				flags,
				0,
				&compiled_blob,
				&errors_blob
			);
			LOG_HR(hr);
			delete[] definitions;

			if (hr != S_OK) {
				SAFE_RELEASE(errors_blob);
				SAFE_RELEASE(compiled_blob);
				return nullptr;
			}

			SHADER_BYTECODE* sb = new SHADER_BYTECODE();
			sb->bytecode.resize(compiled_blob->GetBufferSize());
			memcpy(&sb->bytecode[0], compiled_blob->GetBufferPointer(), sb->bytecode.size());

			sb->flags = 0;
			switch (source.profile.c_str()[0])
			{
			case 'V':
			case 'v':
				sb->flags |= SF_VERTEX_SHADER;
				break;
			case 'P':
			case 'p':
				sb->flags |= SF_PIXEL_SHADER;
				break;
			case 'C':
			case 'c':
				sb->flags |= SF_COMPUTE_SHADER;
				break;
			default:
				ASSERT(0);
				return nullptr;
			}

			SAFE_RELEASE(compiled_blob);
			SAFE_RELEASE(errors_blob);
			return sb;
		}

		struct SHADER_DX11 : public SHADER
		{
			u32 flags;
			union {
				ID3D11VertexShader* vs;
				ID3D11PixelShader* ps;
				ID3D11ComputeShader* cs;
			};

			SHADER_DX11() : flags(0), vs(nullptr) {}

			virtual ~SHADER_DX11()
			{
				SAFE_RELEASE(vs);
			}
		};

		virtual SHADER* create_shader(const SHADER_BYTECODE& sb)
		{
			SHADER_DX11* ptr = new SHADER_DX11();

			HRESULT hr = S_OK;

			const void* pb = &sb.bytecode[0];
			const u32 size = sb.bytecode.size();

			ptr->flags = sb.flags;
			switch (ptr->flags)
			{
			case SF_VERTEX_SHADER:
				hr = s_device->CreateVertexShader(pb, size, NULL, &ptr->vs);
				break;
			case SF_PIXEL_SHADER:
				hr = s_device->CreatePixelShader(pb, size, NULL, &ptr->ps);
				break;
			case SF_COMPUTE_SHADER:
				hr = s_device->CreateComputeShader(pb, size, NULL, &ptr->cs);
				break;
			}
			LOG_HR(hr);
			if (hr != S_OK) {
				delete ptr;
				return nullptr;
			}

			ID3D11ShaderReflection* pReflector = NULL;
			D3DReflect(pb, size, IID_ID3D11ShaderReflection, (void**)& pReflector);

			for (UINT i = 0;; ++i)
			{
				ID3D11ShaderReflectionConstantBuffer* srcb = pReflector->GetConstantBufferByIndex(i);
				if (!srcb) break;

				D3D11_SHADER_BUFFER_DESC desc;
				srcb->GetDesc(&desc);
			}


			return ptr;
		}

		void set_shader(SHADER* shader)
		{
			SHADER_DX11* ptr = (SHADER_DX11*)shader;
			switch (ptr->flags & (SF_VERTEX_SHADER | SF_PIXEL_SHADER | SF_COMPUTE_SHADER))
			{
			case SF_VERTEX_SHADER:
				s_device_context->VSSetShader(ptr->vs, nullptr, 0);
				break;
			case SF_PIXEL_SHADER:
				s_device_context->PSSetShader(ptr->ps, nullptr, 0);
				break;
			case SF_COMPUTE_SHADER:
				s_device_context->CSSetShader(ptr->cs, nullptr, 0);
				break;
			}
		}

		// --- render target setup

		TEXTURE_DX11* m_render_targets[16] = { nullptr };
		TEXTURE_DX11* m_depth_stencil = nullptr;
		int				m_num_render_targets = 0;

		virtual void reset_render_targets()
		{
			for (int i = 0; i < m_num_render_targets; ++i)
				m_render_targets[i] = nullptr;
			m_num_render_targets = 0;
			m_depth_stencil = nullptr;
		}
		virtual void set_render_targets(TEXTURE* ptr, int index)
		{
			m_render_targets[index] = (TEXTURE_DX11*)ptr;
			m_num_render_targets = xr::Max(m_num_render_targets, index + 1);
		}
		virtual void set_depth_stencil(TEXTURE* ptr)
		{
			m_depth_stencil = (TEXTURE_DX11*)ptr;
		}
		virtual void clear_render_target(TEXTURE* texture, const float* rgba)
		{
			TEXTURE_DX11* p = (TEXTURE_DX11*)texture;
			ASSERT(p && p->render_target_view);
			if (!p || !p->render_target_view)
				return;
			FLOAT v[4] = { rgba[0], rgba[1], rgba[2], rgba[3] };
			s_device_context->ClearRenderTargetView(p->render_target_view, v);
		}
		virtual void clear_depth_stencil(TEXTURE* texture, int flags, float depth, u8 stencil)
		{
			TEXTURE_DX11* p = (TEXTURE_DX11*)texture;
			ASSERT(p && p->depth_stencil_view);
			ASSERT(flags);
			if (!p || !p->depth_stencil_view)
				return;

			UINT f = 0;
			if (flags & CLEAR_DEPTH) f |= D3D11_CLEAR_DEPTH;
			if (flags & CLEAR_STENCIL) f |= D3D11_CLEAR_STENCIL;
			ASSERT(f);

			if (f)
				s_device_context->ClearDepthStencilView(p->depth_stencil_view, f, depth, stencil);
		}
		void setup_render_targets()
		{
			ID3D11RenderTargetView* pp_color_views[16];
			for (int i = 0; i < m_num_render_targets; ++i) {
				pp_color_views[i] = m_render_targets[i]->render_target_view;
			}
			ID3D11DepthStencilView* dsv = m_depth_stencil ? m_depth_stencil->depth_stencil_view : NULL;
			s_device_context->OMSetRenderTargets(UINT(m_num_render_targets), pp_color_views, dsv);
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

		virtual void present()
		{
			if (s_swap_chain)
				s_swap_chain->Present(1, 0);
		}

#define XR_TO_DX_FORMAT_MAPPING	\
		ITEM(FMT_UNKNOWN,				DXGI_FORMAT_UNKNOWN) \
		ITEM(FMT_R8G8B8A8,				DXGI_FORMAT_R8G8B8A8_UNORM)	\
		ITEM(FMT_R16F,					DXGI_FORMAT_R16_FLOAT) \
		ITEM(FMT_R16G16F,				DXGI_FORMAT_R16G16_FLOAT) \
		ITEM(FMT_R16G16B16F,			DXGI_FORMAT_R16G16B16A16_FLOAT) \
		ITEM(FMT_R32F,					DXGI_FORMAT_R32_FLOAT) \
		ITEM(FMT_R32G32F,				DXGI_FORMAT_R32G32_FLOAT) \
		ITEM(FMT_R32G32B32F,			DXGI_FORMAT_R32G32B32_FLOAT) \
		ITEM(FMT_R32G32B32A32F,			DXGI_FORMAT_R32G32B32A32_FLOAT) \
		ITEM(FMT_R32U,					DXGI_FORMAT_R32_UINT) \
		ITEM(FMT_D32_FLOAT,				DXGI_FORMAT_D32_FLOAT) \
		ITEM(FMT_D24_S8,				DXGI_FORMAT_D24_UNORM_S8_UINT) \
		ITEM(FMT_D16,					DXGI_FORMAT_D16_UNORM)

		static FORMAT dx11_to_xr_format(DXGI_FORMAT format)
		{
			switch (format)
			{
				#define ITEM(tf,dxgi) case dxgi: return tf;
					XR_TO_DX_FORMAT_MAPPING
				#undef ITEM
			}
			return FMT_UNKNOWN;
		}
		static DXGI_FORMAT xr_to_dx11_format(FORMAT format)
		{
			switch (format)
			{
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
		static D3D11_STENCIL_OP xr_to_dx11_stencilop(STENCILOP op)
		{
			switch (op)
			{
				case STENCILOP_KEEP:			return D3D11_STENCIL_OP_KEEP;
				case STENCILOP_ZERO:			return D3D11_STENCIL_OP_ZERO;
				case STENCILOP_REPLACE:			return D3D11_STENCIL_OP_REPLACE;
				case STENCILOP_INCREASE_SAT:	return D3D11_STENCIL_OP_INCR_SAT;
				case STENCILOP_DECREASE_SAT:	return D3D11_STENCIL_OP_DECR_SAT;
				case STENCILOP_INVERT:			return D3D11_STENCIL_OP_INVERT;
				case STENCILOP_INCREASE:		return D3D11_STENCIL_OP_INCR;
				case STENCILOP_DECREASE:		return D3D11_STENCIL_OP_DECR;
			}
			return D3D11_STENCIL_OP_KEEP;
		}
		static D3D11_CULL_MODE xr_to_dx11_cull_mode(CULL cull)
		{
			switch (cull)
			{
				case CULL_FRONT: return D3D11_CULL_FRONT;
				case CULL_BACK: return D3D11_CULL_BACK;
			}
			return D3D11_CULL_NONE;
		}
		static D3D11_BLEND_OP xr_to_dx11_blendop(BLENDOP op)
		{
			switch (op)
			{
				case BLENDOP_SUB:		return D3D11_BLEND_OP_SUBTRACT; break;
				case BLENDOP_REV_SUB:	return D3D11_BLEND_OP_REV_SUBTRACT; break;
				case BLENDOP_MIN:		return D3D11_BLEND_OP_MIN; break;
				case BLENDOP_MAX:		return D3D11_BLEND_OP_MAX; break;
			}
			return D3D11_BLEND_OP_ADD;
		}
		static D3D11_BLEND xr_to_dx11_blend(BLEND blend)
		{
			switch (blend)
			{
				case BLEND_ONE:					return D3D11_BLEND_ONE;
				case BLEND_SRC_COLOR:			return D3D11_BLEND_SRC_COLOR;
				case BLEND_INV_SRC_COLOR:		return D3D11_BLEND_INV_SRC_COLOR;
				case BLEND_SRC_ALPHA:			return D3D11_BLEND_SRC_ALPHA;
				case BLEND_INV_SRC_ALPHA:		return D3D11_BLEND_INV_SRC_ALPHA;
				case BLEND_DEST_COLOR:			return D3D11_BLEND_DEST_COLOR;
				case BLEND_INV_DEST_COLOR:		return D3D11_BLEND_INV_DEST_COLOR;
				case BLEND_DEST_ALPHA:			return D3D11_BLEND_DEST_ALPHA;
				case BLEND_INV_DEST_ALPHA:		return D3D11_BLEND_INV_DEST_ALPHA;
				case BLEND_BLEND_FACTOR:		return D3D11_BLEND_BLEND_FACTOR;
				case BLEND_INV_BLEND_FACTOR:	return D3D11_BLEND_INV_BLEND_FACTOR;
			}
			return D3D11_BLEND_ZERO;
		}
	};

	RENDER_DEVICE* create_device_directx11(WINDOW* wnd, const RENDER_DEVICE::DEVICE_PARAMS& params)
	{
		RENDER_DEVICE_DX11* ptr = new RENDER_DEVICE_DX11();
		if (!ptr->create(wnd, params))
		{
			delete ptr;
		}
		return ptr;
	}
}