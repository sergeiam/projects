#include <xr/string.h>
#include <xr/hash.h>

namespace xr
{
	void STRING::operator += (const STRING& rhs)
	{
		int len = size();
		m_data.resize(len + rhs.size() + 1);
		memcpy(&m_data[len], rhs.c_str(), rhs.size());
		m_data[m_data.size() - 1] = '\0';
	}
	int STRING::find(const char* str) const
	{
		int len = strlen(str);
		const char* ptr = (len == 1) ? strchr(c_str(), *str) : strstr(c_str(), str);
		return ptr ? ptr - c_str() : -1;
	}
	int STRING::rfind(const char* str) const
	{
		int len = strlen(str);
		const char* ptr = nullptr;
		if (len == 1)
		{
			ptr = strrchr(c_str(), *str);
		}
		else
		{
			for (ptr = strstr(c_str(), str);;)
			{
				const char* next = strstr(ptr + 1, str);
				if (!next) break;
				ptr = next;
			}
		}
		return ptr ? ptr - c_str() : -1;
	}

	bool STRING::starts_with(const char* prefix) const
	{
		int len = strlen(prefix);
		return len <= size() && !strncmp(c_str(), prefix, len);
	}
	bool STRING::ends_with(const char* suffix) const
	{
		int len = strlen(suffix);
		return len <= size() && !strcmp(c_str() + size() - len, suffix);
	}
	STRING STRING::substr(int from, int to)
	{
		if (from >= size())
			return STRING();

		int p0 = from < 0 ? size() + from : from;
		int p1 = to < 0 ? size() + to : to;

		if (p0 > p1) return STRING();

		return STRING(c_str() + p0, p1 - p0 + 1);
	}

	static HASH_MAP<const char*, char*> s_strid_hashmap;

	STR_ID::STR_ID()
	{
		static const char* empty = construct("");
		m_str = empty;
	}

	const char* STR_ID::construct(const char* str)
	{
		char* ptr = nullptr;
		if (s_strid_hashmap.find(str, ptr))
		{
			return ptr;
		}
		char* copy = new char[strlen(str) + 1];
		strcpy(copy, str);
		s_strid_hashmap.insert(str, copy);
		return copy;
	}

	static STRING_HOLDER STRING_HOLDER::duplicate(const char* str)
	{
		char* ptr = new char[strlen(str) + 1];
		strcpy(ptr, str);
		STRING_HOLDER sh(ptr);
		return sh;
	}

}