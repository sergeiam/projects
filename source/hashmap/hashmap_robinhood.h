
// Hash table implementing Robin-Hood collision scheme, Sergey Miloykov (sergei.m()gmail.com)
// TODO:
//		- split keys and values for the case where size of the value type is big, so we can traverse and search the table faster
//		- set the 'return next iterator after erase' as a mode for the table, so it can work both ways
//		- test non-power-of-2 table size
//		- constant iterators


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
	typedef K key_type;
	typedef V value_type;

	struct item
	{
		K	first;
		V	second;
		int	distance;
	};

	hashmap_robinhood(int capacity = 16)
	{
		m_capacity = m_size = 0;
		m_items = nullptr;
		m_allow_resize = true;
		resize(capacity);
	}
	~hashmap_robinhood()
	{
		delete[] m_items;
	}

	class iterator
	{
		friend class hashmap_robinhood;

		hashmap_robinhood* map;
		int index;

	public:
		iterator(hashmap_robinhood* rh, int i) : map(rh), index(i) {}

		void operator++()
		{
			const int n = map->m_capacity;
			const item* ptr = map->m_items + index;
			int i = index;
			do {
				i++;
				ptr++;
			} while (i < n && ptr->distance == hashmap_robinhood::UNUSED);
			index = i;
		}
		item* operator->()
		{
			return &(*map)(index);
		}
		bool operator == (iterator rhs) const
		{
			return index == rhs.index;
		}
		bool operator != (iterator rhs) const
		{
			return index != rhs.index;
		}
		void operator = (iterator& rhs)
		{
			map = rhs.map;
			index = rhs.index;
		}
	};

	iterator begin()
	{
		if (m_items[0].distance != UNUSED)
			return iterator(this, 0);

		iterator it(this, 0);
		++it;
		return it;
	}
	iterator end()
	{
		return iterator(this, m_capacity);
	}

	iterator find(const K& key)
	{
		int h = CLAMP_HASH_RANGE( stdext::hash_value(key) );

		const item* ptr = m_items + h;
		const item* ptr_end = m_items + m_capacity;

		for (int d = 0; ; ++d)
		{
			if (ptr->distance < d) break;

			if (ptr->first == key) return iterator(this, int(ptr - m_items));

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
					resize(m_capacity * 2);
					m_allow_resize = false;
					iterator it = find_or_insert(elem.first);
					m_allow_resize = true;
					it->second = elem.second;
					return it;
				}

				*ptr = elem;
				m_size++;

				return iterator(this, index == -1 ? int(ptr - m_items) : index);
			}
			if (ptr->first == key)
			{
				return iterator(this, int(ptr - m_items));
			}
			if (ptr->distance < elem.distance)
			{
				item next = *ptr;
				*ptr = elem;
				elem = next;
				if (index == -1)
					index = int(ptr - m_items);
			}
			elem.distance++;
			if (++ptr == ptr_end) ptr = m_items;
		}
		return iterator(this, index);
	}

	iterator erase(iterator it)
	{
		bool return_next_iterator = false;

		if (should_shrink())
		{
			const K key = it->first;
			resize(m_capacity / 2);

			auto it = find(key);
			m_allow_resize = false;
			it = erase(it);
			m_allow_resize = true;

			return it;
		}

		item* ptr = m_items + it.index;
		item* ptr_last = ptr;
		const item* ptr_end = m_items + m_capacity;

		m_size--;

		ptr->first.~K();
		ptr->second.~V();
		ptr->distance = UNUSED;

		bool passed_end = false;

		for (int distance = 1;; ++distance)
		{
			if (++ptr == ptr_end)
			{
				ptr = m_items;
				passed_end = true;
			}

			if (distance > ptr->distance) break;

			ptr_last->first = std::move(ptr->first);
			ptr_last->second = std::move(ptr->second);
			ptr_last->distance = ptr->distance - distance;

			ptr->distance = UNUSED;
			ptr_last = ptr;
			distance = 0;
		}

		if (!return_next_iterator)
		{
			return end();
		}

		if (passed_end) return end();

		if( it->distance == UNUSED ) ++it;

		return it;
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
					m_items[i].first.~K();
					m_items[i].second.~V();
					m_items[i].distance = UNUSED;
				}
			}
			m_size = 0;
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

	void resize(int new_capacity)
	{
		int log = 0;
		for (int cap = 1; cap < new_capacity; ++log, cap *= 2);
		new_capacity = 1U << log;

		if (new_capacity < size())
		{
			return;
		}

		item* prev_items = m_items;
		int   prev_capacity = m_capacity;

		m_capacity = new_capacity;
		m_items = new item[new_capacity];
		for (int i = 0; i < new_capacity; m_items[i++].distance = UNUSED);

		if (m_size)
		{
			m_size = 0;
			for (int i = 0; i < prev_capacity; ++i)
			{
				if (prev_items[i].distance != UNUSED)
				{
					auto it = find_or_insert(prev_items[i].first);
					it->second = std::move(prev_items[i].second);
				}
			}
		}
		delete[] prev_items;
	}

private:
	item* m_items;
	int		m_size, m_capacity;
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