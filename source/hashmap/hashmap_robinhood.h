#pragma once

#include <xhash>

template< class K, class V > class hashmap_robinhood
{
	friend class iterator;

	int CLAMP_HASH_RANGE(size_t hash)
	{
		int h = hash ^ (hash >> 32ull);
		return h & (m_capacity - 1);
	}

	const static int UNUSED = -1;

public:
	struct item
	{
		K	first;
		V	second;
		int	distance;
	};

	hashmap_robinhood(int capacity_log = 3)
	{
		m_size = 0;
		m_capacity_log = capacity_log;
		m_capacity = 1 << capacity_log;
		m_items = new item[m_capacity];
		for (int i = 0; i < m_capacity; ++i)
			m_items[i].distance = UNUSED;
	}
	~hashmap_robinhood()
	{
		delete[] m_items;
	}

	struct iterator
	{
		hashmap_robinhood& map;
		int index;

		iterator(hashmap_robinhood& rh, int i) : map(rh), index(i) {}
		//iterator(iterator&& rhs) : map(rhs.map), index(rhs.index) {}

		void operator++()
		{
			const int n = map.m_capacity;
			do {
				index++;
			} while (index < n && map(index).distance == hashmap_robinhood::UNUSED);
		}
		item* operator->()
		{
			return &map(index);
		}
		bool operator == (iterator rhs) const
		{
			return index == rhs.index;
		}
		bool operator != (iterator rhs) const
		{
			return index != rhs.index;
		}
	};

	iterator begin()
	{
		iterator it(*this, 0);
		++it;
		return it;
	}
	iterator end()
	{
		return iterator(*this, m_capacity);
	}

	iterator find(const K& key)
	{
		int h = CLAMP_HASH_RANGE( stdext::hash_value(key) );

		const item* ptr = m_items + h;
		const item* ptr_end = m_items + m_capacity;

		for (int d = 0; ; ++d)
		{
			if (ptr->distance < d) break;

			if (ptr->first == key) return iterator(*this,ptr - m_items);

			if (++ptr == ptr_end) ptr = m_items;
		}
		return end();
	}

	iterator find_or_insert(const K& key)
	{
		int h = CLAMP_HASH_RANGE(stdext::hash_value(key));

		item elem;
		elem.first = key;
		elem.distance = 0;

		int index = -1;

		item* ptr = m_items + h;
		const item* ptr_end = m_items + m_capacity;

		for (;;)
		{
			if (ptr->distance == UNUSED)
			{
				if (should_grow())
				{
					resize(m_capacity_log + 1);
					iterator i = find_or_insert(elem.first);
					i->second = elem.second;
					m_size++;

					return find(key);
				}

				*ptr = elem;
				m_size++;

				return iterator(*this, index == -1 ? ptr - m_items : index);
			}
			if (ptr->first == key)
			{
				return iterator(*this, ptr - m_items);
			}
			if (ptr->distance < elem.distance)
			{
				item next = *ptr;
				*ptr = elem;
				elem = next;
				if (index == -1)
					index = ptr - m_items;
			}
			elem.distance++;
			if (++ptr == ptr_end) ptr = m_items;
		}
		return iterator(*this, index);
	}

	V& operator[] (const K& key)
	{
		iterator it = find_or_insert(key);
		return it->second;
	}

	void clear()
	{
		if (!empty())
		{
			for (int i = 0, n = m_capacity; i < n; ++i)
			{
				if (m_items[i].distance != UNUSED)
				{
					m_items[i].second::~V();
					m_items[i].distance = UNUSED;
				}
			}
		}
	}
	size_t size() const
	{
		return m_size;
	}
	bool empty() const
	{
		return size() == 0;
	}

	struct item& operator()(int index)
	{
		return m_items[index];
	}

	void resize(int new_capacity_log)
	{
		int new_capacity = 1U << new_capacity_log;
		if (new_capacity < size())
		{
			return;
		}

		item* prev_items = m_items;
		int    prev_capacity = m_capacity;

		m_capacity = new_capacity;
		m_capacity_log = new_capacity_log;
		m_items = new item[new_capacity];

		if (m_size)
		{
			m_size = 0;
			for (int i = 0; i < prev_capacity; ++i)
			{
				auto it = find_or_insert(prev_items[i].first);
				it->second = prev_items[i].second;
			}
		}
		delete[] prev_items;
	}

private:
	item* m_items;
	int		m_size, m_capacity, m_capacity_log;

	bool should_grow() const
	{
		return m_size + 1 >= m_capacity * 3 / 4;
	}
};