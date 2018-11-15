#pragma once

#include <functional>

namespace xr {

	void job_manager_init(int num_threads);
	void job_manager_done();

	void job_manager_add_job(std::function<void()> func);
}