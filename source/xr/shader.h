#pragma once

#include <xr/render_device.h>
#include <xr/shader_variant.h>

namespace xr
{
	class SHADER_VARIANT
	{
	public:
		VECTOR<STR_ID>	m_defines;
	};

	class SHADER
	{
	public:
		SHADER();

		bool init(const char* filename, const char* entry, const char* profile, const SHADER_VARIANT& sv);

	private:
		RENDER_DEVICE::SHADER*	m_shader;
		SHADER_VARIANT			m_sv;
	};
}