#pragma once

#include <memory.h>

#define ASSERT(x) do{if(!(x)) __asm{ int 3 }}while(0)

namespace xr
{
	template< class T > T Min(T a, T b)
	{
		return a < b ? a : b;
	}

	template< class T > T Max(T a, T b)
	{
		return a > b ? a : b;
	}

	template< class T > struct POD_TRAIT
	{
		bool operator()() { return false; }
	};

	template<> struct POD_TRAIT<int>
	{
		bool operator() () { return true; }
	};

	struct FILE
	{
		virtual ~FILE() {}

		virtual int		write(const void* ptr, int size) = 0;
		virtual int		read(void* ptr, int size) = 0;

		static FILE*	open(const char* file, const char* mode);
		static FILE*	std_out();
	};

	void fast_printf(FILE* f, const char* format, ...);
	void log(const char* format, ...);
}