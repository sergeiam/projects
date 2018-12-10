#pragma once

#include <xr/core.h>

namespace xr
{
	struct TIME_SCOPE
	{
		TIME_SCOPE();

		void	reset();
		u32		measure_duration_ms();
	private:
		u64		m_ticks;
	};

	u32 get_time_ms();
}