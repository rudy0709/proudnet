/*
ProudNet v1.7


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
#include "PNString.h"
#include "Singleton.h"

//#pragma pack(push,8)

#if (_MSC_VER>=1400)
#pragma managed(push, off)
#endif

#ifdef _WIN32 
#pragma comment(lib, "Shlwapi.lib") // PathCombine() 등을 쓰므로.
#endif
	   
namespace Proud
{
	   
	/** \addtogroup util_group
	*  @{
	*/

	/** 
	\~korean 
	CPU갯수를 구한다. 	
	
	\~englich TODO:translate needed.

	\~chinese
	求CPU个数。

	\~japanese
	\~
	*/
	PROUD_API uint32_t GetNoofProcessors();
	
	/** 
	\~korean 
	프로세스의 이름을 얻어온다.	
	
	\~english TODO:translate needed.

	\~chinese
	获取区域名。

	\~japanese
	\~
	*/
	PROUD_API String GetProcessName();
	
	
	/** 
	 \~korean 
	 ProudNet 사용자가 실수한 경우에만 보여주는 에러다. 
	 - CNetConfig::UserMisuseErrorReaction 의 type에 따라서 MessageBox혹은 디버그뷰로 확인 할 수 있는 에러를 출력해준다. 
	 \param text Error Comment

	 \~english TODO:translate needed.

	 \~chinese
	 ProudNet用户失误的时候显示的错误。
	 - 根据 CNetConfig::UserMisuseErrorReaction%的type，打印用MessageBox或者调试view可以确认的错误。
	 \param text Error Comment

	 \~japanese	 
	 \~
	 */
	PROUD_API void ShowUserMisuseError(const PNTCHAR* text);


	/** 
	\~korean 
	debug output or console에 디버깅용 문구를 출력합니다. newline은 추가되지 않습니다.

	\~english 
	Prints text for debugging to debugger output window or console, without newline.

	\~chinese
	在debug output or console中输出调试用语句。不会添加newline。

	\~japanese
	\~
	*/
	void TraceA(const char * format, ...);


#ifdef _WIN32
	/** 
	\~korean 
	물리적 메모리 점유율을 구한다. 
	
	\~english TODO:translate needed.
	
	\~chinese
	求物理内存占有率。

	\~japanese
	\~
	*/
	PROUD_API size_t GetOccupiedPhysicalMemory();

	/** 
	\~korean 
	총 메모리를 구한다. 	
	
	\~english TODO:translate needed.	
	
	\~chinese
	求总内存。

	\~japanese
	\~
	*/
	PROUD_API size_t GetTotalPhysicalMemory();

	/** 
	\~korean 
	컴퓨터 이름을 구한다.	
	
	\~english TODO:translate needed.	
	
	\~chinese
	求电脑名称。

	\~japanese
	\~
	*/
	PROUD_API String GetComputerName();
	
	/** 
	\~korean
	Windows XP, 2003에서만 지원하는 기능으로 low-fragmentation heap을 쓴다.
	- 이걸 쓰면 SMP 환경에서 heap 접근 속도가 대폭 향상된다. 물론 low-fragmentation의 이익도 얻는다.
	따라서 가능하면 이것을 쓰는 것이 강력 권장된다. 
	- 이 메서드는 Windows 98 등에서는 지원되지 않는데, 이런 경우 이 메서드는 아무 것도 하지 않는다.
	하지만 Windows 98에서 필요 Win32 API를 못 찾아 에러가 발생하지는 않는다.

	\~english TODO:translate needed.

	\~chinese
	是只在Windows XP, 2003支持的功能，使用low-fragmentation heap。
	- 使用这个的话在SMP环境下接近heap速度会大幅度提高。随之也会得到low-fragmentation 利益。所以可以的话强力推荐使用这个。
	- 此方法在Windows 98不支持，这时候此方法不会做任何事情。但是不会因Windows 98没有找到必要Win32 API而发生错误。

	\~japanese
	\~
	*/
	PROUD_API bool EnableLowFragmentationHeap();
	
	/**
	\~korean
	이 프로그램이 실행되고 있는 폴더를 찾아서 current directory로 세팅한다.
	- 이렇게 하는 경우 다른 디렉토리에서 이 프로그램을 실행시켜도 media file을 제대로 찾는다.

	\~english TODO:translate needed.

	\~chinese
	找到此程序正在实行的文件后设置为current directory。
	- 这样的话，在其他directory运行此程序也会正确找到media file。

	\~japanese
	\~
	*/
	PROUD_API void SetCurrentDirectoryWhereProgramExists();

	/**
	\~korean 
	현재 Process가 Service Mode로 실행중인지 여부를 검사합니다.
	\return 서비스모드면 1 아니면 0

	\~english TODO:translate needed.

	\~chinese
	检查现在的Process是否以Service Mode实行。
	\return 是服务模式的话是1不是的话0。

	\~japanese
	\~
	*/
	PROUD_API bool IsServiceMode();

	/** 
	\~korean
	GetQueuedCompletionStatusEx 사용 가능 여부를 검사한다. Server2008이상만 사용할수 있다고 알려져 있다.
	\return 사용가능하면 1 아니면 0

	\~english TODO:translate needed.

	\~chinese
	检查GetQueuedCompletionStatusEx是否可以使用。据说只在Server2008以上可以使用。
	\return 可以使用的话1，不是的话0。

	\~japanese
	\~
	*/
	PROUD_API bool IsGqcsExSupported();

	/** 
	\~korean 
	HyperThreading를 지원하는지를 검사한다.	
	
	\~english TODO:translate needed.	
	
	\~chinese
	检查是否支持HyperThreading。

	\~japanese
	\~
	*/
	PROUD_API bool IsHyperThreading();
	
	/** 
	\~korean
	비트가 1인 수를 체크합니다. 
	\param bitMask 비트를 검사할 변수 
	\return 비트가 1인 수 	
	
	\~english TODO:translate needed.

	\~chinese
	打勾bit为1的数。
	\param bitmask 检查bit的变数。
	\return bit为1的数。

	\~japanese
	\~
	*/
	PROUD_API uint32_t CountSetBits(uintptr_t bitMask);

	typedef WINBASEAPI
		/*__out_opt*/
		HANDLE
		(WINAPI* CreateIoCompletionPortProc)(
		/*__in*/     HANDLE FileHandle,
		/*__in_opt*/ HANDLE ExistingCompletionPort,
		/*__in*/     ULONG_PTR CompletionKey,
		/*__in*/     uint32_t NumberOfConcurrentThreads
		);

	typedef WINBASEAPI
		BOOL
		(WINAPI* HeapSetInformationProc) (
		/*__in_opt*/ HANDLE HeapHandle,
		/*__in*/ HEAP_INFORMATION_CLASS HeapInformationClass,
		/*__in_bcount_opt HeapInformationLength*/ PVOID HeapInformation,
		/*__in*/ size_t HeapInformationLength
		);

	typedef WINBASEAPI
		BOOL
		(WINAPI* QueueUserWorkItemProc) (
		/*__in*/     LPTHREAD_START_ROUTINE Function,
		/*__in_opt*/ PVOID Context,
		/*__in*/     uint32_t Flags
		);

	typedef WINBASEAPI
		BOOL
		(WINAPI* GetQueuedCompletionStatusProc) (
		/*__in*/  HANDLE CompletionPort,
		/*__out*/ LPDWORD lpNumberOfBytesTransferred,
		/*__out*/ PULONG_PTR lpCompletionKey,
		/*__out*/ LPOVERLAPPED *lpOverlapped,
		/*__in*/  uint32_t dwMilliseconds
		);

	// win32 api에서 복사해온 것임
	struct PN_OVERLAPPED_ENTRY {
		ULONG_PTR lpCompletionKey;
		LPOVERLAPPED lpOverlapped;
		ULONG_PTR Internal;
		DWORD dwNumberOfBytesTransferred;
	};

	typedef WINBASEAPI
		BOOL
		(WINAPI* GetQueuedCompletionStatusExProc) (
		/*__in  */HANDLE CompletionPort,
		/*__out_ecount_part(ulCount, *ulNumEntriesRemoved)*/ PN_OVERLAPPED_ENTRY* lpCompletionPortEntries,
		/*__in  */uint32_t ulCount,
		/*__out */PULONG ulNumEntriesRemoved,
		/*__in  */uint32_t dwMilliseconds,
		/*__in  */BOOL fAlertable
		);

	typedef WINBASEAPI
		PVOID
		(WINAPI* AddVectoredExceptionHandlerProc) (
		/*__in*/          uint32_t FirstHandler,
		/*__in*/          PVECTORED_EXCEPTION_HANDLER VectoredHandler
		);

	typedef WINBASEAPI
		BOOL
		(WINAPI* GetLogicalProcessorInformation)(
		/*__out */	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION Buffer,
		/*))inout */ PDWORD ReturnLength
		);

	/** 
	\~korean 
	파일의 절대 경로 이름을 구한다. 이미 절대 경로 이름이면 그대로 리턴한다. 	
	
	\~english TODO:translate needed.

	\~chinese
	求文件的绝对路径名称。已经是绝对路径名称的话原样返回。

	\~japanese
	\~
	*/
	PROUD_API String GetFullPath(const TCHAR* fileName);

	/** 
	\~korean 
	디렉토리를 생성하고 풀패스를 구한다. 	
	
	\~english TODO:translate needed.

	\~chinese
	生成directory，求fullpath。

	\~japanese
	\~
	*/
	PROUD_API bool CreateFullDirectory(const TCHAR* lpPathName, String& outCreatedDirectory);

	/** 
	\~korean 
	현재 디렉토리를 구한다. 
	
	\~english TODO:translate needed.

	\~chinese
	求现在directory。

	\~japanese
	\~
	*/
	PROUD_API String GetCurrentDirectory();

	/** 
	\~korean
	타이머 유저 콜백 함수
	- 타이머 콜백 또는 등록된 대기 콜백을 위한 시작 주소를 제공하는 어플리케이션 정의 함수 이다.
	이 주소는 CreateTimerQueueTimer와 RegisterWaitForSingleObject 함수로 지정 한다.
	
	WaitOrTimerCallbackProc 타입은 콜백 함수로의 포인터를 정의한다. WaitOrTimerCallbackProc은 어플리케이션 정의 함수 이름을 위한 플레이스홀더 이다.

	\param [in] lpParameter 스레드 데이터는 CreateTimerQueueTimer 및 RegisterWaitForSingleObject 함수의 파라미터를 사용하는 함수로 보내진다.
	\param TimerOrWaitFired 만약 이 파라미터가 TRUE 라면 대기시같이 끝난다. 만약 이 파라미터가 FALSE 라면 대기 이벤트는 신호를 발생시킨다. (이 파라미터는 타이머 콜백을 위해 항상 TRUE 이다.)

	\~english
	Timer user callback function
	- An application-defined function that serves as the starting address for a timer callback or a registered wait callback. 
	Specify this address when calling the CreateTimerQueueTimer, RegisterWaitForSingleObject function.

	The WaitOrTimerCallbackProc type defines a pointer to this callback function. WaitOrTimerCallback is a placeholder for the application-defined function name.

	\param [in] lpParameter The thread data passed to the function using a parameter of the CreateTimerQueueTimer or RegisterWaitForSingleObject function.
	\param TimerOrWaitFired If this parameter is TRUE, the wait timed out. If this parameter is FALSE, the wait event has been signaled. (This parameter is always TRUE for timer callbacks.)

	\~chinese
	Timer 用户回调函数
	- 为了timer回拨或者提供已登录的等待回拨的开始地址的应用定义函数。
	此地址是由CreateTimerQueueTimer和RegisterWaitForSingleObject函数指定。

	WaitOrTimerCallbackProc 类型定义以回拨函数的指针。 WaitOrTimerCallbackProc 是为了应用定义函数名称的占位符。

	\param [in] lpParameter 线程数据发往使用CreateTimerQueueTimer及RegisterWaitForSingleObject函数参数的函数。
	\param TimerOrWaitFired 如果此参数是TRUE的话等待时一起结束。如果此参数是FALSE的话，等待event会发生信号。（此参数为了timer回拨一直是TRUE。）

	\~japanese
	\~
	*/
	typedef VOID (NTAPI * WaitOrTimerCallbackProc) (PVOID lpParameter , BOOLEAN TimerOrWaitFired );   

	typedef WINBASEAPI
		HANDLE
		(WINAPI* CreateTimerQueueProc) (
		VOID
		);

	typedef WINBASEAPI
		BOOL
		(WINAPI* CreateTimerQueueTimerProc ) (
		/*__deref_out*/ PHANDLE phNewTimer,
		/*__in_opt   */ HANDLE TimerQueue,
		/*__in       */ WaitOrTimerCallbackProc Callback,
		/*__in_opt   */ PVOID Parameter,
		/*__in       */ uint32_t DueTime,
		/*__in       */ uint32_t Period,
		/*__in       */ uint32_t Flags
		) ;


	typedef WINBASEAPI
		BOOL
		(WINAPI* DeleteTimerQueueTimerProc) (
		/*__in_opt*/ HANDLE TimerQueue,
		/*__in    */ HANDLE Timer,
		/*__in_opt*/ HANDLE CompletionEvent
		);

	typedef WINBASEAPI
		BOOL
		(WINAPI* DeleteTimerQueueExProc) (
		/*__in    */ HANDLE TimerQueue,
		/*__in_opt*/ HANDLE CompletionEvent
		);

	typedef WINBASEAPI
		BOOL
		(WINAPI* InitializeCriticalSectionExProc) (
		/*__out*/ LPCRITICAL_SECTION lpCriticalSection,
		/*__in*/  uint32_t dwSpinCount,
		/*__in  */uint32_t Flags
		);

	/** 
	\~korean
	Windows NT 전용 API를 동적으로 얻어오는 클래스
	- Windows 98에서는 비록 호출은 못하지만 아예 해당 API가 없어서 exe 실행 조차도 못하는 문제는 해결할 필요가
	있을때 이 클래스가 유용하다. 
	- 평소에는 쓸 필요가 없다. 다만, Windows 98에서도 작동해야 하는 모듈을 만든다면 유용하다. 

	\~english
	Class that gets Windows NT custom API as dynamic
	- Though it cannot be called by Windows 98 but this can be very useful sovling troubles that even exe cannot be run. 
	- Does not need to be used normally. But useful when creating a module that is to run on Windows 98.

	\~chinese
	把Windows NT专用API以动态获取的类。
	- 虽然在Windows 98不能呼叫，但需要解决因没有对应的API而根本无法实行exe的问题时，此类会有效果。
	- 平时没有必要使用。但是制造在Windows 98也能运转的模块时会有用。 

	\~japanese
	\~
	*/
	class CKernel32Api/*:protected CSingleton<CKernel32Api> 싱글톤 즐. 어차피 파괴자 콜이 없으므로. 게다가 CSingleton은 미래 업데이트에 따라 느려질 수 있으므로. */
	{
		//modify by rekfkno1 - 이렇게 멤버변수로 두는것은 생성자 호출이 늦는경우가 있더라.
		//그래서 static 함수내의 static 로컬변수로 싱글턴을 하는것이 좋을 듯하다.
		//static CKernel32Api g_instance;
	public:
		CKernel32Api();
	
		PROUD_API static CKernel32Api& Instance();

		// 함수 포인터 멤버 변수들
		HeapSetInformationProc HeapSetInformation;
		GetQueuedCompletionStatusExProc GetQueuedCompletionStatusEx;  // windows 2008 server 이후부터 쓰는 기능

		InitializeCriticalSectionExProc InitializeCriticalSectionEx;
		GetLogicalProcessorInformation GetLogicalProcessInformation;

		HINSTANCE m_dllHandle;               // Handle to DLL
	};

	class CLocale:protected CSingleton<CLocale>
	{
	public:
		int m_languageID;

		CLocale();

		PROUD_API static CLocale& Instance();
	};

	class CSystemInfo:protected CSingleton<CSystemInfo>
	{
	public:
		SYSTEM_INFO m_info;

		CSystemInfo();
		inline uint32_t GetMemoryPageSize() { return m_info.dwPageSize; }

		PROUD_API static CSystemInfo& Instance();
	};

	/**
	\~korean
	현재 프로세스에서 사용되고 있는 총 스레드의 수를 얻어온다.
	
	\~english TODO:translate needed.
	
	\~chinese
	获得现在使用中的总线程数。

	\~japanese
	\~
	*/
	PROUD_API int GetTotalThreadCount();
	
#endif 

	/**  @} */

#ifdef _MSC_VER
	#if !defined(_NO_NTTNTRACE) && (defined(_DEBUG)||defined(IW_DEBUG))
		#define NTTNTRACE Proud::TraceA
	#else
		#define NTTNTRACE __noop // 비 VC++에서는 이것을 사용 못하므로 아래 것이 사용됨
	#endif
#else
	#if !defined(_NO_NTTNTRACE) && (defined(_DEBUG)||defined(IW_DEBUG))
		#define NTTNTRACE(...) Proud::TraceA(__VA_ARGS__)
	#else
		#define NTTNTRACE(...) //VC++에서는 C4390 경고가 나옴. 그래서 위의 것이 사용됨.
	#endif
#endif 

	// _MAX_PATH는 260밖에 안된다. 요즘 세대에서 이것은 너무 작다. 이것을 쓰자.
	const int LongMaxPath = 4000; 

	// non-win32에 없는 것들을 유사 형태로 타 플랫폼을 위해 구현
	// 일부러 namespace Proud 안에 두었다. 없는 것을 global namespace에 구현하는 경우 타사가 구현한 것과 충돌할 수 있으니.
	// 게다가 이 클래스의 멤버로 둔 이유: 안그랬더니 ATL을 추가하는 경우 IsDebuggerPresent 함수를 쓰는 곳에서 컴파일 에러가 발생해서.
	class CFakeWin32
	{
	public:
		PROUD_API static unsigned int GetTickCount();
		PROUD_API static bool IsDebuggerPresent();
		PROUD_API static void OutputDebugStringA(const char *text);
		PROUD_API static void OutputDebugStringW(const wchar_t *text);
	};

#ifdef _UNICODE
#define OutputDebugStringT OutputDebugStringW
#else
#define OutputDebugStringT OutputDebugStringA
#endif


#ifdef _WIN32
	String GetMessageTextFromWin32Error(uint32_t errorCode);
#endif // _WIN32

#ifndef _WIN32
	bool fd_is_valid(int fd);
#endif

}

#if (_MSC_VER>=1400)
#pragma managed(pop)
#endif

//#pragma pack(pop)

