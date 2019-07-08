#pragma once

#include <functional>

namespace xr
{
	typedef int JOB_HANDLE;

	void jobs_init(int num_threads);
	void jobs_done();

	JOB_HANDLE jobs_add(std::function<void()> func, int job_type, int job_type_wait);

	void jobs_wait_job(JOB_HANDLE h);
	void jobs_wait_type(int job_type);
	void jobs_wait_all();
}