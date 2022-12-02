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
#include <sstream>

#include <errno.h>
#if defined(_WIN32)
#pragma warning(disable:4091) // imagehlp.h 자체의 warning을 무시
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

using namespace std;

namespace Proud
{
	// 1개 프로세스 총체에 걸쳐, 지나치게 자주 덤프 파일을 만들어내면 안되므로
	int64_t CriticalSection_BottleneckDetector::m_lastDumpedTimeMs = 0;

	CriticalSectionSettings::CriticalSectionSettings()
	{
		m_updateLastLockedThreadID = false;

		m_spinCount = GetAppropriateSpinCount();
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
		m_lastLockedThreadID = 0;
		m_standard = new CriticalSection_Standard;
		
#if !defined(_WIN32)
		if (settings.m_bottleneckWarningThresholdMs > 0)
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
			// Windows Vista 이후이면, 가장 최신 API를 쓰자.
			if (CKernel32Api::Instance().InitializeCriticalSectionEx) // 반드시 이걸 나중에 체크해야 한다. Instance를 접근하는 순간 critsec을 접근하므로!
			{
				// [1]처럼 해놔도, OwningThread는 제대로 된 값이 나온다. 따라서 [1]을 켜자. 
				if (!CKernel32Api::Instance().InitializeCriticalSectionEx(
					&m_standard->m_cs,
					settings.m_spinCount,
					CRITICAL_SECTION_NO_DEBUG_INFO)) // [1]
				{
					stringstream part;
					part << "InitializeCriticalSectionEx failed! error=" << GetLastError();
					throw Exception(part.str().c_str());
				}
			}
			else
			{
				/* Windows XP에서는 이걸 쓴다.
				InitializeCriticalSection는 안 쓴다.

				◎ 임계영역 & 에러 핸들링

				내부적으로 임계 영역은 두개 이상의 스레드가 동시에 임계 영역을 가지고 경쟁할 경우 이벤트 커널 오브젝트를 사용한다.
				이런 경쟁이 드물면 시스템은 이벤트 커널 오브젝트를 생성하지 않는다. 메모리가 적은 상황에서 임계 영역에대한 경쟁
				상황이 발생할 수 있고 시스템은 요청된 이벤트 커널 오브젝트의 생성에 실패하게 되면  EnterCriticalSection함수는
				EXCEPTION_INVALID_HANDLE 예외를 발생한다. 이런 예외에 대한 처리는 대부분 하지 않기 때문에 치명적인 상황이 발생할 수 있다.

				위의 상황을 방지하기 위해서는 SEH로 예외를 핸들링하는방법과(비추이다.)

				InitializeCriticalSectionAndSpinCount을 사용해서 임계영역을 생성하는것이다. (dwSpincount를 높게 설정해서)
				dwSpinCount가 높게 설정되면 이 함수는 이벤트 커널 오브젝트를 생성하여 이것을 임계영역과 연결한다. 만약 높은
				비트로 설정이 되었는데 이벤트 커널 오브젝트가 생성되지 않으면 이 함수는 FALSE를 리턴한다. 성공적으로 이벤트
				커널 오브젝트가 생성되면 EnterCriticalSection은 잘동작하고 절대 예외를 발생하지 않는다.

				※ EnterCriticalSection 실행 시 다른 스레드가 해당 임계영역을 소유하고있다면 무기한 대기를 하는것이 아니고
				레지스트리에 등록된 시간만큼만 대기하고 하나의 예외를 발생한다.

				출처: http://devnote.tistory.com/208
				*/
				if (!InitializeCriticalSectionAndSpinCount(&m_standard->m_cs, settings.m_spinCount))
				{
					stringstream part;
					part << "InitializeCriticalSectionAndSpinCount failed! error=" << GetLastError();
					throw Exception(part.str().c_str());
				}
			}
#else
			// 유닉스에서
			m_standard->m_mutex = new Mutex;
#endif
		}
		
		
#ifdef _WIN32
		m_platformSpecificObject = &m_standard->m_cs;
#else
		m_platformSpecificObject = &(m_standard->m_mutex->m_mutexHandle);
#endif
	}

	// NetServer는 Start() 할때 StartServerParameter 로 Bottleneck Warning 기능을 On/Off 할지 결정하는데
	// 생성자에서 바로 _Init을 들어가서 세팅을 못함
	// 따라서 해당 함수를 따로 만듬
	void CriticalSection::_Uninitialize()
	{
#ifdef PN_LOCK_OWNER_SHOWN
		// 사용자 실수로 이미 동작중인 CriticalSection 에 Setting 을 바꿔버리면 곤란하므로
		// 작동중인 CS는 막아버린다.
		if ((IsCriticalSectionLocked(*this)) && (IsOwningThreadAlive() == true))
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
		if ((IsCriticalSectionLocked(*this)) && (IsOwningThreadAlive() == true))
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
			m_bottleneckDetector->m_owningThread = Proud::GetCurrentThreadID(); // 해당 유닉스 함수를 나중에 만들어야 함
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
						HANDLE owningThreadHandle = ::OpenThread(THREAD_SUSPEND_RESUME, false, (DWORD)ret.m_owningThread);
						if (owningThreadHandle != NULL) // OpenThread 리턴값이 그렇다고 함
						{
							::SuspendThread(owningThreadHandle);

							// String 객체의 경우 생성시에 내부에서 atomic 연산을 수행합니다
							// 전역으로 선언된 g_atomicOpCritSec을 초기화하는 과정에서 String 인스턴스를 만들게 되면 결국은 내부적으로 CSlowAtomic 계열 함수를 호출하고
							// 아직 초기화가 끝나지 않는 상태인 g_atomicOpCritSec을 사용하기 때문에 NullPointerException가 발생됩니다.
							// 그래서 생성자에서 FilePath 및 FileName 초기화 구문을 삭제 합니다. FilePath 및 FileName이 설정되어 있지 않을 경우, 이곳에서 default로 값을 정하도록 합니다.

							String dumpFilePath = (true == m_settings.m_bottleneckWarningDumpFilePath.IsEmpty() ? _PNT("./") : m_settings.m_bottleneckWarningDumpFilePath);
							String dumpFileName = (true == m_settings.m_bottleneckWarningDumpFileName.IsEmpty() ? _PNT("default") : m_settings.m_bottleneckWarningDumpFileName);

							timespec ts;
							timespec_get_pn(&ts, TIME_UTC);

							tm t;
							localtime_pn(&ts.tv_sec, &t);

							// Format()보다 이것이 더 안전. long int 등에서.
							stringstream part;
							part << "-bottleneck-tid-" << ret.m_owningThread;
							part << "-when";
							part << "-" << t.tm_year + 1900;
							part << "-" << t.tm_mon + 1;
							part << "-" << t.tm_mday;
							part << "-" << t.tm_hour;
							part << "-" << t.tm_min;
							part << "-" << t.tm_sec;
							part << "-" << ts.tv_nsec / 1000000;
							part << ".dmp";

							String fileName = dumpFilePath + dumpFileName + StringA2T(part.str().c_str());

							HANDLE dumpFileHandle = ::CreateFile(fileName.GetString(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
								FILE_ATTRIBUTE_NORMAL, NULL);
							if (dumpFileHandle != INVALID_HANDLE_VALUE)
							{
								MiniDumpWriteDump(GetCurrentProcess(),
									(DWORD)GetCurrentProcessId(), dumpFileHandle,
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
				LockBottleneckDetectorResult ret_;
				_BottleneckDetectorAwareLock(PN_INFINITE, ret_);
				if (ret.m_lockResult == LockResult_Error)
				{
					// 중도에 critsec을 destroy하면 여기에 걸릴 수 있다.
					stringstream part;
					part << "Failed to acquire lock! error=" << ret.m_lockResult;
					throw Exception(part.str().c_str());
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

		if (m_settings.m_updateLastLockedThreadID)
			m_lastLockedThreadID = Proud::GetCurrentThreadID();
	}

	bool CriticalSection::IsOwningThreadAlive() const
	{
#if defined(_WIN32)
		return (Proud::GetThreadStatus(m_standard->m_cs.OwningThread) == STILL_ACTIVE);
#else
		//windows 이외의 OS는 추가적으로 구현을 진행해야 합니다.
		return true;
#endif
	}

	void CriticalSection::UnsafeLock()
	{
		Lock_internal();
	}

	bool CriticalSection::TryLock()
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
		{
			m_tryLockSuccessCount++;
			if (m_settings.m_updateLastLockedThreadID)
				m_lastLockedThreadID = Proud::GetCurrentThreadID();
		}
		else
		{
			m_tryLockFailCount++;
		}

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
				stringstream part;
				part << "Failed to release bottleneck detector's mutex! LockResult=" << e;
				throw Exception(part.str().c_str());
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
		return cs.OwningThread == (HANDLE)::GetCurrentThreadId();
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
		/* https://android.googlesource.com/platform/bionic/+/ndk-release-r16/libc/bionic/pthread_mutex.cpp:218
		 * 위 구글 NDK r16 bionic C 소스 페이지에서 다음과 같은 구조체 형태를 참고해서 수정하였습니다.
		 * 32비트일 경우 기존 코드와 호환됩니다.
		struct pthread_mutex_internal_t {
          _Atomic(uint16_t) state;
        #if defined(__LP64__)
          uint16_t __pad;
          atomic_int owner_tid;
          char __reserved[32];
        #else
          _Atomic(uint16_t) owner_tid;
        #endif
        } __attribute__((aligned(4)));
		*/
#if defined(__LP64__)
        // atomic_int owner_tid에 해당하는 부분만 남기게 비트 연산을 한뒤 앞의 빈 32비트만큼을 쉬프트 연산으로 땡깁니다.
        return (uint64_t)( ( *(uint64_t*)&mutex & 0x0000ffff00000000 ) >> 32 ) == (uint64_t)gettid();
#else //__LP64__
        // owner_tid에 해당하는 부분만 남기게 비트 연산을 한뒤 앞의 빈 16비트만큼을 쉬프트 연산으로 땡깁니다.
		return (uint32_t)( ( *(uint32_t*)&mutex & 0xffff0000 ) >> 16 ) == (uint32_t)gettid();
#endif //__LP64__
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
		/* a mutex is implemented as a 16-bit integer holding the following fields
		*
		* bits:     name     description
		* 15-14     type     mutex type
		* 13        shared   process-shared flag
		* 12-2      counter  counter of recursive mutexes
		* 1-0       state    lock state (0, 1 or 2)
		*/
#if defined(__LP64__)
        return (uint64_t)( *(uint64_t*)&mutex & 0x0000000000000001 ) != (uint64_t)0;
#else //__LP64__
        return (uint32_t)( *(uint32_t*)&mutex & 0x00000001 ) != (uint32_t)0;
#endif //__LP64__
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
		return cs.IsLockedByCurrentThread();
	}

	bool CriticalSection::IsLockedByCurrentThread() const
	{
#if defined (_WIN32)
		if (m_settings.m_bottleneckWarningThresholdMs > 0)
		{
			return m_bottleneckDetector->m_owningThread == Proud::GetCurrentThreadID();
		}
		return IsCriticalSectionLockedByCurrentThread(m_standard->m_cs);
#else
		return IsCriticalSectionLockedByCurrentThread(m_standard->m_mutex->m_mutexHandle);
#endif
	}

	bool IsCriticalSectionLocked(const CriticalSection &cs)
	{
		return cs.IsLocked();
	}

	bool CriticalSection::IsLocked() const
	{
#if defined (_WIN32)
		if (m_settings.m_bottleneckWarningThresholdMs > 0)
		{
			return (m_bottleneckDetector->m_owningThread != NULL && m_bottleneckDetector->m_owningThread != (int64_t)INVALID_HANDLE_VALUE);
		}
		return IsCriticalSectionLocked(m_standard->m_cs);
#else
		return IsCriticalSectionLocked(m_standard->m_mutex->m_mutexHandle);
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
