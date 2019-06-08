#pragma once

#include <xr/core.h>

namespace xr
{
	template< class T, bool POD = true > class VECTOR
	{
	public:
		VECTOR() : m_ptr(nullptr), m_size(0), m_capacity(0)
		{
		}
		VECTOR(int size) : m_ptr(nullptr), m_size(0), m_capacity(0)
		{
			set_capacity(size);
		}
		VECTOR(VECTOR<T>&& rhs)
		{
			m_ptr = rhs.m_ptr;
			m_size = rhs.m_size;
			m_capacity = rhs.m_capacity;
			rhs.m_ptr = nullptr;
			rhs.m_size = rhs.m_capacity = 0;
		}
		~VECTOR()
		{
			delete[] m_ptr;
		}
		void push_back(const T& x)
		{
			if (m_size == m_capacity)
			{
				grow();
			}
			m_ptr[m_size++] = x;
		}
		void resize(int size)
		{
			if (m_capacity && size > m_capacity)
				grow();
			else if (m_capacity == 0)
				set_capacity(size);
			m_size = size;
		}
		int size() const { return m_size; }

		T* begin() { return m_ptr; }
		T* end() { return m_ptr ? m_ptr + m_size : m_ptr; }

		T& front() { return m_ptr[0]; }
		T& back() { return m_ptr[m_size - 1]; }

		void clear()
		{
			m_size = 0;
		}
		bool empty() const
		{
			return m_size == 0;
		}
		T& operator[](int index)
		{
			ASSERT(index >= 0 && index < m_size);
			return m_ptr[index];
		}
		const T& operator[](int index) const
		{
			ASSERT(index >= 0 && index < m_size);
			return m_ptr[index];
		}
		void operator = (const VECTOR<T>& rhs)
		{
			delete[] m_ptr;
			m_size = m_capacity = rhs.size();
			m_ptr = new T[m_size];
			memcpy(m_ptr, rhs.m_ptr, m_size * sizeof(T));
		}
		void operator = (VECTOR<T>&& rhs)
		{
			if (this == &rhs) return;

			clear();
			m_ptr = rhs.m_ptr;
			m_size = rhs.m_size;
			m_capacity = rhs.m_capacity;
			rhs.m_ptr = nullptr;
			rhs.m_size = rhs.m_capacity = 0;
		}
		void fill(const T& x)
		{
			for (int i = 0; i < m_size; ++i)
				m_ptr[i] = x;
		}
		void append(const T* begin, const T* end)
		{
			if (m_size + (end - begin) > m_capacity)
			{
				grow();
			}
			memcpy(m_ptr + m_size, begin, (end - begin) * sizeof(T));
			m_size += end - begin;
		}
		void insert(int index, const T& element)
		{
			resize(size() + 1);
			for (int i = size() - 1; i > index; --i)
				m_ptr[i] = m_ptr[i - 1];
			m_ptr[index] = element;
		}
		void swap(VECTOR<T>& rhs)
		{
			Swap(m_ptr, rhs.m_ptr);
			Swap(m_size, rhs.m_size);
			Swap(m_capacity, rhs.m_capacity);
		}

	private:
		void set_capacity(int new_capacity)
		{
			m_capacity = new_capacity;
			T* ptr = new T[m_capacity];
			if (m_ptr)
			{
				memcpy(ptr, m_ptr, m_size * sizeof(T));
				delete m_ptr;
			}
			m_ptr = ptr;
		}
		void set_size(int new_size)
		{
			if (m_capacity < new_size)
			{
				set_capacity(new_size);
			}
			if (new_size > m_size && !POD)
			{
				for (int i = m_size; i < new_size; ++i)
					m_ptr[i].T();
			}
			m_size = new_size;
		}
		void grow()
		{
			set_capacity(Max(32, m_capacity * 2));
		}

		T*	m_ptr;
		int	m_size, m_capacity;
	};
}