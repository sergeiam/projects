#pragma once

#include <memory.h>

#define ASSERT(x) do{if(!(x)) __asm{ int 3 }}while(0)

typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned long		u32;
typedef unsigned long long	u64;
typedef signed char			i8;
typedef signed short		i16;
typedef signed long			i32;
typedef signed long long	i64;

namespace xr
{
	template< class T > T Min(T a, T b) {
		return a < b ? a : b;
	}

	template< class T > T Max(T a, T b) {
		return a > b ? a : b;
	}

	template< class T > void Swap(T& a, T& b) {
		T temp = a; a = b; b = temp;
	}

	template< class T > T Clamp(T a, T min, T max) {
		return (a < min) ? min : ((a > max) ? max : a);
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
		virtual int		read_line(char* buffer, int max_len) = 0;
		virtual bool	eof() = 0;

		static FILE*	open(const char* file, const char* mode);
		static FILE*	std_out();
	};

	void fast_printf(FILE* f, const char* format, ...);
	void log(const char* format, ...);
}