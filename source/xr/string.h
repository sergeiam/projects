#pragma once

#include <xr/core.h>
#include <xr/vector.h>
#include <string.h>

namespace xr
{
	class STRING
	{
		VECTOR<char> m_data;

	public:
		STRING()
		{
		}
		STRING(const char* str)
		{
			int len = strlen(str);
			m_data.resize(len+1);
			memcpy( &m_data[0], str, len+1 );
		}
		STRING(const char* str, int len)
		{
			m_data.resize(len+1);
			memcpy(&m_data[0], str, len);
			m_data.back() = '\0';
		}
		STRING(const STRING&& rhs)
		{
			m_data = rhs.m_data;
		}

		void operator = (const STRING& rhs)
		{
			m_data = rhs.m_data;
		}
		bool empty() const
		{
			return m_data.empty() || m_data[0] == '\0';
		}
		int size() const
		{
			return empty() ? 0 : m_data.size()-1;
		}
		const char* c_str() const
		{
			return m_data.empty() ? "" : &m_data[0];
		}
		operator const char*() const
		{
			return c_str();
		}
		void operator += (const STRING& rhs);

		int find(const char* str) const;

		int rfind(const char* str) const;

		bool starts_with(const char* prefix) const;

		bool ends_with(const char* suffix) const;

		STRING substr(int from, int to = -1);
	};

	class STR_ID
	{
	public:
		STR_ID();

		STR_ID(const char* str)
		{
			m_str = construct(str);
		}
		STR_ID(const STR_ID& rhs)
		{
			m_str = rhs.m_str;
		}
		STR_ID(STR_ID&& rhs)
		{
			m_str = rhs.m_str;
		}
		bool operator == (const STR_ID& rhs) const
		{
			return m_str == rhs.m_str;
		}
		bool operator < (const STR_ID& rhs) const
		{
			if (m_str == rhs.m_str) return false;
			return strcmp(m_str, rhs.m_str) < 0;
		}
		void operator = (const STR_ID& rhs)
		{
			m_str = rhs.m_str;
		}
		const char* c_str() const
		{
			return m_str;
		}
		bool empty() const
		{
			return !m_str[0];
		}
		u32 size() const
		{
			return strlen(m_str);
		}

	private:
		const char* construct(const char* str);
		const char*	m_str;
	};

	u32 hash(STR_ID x) { return hash(x.c_str()); }

	class STRING_HOLDER
	{
	public:
		STRING_HOLDER() : m_str(nullptr) {}
		STRING_HOLDER(const char* str) : m_str(str) {}
		~STRING_HOLDER() {}

		const char* c_str() { return m_str; }

		void destroy() { delete[] m_str; }

		static STRING_HOLDER duplicate(const char* str);

	private:
		const char* m_str;
	};

	STRING get_filename_name(const char* filename);
	STRING get_filename_extension(const char* filename);
	STRING get_filename_folder(const char* filename);
}