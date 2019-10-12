#pragma once

#include <functional>

namespace xr
{
	typedef int JOB_HANDLE;

	struct JOB_COUNTER
	{
		JOB_COUNTER();
		void wait();

		u32		m_count;
		EVENT	m_event;
	};

	void jobs_init(int num_threads);
	void jobs_done();

	JOB_HANDLE jobs_add(std::function<void()> func, JOB_COUNTER* counter = nullptr, JOB_COUNTER* counter_to_wait = nullptr);

	void jobs_wait_job(JOB_HANDLE h);
	void jobs_wait_type(int job_type);
	void jobs_wait_all();
}