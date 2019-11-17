
// Hash table implementing list-based collision resolving, list allocated inside a continuouis vector, Sergey Miloykov (sergei.m()gmail.com)

#pragma once
#include <xhash>



template< class K, class V > class hashmap_flatlist
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

	hashmap_flatlist(int capacity = 16)
	{
		m_hash = nullptr;
		m_items = nullptr;
		m_capacity = m_size = 0;
		m_allow_resize = true;
		resize(capacity);
	}
	hashmap_flatlist(hashmap_flatlist&& rhs)
	{
		m_hash = rhs.m_hash;
		m_items = rhs.m_items;
		m_free_list = rhs.m_free_list;
		m_size = rhs.m_size;
		m_capacity = rhs.m_capacity;
		rhs.m_hash = nullptr;
		rhs.m_items = nullptr;
		rhs.m_size = rhs.m_capacity = 0;
		rhs.m_free_list = -1;
	}
	~hashmap_flatlist()
	{
		delete[] m_hash;
		delete[] m_items;
	}

	class iterator
	{
		friend class hashmap_flatlist;

		hashmap_flatlist* map;
		int hash_index, list_index;

	public:
		iterator(hashmap_flatlist* _map, int hi, int li) : map(_map), hash_index(hi), list_index(li) {}

		void operator++()
		{
			const int n = map->m_capacity;
			const int* ph = map->m_hash + hash_index;
			const item* pi = map->m_items + list_index;

			if (list_index >= 0)
			{
				list_index = map->m_items[list_index].next;
			}
				
			if(list_index == -1)
			{
				if (hash_index == n) return;

				int* ph = map->m_hash;
				do
				{
					hash_index++;
				}
				while (hash_index < n && ph[hash_index] == -1);
				list_index = (hash_index < n) ? ph[hash_index] : -1;
			}
		}
		item* operator->()
		{
			return &map->m_items[list_index];
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
		if (!m_capacity) return end();

		if (m_hash[0] != -1)
			return iterator(this, 0, m_hash[0]);

		iterator it(this, 0, -1);
		++it;
		return it;
	}
	iterator end()
	{
		return iterator(this, m_capacity, -1);
	}

	iterator find(const K& key)
	{
		int h = CLAMP_HASH_RANGE(stdext::hash_value(key));

		for (int li = m_hash[h]; li != -1; li = m_items[li].next)
		{
			if (m_items[li].first == key)
			{
				return iterator(this, h, li);
			}
		}
		return end();
	}

	iterator find_or_insert(const K& key)
	{
		int h = CLAMP_HASH_RANGE(stdext::hash_value(key));

		int li = m_hash[h];

		if (li == -1)
		{
			if (should_grow())
			{
				resize(m_capacity * 2);
				return find_or_insert(key);
			}

			li = m_free_list;
			m_hash[h] = li;
			m_free_list = m_items[m_free_list].next;

			m_items[li].first = key;
			m_items[li].next = -1;
			m_size++;

			return iterator(this, h, li);
		}

		for(;;)
		{
			if (m_items[li].first == key)
			{
				return iterator(this, h, li);
			}
			if (m_items[li].next == -1)
			{
				if (should_grow())
				{
					resize(m_capacity * 2);
					return find_or_insert(key);
				}

				int next = m_free_list;
				m_free_list = m_items[m_free_list].next;
				m_items[li].next = next;

				m_items[next].first = key;
				m_items[next].next = -1;

				m_size++;

				return iterator(this, h, li);
			}
			li = m_items[li].next;
		}
	}

	iterator erase(iterator it)
	{
		const bool return_next_iterator = false;

		int li = m_hash[it.hash_index];
		if (li == it.list_index)
		{
			int next = m_items[li].next;

			m_items[li].first.~K();
			m_items[li].second.~V();

			m_hash[it.hash_index] = next;

			m_items[li].next = m_free_list;
			m_free_list = li;

			m_size--;

			if (!return_next_iterator) return end();

			it.list_index = next;
			return it;
		}

		for (int i = li;; i = m_items[li].next)
		{
			int next = m_items[i].next;
			if (next == li)
			{
				m_items[i].next = m_items[li].next;
				m_items[li].first.~K();
				m_items[li].second.~V();
				m_items[li].next = m_free_list;
				m_free_list = li;

				m_size--;

				if (!return_next_iterator) return end();

				++it;
				return it;
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
		delete[] m_items;
		delete[] m_hash;
		m_size = m_capacity = 0;
		m_items = nullptr;
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

		hashmap_flatlist<K, V> temp(std::move(*this));

		m_capacity = new_capacity;
		m_hash = new int[new_capacity];
		m_items = new item[new_capacity];

		for (int i = 0; i < new_capacity; ++i)
		{
			m_items[i].next = i+1;
			m_hash[i] = -1;
		}
		m_items[new_capacity - 1].next = -1;
		m_free_list = 0;
		m_size = 0;

		for (auto it = temp.begin(); it != temp.end(); ++it)
		{
			(*this)[it->first] = it->second;
		}
	}

private:
	int*	m_hash;
	item*	m_items;
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