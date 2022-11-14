/*
ProudNet v1


이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의 : 저작물에 관한 위의 명시를 제거하지 마십시오.


This program is soley copyrighted by Nettention.
Any use, correction, and distribution of this program are subject to the terms and conditions of the License Agreement.
Any violated use of this program is prohibited and will be cause of immediate termination according to the License Agreement.

** WARNING : PLEASE DO NOT REMOVE THE LEGAL NOTICE ABOVE.


此程序的版权归Nettention公司所有。
与此程序的修改、使用、发布相关的事项要遵守此程序的所有权者的协议。
不遵守协议时要原则性的禁止擅自使用。
擅自使用的责任明示在与此程序所有权者的合同书里。

** 注意：不要移除关于制作物的上述明示。


このプログラムの著作権はNettentionにあります。
このプログラムの修正、使用、配布に関する事項は本プログラムの所有権者との契約に従い、
契約を遵守しない場合、原則的に無断使用を禁じます。
無断使用による責任は本プログラムの所有権者との契約書に明示されています。

** 注意：著作物に関する上記の明示を除去しないでください。

*/

#pragma once
#include "BasicTypes.h"
#include "Ptr.h"
#include "Event.h"
#include "CallbackContext.h"
#include "LambdaWrap.h"

#ifdef SUPPORTS_LAMBDA_EXPRESSION
#include <functional>
#endif
#include <memory>

//#pragma pack(push,8)

#if (_MSC_VER>=1400)
#pragma managed(push, off)
#endif

namespace Proud
{
	class ThreadProcContext;

	/** \addtogroup util_group
	*  @{
	*/

	/**
	\~korean
	단순 스레드 Wrapper 클래스입니다.
	- 이 인스턴스를 생성한 후 Start를 호출하면 스레드가 생성됩니다. 그리고 이 인스턴스가 파괴되면 생성되었던 스레드가 종료할 때까지 블러킹됩니다.
	- 이 클래스는 .net framework의 System.Threading.Thread 클래스와 같은 작동 방식을 가집니다.

	일반적 사용법
	- Thread 객체를 생성하되, 파라메터로 스레드 함수를 지정합니다. 스레드 함수는 생성자에서 지정합니다.
	- Thread 객체를 생성한다고 해서 바로 스레드가 작동하지는 않습니다. Start를 실행해야만 합니다.
	- Join을 호출하거나 Thread 객체가 파괴될 때 실행중인 스레드가 종료할 때까지 기다립니다.

	\~english
	Simple thread wrapper class
	- After creating this instance, a thread will be created by calling Start. And if this instance is destructed then it will be blocked until created thread is terminated.
	- This class has the same operating process as System.Threading.Thread class of .NET framework.

	General usage
	- Creates a thread object and designates thread function as a parameter. The thread function is designated by constructor.
	- Creating a thread object does not mean immediate thread execution. Start must be run.
	- Either when calls Join or destructing the thread object, this waits until the running thread ends.

	\~chinese
	单线程Wrapper类
	- 生成此实例后呼出Start即可生成线程。此实例被破坏后，在所生成的线程终止为止进行Blocking。
	- 此类与 .net framework的 System.Threading.Thread 类具有相同的运行方式。

	一般使用方法
	- 生成Thread对象并用参数指定线程函数。线程函数在生成者中指定。
	- 即使生成了Thread对象也不会立即运行线程，需要运行Start。
	- 呼出Join或Thread对象被破坏时一直等到运行中的线程终止为止。
	\~japanese
	シンプルスレッドWrapperクラス
	- このインスタンスを生成してStartを呼び出すとスレッドが生成されます。そしてこのインスタンスが破壊されると生成されてたスレッドが終了されるまでブロックされます。
	-このクラスは.net frameworkのSystem.Threading.Threadクラスと同じような作動方式を持ちます。

	一般的な使い方
	-Threadオブジェクトを生成しますが、パラメーターでスレッドの関数を指定します。スレッド関数は構築子で指定します。
	-Threadオブジェクトを生成しても、すぐにスレッドが作動するわけではありません。Startを実行しなければなりません。
	-Joinを呼び出したりThreadオブジェクトが破壊される際、実行中のスレッドが終了されるまで待ちます。
	\~

	\~korean 여러분은 스레드가 실행할 루틴을 아래처럼 람다식(lambda expression)으로 만드셔도 됩니다.
	\~english You may create a routine that a thread will execute as Lambda Expression below.
	\~chinese 对于线程将运行的Routine您可以用以下Lambda形式创建。
	\~japanese スレッドが実行するルーチンを下記のようにラムダ式(lambda expression)に作っても問題ありません。

	\~
	\code
	CriticalSection critSec;
	int totalWorkCount = 0;
	volatile bool stopThread = false;
	ThreadPtr th = ThreadPtr(new Thread([&]()
	{
		// note that thread function is defined exactly here
		// and even the variables out of the scope are
		// used here, thankfully by lambda capture above.
		while (!stopThread)
		{
			CriticalSectionLock lock(critSec, true);
			do_something();
			totalWorkCount++;
		}
	});
	th->Start();
	do_something_or_wait();
	stopThread = true;
	th->Join();
	print(totalWorkCount);
	\endcode
	*/
	class Thread
	{
		friend class ThreadProcContext;

#if defined(_WIN32)
		//static void __cdecl InternalThreadProc(void* ctx);
		static DWORD __stdcall InternalThreadProc(void* ctx);
#else
		static void* InternalThreadProc(void* ctx);
#endif

#if defined(_WIN32)
		HANDLE m_handle;
		uint32_t m_ID;
#else
		pthread_t m_thread;
#endif
		bool m_neededJoin;
	public:
		static bool m_dllProcessDetached_INTERNAL;

		/**
		\~korean
		생성자
		\param threadProc 이 인스턴스가 쥐고 있을 스레드의 메인 함수
		\param ctx threadProc에 넘겨질 파라메터
		\param neededJoin Join 함수의 사용 여부
		기본값은 true입니다. 만약 본 스레드가 메인 스레드보다 먼저 종료됨이 보장되며, Join 기능을 사용하지 않으시려면 false로 지정해주시면 됩니다.

		\~english TODO:translate needed.
		Constructor
		\param threadProc Main function of the thread that is occupied by this instance
		\param ctx The parameter is to be passed to threadProc

		\~chinese TODO:translate needed.
		生成者
		\param threadProc 此实例将拥有的线程主函数。
		\param ctx 将传达给threadProc的参数。

		\~japanese TODO:translate needed.

		\~
		*/
	private:
		void Init_INTERNAL();
	public:
		// 고전적 방식(C function with context ptr)
		typedef void(*ThreadProc)(void* ctx);
		PROUD_API Thread(ThreadProc threadProc, void *ctx, bool neededJoin = true);

	public:
		/**
		\~korean
		파괴자
		- 스레드가 미실행중이면 즉시 리턴하나, 스레드가 이미 실행중이면 스레드가 종료할 때까지 기다린다.

		\~english
		Destructor
		- Immediately returns if thread is not running but waits until the thread terminates if the thread is already running.

		\~chinese
		破坏者。
		- 如果线程为未运行状态则立即进行Return，如果线程已经运行则等到线程终止为止。

		\~japanese

		\~
		*/
		PROUD_API ~Thread();

		/**
		\~korean
		스레드를 생성한다.
		- 이미 생성한 상태면 예외가 발생한다.

		\~english
		Creates thread
		- An exeption will ocur if already created.

		\~chinese
		生成线程。
		- 如果已经生成了线程则发生例外。

		\~japanese

		\~
		*/
		PROUD_API void Start();

		/**
		\~korean
		스레드가 종료할 때까지 기다린다.

		\~english
		Waits until the thread terminates

		\~chinese
		等到线程终止。

		\~japanese

		\~
		*/
		PROUD_API void Join();

		/**
		\~korean
		스레드 핸들

		\~english
		Thread handle

		\~chinese
		线程Handle。

		\~japanese

		\~
		*/
#if defined(_WIN32)
		__declspec(property(get = GetHandle)) HANDLE Handle;
#endif
		/**
		\~korean
		스레드 핸들을 얻는다.

		\~english
		Gets thread handle

		\~chinese
		获得线程Handle。

		\~japanese

		\~
		*/
#if defined(_WIN32)
		inline HANDLE GetHandle()
		{
			return m_handle;
		}
#else
		inline pthread_t GetHandle()
		{
			return m_thread;
		}
#endif

		/**
		\~korean
		스레드 아이디

		\~english
		Thread ID

		\~chinese
		获得线程ID。

		\~japanese

		\~
		*/
#if defined(_WIN32)
		__declspec(property(get = GetID)) uint32_t ID;
#endif

		/**
		\~korean
		스레드 아이디를 얻는다

		\~english
		Gets thread ID

		\~chinese
		获得线程ID。

		\~japanese

		\~
		*/
#if defined(_WIN32)
		inline uint32_t GetID()
		{
			return m_ID;
		}
#endif

		/**
		\~korean
		Static library로서의 ProudNet이 DLL에서 사용되는 경우
		DllMain의 Process detach case에서 이 메서드를 꼭 호출해야 한다.

		\~english
		When ProudNet is used at DLL as a ststic library, this method must be called at Process detach case of DllMain.

		\~chinese
		作为Static library在DLL中使用ProudNet的情况。
		一定要在DllMain的Process detach case中呼出此方法。

		\~japanese

		\~
		*/
		PROUD_API static void NotifyDllProcessDetached();
	private:
		// 사용자가 입력한 함수와 데이터 (고전)
		ThreadProc m_threadProc;
		void* m_ctx;

		// C++11 방식(lambda expression)
#ifdef SUPPORTS_LAMBDA_EXPRESSION
	private:
		// 사용자가 입력한 함수와 데이터 (새로운)
		std::shared_ptr<LambdaBase_Param0<void>> m_lambdaProc;
	public:
		Thread(const std::function<void()> &function)
#ifdef _WIN32
			:m_threadStopped(true, true)
#endif // _WIN32
		{
			m_threadProc = NULL;
			m_ctx = NULL;
			m_lambdaProc = std::shared_ptr<LambdaBase_Param0<void>>(new Lambda_Param0<void>(function));
			m_neededJoin = true;
			Init_INTERNAL();
		}
#endif
	private:
#if defined(_WIN32)
		// non-win32에서는 pthread_join이 쓰이므로 막혀도 됨
		Event m_threadStopped;

	public:
		// true이면 CoInitialize가 스레드 시작시 호출된다.
		bool m_useComModel;
#endif
	};

	/**
	\~korean
	\fn typedef RefCount<Thread> ThreadPtr;
	Thread 객체의 smart pointer 클래스이다.

	\~english
	\fn typedef RefCount<Thread> ThreadPtr;
	The smart pointer class of thread object

	\~chinese
	是Thread对象的 smart pointer类。

	\~japanese

	\~
	*/
	typedef RefCount<Thread> ThreadPtr;

	/**  @} */
}

#if (_MSC_VER>=1400)
#pragma managed(pop)
#endif



//#pragma pack(pop)
