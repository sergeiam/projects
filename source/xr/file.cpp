#include <xr/file.h>

#include <stdio.h>
#include <stdarg.h>
#include <Windows.h>

namespace xr
{
	struct FILE_STDIO : public FILE
	{
		HANDLE	m_fp;
		//::FILE*	m_fp;
		char	m_buffer[2048];
		int		m_pos, m_buffer_size;
		u64		m_size;

		FILE_STDIO(::FILE* fp) : m_fp(fp), m_pos(0), m_buffer_size(0)
		{
			long now = ftell(fp);
			fseek(fp, 0, SEEK_END);
			m_size = ftell(fp);
			fseek(fp, now, SEEK_SET);
		}

		virtual ~FILE_STDIO()
		{
			if (m_pos)
			{
				u32 written = 0;
				WriteFile(m_fp, m_buffer, m_pos, &written, NULL);
			}
			CloseHandle(m_fp);
		}
		virtual int write(const void* ptr, int size)
		{
			if (!size)
			{
				return 0;
			}
			if (m_pos + size <= 2048)
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

			if ((int)fwrite(m_buffer, 1, m_pos, m_fp) < m_pos)
				return 0;
			m_pos = 0;

			if (size >= 2048)
			{
				return fwrite(ptr, 1, size, m_fp);
			}
			return write(ptr, size);
		}
		virtual int read(void* ptr, int size)
		{
			int read_size = 0;
			while (read_size < size)
			{
				if (m_pos == m_buffer_size)
				{
					m_buffer_size = fread(m_buffer, 1, 2048, m_fp);
					if (!m_buffer_size) return read_size;
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
		virtual int	read_line(char* buffer, int max_len)
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
			return feof(m_fp);
		}
		virtual u64 size()
		{
			return m_size;
		}
	};

	struct FILE_STDOUT : public FILE
	{
		virtual int write(const void* ptr, int size)
		{
			char buf[1025];
			const char* p = (const char*)ptr;
			for (; size > 0; size -= 1024, p += 1024)
			{
				int len = Min(size, 1024);
				strncpy(buf, p, len);
				buf[len] = 0;
				printf("%s", buf);
				OutputDebugStringA(buf);
			}
			return size;
		}
		virtual int read(void*, int)
		{
			return 0;
		}
		virtual int	read_line(char* buffer, int max_len)
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
	};

	FILE* FILE::open(const char* file, const char* mode)
	{
		DWORD dwAccess = 0;
		DWORD dwShareMode = 0;
		DWORD dwCreation = 0;
		DWORD dwFlags = 0;



		HANDLE h = ::CreateFile(file, 0, 0, NULL, 0, 0, NULL);
		//::FILE* fp = fopen(file, mode);
		return (h != INVALID_HANDLE_VALUE) ? new FILE_STDIO(h) : nullptr;
	}

	FILE* FILE::std_out()
	{
		static FILE_STDOUT fso;
		return &fso;
	}

	u32 FILE::datetime(const char* file)
	{

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

	void fast_printf(FILE* f, const char* format, ...)
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
				f->write(str_begin, format - str_begin - 1);
				va_end(args);
				return;
			case '%':
			{
				f->write(str_begin, format - str_begin - 1);

				int precision = 2;

				char cn = *format++;
				switch (cn)
				{
				case '%':
					f->write("%", 1);
					break;
				case 'd':
				{
					int value = va_arg(args, int);
					int len = print_int(buf, value);
					f->write(buf, len);
					break;
				}
				case 's':
				{
					const char* str = va_arg(args, const char*);
					f->write(str, strlen(str));
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
					f->write(buf, len);
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