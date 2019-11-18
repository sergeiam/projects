
// Hash table implementing list-based collision resolving, list allocated inside a continuouis vector, Sergey Miloykov (sergei.m()gmail.com)
// TODO:
//		- try hybrid approach, where the first element actually reside in the hash array itself, so we can hit the element direcly in a sparse table

#pragma once
#include <xhash>



template< class K, class V > class hashmap_flatlist2
{
	friend class iterator;

	int CLAMP_HASH_RANGE(size_t hash)
	{
		int h = hash ^ (hash >> 32ull);
		return h & (m_capacity - 1);
	}

public:
	typedef K key_type;
	typedef V value_type;

	struct item
	{
		K	first;
		V	second;
		int	next;
	};

	hashmap_flatlist2(int capacity = 16)
	{
		m_hash = nullptr;
		m_capacity = m_size = 0;
		m_allow_resize = true;
		resize(capacity);
	}
	hashmap_flatlist2(hashmap_flatlist2&& rhs)
	{
		m_hash = rhs.m_hash;
		m_free_list = rhs.m_free_list;
		m_size = rhs.m_size;
		m_capacity = rhs.m_capacity;
		rhs.m_hash = nullptr;
		rhs.m_size = rhs.m_capacity = 0;
		rhs.m_free_list = -1;
	}
	~hashmap_flatlist2()
	{
		delete[] m_hash;
	}

	class iterator
	{
		friend class hashmap_flatlist2;

		hashmap_flatlist2* map;
		int hash_index, list_index;

	public:
		iterator(hashmap_flatlist2* _map, int hi, int li) : map(_map), hash_index(hi), list_index(li) {}

		void operator++()
		{
			const int n = map->m_capacity;

			if (list_index >= 0)
			{
				list_index = map->m_hash[list_index].next;
			}

			if (list_index == -1)
			{
				if (hash_index == n) return;

				const item* ph = map->m_hash;
				do
				{
					hash_index++;
				}
				while (hash_index < n && ph[hash_index].next == -2);

				list_index = (hash_index < n) ? hash_index : -1;
			}
		}
		item* operator->()
		{
			return &map->m_hash[list_index];
		}
		bool operator == (iterator rhs) const
		{
			return hash_index == rhs.hash_index && list_index == rhs.list_index;
		}
		bool operator != (iterator rhs) const
		{
			return !(*this == rhs);
		}
		void operator = (iterator& rhs)
		{
			map = rhs.map;
			hash_index = rhs.hash_index;
			list_index = rhs.list_index;
		}
	};

	iterator begin()
	{
		if (!m_capacity)
			return end();

		if (m_hash[0].next == -2)
		{
			iterator it(this, 0, -1);
			++it;
			return it;
		}
		return iterator(this, 0, 0);
	}
	iterator end()
	{
		return iterator(this, m_capacity, -1);
	}

	iterator find(const K& key)
	{
		int h = CLAMP_HASH_RANGE(stdext::hash_value(key));

		if (m_hash[h].next == -2)
		{
			return end();
		}

		for (int li = h; li != -1; li = m_hash[li].next)
		{
			if (m_hash[li].first == key)
			{
				return iterator(this, h, li);
			}
		}
		return end();
	}

	iterator find_or_insert(const K& key)
	{
		int h = CLAMP_HASH_RANGE(stdext::hash_value(key));

		if (m_hash[h].next == -2)
		{
			if (should_grow())
			{
				resize(m_capacity * 2);
				return find_or_insert(key);
			}

			m_hash[h].first = key;
			m_hash[h].next = -1;
			m_size++;

			return iterator(this, h, h);
		}

		for (int li = h; ; li = m_hash[li].next)
		{
			if (m_hash[li].first == key)
			{
				return iterator(this, h, li);
			}
			if (m_hash[li].next == -1)
			{
				if (should_grow())
				{
					resize(m_capacity * 2);
					return find_or_insert(key);
				}

				int next = m_free_list;
				m_free_list = m_hash[m_free_list].next;
				m_hash[li].next = next;

				m_hash[next].first = key;
				m_hash[next].next = -1;

				m_size++;

				return iterator(this, h, next);
			}
		}
	}

	iterator erase(iterator it)
	{
		const bool return_next_iterator = false;

		m_size--;

		if (it.hash_index == it.list_index)
		{
			if (m_hash[it.hash_index].next == -1)
			{
				m_hash[it.hash_index].next = -2;

				if (return_next_iterator)
				{
					++it;
					return it;
				}
				return end();
			}


			int next = m_hash[it.hash_index].next;

			m_hash[it.hash_index].first = m_hash[next].first;
			m_hash[it.hash_index].second = m_hash[next].second;
			m_hash[it.hash_index].next = m_hash[next].next;

			m_hash[next].next = m_free_list;
			m_free_list = next;

			return return_next_iterator ? it : end();
		}

		for (int i = it.hash_index; ; i = m_hash[i].next)
		{
			if (m_hash[i].next == it.list_index)
			{
				int next = m_hash[it.list_index].next;

				m_hash[i].next = next;
				m_hash[it.list_index].next = m_free_list;
				m_free_list = it.list_index;

				if (return_next_iterator)
				{
					it.list_index = i;
					++it;
					return it;
				}
				return end();
			}
		}
	}

	V& operator[] (const K& key)
	{
		iterator it = find_or_insert(key);
		return it->second;
	}

	void clear()
	{
		delete[] m_hash;
		m_size = m_capacity = 0;
		m_hash = nullptr;

		resize(16);
	}
	size_t size() const
	{
		return m_size;
	}
	bool empty() const
	{
		return size() == 0;
	}

	void resize(int new_capacity)
	{
		int log = 0;
		for (int cap = 1; cap < new_capacity; ++log, cap *= 2);
		new_capacity = 1U << log;

		if (new_capacity < size())
		{
			return;
		}

		hashmap_flatlist2<K, V> temp(std::move(*this));

		m_capacity = new_capacity;
		m_hash = new item[new_capacity*2];

		for (int i = 0; i < new_capacity*2; ++i)
		{
			m_hash[i].next = i < new_capacity ? -2 : i + 1;
		}
		m_hash[new_capacity*2 - 1].next = -1;
		m_free_list = new_capacity;
		m_size = 0;

		for (auto it = temp.begin(); it != temp.end(); ++it)
		{
			(*this)[it->first] = it->second;
		}
	}

private:
	item*	m_hash;
	int		m_size, m_capacity, m_free_list;
	bool	m_allow_resize;

	bool should_grow() const
	{
		return m_allow_resize && m_size + 1 >= m_capacity * 3 / 4;
	}
	bool should_shrink() const
	{
		return m_allow_resize && m_size < m_capacity * 3 / 8;
	}
};