#include <string.h>
#include <unordered_map>
#include <mutex>

class STRING
{
	char* m_str;

public:
	STRING()
	{
		m_str = empty_string();
	}
	STRING(const char* str)
	{
		m_str = construct(str, strlen(str));
	}
	STRING(const char* str, int len)
	{
		m_str = construct(str, len);
	}
	STRING(const STRING& rhs)
	{
		m_str = rhs.m_str;
	}

	void operator = (const STRING& rhs)
	{
		m_str = rhs.m_str;
	}
	bool empty() const
	{
		return m_str[0] == '\0';
	}
	int size() const
	{
		return strlen(m_str);
	}
	const char* c_str() const
	{
		return m_str;
	}
	operator const char* () const
	{
		return c_str();
	}
	void operator += (const STRING& rhs)
	{
		int len = size();
		int len2 = rhs.size();
		char* ptr = new char[len + len2 + 1];
		memcpy(ptr, c_str(), len);
		memcpy(ptr + len, rhs.c_str(), len2);
		ptr[len + len2] = 0;

		m_str = construct(ptr, len + len2);
	}
	int find(const char* str) const
	{
		int len = strlen(str);
		const char* ptr = (len == 1) ? strchr(c_str(), *str) : strstr(c_str(), str);
		return ptr ? ptr - c_str() : -1;
	}
	int rfind(const char* str) const;

	bool starts_with(const char* prefix) const;

	bool ends_with(const char* suffix) const;

	STRING substr(int from, int to = -1)
	{
		if (from >= size())
			return STRING();

		int p0 = from < 0 ? size() + from : from;
		int p1 = to < 0 ? size() + to : to;

		if (p0 > p1) return STRING();

		return STRING(c_str() + p0, p1 - p0 + 1);
	}

private:
	struct hash_func
	{
		size_t operator()(const std::pair<const char*, int>& x)
		{
			size_t h = 31;
			for (int i = 0; i < x.second; ++i)
			{
				h = (h << 1) ^ (h >> 31) ^ x.first[i];
			}
			return h;
		}
	};

	struct equal_func
	{
		bool operator()(const std::pair<const char*, int>& a, const std::pair<const char*, int>& b)
		{
			if (a.second != b.second) return false;
			return strncmp(a.first, b.first, a.second) == 0;
		}
	};

	std::unordered_map<std::pair<const char*, int>, char*, hash_func, equal_func> m_cache;

	std::mutex m_critical_section;

	char* construct(const char* str, int len)
	{
		std::lock_guard<std::mutex> lock(m_critical_section);

		auto it = m_cache.find(std::make_pair(str, len));
		if (it != m_cache.end())
		{
			return it->second;
		}

		char* ptr = new char[len + 1];
		memcpy(ptr, str, len);
		ptr[len] = 0;

		m_cache[std::make_pair(ptr, len)] = ptr;
		return ptr;
	}
	char* empty_string()
	{
		static char* empty = construct("", 0);
		return empty;
	}
};


int main()
{

	return 0;
}