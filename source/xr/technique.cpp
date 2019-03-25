#include <xr/technique.h>
#include <sdk/pugixml/src/pugixml.hpp>

namespace xr
{
	struct KEY
	{
		STR_ID	shader, pass;
	};

	struct VALUE
	{
		RENDER_DEVICE::RENDER_STATE*	rs;
		HASH_MAP<SHADER_VARIANT, ROP*>	rops;
	};

	static HASH_MAP<KEY, VALUE>	s_techniques;

	bool technique_load(const char* filename)
	{
		pugi::xml_document doc;
		pugi::xml_parse_result res = doc.load_file(filename);

		if (res == pugi::status_ok)
		{
			asset_error( "Failed to load '%s'", filename );
			return false;
		}

		STR_ID sid_shader = get_filename_name(filename).c_str();

		const pugi::xml_node& root = doc.document_element();
		for (auto it = root.begin(); it != root.end(); ++it)
		{
			if (strcmp(it->name(), "technique"))
			{
				asset_error("Unknown tag found in technique '%s' - '%s'", filename, it->name());
				continue;
			}

			pugi::xml_attribute pass = it->attribute("pass");
			if (pass.empty())
			{
				asset_error("Technique '%s' in ")
			}

			STR_ID sid_pass(pass.name());

			KEY key;
			key.shader = sid_shader;
			key.pass = sid_pass;

			if()


			if (!s_techniques.find(sid_shader))
			{
				s_techniques.insert(sid_shader, new SHADER_PASSES());
			}
			if (!s_techniques[sid_shader]->find(sid_pass))
			{
				s_techniques[sid_shader]->insert(sid_pass, new PASS_VARIANTS());
			}

			SHADER_VARIANT sv;

			s_techniques[sid_shader][sid_pass][sv] = x;





		}
	}

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

		ROP* get_rop(STR_ID shader, STR_ID pass, const SHADER_VARIANT& variant);

		struct KEY
		{
			STR_ID shader, pass;
			SHADER_VARIANT
		};
		HASH_MAP<KEY, TECHNIQUE*>	m_cache;
	};


}