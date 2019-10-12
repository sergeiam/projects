#include <string>
#include <vector>



struct STRING_RANGE
{
	int start, end;
};

class STRING_POOL
{
	const char*					m_string;
	STRING_RANGE				m_ranges[256];
	int							m_num_allocated;

	std::vector<STRING_RANGE>	m_vranges;
	STRING_RANGE*				m_ranges_ptr;

public:
	STRING_POOL(const char* string)
	{
		m_string = string;
		m_num_allocated = 0;
		m_ranges_ptr = m_ranges;
	}

	STRING_RANGE& operator[](int index)
	{
		return m_ranges_ptr[index];
	}

	const char* get_string(int range)
	{
		return m_string + m_ranges_ptr[range].start;
	}

	const char* get_string()
	{
		return m_string;
	}

	int allocate_range()
	{
		if (m_vranges.empty())
		{
			int ret = m_num_allocated;
			m_num_allocated++;
			if (m_num_allocated <= sizeof(m_ranges) / sizeof(m_ranges[0]))
				return ret;

			m_vranges.resize(m_num_allocated);
			memcpy(&m_vranges[0], m_ranges, ret * sizeof(m_ranges[0]));
			return ret;
		}

		int ret = m_vranges.size();
		m_vranges.resize(m_vranges.size() + 1);
		m_ranges_ptr = &m_vranges[0];
		return ret;
	}
};

class STRING_RANGE_LIST
{
	STRING_POOL& m_pool;

	int	m_begin, m_end;

public:
	STRING_RANGE_LIST(STRING_POOL& pool, bool top_level) : m_pool(pool)
	{
		m_begin = m_end = 0;
		add_range(0, strlen(pool.get_string()));
	}

	STRING_RANGE& at(int index)
	{
		return m_pool[m_begin + index];
	}
	STRING_RANGE& operator[](int index)
	{
		return at(index);
	}

	std::pair<const char*, int> get_string(int range) const
	{
		const STRING_RANGE& r = m_pool[range];
		return std::make_pair(m_pool.get_string() + r.start, r.end - r.start);
	}

	int size() { return m_end - m_begin; }

	STRING_RANGE_LIST split(int range, char separator)
	{
		STRING_RANGE_LIST result(m_pool);

		const char* s = m_pool.get_string();

		for (int start = at(range).start, end = at(range).end; start < end; )
		{
			const char* n = strchr(s + start, separator);
			int next = n ? n - s : end;

			if (next > end) next = end;

			result.add_range(start, next);
			start = next + 1;
		}
		return result;
	}

private:
	STRING_RANGE_LIST(STRING_POOL& pool) : m_pool(pool)
	{
		m_begin = m_end = 0;
	}

	void add_range(int a, int b)
	{
		int x = m_pool.allocate_range();
		if (m_begin == m_end)
			m_begin = m_end = x;

		m_pool[m_end].start = a;
		m_pool[m_end].end = b;
		m_end++;
	}
};

int main()
{
	const char* str = "a,b,c; d,e,f; g,h,i / 1,2,3; 4,5,6; 7,8,9 / ou ; omaya";

	STRING_POOL pool(str);

	STRING_RANGE_LIST root(pool, true);

	STRING_RANGE_LIST groups = root.split(0, '/');
	for (int i = 0; i < groups.size(); ++i)
	{
		STRING_RANGE_LIST inner = groups.split(i, ';');

		for (int j = 0; j < inner.size(); ++j)
		{
			STRING_RANGE_LIST elements = inner.split(j, ',');

			for (int k = 0; k < elements.size(); ++k)
			{
				std::string s;
				elements.get_string(k, s);
				printf("%s\n", s.c_str());
			}
		}
	}
	return 0;
}
