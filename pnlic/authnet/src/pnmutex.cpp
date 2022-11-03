#include "stdafx.h"
#include "../include/pnmutex.h"
#include "../include/PNString.h"
#include "../include/Exception.h"
#include "../include/sysutil.h"
namespace Proud 
{
	Mutex::Mutex()
	{
#if defined(_WIN32)
		m_mutexHandle = CreateMutex(NULL, false, NULL);
		if (m_mutexHandle == NULL)
		{
			throw Exception("CreateMutex failure! error=%d", GetLastError());
		}
#else
		// 재귀 뮤텍스를 사용한다.
		pthread_mutexattr_t attr;
		int ret1 = pthread_mutexattr_init(&attr);
		int ret2 = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
		int ret3 = pthread_mutex_init(&m_mutexHandle, &attr);
		if (ret1 != 0 || ret2 != 0 || ret3 != 0)
			throw Exception("pthread mutex initialize failed. error:%d,%d,%d", ret1, ret2, ret3);
#endif
	}
	Mutex::~Mutex()
	{
#if defined(_WIN32)
		CloseHandle(m_mutexHandle);
#else
		pthread_mutex_destroy(&m_mutexHandle);
#endif
	}
	LockResult Mutex::Lock(int timeout)
	{
#ifdef _WIN32
		DWORD ret = ::WaitForSingleObject(m_mutexHandle, timeout);
		switch (ret)
		{
		case WAIT_OBJECT_0:
			return LockResult_Success;
		case WAIT_TIMEOUT:
			return LockResult_Timeout;
		default:
			return LockResult_Error;
		}
#else
		// 유닉스
		if(timeout == PN_INFINITE)
		{
			int ret = pthread_mutex_lock(&m_mutexHandle);
			if (ret == 0)
				return LockResult_Success;
			else
				return LockResult_Error; // 무제한 시간을 걸었으므로 타임아웃이 없다.
		}
		else
		{
			int ret(0);
			if(timeout == 0)		// 사용자가 try-lock을 시도한 경우
			{
				ret = pthread_mutex_trylock(&m_mutexHandle);
			}
			else // 시간 제한 있는 잠금
			{
				// iOS에서 timed lock이 존재하지 않는다.
				// 일단 임시로 에러 처리
				//ret = pthread_mutex_timedlock(&m_mutexHandle, timeout);
                ShowUserMisuseError(_PNT("Sorry... mutex timed lock for iOS is not implemented yet."));
			}
			switch (ret)
			{
			case 0:
			case EDEADLK:
				return LockResult_Success;
			case ETIMEDOUT:
			case EBUSY:
				return LockResult_Timeout;
			default:
				return LockResult_Error;
			}
		}
#endif
	}
	LockResult Mutex::Unlock()
	{
#ifdef _WIN32
		return ReleaseMutex(m_mutexHandle) != 0 ? LockResult_Success : LockResult_Error;
#else
		return pthread_mutex_unlock(&m_mutexHandle) == 0 ? LockResult_Success : LockResult_Error;
#endif
	}
}