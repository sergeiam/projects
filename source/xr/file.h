#pragma once

#include <xr/core.h>

namespace xr
{
	struct FILE
	{
		enum
		{
			READ	= 1 << 0,
			WRITE	= 1 << 1,
			TRUNC	= 1 << 2,
		};

		virtual ~FILE() {}

		virtual u32		write(const void* ptr, u32 flags) = 0;
		virtual u32		read(void* ptr, u32 size) = 0;
		virtual u32		read_line(char* buffer, u32 max_len) = 0;
		virtual bool	eof() = 0;
		virtual u64		size() = 0;
		virtual u64		get_last_write_time() = 0;

		void			printf(const char* format, ...);

		static FILE*	open(const char* file, u32 flags);
		static FILE*	std_out();
	};
}