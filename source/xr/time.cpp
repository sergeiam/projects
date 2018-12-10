#include <xr/time.h>
#include <Windows.h>

namespace xr
{
	static u64	g_time_app_start;

	static u64 get_ticks()
	{
		LARGE_INTEGER li;
		::QueryPerformanceCounter(&li);
		return li.QuadPart;
	}

	struct TIME_INITIALIZATION
	{
		TIME_INITIALIZATION()
		{
			g_time_app_start = get_ticks();
		}
	}
	g_time_initialization;

	static u32 time_difference_ms(u64 t1, u64 t2)
	{
		static LARGE_INTEGER freq = { 0 };
		if(!freq.QuadPart)
			::QueryPerformanceFrequency(&freq);
		return (t2 - t1) * 1000 / freq.QuadPart;
	}

	TIME_SCOPE::TIME_SCOPE()
	{
		reset();
	}

	void TIME_SCOPE::reset()
	{
		m_ticks = get_ticks();
	}

	u32 TIME_SCOPE::measure_duration_ms()
	{
		return time_difference_ms(m_ticks, get_ticks());
	}

	u32 get_time_ms()
	{
		return time_difference_ms(g_time_app_start, get_ticks());
	}
}