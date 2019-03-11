#pragma once

#include <xr/vector.h>
#include <xr/hash.h>
#include <xr/string.h>
#include <xr/render_device.h>
#include <xr/shader.h>



namespace xr
{
	class ROP
	{
		SHADER*	vs;
		SHADER*	ps;
		SHADER*	cs;
		RENDER_DEVICE::RENDER_STATE* rs;
	};

	class TECHNIQUE
	{
	public:
		TECHNIQUE() {}

		ROP* get_rop(const SHADER_VARIANT& var);

	private:
		struct SHADER_PARAMS
		{
			STRING m_filename;
			STRING m_entry;
		};
		SHADER_PARAMS	m_vs, m_ps, m_cs;
		RENDER_DEVICE::RENDER_STATE_PARAMS	m_rs;

		HASH_MAP<SHADER_VARIANT, ROP*>	m_cache;
	};

	class TECHNIQUE_MANAGER
	{
	public:
		bool load(const char* filename);

		TECHNIQUE* get_technique(STR_ID shader, STR_ID pass);

		struct KEY
		{
			STR_ID shader, pass;
		};
		HASH_MAP<KEY, TECHNIQUE*>	m_cache;
	};

}