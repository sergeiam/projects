#include <xr/core.h>
#include <stdarg.h>
#include <stdio.h>
#include <Windows.h>

namespace xr
{
	u32 hash_string(const char* x)
	{
		u32 hash = 0;
		for (; *x; ++x)
		{
			hash = (hash << 1) ^ *x;
		}
		return hash;
	}

	void log(const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		char buf[2048];
		vsprintf(buf, format, args);
		va_end(args);

		OutputDebugStringA(buf);
		printf(buf);
	}

	void asset_error(const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		char buf[2048] = "[Asset Error] ";
		vsprintf(buf + strlen(buf), format, args);
		va_end(args);

		OutputDebugStringA(buf);
		printf(buf);
	}
}