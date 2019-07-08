#pragma once

namespace xr
{
	#define CS_SCOPE(cs) CRITICAL_SECTION_SCOPE css(cs)

	class CRITICAL_SECTION
	{
	public:
		CRITICAL_SECTION();
		~CRITICAL_SECTION();

		void enter();
		void leave();

	private:
		void* m_data;
	};

	struct CRITICAL_SECTION_SCOPE
	{
		CRITICAL_SECTION_SCOPE(CRITICAL_SECTION& cs) : m_cs(cs)
		{
			m_cs.enter();
		}
		~CRITICAL_SECTION_SCOPE()
		{
			m_cs.leave();
		}
		CRITICAL_SECTION& m_cs;
	};

	struct EVENT
	{
		EVENT();
		~EVENT();

		void reset();
		void signal();
		void wait();

	private:
		void* m_data;
	};

}