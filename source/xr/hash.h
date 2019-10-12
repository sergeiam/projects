#pragma once

#include <xr/core.h>
#include <xr/vector.h>

namespace xr
{
	template< class K, class V > class HASH_MAP
	{
	public:
		HASH_MAP()
		{
			m_pool_mask = 31;
			m_pool.resize(32);
		}

		bool empty() const { return size() == 0; }
		u32 size() const { return m_size; }

		bool find(const K& key) const
		{
			const u32 k = hash(key) & m_pool_mask;
			for (u32 i = 0, n = m_pool[k].size(); i < n; ++i)
			{
				if (m_pool[k][i].key == key)
				{
					return true;
				}
			}
			return false;
		}
		bool find(const K& key, V& value) const
		{
			const u32 k = hash(key) & m_pool_mask;
			for (u32 i = 0, n = m_pool[k].size(); i < n; ++i)
			{
				if (m_pool[k][i].key == key)
				{
					value = m_pool[k][i].value;
					return true;
				}
			}
			return false;
		}
		bool insert(const K& key, const V& value)
		{
			const u32 k = hash(key) & m_pool_mask;
			for (u32 i = 0, n = m_pool[k].size(); i < n; ++i)
			{
				if (m_pool[k][i].key == key)
				{
					return false;
				}
			}
			m_pool[k].push_back(ITEM(key, value));
			m_size++;

			if (m_size > m_pool_mask * 2)
			{
				VECTOR<VECTOR<ITEM>> old_pool;
				old_pool.swap(m_pool);

				u32 in = m_pool_mask + 1;

				m_pool.resize((m_pool_mask + 1) * 2);
				m_pool_mask = m_pool_mask*2 + 1;

				for (u32 i = 0; i < in; ++i)
				{
					for (int j = 0, jn = old_pool[i].size(); j < jn; ++j)
					{
						insert(old_pool[i][j].key, old_pool[i][j].value);
					}
				}
			}

			return true;
		}

		V& operator[](const K& key)
		{
			const u32 k = hash(key) & m_pool_mask;
			for (u32 i = 0, n = m_pool[k].size(); i < n; ++i)
			{
				if (m_pool[k][i].key == key)
				{
					return m_pool[k][i].value;
				}
			}
			m_pool[k].push_back(ITEM());
			return m_pool[k].back().value;
		}
		const V& operator[](const K& key) const
		{
			const u32 k = hash(key) & m_pool_mask;
			for (u32 i = 0, n = m_pool[k].size(); i < n; ++i)
			{
				if (m_pool[k][i].key == key)
				{
					return m_pool[k][i].value;
				}
			}
			m_pool[k].push_back(ITEM());
			return m_pool[k].back().value;
		}

		bool erase(const K& key)
		{
			const u32 k = hash(key) & m_pool_mask;
			for (u32 i = 0, n = m_pool[k].size(); i < n; ++i)
			{
				if (m_pool[k][i].key == key)
				{
					m_pool[k][i] = m_pool[k].back();
					m_pool[k].resize(m_pool[k].size() - 1);
					m_size--;
					return true;
				}
			}
			return false;
		}
		void clear()
		{
			for (u32 i = 0; i <= m_pool_mask; ++i)
			{
				m_pool[i].clear();
			}
			m_size = 0;
		}
		
		struct iterator
		{
			iterator(int b, int e) : bucket(b), element(e) {}

			void operator++ (int)
			{
				increment();
			}
			void operator++ ()
			{
				increment();
			}
			V& operator*()
			{
				return m_pool[bucket][element].value;
			}
		private:
			int bucket, element;

			void increment()
			{
				if (element + 1 < m_pool[bucket].size())
					element++;
				else
				{
					bucket = next_bucket(bucket + 1);
					element = 0;
				}
			}
		};

		iterator begin()
		{
			return iterator(next_bucket(0), 0);
		}
		iterator end()
		{
			return iterator(-1, 0);
		}

	private:
		int next_bucket(int curr) const
		{
			for (int i = curr, n = m_pool.size(); i < n; ++i)
			{
				if (m_pool[i].empty()) continue;
				return iterator(i, 0);
			}
			return -1;
		}

		struct ITEM
		{
			K	key;
			V	value;

			ITEM() {}
			ITEM(const K& k, const V& v)
			{
				key = k;
				value = v;
			}
		};

		VECTOR<VECTOR<ITEM>>	m_pool;
		u32						m_pool_mask;
		u32						m_size;
	};
}