/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <new>

#if defined(_WIN32)
#include <process.h>
#else
#include <errno.h>
#include <pthread.h>
#endif


#include "../include/PnThread.h"
#include "../include/Exception.h"
#include "../include/sysutil.h"


namespace Proud
{
	// Thread 객체가 힙이 아닌 스택에 생성되고 파괴 된 뒤, Procedure 가 불러졌을 때
	// Thread 객체의 직접 참조로 인한 크래쉬 문제 해결 방안
	class ThreadParam
	{
	public:
		Thread* m_owner;
		bool	m_neededJoin;

		Thread::ThreadProc m_threadProc;
		void* m_ctx;

		RefCount< LambdaBase_Param0<void> > m_lambdaProc;

#ifdef _WIN32
		bool m_useComModel;
#endif
	};

	// Proud.Thread가 예외를 일으키고 종료하는 경우도 파괴자를 통해 뒷처리를 수행하기 위함.
	// try-catch-finally를 모든 컴파일러에서 쓸 수 있다면 이런 짓이 불필요하다.
	class ThreadProcContext
	{
	private:
		ThreadParam* m_param;

	public:
		ThreadProcContext(ThreadParam* param)
		{
			assert(param != NULL);

			// param 객체는 Thread.Start 에서 힙 메모리에 생성 된 뒤, 온것이므로 안전하다.
			m_param = param;

#ifdef _WIN32
			if (m_param->m_useComModel)
			{
				// CoInitialize(0)은 STA로 작동. 즉, C# BeginInvoke처럼 작동해서 느리다. 따라서 아래와 같이 옵션을 붙여야.
				CoInitializeEx(NULL, COINIT_MULTITHREADED);
			}
#endif
		}

		~ThreadProcContext()
		{
#ifdef _WIN32
			//NTTNTRACE("Event %d signalled.\n", m_thread->m_threadStopped.m_event);
			if (m_param->m_neededJoin)
				m_param->m_owner->m_threadStopped.Set();

			if (m_param->m_useComModel)
				CoUninitialize();
#endif
			// param 객체 제거
			delete m_param;
		}
	};




	void Thread::Start()
	{
		// Thread 의 스택 메모리 생성 대비로 사본을 생성하여 Procedure 에 넘김
		ThreadParam* param = new ThreadParam();
		param->m_owner = this;
		param->m_neededJoin = m_neededJoin;
#ifdef _WIN32
		param->m_useComModel = m_useComModel;
#endif
		param->m_threadProc = m_threadProc;
		param->m_ctx = m_ctx;
		param->m_lambdaProc = m_lambdaProc;

		// 쓰레드 생성 실패시에 ThreadParam 을 메모리 해제 시켜야 하는데
		// Throw 코드가 많다 ㄱ- 따라서 일괄로 try-catch 묶어서
		// catch 후에 re-throw 방식으로 깔끔하게 해결
		try
		{
#if defined(_WIN32)
			if (m_handle != INVALID_HANDLE_VALUE)
				throw Exception("Thread is already started!");

			m_threadStopped.Reset();

			// 예전에는 _beginthread를 썼지만 ProudNet DLL과 CLR이 혼용될 경우 강제로 worker thread termination이
			// 발생하므로 이것으로 교체!

			// NOTE: 테스트용도로 나중에 _beginthreadexe 로 바꿔본후 pntest 돌려보자. (test case 1번 데드락 현상)
			m_handle = ::CreateThread(NULL, 0, InternalThreadProc, param, 0, (LPDWORD)&m_ID);
#else
			int ret = 0;

			/* seungchul.lee : 2014.07.29
			* http://man7.org/linux/man-pages/man3/pthread_attr_init.3.html
			* Once a thread attributes object has been destroyed, it can be reinitialized using pthread_attr_init().
			* Any other use of a destroyed thread attributes object has undefined results. */
			pthread_attr_t attr;

			ret = pthread_attr_init(&attr);
			if (ret != 0)
			{
				Tstringstream part;
				part << "pthread_attr_init function is failed. errno:" << errno;
				throw Exception(part.str().c_str());
			}

			// 사용자가 Join 기능이 필요하지 않다고 명시한 경우에는 스레드를 Detach로 생성
			if (!m_neededJoin)
			{
				ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
				if (ret != 0)
				{
					Tstringstream part;
					part << "pthread_attr_setdetachstate function is failed. errono:" << errno;
					throw Exception(part.str().c_str());
				}
			}

			ret = pthread_create(&m_thread, &attr, InternalThreadProc, param);
			if(ret != 0)
			{
				Tstringstream part;
				part << "pthread_create funtion is failed. errno:" << errno;
				throw Exception(part.str().c_str());
			}

			m_ID = (uint64_t)(m_thread);

			/* seungchul.lee : 2014.07.29
			* http://man7.org/linux/man-pages/man3/pthread_attr_init.3.html
			* When a thread attributes object is no longer required,
			* it should be destroyed using the pthread_attr_destroy() function.
			* Destroying a thread attributes object has no effect on threads that were created using that object. */
			ret = pthread_attr_destroy(&attr);
			if (ret != 0)
			{
				Tstringstream part;
				part << "pthread_attr_destroy function is failed. errno:" << errno;
				throw Exception(part.str().c_str());
			}
#endif
		}
		catch (Exception e)
		{
			delete param;
			throw e;
		}
	}

	Thread::Thread(ThreadProc threadProc, void *ctx, bool neededJoin)
#ifdef _WIN32
		:m_threadStopped(true, true)
#endif // _WIN32
	{
		m_threadProc = threadProc;
		m_ctx = ctx;
		m_neededJoin = neededJoin;

		Init_INTERNAL();
	}

#ifdef _WIN32
	DWORD __stdcall Thread::InternalThreadProc(void* ctx)
#else
	void* Thread::InternalThreadProc(void* ctx)
#endif
	{
		// 이 param 객체는 누구도 관리하지 않던 힙 메모리 영역의 데이터이므로
		// 여기서 부터 Context 가 메모리 소유주가 되어 관리 하도록 한다.
		ThreadParam* th = (ThreadParam*)ctx;
		ThreadProcContext cc(th); // 파괴자에서 뒷정리 위해.

		if ( th->m_lambdaProc != nullptr )
		{
			assert(th->m_threadProc == NULL);
			th->m_lambdaProc->Run();
		}
		if ( th->m_threadProc != NULL )
		{
			assert(th->m_lambdaProc == nullptr);
			th->m_threadProc(th->m_ctx);
		}

#ifdef _WIN32
		return 333;
#else
		return (void*)333;
#endif // _WIN32
	}

	void Thread::Join()
	{
		/* seungchul.lee : 2014.07.29
		* 스레드를 정상적으로 이미 종료하였거나 사용자가 아래의 기능을 끈 상태로 생성한 경우 */
		if ( !m_neededJoin ) {
#if defined(_WIN32)
			if ( m_handle != INVALID_HANDLE_VALUE )
			{
				::CloseHandle(m_handle);
				m_handle = INVALID_HANDLE_VALUE;
			}
#endif
			return;
		}

#if defined(_WIN32)
		// 이미 스레드가 종료한 상태일 수도 있으므로 체크하도록 하자.
		// (.net framework와 연동된 프로그램의 경우 worker thread가 먼저 종료되는 경우가 있는 것 같다. 끙.)

		int waitTimeMs = 200;
		int alertTimeMs = 0;

		while (true)
		{
			// Stop Event만으로 쓰레드 종료를 판단할 경우,
			// 비정상적인 쓰레드 종료가 발생시 무한루프에 빠질수 있다.
			// 따라서 thread handle을 통해 쓰레드 상태도 확인한다.
			HANDLE ws[2] = { m_threadStopped.m_event, m_handle };

			// 흔히 발생하는 사건이 아니므로 200이다.
			if (::WaitForMultipleObjects(2, ws, FALSE, waitTimeMs) != WAIT_TIMEOUT)
				break;

			// 이 클래스를 쓰는 모듈이 DLL 모듈인 경우 프로세스 종료시 그냥 이게 세팅되어 버리는 사태도 있어서
			if (m_dllProcessDetached_INTERNAL)
				break;

			// 스레드가 오랫동안 join을 못하면 뭔가 이상한 상황이다. 경고 표시.
			if (alertTimeMs >= 5000)
			{
				alertTimeMs = -1;
				String str;
				str.Format(_PNT("[%d] Thread is not yet ended.\n"), GetID());
				OutputDebugString(str.GetString());
			}
			else
			{
				alertTimeMs += waitTimeMs;
			}
		}

		// 스레드 핸들 닫기
		//이게 없으면 핸들 증가로 이어진다.
		::CloseHandle(m_handle);

		m_handle = INVALID_HANDLE_VALUE;
#else
		// 비 윈도에서는 그냥 이걸로 수행.
		// 아직 Proud.Event가 미구현이라.
		int ret = pthread_join(m_thread, NULL);
		if(ret != 0 && ret != -1) // 이미 종료된 스레드면 -1이 리턴됨?
		{
			// man page에 의하면 pthread_join은 errno값을 리턴한다.
			Tstringstream part;
			part << "pthread_join function is failed. errno:" << ret;
			throw Exception(part.str().c_str());
		}
#endif
		m_ID = 0;

		m_neededJoin = false;
	}

	// 스레드가 종료할 때까지 대기
	Thread::~Thread()
	{
		Join();
	}

	bool Thread::m_dllProcessDetached_INTERNAL = false;

#if defined(_WIN32)
	bool Thread::IsAlive() const
	{
		return (Proud::GetThreadStatus(m_handle) == STILL_ACTIVE);
	}
#endif

	void Thread::NotifyDllProcessDetached()
	{
		m_dllProcessDetached_INTERNAL = true;
	}

	// 중복코드 방지차
	void Thread::Init_INTERNAL()
	{
		m_ID = 0;
#if defined(_WIN32)
		m_handle = INVALID_HANDLE_VALUE;
		m_useComModel = false;
#endif
	}

}



