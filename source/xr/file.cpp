#include <xr/file.h>

#include <stdio.h>
#include <stdarg.h>
#include <Windows.h>
#include <math.h>

#define FILE_BUFFER_SIZE 4096

namespace xr
{
	struct FILE_WIN32 : public FILE
	{
		HANDLE	m_fp;
		u8		m_buffer[FILE_BUFFER_SIZE];
		u32		m_pos, m_buffer_size;
		bool	m_read_once;

		FILE_WIN32(const char* file, u32 flags) : m_pos(0), m_buffer_size(0)
		{
			DWORD access = 0;
			if (flags & READ) access |= GENERIC_READ;
			if (flags & WRITE) access |= GENERIC_WRITE;

			DWORD share_mode = 0;
			if ((flags & READ) != 0 && (flags & WRITE) == 0) share_mode |= FILE_SHARE_READ;

			DWORD creation = 0;
			switch (flags)
			{
				case WRITE: creation |= OPEN_ALWAYS; break;
				case READ: creation |= OPEN_EXISTING; break;
				case WRITE | READ: creation |= OPEN_ALWAYS; break;
				case WRITE | TRUNC: creation |= CREATE_ALWAYS; break;
				case WRITE | READ | TRUNC: creation |= CREATE_ALWAYS; break;
			}
			DWORD f = FILE_ATTRIBUTE_NORMAL;

			m_fp = ::CreateFileA(file, access, share_mode, NULL, creation, f, NULL);

			m_read_once = false;
		}

		bool initialized()
		{
			return m_fp != INVALID_HANDLE_VALUE;
		}

		virtual ~FILE_WIN32()
		{
			if (m_pos)
			{
				u32 written = 0;
				WriteFile(m_fp, m_buffer, m_pos, &written, NULL);
			}
			if( m_fp != INVALID_HANDLE_VALUE )
				CloseHandle(m_fp);
		}
		virtual u32 write(const void* ptr, u32 size)
		{
			if (!size)
			{
				return 0;
			}
			if (m_pos + size <= FILE_BUFFER_SIZE)
			{
				if (size == 1)
				{
					m_buffer[m_pos++] = *(const char*)ptr;
					return 1;
				}
				memcpy(m_buffer + m_pos, ptr, size);
				m_pos += size;
				return size;
			}

			u32 written = 0;

			if (!WriteFile(m_fp, m_buffer, m_pos, &written, NULL))
				return 0;

			m_pos = 0;

			if (size >= FILE_BUFFER_SIZE)
			{
				WriteFile(m_fp, ptr, size, &written, NULL);
				return written;
			}
			return write(ptr, size);
		}
		virtual u32 read(void* ptr, u32 size)
		{
			m_read_once = true;

			u32 read_size = 0;
			while (read_size < size)
			{
				if (m_pos == m_buffer_size)
				{
					ReadFile(m_fp, m_buffer, FILE_BUFFER_SIZE, &m_buffer_size, NULL);
					if (!m_buffer_size)
						return read_size;
					m_pos = 0;
				}

				int sz = xr::Min(m_buffer_size - m_pos, size - read_size);
				memcpy(ptr, m_buffer + m_pos, sz);
				ptr = (u8*)ptr + sz;
				m_pos += sz;
				read_size += sz;
			}
			return read_size;
		}
		virtual u32	read_line(char* buffer, u32 max_len)
		{
			for (int i = 0; i + 1 < max_len; )
			{
				char ch;
				if (!read(&ch, 1)) {
					buffer[i] = 0;
					return i;
				}
				bool NL = ch == '\n' || ch == '\r';
				if (!NL) {
					buffer[i++] = ch;
				}
				else if (i) {
					buffer[i] = 0;
					return i;
				}
			}
			buffer[max_len - 1] = 0;
			return max_len;
		}
		virtual bool eof()
		{
			return m_read_once && m_pos == m_buffer_size && m_buffer_size < FILE_BUFFER_SIZE;
		}
		virtual u64 size()
		{
			u32 high;
			u32 low = GetFileSize(m_fp, &high);
			return low | (u64(high) << 32);
		}
		virtual u64 get_last_write_time()
		{
			FILETIME ft;
			GetFileTime(m_fp, NULL, NULL, &ft);
			return ft.dwLowDateTime | (u64(ft.dwHighDateTime) << 32);
		}
	};

	struct FILE_STDOUT : public FILE
	{
		virtual u32 write(const void* ptr, u32 size)
		{
			char buf[1025];
			const char* p = (const char*)ptr;
			for (; size > 0; size -= 1024, p += 1024)
			{
				int len = Min(size, (u32)1024);
				strncpy_s(buf, p, len);
				buf[len] = 0;
				printf("%s", buf);
				OutputDebugStringA(buf);
			}
			return size;
		}
		virtual u32 read(void*, u32)
		{
			return 0;
		}
		virtual u32	read_line(char* buffer, u32 max_len)
		{
			return 0;
		}
		virtual bool eof()
		{
			return false;
		}
		virtual u64	size()
		{
			return 0;
		}
		virtual u64 get_last_write_time()
		{
			return 0;
		}
	};

	FILE* FILE::open(const char* file, u32 flags)
	{
		FILE_WIN32* ptr = new FILE_WIN32(file, flags);
		if (ptr->initialized())
			return ptr;

		delete ptr;
		return nullptr;
	}

	FILE* FILE::std_out()
	{
		static FILE_STDOUT fso;
		return &fso;
	}

	static int print_int(char* buf, int value)
	{
		char d[32];
		int	count = 0;

		char* ptr = buf;

		if (value < 0)
		{
			*ptr++ = '-';
			value = -value;
		}

		do
		{
			int div10 = value / 10;
			d[count++] = (value - div10 * 10) + '0';
			value = div10;
		} while (value > 0);

		while (count)
		{
			*ptr++ = d[--count];
		}
		return ptr - buf;
	}

	static int print_float(char* buf, float value, int precision)
	{
		int	count = 0, ret = 0;

		char* ptr = buf;

		if (value < 0)
		{
			*ptr++ = '-';
			value = -value;
		}

		float intp = floorf(value);
		value -= intp;

		ptr += print_int(ptr, int(intp));

		if (precision)
		{
			*ptr++ = '.';
			for (int i = 0; i < precision; ++i)
			{
				value *= 10.0f;
				float iv = floorf(value);
				*ptr++ = '0' + int(iv);
				value -= iv;
			}
		}
		return ptr - buf;
	}

	void FILE::printf(const char* format, ...)
	{
		va_list args;
		va_start(args, format);

		const char* str_begin = format;

		char buf[32];

		for (;;)
		{
			char c = *format++;
			switch (c)
			{
			case 0:
				write(str_begin, format - str_begin - 1);
				va_end(args);
				return;
			case '%':
			{
				write(str_begin, format - str_begin - 1);

				int precision = 2;

				char cn = *format++;
				switch (cn)
				{
				case '%':
					write("%", 1);
					break;
				case 'd':
				{
					int value = va_arg(args, int);
					int len = print_int(buf, value);
					write(buf, len);
					break;
				}
				case 's':
				{
					const char* str = va_arg(args, const char*);
					write(str, strlen(str));
					break;
				}
				case '.':
					precision = *format++ - '0';
					if (*format++ != 'f')
					{
						//error
					}
				case 'f':
				{
					float value = va_arg(args, double);
					int len = print_float(buf, value, precision);
					write(buf, len);
					break;
				}
				}
				str_begin = format;
				break;
			}
			}
		}
	}
}