/* ProudNet
이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.
** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include "../include/MilisecTimer.h"
#include "../include/CriticalSect.h"
#include "../include/sysutil.h"
#include "../include/BasicTypes.h"
#include "../include/PnTime.h"
#include "CriticalSectImpl.h"
#include <errno.h>
#if defined(_WIN32)
#include <imagehlp.h>
#endif

#if defined(__ANDROID__)
#include <android/log.h>
#endif

#if defined(__linux__)
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <signal.h>
#endif

#include "../include/pnstdint.h"

namespace Proud
{
	// 1개 프로세스 총체에 걸쳐, 지나치게 자주 덤프 파일을 만들어내면 안되므로
	int64_t CriticalSection_BottleneckDetector::m_lastDumpedTimeMs = 0;

	CriticalSectionSettings::CriticalSectionSettings()
	{
		m_spinCount = GetAppropriateSpinCount();
#if defined(_WIN32)
#if defined(_DEBUG)
		// 디버그 빌드에서는 faster critsec이 아님
		m_fasterForWindows6 = true;
#else
		m_fasterForWindows6 = false;
#endif // _DEBUG
#endif // _WIN32
		// 설정하지 않고 테스트를 진행하면 원래처럼 작동
		m_bottleneckWarningThresholdMs = 0;
	}


	enum { ValidKeyValue = 66778899 };


	CriticalSection::CriticalSection() 
	{
		CriticalSectionSettings temp;
		_Initialize(temp);
	}

	CriticalSection::CriticalSection(CriticalSectionSettings &settings) 
	{
		_Initialize(settings);
	}

	// 초기화 내부 함수
	void CriticalSection::_Initialize(CriticalSectionSettings &settings)
    {
		m_standard = new CriticalSection_Standard;

#if !defined(_WIN32)
		if(settings.m_bottleneckWarningThresholdMs > 0)
		{
			throw Exception("Bottleneck detector is not supported on non-Windows yet.");
		}
#endif
		m_tryLockSuccessCount = 0;
		m_tryLockFailCount = 0;

		m_bottleneckDetector = NULL;
		m_settings = settings;
        m_neverCallDtor = false;
		m_validKey = ValidKeyValue;
		// BottleneckTest 를 할때는 특별하게
		if (IsBottleneckWarningEnabled())
		{
			m_bottleneckDetector = new CriticalSection_BottleneckDetector;
		}
		else
		{
#ifdef _WIN32
			// 원래 속도 빠른 놈으로
			if (settings.m_fasterForWindows6) // CSingleton에서는 이걸 접근할 때 false이므로 아래 CKernel32Api 싱글톤을 또 접근하는 우가 없음
			{
				if (CKernel32Api::Instance().InitializeCriticalSectionEx) // 반드시 이걸 나중에 체크해야 한다. Instance를 접근하는 순간 critsec을 접근하므로!
				{
					if (!CKernel32Api::Instance().InitializeCriticalSectionEx(
						&m_standard->m_cs,
						settings.m_spinCount,
						settings.m_fasterForWindows6 ? CRITICAL_SECTION_NO_DEBUG_INFO : 0))
					{
						throw Exception("InitializeCriticalSectionEx failed! error=%d", GetLastError());
					}
				}
			}
			else
			{
				// 옛날 운영체제에서
				if (settings.m_spinCount > 0)
				{
					if (!InitializeCriticalSectionAndSpinCount(&m_standard->m_cs, settings.m_spinCount))
					{
						throw Exception("InitializeCriticalSectionAndSpinCount failed! error=%d", GetLastError());
					}
				}
				else
				{
					InitializeCriticalSection(&m_standard->m_cs);
				}
			}
#else
			// 유닉스에서
			m_standard->m_mutex = new Mutex;
#endif
		}
	}

	// NetServer는 Start() 할때 StartServerParameter 로 Bottleneck Warning 기능을 On/Off 할지 결정하는데
	// 생성자에서 바로 _Init을 들어가서 세팅을 못함
	// 따라서 해당 함수를 따로 만듬
	void CriticalSection::_Uninitialize()
	{
#ifdef PN_LOCK_OWNER_SHOWN
		// 사용자 실수로 이미 동작중인 CriticalSection 에 Setting 을 바꿔버리면 곤란하므로
		// 작동중인 CS는 막아버린다.
		if (IsCriticalSectionLocked(*this))
		{
			throw Exception("Critical section cannot be reset if it is locked by any thread.");
		}
#endif
		if (m_bottleneckDetector)
		{
			delete m_bottleneckDetector;
			m_bottleneckDetector = NULL;
		}
		else
		{
#ifdef _WIN32
			DeleteCriticalSection(&m_standard->m_cs);
			ZeroMemory(&m_standard->m_cs, sizeof(m_standard->m_cs)); // 이렇게 지워버려야 디버깅중 무효화된 객체인지 체크가 가능하다.
#else
			delete m_standard->m_mutex;
			m_standard->m_mutex = NULL;
#endif
		}
		// 디버깅할 때, 파괴자가 이미 호출되었는지 여부를 체크하기 위해 값을 리셋한다.
		m_validKey = 0;

		delete m_standard;
	}

	CriticalSection::~CriticalSection( void )
	{
		if(m_neverCallDtor)
			return;
#ifdef PN_LOCK_OWNER_SHOWN
		if (IsCriticalSectionLocked(*this))
		{
			// 과거에는 throw였으나 dtor에서 throw는 말도 안됨. 따라서 ShowUserMisuseError를 쓰자.
			ShowUserMisuseError(_PNT("Critical section is still in use! Destruction may cause problems!"));
		}
#endif
		_Uninitialize();
	}

	// bottleneck checker가 관여하는 lock을 건다.
	// lock을 성공적으로 걸면 건 스레드가 뭔지를 기록한다.
	void CriticalSection::_BottleneckDetectorAwareLock(int32_t timeout, LockBottleneckDetectorResult& result)
	{
#if defined(_WIN32)
		// timeout을 설정 가능한 mutex부터 먼저 lock을 한다.
		result.m_lockResult = m_bottleneckDetector->m_mutex.Lock(timeout);
		CriticalSectionLock lock(m_bottleneckDetector->m_smallCritSec, true);
		// lock이 성공한 경우 acquire한 스레드의 정보를 기입한다.
		if (result.m_lockResult == LockResult_Success)
		{
	
			// recursionCount 와 owningThread 값을 변경
			m_bottleneckDetector->m_recursionCount++;
			m_bottleneckDetector->m_owningThread = (int32_t)GetCurrentThreadId(); // 해당 유닉스 함수를 나중에 만들어야 함
		}
		// 현재 상태를 리턴. 성공하건 실패하건.
		result.m_owningThread = m_bottleneckDetector->m_owningThread;
#else
		ShowUserMisuseError(_PNT("Sorry. Bottleneck detector for non-Windows is not implemented yet."));
#endif
	}
   
	void CriticalSection::Lock()
	{
		ShowErrorOnInvalidState();
		Lock_internal();
	}

	void CriticalSection::Lock_internal()
	{
		if (m_bottleneckDetector)
		{
#ifdef _WIN32
			LockBottleneckDetectorResult ret;
			assert(m_settings.m_bottleneckWarningThresholdMs > 0); // try-lock은 아니므로
			_BottleneckDetectorAwareLock(m_settings.m_bottleneckWarningThresholdMs, ret);
			if (ret.m_lockResult == LockResult_Timeout)
			{
				// state 값 상태
				// 0. Dump 써도 되는 상황
				// 1. Dump 쓰는중
				int currentState = AtomicCompareAndSwap32(0, 1, &m_bottleneckDetector->m_writeDumpState);
				if (currentState == 0)
				{
					int64_t currTime = GetPreciseCurrentTimeMs();
					if (currTime - CriticalSection_BottleneckDetector::m_lastDumpedTimeMs >= 1000) 
					{
						// Warning Interval 시간도 지났다.
						// 실제로 경고를 남기자.
						// 소유 스레드가 Unlock 을 진행 할 수 있으므로 Suspend 시킴
						HANDLE owningThreadHandle = ::OpenThread(THREAD_SUSPEND_RESUME, false, ret.m_owningThread);
						if (owningThreadHandle != NULL) // OpenThread 리턴값이 그렇다고 함
						{
							::SuspendThread(owningThreadHandle);

							// String 객체의 경우 생성시에 내부에서 atomic 연산을 수행합니다
							// marmalade의 경우 CSlowAtomic 함수를 쓰게 되는데,  CSlowAtomic 함수를 호출할 경우 전역으로 선언된 g_atomicOpCritSec(Critical Section)을 사용합니다.
							// 전역으로 선언된 g_atomicOpCritSec을 초기화하는 과정에서 String 인스턴스를 만들게 되면 결국은 내부적으로 CSlowAtomic 계열 함수를 호출하고
							// 아직 초기화가 끝나지 않는 상태인 g_atomicOpCritSec을 사용하기 때문에 NullPointerException가 발생됩니다.
							// 그래서 생성자에서 FilePath 및 FileName 초기화 구문을 삭제 합니다. FilePath 및 FileName이 설정되어 있지 않을 경우, 이곳에서 default로 값을 정하도록 합니다.
							
							String dumpFilePath = (true == m_settings.m_bottleneckWarningDumpFilePath.IsEmpty() ? _PNT("./") : m_settings.m_bottleneckWarningDumpFilePath);
							String dumpFileName = (true == m_settings.m_bottleneckWarningDumpFileName.IsEmpty() ? _PNT("default") : m_settings.m_bottleneckWarningDumpFileName);

							CPnTime currentTime = CPnTime::GetCurrentTime();
							String fileName = String::NewFormat(_PNT("%sThread%d-%d-%d-%d-%d-%d-%d-%d_%s.dmp"),
								dumpFilePath,
								ret.m_owningThread,
								currentTime.GetYear(),
								currentTime.GetMonth(),
								currentTime.GetDay(),
								currentTime.GetHour(),
								currentTime.GetMinute(),
								currentTime.GetSecond(),
								currentTime.GetMillisecond(),
								dumpFileName);
							HANDLE dumpFileHandle = ::CreateFile(fileName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
								FILE_ATTRIBUTE_NORMAL, NULL);
							if (dumpFileHandle != INVALID_HANDLE_VALUE)
							{
								MiniDumpWriteDump(GetCurrentProcess(),
									GetCurrentProcessId(), dumpFileHandle,
									MiniDumpNormal,
									NULL, NULL, NULL);
								CloseHandle(dumpFileHandle);
							}
							CriticalSection_BottleneckDetector::m_lastDumpedTimeMs = currTime;
							// 소유 스레드 다시 Resume
							::ResumeThread(owningThreadHandle);
							CloseHandle(owningThreadHandle);
						}
						AtomicCompareAndSwap32(1, 0, &m_bottleneckDetector->m_writeDumpState);
					}
				}
				// 아직 병목 상황이다. 두번째 시도에서는, 병목 체크 기능 없이 무제한으로 기다리는 잠금을 한다.
				LockBottleneckDetectorResult ret;
				_BottleneckDetectorAwareLock(PN_INFINITE, ret);
				if (ret.m_lockResult == LockResult_Error)
				{
					// 중도에 critsec을 destroy하면 여기에 걸릴 수 있다.
					throw Exception("Failed to acquire lock! error=%d", ret.m_lockResult);
				}
			}
            
#else
            ShowUserMisuseError(_PNT("Sorry... bottleneck detector for unix is not implemented yet."));
#endif
		}
		else
		{
#if defined(_WIN32)
			// 예전에는 spin lock 믿고 바로 Enter를 들어갔지만 코드 프로필러 결과 이렇게 하니까 
			// 이 처리에 소요 시간이 20% 정도 개선됨.
			if (!TryEnterCriticalSection(&m_standard->m_cs))
			{
				m_tryLockFailCount++;
				EnterCriticalSection(&m_standard->m_cs);
			}
			else
			{
				m_tryLockSuccessCount++;
			}
#else
			for (uint32_t i = 0; i < m_settings.m_spinCount; i++)
			{
				if (m_standard->m_mutex->TryLock() == LockResult_Success)
				{
					m_tryLockSuccessCount++;
					return;
				}
				m_tryLockFailCount++;
			}

			m_standard->m_mutex->Lock();
#endif
		}
	}

	void CriticalSection::UnsafeLock()
	{
		Lock_internal();
	}

	PROUD_API bool CriticalSection::TryLock()
	{
		ShowErrorOnInvalidState();
		
		bool trylockSuccess;
		// NOTE: 이 함수는 리턴값을 뱉어야.
		if (IsBottleneckWarningEnabled())
		{
			LockBottleneckDetectorResult ret;
			_BottleneckDetectorAwareLock(0, ret);
			trylockSuccess = (ret.m_lockResult == LockResult_Success);
		}
		else
		{
#ifdef _WIN32
			trylockSuccess = TryEnterCriticalSection(&m_standard->m_cs) ? true : false;
#else
			trylockSuccess = (m_standard->m_mutex->TryLock() == LockResult_Success);
#endif
		}
		if (trylockSuccess)
			m_tryLockSuccessCount++;
		else
			m_tryLockFailCount++;

		return trylockSuccess;
	}

	void CriticalSection::Unlock()
	{
		if (m_bottleneckDetector)
		{
			LockResult e = m_bottleneckDetector->m_mutex.Unlock();
			if (e == LockResult_Success)
			{
				// 여기서부터 해당 CS 소유의 thread 만 들어온다게 보장됨
				CriticalSectionLock lock(m_bottleneckDetector->m_smallCritSec, true);
				m_bottleneckDetector->m_recursionCount--;
				if (m_bottleneckDetector->m_recursionCount == 0)
				{
					// recursion count가 0가 되었으므로 실제로 놔준거다. 따라서...
					m_bottleneckDetector->m_owningThread = 0;
				}
			}
			else
			{
				// 실패 처리
				throw Exception("Failed to release bottleneck detector's mutex! LockResult=%d", e);
			}
		}
		else
		{
#ifdef _WIN32
			LeaveCriticalSection(&m_standard->m_cs);
#else
			m_standard->m_mutex->Unlock();
#endif
		}
	}

	bool CriticalSection::IsValid()
	{
		return ValidKeyValue == m_validKey;
	}

#ifdef PN_LOCK_OWNER_SHOWN

#if defined(_WIN32)
#pragma warning(disable:4312)
	/** Critical Section을 이 함수를 호출하는 스레드가 Lock했는가 검사한다.
	for x86 NT/2000 only */
	bool IsCriticalSectionLockedByCurrentThread(const CRITICAL_SECTION &cs)
	{
		return cs.OwningThread == (HANDLE)GetCurrentThreadId();
	}

	bool IsCriticalSectionLocked(const CRITICAL_SECTION &cs)
	{
		return (cs.OwningThread != NULL && cs.OwningThread != INVALID_HANDLE_VALUE);
	}
#else
	bool IsCriticalSectionLockedByCurrentThread(const pthread_mutex_t &mutex)
	{
#if defined(__linux__) && !defined(__ANDROID__)
		return (mutex.__data.__owner == syscall(SYS_gettid));
#elif defined(__ANDROID__)
		/* a mutex is implemented as a 32-bit integer holding the following fields
		*
		* bits:     name     description
		* 31-16     tid      owner thread's kernel id (recursive and errorcheck only)
		* 15-14     type     mutex type
		* 13        shared   process-shared flag
		* 12-2      counter  counter of recursive mutexes
		* 1-0       state    lock state (0, 1 or 2)
		*/
		return (short)( ( mutex.value & 0xffff0000 ) >> 16 ) == (short)gettid();
#elif defined(__MACH__)
		uint64_t tid;
		pthread_threadid_np(NULL, &tid);
	#if defined(__LP64__)   // 64bit
		return ( *(uint32_t*)&mutex.__opaque[24] ) == tid;
	#else   // 32bit
		return ( *(uint32_t*)&mutex.__opaque[28] ) == tid;
	#endif  // defined(__LP64__)
#else
		!!!; // 타 플랫폼 개발시 해당 메소드가 구현되어 있지 않을 경우 구현 해야함
			 // 따라서 !!! 로 남긴 뒤, 컴파일 시 작업자가 알 수 있도록 놔둠
#endif
	}

	bool IsCriticalSectionLocked(const pthread_mutex_t &mutex)
	{
#if defined(__linux__) && !defined(__ANDROID__)
		return (mutex.__data.__owner != 0);
#elif defined(__ANDROID__)
		/* a mutex is implemented as a 32-bit integer holding the following fields
		*
		* bits:     name     description
		* 31-16     tid      owner thread's kernel id (recursive and errorcheck only)
		* 15-14     type     mutex type
		* 13        shared   process-shared flag
		* 12-2      counter  counter of recursive mutexes
		* 1-0       state    lock state (0, 1 or 2)
		*/
		return (short)( mutex.value & 0x00000001 ) != (short)0;
#elif defined(__MACH__)
		uint64_t tid;
		pthread_threadid_np(NULL, &tid);
	#if defined(__LP64__)   // 64bit
		return ( *(uint32_t*)&mutex.__opaque[24] ) == tid;
	#else   // 32bit
		return ( *(uint32_t*)&mutex.__opaque[28] ) == tid;
	#endif  // defined(__LP64__)
#else
		!!!; // 타 플랫폼 개발시 해당 메소드가 구현되어 있지 않을 경우 구현 해야함
			 // 따라서 !!! 로 남긴 뒤, 컴파일 시 작업자가 알 수 있도록 놔둠
#endif
	}
#endif

	bool IsCriticalSectionLockedByCurrentThread(const CriticalSection &cs)
	{
#if defined (_WIN32)
		if (cs.m_settings.m_bottleneckWarningThresholdMs > 0)
		{
			return cs.m_bottleneckDetector->m_owningThread == GetCurrentThreadId();
		}
		return IsCriticalSectionLockedByCurrentThread(cs.m_standard->m_cs);
#else
		return IsCriticalSectionLockedByCurrentThread(cs.m_standard->m_mutex->m_mutexHandle);
#endif
	} 

	bool IsCriticalSectionLocked(const CriticalSection &cs)
	{
#if defined (_WIN32)
		if (cs.m_settings.m_bottleneckWarningThresholdMs > 0)
		{
			return ( cs.m_bottleneckDetector->m_owningThread != NULL && cs.m_bottleneckDetector->m_owningThread != (int32_t)INVALID_HANDLE_VALUE );
		}
		return IsCriticalSectionLocked(cs.m_standard->m_cs);
#else
		return IsCriticalSectionLocked(cs.m_standard->m_mutex->m_mutexHandle);
#endif
	}
#endif // PN_LOCK_OWNER_SHOWN

	uint32_t GetAppropriateSpinCount()
	{
		// 테스트 해보니, 하이퍼 스레딩시에도 0을 리턴해야
		// Cpu사용률이 줄어 든다. 5배정도 차이.
		// hyperthread를 쓸 경우 spin count를 설정하면 오히려 해악 (http://software.intel.com/en-us/articles/managing-lock-contention-large-and-small-critical-sections/) 
		// TODO: hyperthread 여부를 감지하도록 수정하자. 이 함수는 자주 콜 됨을 감안해서.
		//if(IsHyperThreading())
		//	return 1000;
		//return 8000;
		//hyperthreading가 아니더라도, 0을 쓰는게 안정적이다.
		// 멀티코어 CPU 자체는 내부적으로 spin count를 하지 않아도 CPU간 동기화 시간이 없다고 한다
		// (출처는 stack overflow에서 봤던 것 같은데-_-)
		// 그리고 요새는 여러 CPU를 가져도 NUMA의 경우 한 프로세스가 여러 CPU에 걸쳐 작동을 안함
		// 따라서 그냥 0으로 고고고
		/* ObjPoolWithSpinLockPerfTest에 의하면, 0과 양수는 큰 차이가 나나 10부터는 값이 커지니까 오히려 처리량이 적다.
		0보다 양수가 효과적인 것은 context switch의 비용이 상당해서 그런 듯.
		값이 커질수록 부정적인 것은 busy wait 공회전이 많아서 그런 듯.
		spin count=0
		Starting test...done!
		total work count=27264600
		spin count=2
		Starting test...done!
		total work count=47800600
		spin count=4
		Starting test...done!
		total work count=47429600
		spin count=8
		Starting test...done!
		total work count=41455800
		spin count=16
		Starting test...done!
		total work count=38202200
		따라서 0에서 5로 바꿈.
		*/
		static int cpuCount = GetNoofProcessors();
		if (cpuCount == 1)
		{
			// CPU가 1개이면 실행중인 다른 스레드가 존재할 수 없으므로 spin count는 항상 무의미하다. 따라서 이때는 0으로 가야.
			// 1 vcore 클라우드 서버 인스턴스는 이런 상황 쉽게 부딪힘.
			return 0;
		}
		else
		{
			return 5;
		}
	}

	void AssertIsLockedByCurrentThread(const CriticalSection &cs)
	{
#ifdef PN_LOCK_OWNER_SHOWN
		assert(IsCriticalSectionLockedByCurrentThread(cs));
#endif
	}

	void AssertIsNotLockedByCurrentThread(const CriticalSection &cs)
	{
#ifdef PN_LOCK_OWNER_SHOWN
		assert(!IsCriticalSectionLockedByCurrentThread(cs));
#endif
	}

	// 이미 생성한 Critical Section 의 Setting 값을 바꿀때 쓰이는 메소드 
	// 해당 CS 를 해제해 버리고 새로 세팅한다.
	void CriticalSection::Reset(CriticalSectionSettings& settings)
	{
		_Uninitialize();			// CS 해제
		_Initialize(settings);	// CS 재 세팅
	}

	void CriticalSection::ShowErrorOnInvalidState()
	{
		if (!IsValid())
		{
			ShowUserMisuseError(_PNT("Cannot enter critical section which has been already destroyed! NOTE: This may be solved by deleting CNetClient instance before your WinMain() finishes working."));
		}
	}


}
