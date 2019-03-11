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

	void log(const char* format, ...);

	u32 hash(i8 x) { return x; }
	u32 hash(i16 x) { return x; }
	u32 hash(i32 x) { return x; }
	u32 hash(u8 x) { return x; }
	u32 hash(u16 x) { return x; }
	u32 hash(u32 x) { return x; }
	u32 hash(void* x) { return sizeof(x) == 4 ? u32(x) : (u32(x) ^ (u64(x) >> 32U)); }
	u32 hash_string(const char* str);
}