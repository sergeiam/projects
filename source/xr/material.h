#pragma once

#include <xr/vector.h>
#include <xr/string.h>
#include <xr/render_device.h>
#include <xr/texture.h>

namespace xr
{
	class MATERIAL
	{
		struct PAIR
		{
			STR_ID		name;
			STRING		value;
			TEXTURE*	obj;
		};
		VECTOR<PAIR>	textures;
		VECTOR<PAIR>	floats;
	};
}