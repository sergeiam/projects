#pragma once

#include <xr/core.h>
#include <xr/render_device.h>

namespace xr
{
	class TEXTURE
	{
	public:
		struct PARAMS
		{
			u32						width, height, depth;
			RENDER_DEVICE::FORMAT	format;
		};
		TEXTURE(const char* filename);
		TEXTURE(const PARAMS& params);
		~TEXTURE();

		bool init(const char* filename);
		bool init(const PARAMS& params);

		RENDER_DEVICE::TEXTURE*		get_texture() { return m_texture; }

	private:
		RENDER_DEVICE::TEXTURE*		m_texture;
	};

	class HR_TEXTURE_MANAGER
	{
	public:

	};
}