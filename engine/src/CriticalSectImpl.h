#pragma once

#include "../include/BasicTypes.h"
#include "../include/CriticalSect.h"


namespace Proud
{
#ifndef CRITICAL_SECTION_NO_DEBUG_INFO
#define CRITICAL_SECTION_NO_DEBUG_INFO 0x01000000
#endif

	class CriticalSection_Standard
	{
	public:
#if !defined(_WIN32)
		// 리눅스 등은 crit sec이 없고 mutex가 대신 사용됨
		Mutex* m_mutex;
#else
		// 윈도에서는 당연히 이거
		CRITICAL_SECTION m_cs;

		// ikpil.choi 2016-11-02 - 크리티컬 색션이 이상해 지는 버그가 있어, 다른 영역 힙에 올려봅니다.
		DECLARE_NEW_AND_DELETE_THROWABLE
#endif
	};

	// CriticalSection 병목 현상 감지기
	class CriticalSection_BottleneckDetector
	{
	public:
		// 타임아웃을 걸고 잠금을 할 수 있어야 해서 win32 mutex가 굳이 쓰임
		Mutex m_mutex;

		// 아래 변수들을 보호한다.
		CriticalSection m_smallCritSec;

		// 재귀 잠금 횟수
		volatile int m_recursionCount;

		// 잠금한 스레드의 ID
		volatile uint64_t m_owningThread;

		// 0: Dump 를 써도 되는 상황 1: Dump 를 다른 쓰레드가 쓰고 있는 상황
		volatile int32_t m_writeDumpState;
		// 마지막으로 덤프를 남겼던 시간
		static int64_t m_lastDumpedTimeMs;

		PROUD_API CriticalSection_BottleneckDetector()
		{
			m_owningThread = 0;
			m_recursionCount = 0;
			m_writeDumpState = 0;
		}
	};


	PROUD_API void AssertIsLockedByCurrentThread(const CriticalSection &cs);
	PROUD_API void AssertIsNotLockedByCurrentThread(const CriticalSection &cs);

#ifdef PN_LOCK_OWNER_SHOWN
	/**
	\~korean
	Critical Section을 Lock했는가 검사한다.
	- for x86 NT/2000 only
	\~english
	This checks if the Critical Section is locked.
	- for x86 NT/2000 only
	\~chinese
	检查是否lock了Critical Section。
	- for x86 NT/2000 only
	\~japanese
	\~
	*/
	PROUD_API bool IsCriticalSectionLocked(const CriticalSection &cs);

	/**
	\~korean
	Critical Section을 Lock했는가 검사한다.
	- for x86 NT/2000 only
	\~english
	This checks if the Critical Section is locked.
	- for x86 NT/2000 only
	\~chinese
	检查是否lock了Critical Section。
	- for x86 NT/2000 only
	\~japanese
	\~
	*/
#if defined(_WIN32)
	PROUD_API bool IsCriticalSectionLocked(const CRITICAL_SECTION &cs);
#else
	PROUD_API bool IsCriticalSectionLocked(const pthread_mutex_t &mutex);
#endif

	/**
	\~korean
	Critical Section을 Lock했는가 검사한다.
	- for x86 NT/2000 only
	\~english
	This checks if the Critical Section is locked.
	- for x86 NT/2000 only
	\~chinese
	检查是否lock了Critical Section。
	- for x86 NT/2000 only
	\~japanese
	\~
	*/
	PROUD_API bool IsCriticalSectionLockedByCurrentThread(const CriticalSection &cs);

	/**
	\~korean
	Critical Section을 Lock했는가 검사한다.
	- for x86 NT/2000 only
	\~english
	This checks if the Critical Section is locked.
	- for x86 NT/2000 only
	\~chinese
	检查是否lock了Critical Section。
	- for x86 NT/2000 only
	\~japanese
	\~
	*/
#if defined(_WIN32)
	PROUD_API bool IsCriticalSectionLockedByCurrentThread(const CRITICAL_SECTION &cs);
#else
	PROUD_API bool IsCriticalSectionLockedByCurrentThread(const pthread_mutex_t &mutex);
#endif

#endif // PN_LOCK_OWNER_SHOWN
}

#include "CriticalSect.inl"
