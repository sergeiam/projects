#pragma once

#include <xr/core.h>

namespace xr
{
	class WINDOW;

	struct RENDER_DEVICE
	{
		enum FORMAT
		{
			FMT_UNKNOWN,
			FMT_BYTE4,

			FMT_FLOAT16_1,
			FMT_FLOAT16_2,
			FMT_FLOAT16_4,

			FMT_FLOAT32_1,
			FMT_FLOAT32_2,
			FMT_FLOAT32_3,
			FMT_FLOAT32_4,

			FMT_UINT32_1,

			FMT_D32_FLOAT,
			FMT_D24_S8,
			FMT_D16,

			FMT_MAX
		};

		enum {
			TF_TEXTURE2D		= 1 << 0,
			TF_TEXTURE3D		= 1 << 1,
			TF_CUBEMAP			= 1 << 2,
			TF_BACKBUFFER		= 1 << 3,
			TF_DEPTH_STENCIL	= 1 << 4,
			TF_RENDER_TARGET	= 1 << 5,
			TF_SHADER_RESOURCE	= 1 << 6,
			TF_DYNAMIC			= 1 << 7,

			BF_VERTEX_BUFFER	= 1 << 0,
			BF_INDEX_BUFFER		= 1 << 1,
			BF_CONSTANT_BUFFER	= 1 << 2,
			BF_SHADER_RESOURCE	= 1 << 3,
			BF_DYNAMIC			= 1 << 4,
			BF_INDICES_16		= 1 << 5,
			BF_INDICES_32		= 1 << 6,

			MAP_READ			= 1 << 0,
			MAP_WRITE			= 1 << 1,
			MAP_DISCARD			= 1 << 2,
			MAP_NO_OVERWRITE	= 1 << 3,

			SF_VERTEX_SHADER	= 1 << 0,
			SF_PIXEL_SHADER		= 1 << 1,
			SF_COMPUTE_SHADER	= 1 << 2,
		};

		struct TEXTURE_PARAMS
		{
			FORMAT	format;
			u32		width, height, depth;
			u32		array_size;
			u32		mips;
			u32		flags;
		};

		struct TEXTURE
		{
			virtual ~TEXTURE() {}

			const TEXTURE_PARAMS& params() const { return m_params; }
		protected:
			TEXTURE_PARAMS	m_params;
		};



		struct BUFFER_PARAMS
		{
			u32		size;
			u32		flags;
			void*	init_data;
		};

		struct MAPPED_DATA
		{
			void *	data;
			u32		row_pitch;
			u32		depth_pitch;
		};

		struct BUFFER
		{
			virtual ~BUFFER() {}

			virtual bool map(MAPPED_DATA& md, u32 flags) = 0;
			virtual void unmap() = 0;

			const BUFFER_PARAMS& params() const { return m_params; }
		protected:
			BUFFER_PARAMS	m_params;
		};



// --- RENDER STATE

		enum CMP : unsigned
		{
			CMP_ALWAYS,
			CMP_LESS,
			CMP_LESS_EQUAL,
			CMP_EQUAL,
			CMP_GREATER_EQUAL,
			CMP_GREATER,
			CMP_NEVER
		};

		enum STENCIL_OP : unsigned
		{
			STENCILOP_KEEP,
			STENCILOP_ZERO,
			STENCILOP_REPLACE,
			STENCILOP_INCREASE_SAT,
			STENCILOP_DECREASE_SAT,
			STENCILOP_INVERT,
			STENCILOP_INCREASE,
			STENCILOP_DECREASE
		};

		struct STENCIL_FACE_OP
		{
			STENCIL_OP	fail : 3, depth_fail : 3, pass : 3;
			CMP			cmp_func : 3;

			STENCIL_FACE_OP() {
				fail = depth_fail = pass = STENCILOP_KEEP;
				cmp_func = CMP_ALWAYS;
			}
		};

		enum CULL : unsigned
		{
			CULL_NONE,
			CULL_FRONT,
			CULL_BACK
		};

		enum BLEND : unsigned
		{
			BLEND_ZERO,
			BLEND_ONE,
			BLEND_SRC_COLOR,
			BLEND_INV_SRC_COLOR,
			BLEND_SRC_ALPHA,
			BLEND_INV_SRC_ALPHA,
			BLEND_DEST_COLOR,
			BLEND_INV_DEST_COLOR,
			BLEND_DEST_ALPHA,
			BLEND_INV_DEST_ALPHA,
			BLEND_BLEND_FACTOR,
			BLEND_INV_BLEND_FACTOR
		};

		enum BLENDOP : unsigned
		{
			BLENDOP_ADD,
			BLENDOP_SUB,
			BLENDOP_REV_SUB,
			BLENDOP_MIN,
			BLENDOP_MAX
		};

		struct RENDER_TARGET_BLEND_STATE
		{
			BLEND		src_blend : 4;
			BLEND		dest_blend : 4;
			BLEND		src_blend_alpha : 4;
			BLEND		dest_blend_alpha : 4;
			BLENDOP		blend_op : 3;
			BLENDOP		blend_op_alpha : 3;
			u8			write_mask;

			RENDER_TARGET_BLEND_STATE() {
				src_blend = src_blend_alpha = BLEND_ONE;
				dest_blend = dest_blend_alpha = BLEND_ZERO;
				blend_op = blend_op_alpha = BLENDOP_ADD;
				write_mask = 0xFF;
			}
		};

		enum {
			RSF_DEPTH_WRITE			= 1 << 0,
			RSF_WIREFRAME			= 1 << 1,
			RSF_DEPTH_CLIP			= 1 << 2,
			RSF_ALPHA_TO_COVERAGE	= 1 << 3,
			RSF_INDEPENDENT_BLEND	= 1 << 4,
			RSF_SCISSOR				= 1 << 5
		};

		struct RENDER_STATE_STENCIL
		{
			u8					stencil_read_mask = 0;
			u8					stencil_write_mask = 0;
			STENCIL_FACE_OP		stencil_front;
			STENCIL_FACE_OP		stancil_back;
		};

		struct RENDER_STATE_PARAMS
		{
			u8						flags = RSF_DEPTH_WRITE;
			// depth stencil state
			CMP						depth_func : 8;
			RENDER_STATE_STENCIL*	stencil = nullptr;

			// rasterizer state
			CULL					cull_mode = CULL_BACK;
			i32						depth_bias = 0;
			float					slope_scale_depth_bias = 0.0f;
			// blend state
			xr::VECTOR<RENDER_TARGET_BLEND_STATE>	blend_states;

			RENDER_STATE_PARAMS() : depth_func(CMP_LESS) {}
			~RENDER_STATE_PARAMS()
			{
				delete stencil;
			}
		};

		struct RENDER_STATE
		{
			virtual ~RENDER_STATE() {}

			const RENDER_STATE_PARAMS& params() const { return m_params; }

		protected:
			RENDER_STATE_PARAMS	m_params;
		};

		struct SHADER_BYTECODE
		{
			VECTOR<u8>	bytecode;
		};

		class SHADER
		{

		};



		struct INPUT_LAYOUT_ELEMENT
		{
			u32		semantic;
			u32		semantic_index;
			u32		slot;
			u32		offset;
			u32		instance_step_rate;
			FORMAT	format;
		};

		struct INPUT_LAYOUT_PARAMS
		{
			INPUT_LAYOUT_ELEMENT*	elements;
			u32						elements_count;
			SHADER_BYTECODE*		vs_bytecode;
		};

		struct INPUT_LAYOUT
		{
			virtual ~INPUT_LAYOUT() {}

			virtual bool map(MAPPED_DATA& md, u32 flags) = 0;
			virtual void unmap() = 0;

			const BUFFER_PARAMS& params() const { return m_params; }
		protected:
			BUFFER_PARAMS	m_params;
		};

		struct DEVICE_PARAMS
		{
			FORMAT	back_buffer_format;
			int		msaa;
		};

		virtual bool			create(WINDOW* window, const DEVICE_PARAMS& params) = 0;
		virtual TEXTURE*		create_texture(const TEXTURE_PARAMS& params) = 0;
		virtual BUFFER*			create_buffer(const BUFFER_PARAMS& params) = 0;
		virtual RENDER_STATE*	create_render_state(const RENDER_STATE_PARAMS& params) = 0;

		virtual void			set_buffers(const BUFFER** ptr, u32 count, u32 slot, const u32 * strides, const u32 * offsets, u32 flags) = 0;
		virtual void			set_render_state(const RENDER_STATE* rs, u8 stencil = 0, const float* blend_factors = nullptr) = 0;

		virtual void			set_render_state(RENDER_STATE*) = 0;


		virtual ~RENDER_DEVICE() {}
	};

	RENDER_DEVICE* create_device_directx11(WINDOW* wnd);
}