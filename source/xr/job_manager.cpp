#include <xr/job_manager.h>
#include <xr/vector.h>
#include <xr/threads.h>
#include <process.h>
#include <windows.h>

#define MAX_JOBS 4096
#define MAX_WORKER_THREADS 32

namespace xr
{
	int s_job_type_count[256];

	struct JOB
	{
		std::function<void()> f;
		u32*	counter;
		u32*	counter_to_wait;
		int		next;
		EVENT	event;
	};

	JOB		s_jobs[MAX_JOBS];
	int		s_free;
	int		s_first;
	int		s_num_jobs;

	EVENT	s_any_job;
	EVENT	s_no_jobs;

	CRITICAL_SECTION s_cs;

	uintptr_t	s_worker_threads[MAX_WORKER_THREADS];


	JOB_HANDLE jobs_add(std::function<void()> func, u32& counter, u32* counter_to_wait)
	{
		CS_SCOPE(s_cs);

		if (s_free == -1)
		{
			return -1;
		}

		int curr = s_free;
		s_free = s_jobs[s_free].next;

		JOB& job = s_jobs[curr];

		job.f = func;
		job.counter_to_wait = counter_to_wait;
		job.counter = &counter;
		job.next = s_first;

		s_first = curr;

		if (counter)
			InterlockedIncrement(&counter);

		s_num_jobs++;

		s_any_job.signal();
		s_no_jobs.reset();

		return JOB_HANDLE(curr);
	}

	void jobs_wait_job(JOB_HANDLE h)
	{
		s_jobs[h].event.wait();
	}

	void jobs_wait_type(int job_type)
	{
		job_type;
	}

	void jobs_wait_all()
	{
		if (s_num_jobs)
		{
			s_no_jobs.wait();
		}
	}

	unsigned __stdcall jobs_worker_thread(void*)
	{
		for (;;)
		{
			s_cs.enter();

			int job = -1, prev_job = -1;
			for ( job = s_first; job >= 0; prev_job = job, job = s_jobs[job].next)
			{
				JOB& curr_job = s_jobs[job];
				if (curr_job.type_wait && s_job_type_count[curr_job.type_wait] > 0) continue;
				break;
			}

			if (job >= 0)
			{
				if (prev_job > -1)
					s_jobs[prev_job].next = s_jobs[job].next;
				else
					s_first = s_jobs[job].next;

				s_cs.leave();

				s_jobs[job].f();

				s_cs.enter();

				s_jobs[job].next = s_free;
				s_free = job;
				s_job_type_count[s_jobs[job].type] --;

				s_num_jobs--;
				if (s_num_jobs == 0) s_no_jobs.signal();

				s_jobs[job].event.signal();

				s_cs.leave();
			}
			else
			{
				s_cs.leave();
				s_any_job.wait();
			}
		}
	}

	void jobs_init(int num_threads)
	{
		for (int i = 0; i < MAX_JOBS; ++i)
		{
			s_jobs[i].next = (i + 1 == MAX_JOBS) ? -1 : i + 1;
		}
		s_first = -1;
		s_free = 0;

		for (int i = 0; i < MAX_WORKER_THREADS; ++i)
		{
			if (i < num_threads)
			{
				s_worker_threads[i] = _beginthreadex(NULL, 1024 * 1024, jobs_worker_thread, NULL, 0, NULL);
			}
			else
				s_worker_threads[i] = 0;
		}

		s_no_jobs.signal();
	}
	void jobs_done()
	{
		for (int i = 0; i < MAX_WORKER_THREADS; ++i)
		{
			if (s_worker_threads[i])
			{
				//ExitThread(s_worker_threads[i]);
			}
		}
	}
}