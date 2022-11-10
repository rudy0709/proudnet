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

//#include <string> 사용자에게 노출되는 헤더 파일은 STL을 사용할 수 없으므로
#include "BasicTypes.h"
#include "atomic.h"
#include "PNString.h"
#include "pnmutex.h"
#include "ClassBehavior.h"

#if !defined(_WIN32)
    #include <pthread.h>
    #include <time.h>
#endif

#if defined(_MSC_VER) && defined(_WIN32)
#pragma comment(lib,"dbghelp.lib") // Lock_Interval에서 MiniDumpWriteDump를 쓰기 때문에.
#endif

//#pragma pack(push,8)
namespace Proud
{
#if (_MSC_VER>=1400)
#pragma managed(push, off)
#endif
	class CriticalSection_BottleneckDetector;
	class CriticalSection_Standard;

	uint32_t GetAppropriateSpinCount();

	/** parameters for critical section initialization.
	*/
	class CriticalSectionSettings
	{
	public:
		/// spin count. will be set to appropriate value by default.
		/// This value will be set to 0 if only one CPU core is being used.
		uint32_t m_spinCount;
#ifdef _WIN32
		/// true if you want to perform faster for Windows 6 or later.
		bool m_fasterForWindows6;
#endif
		/**
		\~korean
		병목 발생시 덤프 파일을 저장할 이름을 설정합니다.
		(ex. L"example_dump_file_name/")
		\~english
		\~chinese
		\~japanese
		\~
		*/
		String m_bottleneckWarningDumpFileName;

		/**
		\~korean
		병목 발생시 덤프 파일을 저장할 경로를 설정합니다.
		(ex. L"C:/")
		\~english
		\~chinese
		\~japanese
		\~
		*/
		String m_bottleneckWarningDumpFilePath;

		/**
		\~korean
		경고가 발생할때의 최대 시간값을 설정합니다.
		해당값이 0 이외의 값으로 세팅되면 NetServer 내부에서 해당 값 만큼 병목 발생시 경고와 덤프 파일을 남깁니다.
		\~english
		\~chinese
		\~japanese
		\~
		*/
		uint32_t m_bottleneckWarningThresholdMs;

		CriticalSectionSettings();
	};


	class CriticalSection_BottleneckDetector;
	// _BottleneckDetectorAwareLock 반환값
	class LockBottleneckDetectorResult
	{
	public:
		LockResult m_lockResult;
		int32_t m_owningThread; // lock을 성공시킨 스레드의 ID
	};



	/** \addtogroup util_group
	*  @{
	*/
	/**
	\~korean
	Critical section 클래스
	- MFC의 CCriticalSection보다 기능이 더 보강되어 있다.
	- 특히 (SMP 환경에서) thread sleep state를 최대한 줄이기 위해 기본적으로 spin lock이 설정되어 있다.
	일반적 용도
	- CriticalSection 객체를 먼저 만든 후, CriticalSectionLock 객체를 통해 critical section lock & unlock을 할 수 있다.
	\~english
	Critical section (class)
	- More fortified features than CCriticalSection of MFC environment
	- A spin lock is set as default in order to minimize thread sleep state under SMP environment.
	General usage
	- It is possible to perform critical section lock/unlock through CriticalSectionLock object after creating it.
	\~chinese
	Critical section类
	- 比MFC的 CCriticalSection%功能加强了。
	- 特别是（SMP环境下）为了把thread sleep state最大程度减少，基本都设置了spin lock。
	一般用途
	先把CriticalSection对象创建后，通过CriticalSectionLock对象可以critical section lock & unlock。
	\~japanese
	\~
	 */
	class CriticalSection
	{
        //String m_name;
		uint32_t m_validKey;
	public:
		CriticalSectionSettings m_settings;

		CriticalSection_Standard* m_standard;

		// 0: 덤프 안 남기는 중, 1: 덤프 남기고 있는 중
		CriticalSection_BottleneckDetector* m_bottleneckDetector;

		/** Initializes a critical section object. */
        PROUD_API CriticalSection();

		/** Initializes a critical section object. */
		PROUD_API CriticalSection(CriticalSectionSettings &settings);

	private:
		void _Initialize(CriticalSectionSettings &settings);
		void _Uninitialize();

		void _BottleneckDetectorAwareLock(int32_t timeout, LockBottleneckDetectorResult& result);

		void Lock_internal();
		inline bool IsBottleneckWarningEnabled()
		{
			return (bool)(m_settings.m_bottleneckWarningThresholdMs > 0);
		}

	public:
		/**
		\~korean
		파괴자
		- 파괴하기 전에, 이 critical section을 점유하고 있는 스레드가 없어야 한다!
		\~english
		Destroyer
		- There must be no thread occupying the critical section before the desruction.
		\~chinese
		破坏者
		- 破坏之前，不能有占有此critical section的线程！
		\~japanese
		\~
		  */
		PROUD_API ~CriticalSection(void);

		/**
		\~korean
		critical section을 이 메서드를 호출하는 스레드가 점유한다.
		잘못된 호출이 일어났을 시 MessageBox를 띄워줍니다.
		\~english
		The critical section is to be occupied by the thread calling this method.
		\~chinese
		呼叫此方法的线程占有critical section。
		发生错误呼叫时打开MessageBox。
		\~japanese
		\~
		 */
		PROUD_API void Lock();

		/**
		\~korean
		critical section을 이 메서드를 호출하는 스레드가 점유한다.
		잘못된 호출이 일어났을 시 아무 처리하지 않고 넘어갑니다.
		\~english TODO:translate needed.
		The critical section is to be occupied by the thread calling this method.
		\~chinese
		呼叫此方法的线程占有critical section。
		发生错误的呼时不做任何处理。
		\~japanese
		\~
		 */
		PROUD_API void UnsafeLock();

		/**
		\~korean
		critical section을 이 메서드를 호출하는 스레드가 점유 해제한다.
		\~english
		The critical section is to be relieved by the thread calling this method.
		\~chinese
		呼叫此方法的线程解除占有critical section。
		\~japanese
		\~
		 */
		PROUD_API void Unlock();

		/**
		\~korean
		EnterCriticalSection 대신 TryEnterCriticalSection을 사용한다.
		\return 성공적으로 잠금 했으면 true.
		\~english
		Use TryEnterCriticalSection rather than EnterCriticalSection.
		\return If it locked successfully, it is true.
		\~chinese
		TryEnterCriticalSection代替EnterCriticalSection使用。
		\return 锁定成功的话true。
		\~japanese
		\~
		 */
		PROUD_API bool TryLock();
		PROUD_API bool IsValid();
		//void SetName(String name);

		// 이 값이 true이면 파괴자에서 내부 객체를 파괴 안함.
		// 이 값 다루기가 필요한 경우: 이 타입의 전역 객체이고, 프로세스 종료시, 객체 파괴 순서와 무관하게 다루고자 할 때, 누수를 감수하더라도.
		bool m_neverCallDtor;
		// Critical Section 의 Setting 값을 바꾸고 싶을때
		void Reset(CriticalSectionSettings& settings);

		void ShowErrorOnInvalidState();

		//////////////////////////////////////////////////////////////////////////
		// lock을 할 때 context switch가 발생했는지를 알 수 있다.
		// 성능 예: object pool을 위한 경우로 사용되는 경우 보통 700:1 정도의 contention을 일으킨다.

		// try-lock 성공 회수
		int m_tryLockSuccessCount;

		// try-lock 실패 횟수. 즉 일반 lock 횟수.
		int m_tryLockFailCount;

		NO_COPYABLE(CriticalSection)
	};

	//typedef RefCount<CriticalSection,false> CriticalSectionPtr;

	/**
	\~korean
	\brief CriticalSection 객체를 lock access하는 객체
	- 반드시 로컬 변수로 생성하여 하나의 쓰레드 내에서만 사용한다.
	- 이 객체가 파괴될 때 자동으로 lock하고 있던 critical section을 unlock한다.
	\~english
	 \brief The object that 'lock accesses' CriticalSection object
	- Usually created and used as local vaiable
	- When this object is destroyed, it automatically unlocks the critical section that was locked by it.
	\~chinese
	\brief CriticalSection 把对象lock access的对象。
	- 主要生成本地变数后使用。
	- 此对象被破坏的时候自动unlock正lock的critical section。
	\~japanese
	\~
	*/
	class CriticalSectionLock
	{
		CriticalSection* m_cs;

		// 몇 번의 Lock을 호출했는가?
		// 주의: m_cs의 실제 lock 횟수와 다를 수 있다.
		// 이 값은 이 CriticalSectionLock에 의해서 Lock한 것들만 카운팅한다.
		int m_recursionCount;

	public:
		/**
		\~korean
		critical section 객체를 생성자에서 바로 lock할 수 있다.
		\param cs 사용할 critical section 객체
		\param initialLock true이면 생성자에서 바로 lock한다.
		\~english
		It is possible to directly lock the critical section object at constructor.
		\param cs The critical section object to use
		\param initialLock If it is true then the constructor immediately locks it.
		\~chinese
		在生成者可以立即lock critical section对象。
		\param cs 要使用的critical section对象
		\param initialLock true的话在生成者立即lock。
		\~japanese
		\~
		 */
		inline CriticalSectionLock(CriticalSection& cs, bool initialLock)
		{
			SetCriticalSection(cs, initialLock);
		}

		/**
		\~korean
		critical section을 나중에 세팅할때에 사용할 생성자입니다.
		\~english
		This is constructor that using set critical section later.
		\~chinese
		以后设置critical section的时候要使用的生成者。
		\~japanese
		\~
		*/
		inline CriticalSectionLock()
		{
			m_cs = NULL;
			m_recursionCount = 0;
		}
		inline void SetCriticalSection(CriticalSection &cs,bool initialLock)
		{
			m_cs = &cs;
			m_recursionCount = 0;
			if (initialLock)
			{
				Lock();
			}
		}
		// 쓰일 일이 없어 보이지만, 현재 스레드가 이 critsec을 잠그고 있는 상태인지 검사를 해야 할 때가 있어서
		// 이 함수는 존재함.
		inline CriticalSection* GetCriticalSection() const
		{
			return m_cs;
		}

		/**
		\~korean
		잠금이 되어있나?
		\~english
		Is it locked?
		\~chinese
		锁定了吗？
		\~japanese
		\~
		 */
		bool IsLocked() const
		{
			assert(m_recursionCount >= 0);
			return m_recursionCount > 0;
		}

		/**
		\~korean
		파괴자
		- 이미 이 객체가 점유하고 있던 CriticalSection이 있을 경우 점유 해제를 자동으로 한다.
		\~english
		Destructor
		- If there is a Critical Section occupied by this object then it automatically relieves.
		\~chinese
		破坏者
		- 此对象已经占有CriticalSection的时候自动解除。
		\~japanese
		\~
		 */
		inline ~CriticalSectionLock()
		{
			assert(m_recursionCount >= 0);
			for ( ; m_recursionCount > 0; --m_recursionCount )
			{
				m_cs->Unlock();
			}
		}

		/**
		\~korean
		critical section을 점유한다.
		\~english
		This occupies the critical section.
		\~chinese
		占有critical section。
		\~japanese
		\~
		 */
		inline void Lock()
		{
			m_cs->Lock();
			++m_recursionCount;
		}

		/**
		\~korean
		Try Lock을 수행한다.
		\return CriticalSection.TryLock()과 같은 값
		\~english
		Perform Try Lock.
		\return Same value as CriticalSection.TryLock()
		\~chinese
		执行Try Lock。
		\return 与CriticalSection.TryLock()一样的值。
		\~japanese
		\~
		 */
		inline bool TryLock()
		{
			bool r = m_cs->TryLock();
			if ( r )
			{
				++m_recursionCount;
			}
			return r;
		}

		// 내부적 용도로만
		inline void UnsafeLock()
		{
			m_cs->UnsafeLock();
			++m_recursionCount;
		}

		/**
		\~korean
		critical section을 점유 해제한다.
		\~english
		This relieves occupied critical section.
		\~chinese
		解除占有critical section。
		\~japanese
		\~
		 */
		inline void Unlock()
		{
			if ( IsLocked() )
			{
				--m_recursionCount;
				m_cs->Unlock();
			}
		}

		/**
		\~korean
		락이 중첩된 횟수를 조회한다.

		\~english

		\~chinese

		\~japanese
		\~
		*/
		inline int GetRecursionCount()
		{
			assert(m_recursionCount >= 0);
			return m_recursionCount;
		}

		NO_COPYABLE(CriticalSectionLock)
	};

	/**  @} */
#if (_MSC_VER>=1400)
#pragma managed(pop)
#endif
};
//#pragma pack(pop)
