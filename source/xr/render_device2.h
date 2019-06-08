#pragma once

#include <xr/core.h>
#include <xr/vector.h>
#include <xr/string.h>
#include <xr/window.h>

namespace xr
{
	struct rd_texture;
	struct rd_buffer;
	struct rd_input_layout;
	struct rd_render_state;
	struct rd_shader;

	enum rd_format
	{
		RDFMT_UNKNOWN,
		RDFMT_R8G8B8A8,

		RDFMT_R16F,
		RDFMT_R16G16F,
		RDFMT_R16G16B16F,

		RDFMT_R32F,
		RDFMT_R32G32F,
		RDFMT_R32G32B32F,
		RDFMT_R32G32B32A32F,

		RDFMT_R32U,

		RDFMT_D32_FLOAT,
		RDFMT_D24_S8,
		RDFMT_D16,

		RDFMT_MAX
	};

	enum
	{
		// render device texture flags
		RDTF_TEXTURE2D = 1 << 0,
		RDTF_TEXTURE3D = 1 << 1,
		RDTF_CUBEMAP = 1 << 2,
		RDTF_BACKBUFFER = 1 << 3,
		RDTF_DEPTH_STENCIL = 1 << 4,
		RDTF_RENDER_TARGET = 1 << 5,
		RDTF_SHADER_RESOURCE = 1 << 6,
		RDTF_DYNAMIC = 1 << 7,

		// render device buffer flags
		RDBF_VERTEX_BUFFER = 1 << 0,
		RDBF_INDEX_BUFFER = 1 << 1,
		RDBF_CONSTANT_BUFFER = 1 << 2,
		RDBF_SHADER_RESOURCE = 1 << 3,
		RDBF_DYNAMIC = 1 << 4,
		RDBF_INDICES_16 = 1 << 5,
		RDBF_INDICES_32 = 1 << 6,

		RDMAP_READ = 1 << 0,
		RDMAP_WRITE = 1 << 1,
		RDMAP_DISCARD = 1 << 2,
		RDMAP_NO_OVERWRITE = 1 << 3,

		RDSF_VERTEX_SHADER = 1 << 0,
		RDSF_PIXEL_SHADER = 1 << 1,
		RDSF_COMPUTE_SHADER = 1 << 2,

		RDCLEAR_DEPTH = 1 << 0,
		RDCLEAR_STENCIL = 1 << 1,
	};

// --- texture

	struct rd_create_texture_params
	{
		rd_format	format;
		u32			width, height, depth;
		u32			array_size;
		u32			mips;
		u32			flags;
	};

	rd_texture*						rd_texture_create(const rd_create_texture_params* params);
	void							rd_texture_destroy(rd_texture* handle);
	const rd_create_texture_params* rd_texture_get_params(rd_texture* handle);

// --- buffer

	struct rd_buffer_params
	{
		u32		size;
		u32		flags;
		void*	init_data;
	};

	struct rd_mapped_data
	{
		void*	data;
		u32		row_pitch;
		u32		depth_pitch;
	};

	u32		rd_buffer_create(const rd_buffer_params* params);
	void	rd_buffer_destroy(u32 handle);
	bool	rd_buffer_map(u32 handle, rd_mapped_data* data, u32 flags);
	void	rd_buffer_unmap(u32 handle);

// --- render state

	enum rd_compare : unsigned
	{
		RDCMP_ALWAYS,
		RDCMP_LESS,
		RDCMP_LESS_EQUAL,
		RDCMP_EQUAL,
		RDCMP_GREATER_EQUAL,
		RDCMP_GREATER,
		RDCMP_NEVER
	};

	enum rd_stencil_operation : unsigned
	{
		RDSTENCILOP_KEEP,
		RDSTENCILOP_ZERO,
		RDSTENCILOP_REPLACE,
		RDSTENCILOP_INCREASE_SAT,
		RDSTENCILOP_DECREASE_SAT,
		RDSTENCILOP_INVERT,
		RDSTENCILOP_INCREASE,
		RDSTENCILOP_DECREASE
	};

	struct rd_stencil_faceop
	{
		rd_stencil_operation	fail : 3, depth_fail : 3, pass : 3;
		rd_compare				cmp_func : 3;

		rd_stencil_faceop() {
			fail = depth_fail = pass = RDSTENCILOP_KEEP;
			cmp_func = RDCMP_ALWAYS;
		}
	};

	enum rd_cull : unsigned
	{
		RDCULL_NONE,
		RDCULL_FRONT,
		RDCULL_BACK
	};

	enum rd_blend : unsigned
	{
		RDBLEND_ZERO,
		RDBLEND_ONE,
		RDBLEND_SRC_COLOR,
		RDBLEND_INV_SRC_COLOR,
		RDBLEND_SRC_ALPHA,
		RDBLEND_INV_SRC_ALPHA,
		RDBLEND_DEST_COLOR,
		RDBLEND_INV_DEST_COLOR,
		RDBLEND_DEST_ALPHA,
		RDBLEND_INV_DEST_ALPHA,
		RDBLEND_RDBLEND_FACTOR,
		RDBLEND_INV_RDBLEND_FACTOR
	};

	enum rd_blendop : unsigned
	{
		RDBLENDOP_ADD,
		RDBLENDOP_SUB,
		RDBLENDOP_REV_SUB,
		RDBLENDOP_MIN,
		RDBLENDOP_MAX
	};

	struct rd_render_target_blend_state
	{
		rd_blend	src_blend : 4;
		rd_blend	dest_blend : 4;
		rd_blend	src_blend_alpha : 4;
		rd_blend	dest_blend_alpha : 4;
		rd_blendop	blendop : 3;
		rd_blendop	blendop_alpha : 3;
		u8			write_mask;

		rd_render_target_blend_state()
		{
			src_blend = src_blend_alpha = RDBLEND_ONE;
			dest_blend = dest_blend_alpha = RDBLEND_ZERO;
			blendop = blendop_alpha = RDBLENDOP_ADD;
			write_mask = 0xFF;
		}
	};

	enum {
		RDRSF_DEPTH_WRITE = 1 << 0,
		RDRSF_WIREFRAME = 1 << 1,
		RDRSF_DEPTH_CLIP = 1 << 2,
		RDRSF_ALPHA_TO_COVERAGE = 1 << 3,
		RDRSF_INDEPENDENT_BLEND = 1 << 4,
		RDRSF_SCISSOR = 1 << 5
	};

	struct rd_stencil_state
	{
		u8					read_mask = 0;
		u8					write_mask = 0;
		rd_stencil_faceop	front;
		rd_stencil_faceop	back;
	};

	struct rd_render_state_params
	{
		u8						flags = RDRSF_DEPTH_WRITE;
		// depth stencil state
		rd_compare				depth_func : 8;
		rd_stencil_state*		stencil = nullptr;

		// rasterizer state
		rd_cull					cull_mode = RDCULL_BACK;
		i32						depth_bias = 0;
		float					slope_scale_depth_bias = 0.0f;
		// blend state
		VECTOR<rd_render_target_blend_state>	blend_states;

		rd_render_state_params() : depth_func(RDCMP_LESS) {}
		~rd_render_state_params()
		{
			delete stencil;
		}
	};

	u32		rd_render_state_create(const rd_render_state_params* params);
	void	rd_render_state_destroy(u32 handle);
	void	rd_render_state_set(u32 handle);

// --- shader

	struct rd_shader_bytecode
	{
		VECTOR<u8>	bytecode;
		u32			flags;
	};

	struct rd_shader_source
	{
		STRING			source;
		STR_ID			filename;
		STR_ID			entry_function_name;
		STR_ID			profile;
		u32				flags;
		u32				optimization_level;
		VECTOR<STR_ID>	macro_definitions;
	};

	bool	rd_shader_compile(const rd_shader_source* source, rd_shader_bytecode* bytecode);
	u32		rd_shader_create(const rd_shader_bytecode* bytecode);
	void	rd_shader_destroy(u32 handle);
	void	rd_shader_set_vs(u32 handle);
	void	rd_shader_set_ps(u32 handle);
	void	rd_shader_set_cs(u32 handle);

// --- input layout

	enum rd_input_layout_semantic
	{
		RDIL_SEMANTIC_POSITION,
		RDIL_SEMANTIC_TEXCOORD,
		RDIL_SEMANTIC_NORMAL,
		RDIL_SEMANTIC_COLOR,
	};

	struct rd_input_layout_element
	{
		rd_input_layout_semantic	semantic;
		u32							semantic_index;
		u32							slot;
		u32							offset;
		u32							instance_step_rate;
		rd_format					format;
	};

	struct rd_input_layout_params
	{
		VECTOR<rd_input_layout_element>	elements;
		rd_shader_bytecode*				vs_bytecode;
	};

	u32		rd_input_layout_create(const rd_input_layout_params* params);
	void	rd_input_layout_destroy(u32 handle);
	void	rd_input_layout_set(u32 handle);

// --- render targets

	void	rd_render_target_clear(u32 handle, const float* rgba);
	void	rd_depth_stencil_clear(u32 handle, u32 flags, float depth, u8 stencil);
	void	rd_render_target_set(u32* handles, u32 count);
	void	rd_depth_stencil_set(u32 handle);

// --- render device

	struct rd_device_params
	{
		rd_format	back_buffer_format;
		int			msaa;
	};

	void	rd_device_create(WINDOW* window, const rd_device_params* params);
	void	rd_device_destroy();
	void	rd_present();
	void	rd_draw_indexed_primitive(u32 index_count, u32 start_index, u32 base_vertex);
	void	rd_draw_indexed_instanced_primitive(
		u32 index_count_per_instance,
		u32 instance_count,
		u32 start_index_location,
		u32 base_vertex,
		u32 start_instance_location
	);

}