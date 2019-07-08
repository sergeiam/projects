#include <xr/threads.h>
#include <Windows.h>



namespace xr
{
	CRITICAL_SECTION::CRITICAL_SECTION()
	{
		m_data = new ::CRITICAL_SECTION();
		InitializeCriticalSection((::CRITICAL_SECTION*)m_data);
	}
	CRITICAL_SECTION::~CRITICAL_SECTION()
	{
		DeleteCriticalSection((::CRITICAL_SECTION*)m_data);
	}
	void CRITICAL_SECTION::enter()
	{
		EnterCriticalSection((::CRITICAL_SECTION*)m_data);
	}
	void CRITICAL_SECTION::leave()
	{
		LeaveCriticalSection((::CRITICAL_SECTION*)m_data);
	}

	EVENT::EVENT()
	{
		m_data = (void*)CreateEventA(NULL, FALSE, FALSE, "");
	}
	EVENT::~EVENT()
	{
		CloseHandle((HANDLE)m_data);
	}
	void EVENT::reset()
	{
		ResetEvent((HANDLE)m_data);
	}
	void EVENT::signal()
	{
		SetEvent((HANDLE)m_data);
	}
	void EVENT::wait()
	{
		WaitForSingleObject((HANDLE)m_data, INFINITE);
	}
}