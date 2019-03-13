#pragma once

#include <xr/core.h>

namespace xr
{
	struct FILE
	{
		virtual ~FILE() {}

		virtual u32		write(const void* ptr, u32 size) = 0;
		virtual u32		read(void* ptr, u32 size) = 0;
		virtual u32		read_line(char* buffer, u32 max_len) = 0;
		virtual bool	eof() = 0;
		virtual u64		size() = 0;
		virtual u64		get_last_write_time() = 0;

		static FILE*	open(const char* file, const char* mode);
		static FILE*	std_out();
	};

	void fast_printf(FILE* f, const char* format, ...);
}