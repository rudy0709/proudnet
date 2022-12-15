/* ProudNet
이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.
** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <new>
#include <errno.h>
#include <time.h>
#include "../include/PNSemaphore.h"
#include "../include/PNString.h"
#include "../include/Exception.h"
#include "../include/TimeUtil.h"
#include "CriticalSectImpl.h"
#include "../include/sysutil.h"

namespace Proud
{

	Semaphore::Semaphore( int initialCount, int maxCount )
	{
#ifdef _WIN32
		m_sema =::CreateSemaphore(NULL, initialCount, maxCount, NULL);
#else
		m_count = initialCount;
		m_maxCount = maxCount;
        int ret = pthread_cond_init(&m_cond, NULL);
        if(ret != 0)
        {
			Tstringstream part;
			part << "pthread_cond_init function is failed. errno:" << errno;
            throw Exception(part.str().c_str());
        }
#endif
    }

	bool Semaphore::WaitOne( uint32_t timeOut )
	{
#ifdef _WIN32
		return WaitForSingleObject(m_sema, timeOut) == WAIT_OBJECT_0;
#else
        struct timespec ts;
		if(timeOut != PN_INFINITE)
		{
			// wait 하기 전에 mutex 를 잠근다.
			TimeGetAbsolute(&ts);
			TimeAddMilisec(&ts, timeOut);
		}
		CriticalSectionLock lock(m_cs, true);
		if(m_count > 0)
		{
			m_count--;
			return true; // 성공적으로 값 차감과 동시에 awake 확정
		}
		if(timeOut != PN_INFINITE)
		{
			// 이 함수는 내부적으로 cs unlock 후 wait하다가, 다른 스레드에서 post or broadcast를 던지면 awake하면서 lock을 해준다.
			// 즉 이 함수가 리턴할 때는 cs lock이 되어 있음을 의미. 그러나 위 cslock 로컬 변수의 파괴자에서 unlock이 되므로 안전.
			pthread_cond_timedwait(&m_cond, m_cs.GetMutexObject(), &ts);
		}
		else
		{
			// 이 함수는 내부적으로 cs unlock 후 wait하다가, 다른 스레드에서 post or broadcast를 던지면 awake하면서 lock을 해준다.
			// 즉 이 함수가 리턴할 때는 cs lock이 되어 있음을 의미. 그러나 위 cslock 로컬 변수의 파괴자에서 unlock이 되므로 안전.
			pthread_cond_wait(&m_cond, m_cs.GetMutexObject());
		}
		// 타 스레드에서 post를 던질 때는 이미 count가 양수임을 전제할 때 뿐이다. 따라서 이 비교는 안전하다.
		if(m_count > 0)
		{
			m_count--;
			return true;
		}
		else
		{
			return true;
		}

#endif // _WIN32
	}

	void Semaphore::Release( int releaseCount )
	{
#ifdef _WIN32
		LONG prevCount = 0;
		::ReleaseSemaphore(m_sema, releaseCount, &prevCount);
#else
//         if(releaseCount > 1)
//         {
//             // sem_post_multiple이 ios 에서 아직 미지원함. 통탄할 노릇일세.
//             throw Exception("Semaphore.Release with >1 is not supported!");
//         }
        // mutex 잠금
        CriticalSectionLock lock(m_cs, true);

		// 값 상향
		m_count += releaseCount;
		m_count = min(m_count, m_maxCount);
        // 시그널을 발생시킨다. 그래서 대기 중이던 스레드 하나가 awake하게 한다.
        int ret = pthread_cond_signal(&m_cond);
        if(ret != 0)
        {
            lock.Unlock();
			Tstringstream part;
			part << "pthread_cond_signal functon is failed. errno:" << errno;
            throw Exception(part.str().c_str());
        }
#endif
	}

	Semaphore::~Semaphore()
	{
#ifdef _WIN32
		CloseHandle(m_sema);
#else
        int ret = pthread_cond_destroy(&m_cond);
        if(ret != 0)
        {
			Tstringstream part;
			part << "pthread_cond_destroy function is failed. errno:" << errno;
            ShowUserMisuseError(part.str().c_str());
        }
#endif
	}
}
