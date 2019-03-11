#include <xr/shader.h>
#include <xr/hash.h>

namespace xr
{
	u32 hash(const RENDER_DEVICE::SHADER_SOURCE& ss)
	{

		return h;
	}

	SHADER::SHADER():
		m_shader(nullptr)
	{
	}

	struct TEXT_ITEM
	{
		STRING_HOLDER	text;
		u32				hash;
	};

	struct SOURCE_ITEM : public RENDER_DEVICE::SHADER_SOURCE
	{
		u32 m_hash;

		void compute_hash(u32 text_hash)
		{
			u32 h = text_hash;
			if (macro_definitions)
			{
				for (u32 i = 0; i < macro_definitions->size(); ++i)
				{
					h ^= hash((*macro_definitions)[i]);
				}
			}
			h = (h << 1) ^ hash(filename);
			h = (h << 1) ^ hash(entry_function_name);
			h = (h << 1) ^ hash(profile);
			h = (h << 1) ^ hash(flags);
			h = (h << 1) ^ hash(optimization_level);
			m_hash = h;
		}
	};

	u32 hash(const SOURCE_ITEM& si)
	{
		return si.m_hash;
	}

	static HASH_MAP<STR_ID, TEXT_ITEM>	s_text_map;
	static HASH_MAP<SOURCE_ITEM, RENDER_DEVICE::SHADER_BYTECODE*> s_bytecode_map;

	bool SHADER::init(const char* filename, const char* entry, const char* profile, const SHADER_VARIANT& sv)
	{
		// filename -> shader source

		STR_ID file(filename);
		TEXT_ITEM ti;

		if (!s_text_map.find(file))
		{
			FILE* fp = FILE::open(filename, "rb");
			if (!fp)
			{
				return false;
			}
			char* text = new char[fp->size() + 1];
			fp->read(text, fp->size());
			text[fp->size()] = 0;
			delete fp;

			ti.text = STRING_HOLDER(text);
			ti.hash = hash_string(text);
			s_text_map[file] = ti;
		}
		else
		{
			ti = s_text_map[file];
		}

		// shader source -> bytecode

		SOURCE_ITEM si;

		si.source = ti.text.c_str();
		si.filename = file;
		si.entry_function_name = entry;
		si.profile = profile;
		si.flags = 0;
		si.optimization_level = 0;
		si.macro_definitions = sv.m_defines.empty() ? nullptr : &sv.m_defines;

		si.compute_hash(ti.hash);

		RENDER_DEVICE::SHADER_BYTECODE* sbc = nullptr;
		if (!s_bytecode_map.find(si, sbc))
		{
			sbc = g_render_device->compile_shader(si);
			if (!sbc)
			{
				return false;
			}
			s_bytecode_map[si] = sbc;
		}

		// bytecode -> device shader

		m_shader = g_render_device->create_shader(*sbc);
		if (!m_shader)
		{
			return false;
		}

		return true;
	}


}