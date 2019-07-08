#include <xr/string.h>
#include <xr/hash.h>
#include <mutex>

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

	STR_ID::STR_ID()
	{
		static auto empty = construct("");
		m_str = empty;
	}

	static std::mutex	s_strid_mutex;

#ifdef STRING_ID_USE_POOL
	static HASH_MAP<const char*, u32> s_strid_hashmap;

	char* STR_ID::m_pool;
	static u32	s_strid_pool_size;
	static u32	s_strid_pool_capacity;


	u32 STR_ID::construct(const char* str)
	{
		s_strid_mutex.lock();

		u32 ptr;
		if (s_strid_hashmap.find(str, ptr))
		{
			s_strid_mutex.unlock();
			return ptr;
		}

		int len = strlen(str);

		if (s_strid_pool_size + len >= s_strid_pool_capacity)
		{
			s_strid_pool_capacity += STRING_ID_POOL_GROW;
			char* new_pool = new char[s_strid_pool_capacity];
			memcpy(new_pool, m_pool, s_strid_pool_size);
			delete[] m_pool;
			m_pool = new_pool;
		}

		ptr = s_strid_pool_size;
		strcpy(m_pool + ptr, str);
		s_strid_pool_size += len + 1;

		s_strid_hashmap.insert(m_pool + ptr, ptr);

		s_strid_mutex.unlock();
		return ptr;
	}

#else

	static HASH_MAP<const char*, char*> s_strid_hashmap;

	const char* STR_ID::construct(const char* str)
	{
		s_strid_mutex.lock();

		char* ptr = nullptr;
		if (s_strid_hashmap.find(str, ptr))
		{
			s_strid_mutex.unlock();
			return ptr;
		}

		char* copy = new char[strlen(str) + 1];
		strcpy(copy, str);
		s_strid_hashmap.insert(str, copy);

		s_strid_mutex.unlock();
		return copy;
	}
#endif

	STRING_HOLDER STRING_HOLDER::duplicate(const char* str)
	{
		char* ptr = new char[strlen(str) + 1];
		strcpy(ptr, str);
		STRING_HOLDER sh(ptr);
		return sh;
	}

	static void decompose_filename(const char* filename, int& fb, int& fe, int& nb, int& ne, int& eb, int& ee)
	{
		const char* r1 = strrchr(filename, '/');
		const char* r2 = strrchr(filename, '\\');
		const char* r = Max(r1, r2);

		fb = 0;
		fe = r ? r - filename : 0;

		const char* name = r ? r + 1 : filename;

		const char* d = strchr(filename, '.');

		nb = name - filename;
		ne = d ? d - filename : strlen(filename);

		if (d)
		{
			eb = d + 1 - filename;
			ee = strlen(filename);
		}
		else
		{
			eb = ee = 0;
		}
	}

	STRING get_filename_name(const char* filename)
	{
		int fb, fe, nb, ne, eb, ee;
		decompose_filename(filename, fb, fe, nb, ne, eb, ee);
		return ne > nb ? STRING(filename + nb, ne - nb) : STRING();
	}

	STRING get_filename_extension(const char* filename)
	{
		int fb, fe, nb, ne, eb, ee;
		decompose_filename(filename, fb, fe, nb, ne, eb, ee);
		return ee > eb ? STRING(filename + eb, ee - eb) : STRING();
	}

	STRING get_filename_folder(const char* filename)
	{
		int fb, fe, nb, ne, eb, ee;
		decompose_filename(filename, fb, fe, nb, ne, eb, ee);
		return fe > fb ? STRING(filename + fb, fe - fb) : STRING();
	}
}