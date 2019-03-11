#pragma once

#include <xr/core.h>
#include <xr/vector.h>
#include <xr/string.h>



namespace xr
{
	class WINDOW;

	struct RENDER_DEVICE
	{
		enum FORMAT
		{
			FMT_UNKNOWN,
			FMT_R8G8B8A8,

			FMT_R16F,
			FMT_R16G16F,
			FMT_R16G16B16F,

			FMT_R32F,
			FMT_R32G32F,
			FMT_R32G32B32F,
			FMT_R32G32B32A32F,

			FMT_R32U,

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

			CLEAR_DEPTH = 1 << 0,
			CLEAR_STENCIL = 1 << 1,
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

		enum STENCILOP : unsigned
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
			STENCILOP	fail : 3, depth_fail : 3, pass : 3;
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
			BLENDOP		blendop : 3;
			BLENDOP		blendop_alpha : 3;
			u8			write_mask;

			RENDER_TARGET_BLEND_STATE() {
				src_blend = src_blend_alpha = BLEND_ONE;
				dest_blend = dest_blend_alpha = BLEND_ZERO;
				blendop = blendop_alpha = BLENDOP_ADD;
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

		struct STENCIL
		{
			u8					read_mask = 0;
			u8					write_mask = 0;
			STENCIL_FACE_OP		front;
			STENCIL_FACE_OP		back;
		};

		struct RENDER_STATE_PARAMS
		{
			u8						flags = RSF_DEPTH_WRITE;
			// depth stencil state
			CMP						depth_func : 8;
			STENCIL*				stencil = nullptr;

			// rasterizer state
			CULL					cull_mode = CULL_BACK;
			i32						depth_bias = 0;
			float					slope_scale_depth_bias = 0.0f;
			// blend state
			VECTOR<RENDER_TARGET_BLEND_STATE>	blend_states;

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
			u32			flags;
		};

		struct SHADER_SOURCE
		{
			const char*		source;
			STR_ID			filename;
			STR_ID			entry_function_name;
			STR_ID			profile;
			u32				flags;
			u32				optimization_level;
			const VECTOR<STR_ID>*	macro_definitions;
		};

		struct SHADER
		{
			virtual ~SHADER() {}
		};

		enum IL_SEMANTIC
		{
			IL_SEMANTIC_POSITION,
			IL_SEMANTIC_TEXCOORD,
			IL_SEMANTIC_NORMAL,
			IL_SEMANTIC_COLOR,
		};

		struct IL_ELEMENT
		{
			IL_SEMANTIC		semantic;
			u32				semantic_index;
			u32				slot;
			u32				offset;
			u32				instance_step_rate;
			FORMAT			format;
		};

		struct IL_PARAMS
		{
			VECTOR<IL_ELEMENT>	elements;
			SHADER_BYTECODE*	vs_bytecode;
		};

		struct INPUT_LAYOUT
		{
			virtual ~INPUT_LAYOUT() {}

			const IL_PARAMS& params() const { return m_params; }
		protected:
			IL_PARAMS	m_params;
		};

		struct DEVICE_PARAMS
		{
			FORMAT	back_buffer_format;
			int		msaa;
		};

		virtual bool				create(WINDOW* window, const DEVICE_PARAMS& params) = 0;
		virtual TEXTURE*			create_texture(const TEXTURE_PARAMS& params) = 0;
		virtual BUFFER*				create_buffer(const BUFFER_PARAMS& params) = 0;
		virtual RENDER_STATE*		create_render_state(const RENDER_STATE_PARAMS& params) = 0;
		virtual INPUT_LAYOUT*		create_input_layout(const IL_PARAMS& params) = 0;
		virtual SHADER_BYTECODE*	compile_shader(const SHADER_SOURCE& source) = 0;
		virtual SHADER*				create_shader(const SHADER_BYTECODE& bytecode) = 0;

		virtual void				set_buffers(const BUFFER** ptr, u32 count, u32 slot, const u32 * strides, const u32 * offsets, u32 flags) = 0;
		virtual void				set_render_state(const RENDER_STATE* rs, u8 stencil = 0, const float* blend_factors = nullptr) = 0;
		virtual void				set_input_layout(const INPUT_LAYOUT* ptr) = 0;
		virtual void				set_shader(SHADER* shader) = 0;

		virtual void				reset_render_targets() = 0;
		virtual void				set_render_targets(TEXTURE* ptr, int index) = 0;
		virtual void				set_depth_stencil(TEXTURE* ptr) = 0;
		virtual void				clear_render_target(TEXTURE* ptr, const float* rgba) = 0;
		virtual void				clear_depth_stencil(TEXTURE* ptr, int flags, float depth, u8 stencil) = 0;

		virtual void				present() = 0;

		virtual ~RENDER_DEVICE() {}
	};

	RENDER_DEVICE* create_device_directx11(WINDOW* wnd, const RENDER_DEVICE::DEVICE_PARAMS& params);

	RENDER_DEVICE*	g_render_device;
}