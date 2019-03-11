#pragma once

#include <xr/core.h>

namespace xr
{
	struct FILE
	{
		virtual ~FILE() {}

		virtual int		write(const void* ptr, int size) = 0;
		virtual int		read(void* ptr, int size) = 0;
		virtual int		read_line(char* buffer, int max_len) = 0;
		virtual bool	eof() = 0;
		virtual u64		size() = 0;
		virtual u64		last_write_time() = 0;

		static FILE*	open(const char* file, const char* mode);
		static FILE*	std_out();
		static u32		datetime(const char* file);
	};

	void fast_printf(FILE* f, const char* format, ...);
}