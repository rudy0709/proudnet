/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"

#include "../include/ThreadUtil.h"
#include "../include/sysutil.h"

#if !defined(PROUD_STATIC_LIB) && defined(_WIN32)
#include "../build/ProudDll/dllmain.h"
#endif

namespace Proud
{
#if defined(_WIN32)
	NewTimerParam::NewTimerParam(WaitOrTimerCallbackProc callback, void *pCtx, uint32_t period, uint32_t DueTime) 
		: m_callback(callback), m_pCtx(pCtx), 
		m_period(period), m_DueTime(DueTime)
	{
	}
	//
	// Usage: SetThreadName (-1, "MainThread");
	//
	#define MS_VC_EXCEPTION 0x406D1388
//#pragma pack(push,8)
	typedef struct tagTHREADNAME_INFO
	{
		uint32_t dwType; // Must be 0x1000.
		const char* szName; // Pointer to name (in user addr space).
		uint32_t dwThreadID; // Thread ID (-1=caller thread).
		uint32_t dwFlags; // Reserved for future use, must be zero.
	} THREADNAME_INFO;
//#pragma pack(pop)

	void SetThreadName( uint32_t dwThreadID, const char* threadName)
	{
		/* 이 메서드는 MiniDumper에서 AddVectoredExceptionHandler를 쓰는 이후부터, 여기서 덤프를 남겨버리기 때문에 막아버렸다. 이 문제를 추후 해결하면 다시 풀던가. 
// 		Sleep(10); 
// 		THREADNAME_INFO info;
// 		info.dwType = 0x1000;
// 		info.szName = threadName;
// 		info.dwThreadID = dwThreadID;
// 		info.dwFlags = 0;
// 
// 		__try
// 		{
// 			RaiseException( MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info );
// 		}
// 		__except(EXCEPTION_EXECUTE_HANDLER)
// 		{
// 		}
		*/
	}

	HANDLE GetCurrentRealThread()
	{
		// Article ID: Q90470 참고.
		HANDLE hThread;
		HANDLE t =::GetCurrentThread();
		::DuplicateHandle(
			::GetCurrentProcess(),
			t,
			::GetCurrentProcess(),
			&hThread,
			THREAD_ALL_ACCESS,
			FALSE,
			DUPLICATE_SAME_ACCESS);
		return hThread;
	}

	CTimerQueueTimer::~CTimerQueueTimer()
	{
		if(::DeleteTimerQueueTimer(m_manager->m_timerQueue,m_timer,m_endEvent.m_event) ||
			GetLastError() == ERROR_IO_PENDING)
		{
			// 호출중이던 함수가 더 이상 실행되지 않을 때까지 기다린다.
			m_endEvent.WaitOne();
		}
	}

	CTimerQueueTimer* CTimerQueue::NewTimer( NewTimerParam& Param )  /*WAITORTIMERCALLBACK callback,void *ctx,DWORD period, double DueTime*/
	{
		HANDLE timer;
		if(!::CreateTimerQueueTimer(&timer,m_timerQueue,Param.m_callback,Param.m_pCtx,Param.m_DueTime,Param.m_period,WT_EXECUTEDEFAULT))
			return NULL;

		CTimerQueueTimer* ret=new CTimerQueueTimer;
		ret->m_manager=this;
		ret->m_timer=timer;

		return ret;
	}

	CTimerQueue::CTimerQueue()
	{
		m_timerQueue = ::CreateTimerQueue();
	}

	CTimerQueue::~CTimerQueue()
	{		
		if(::DeleteTimerQueueEx(m_timerQueue,m_endEvent.m_event))
		{
#ifndef PROUD_STATIC_LIB
			if(!g_ProudDllsAreUnloading)
			{
				// 호출중이던 함수가 더 이상 실행되지 않을 때까지 기다린다.
				m_endEvent.WaitOne();
			}
			else
			{
				// 이미 DLL이 detach되는 과정이렸다! DLL의 내용을 의존하던 스레드는 이미 OS에 의해 강제 파괴된 상태이므로 기다릴 필요가 없다
				// 이런 로직이 없는 경우 프로세스 종료시 데드락으로 이어지더라.
			}
#endif
		}
	}

#endif // _WIN32

	// win32 or not을 위해
	void* TlsGetValue(uint32_t tlsIndex)
	{
#ifdef _WIN32
		return ::TlsGetValue(tlsIndex);
#else
		return pthread_getspecific(tlsIndex);
#endif
	}

	// win32 or not을 위해
	bool TlsSetValue(uint32_t tlsIndex, void* tlsValue)
	{
#ifdef _WIN32
		return ::TlsSetValue(tlsIndex, tlsValue) != 0;
#else
		return pthread_setspecific(tlsIndex, (const void*)tlsValue) == 0;
#endif
	}

	// win32 or not을 위해
	uint32_t TlsAlloc()
	{
#ifdef _WIN32
		return ::TlsAlloc();
#else
		pthread_key_t ret;
		pthread_key_create(&ret, NULL);
		return ret; // pthread_key_t는 실제로 uint32다.
#endif
	}

}