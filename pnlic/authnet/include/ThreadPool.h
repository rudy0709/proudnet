/*
ProudNet v1.x.x


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

//#pragma pack(push,8)

namespace Proud
{
	/** \addtogroup net_group
	*  @{
	*/

	/**
	\~korean
	CThreadPool 에서 발생하는 이벤트 콜백을 받아 처리하는 인터페이스 클래스입니다.

	자세한 것은 \ref thread_pool_sharing 을 참고하십시오.

	사용법
	- 객체를 상속받아 메서드를 구현합니다.
	- CThreadPool 에 이 객체를 파라메터로 전달합니다.

	\~english
	This is interface class that hand event callback occured from CThreadPool.

	Please refer to \ref thread_pool_sharing for more detail.

	Usage
	- Realize this method to inherit object.
	- Pass this object to parameter at CThreadPool

	\~chinese
	接收并处理从 CThreadPool%发生的event回调的界面类。

	详细的请参考\ref thread_pool_sharing%。

	使用方法
	- 继承对象以后体现此方法。
	- 把此对象为参数传送给 CThreadPool%。

	\~japanese
	\~
	*/
	class IThreadPoolEvent
	{
	public:
		/**
		\~korean
		thread pool의 스레드가 시작할 때 이 메서드가 호출됩니다.

		\~english
		When thread pool started thread, call this method.

		\~chinese
		thread pool 线程开始的时候呼叫此方法。

		\~japanese
		\~
		*/
		virtual void OnThreadBegin() = 0;

		/**
		\~korean
		thread pool의 스레드가 종료할 때 이 메서드가 호출됩니다.

		\~english
		When thread pool is terminated, call this method.

		\~chinese
		thread pool 线程结束的时候呼叫此方法。

		\~japanese
		\~
		*/
		virtual void OnThreadEnd() = 0;
	};


	/**
	\~korean
	서버사이드 호스트 모듈(CNetServer,CLanServer,CLanClient)을 위한 thread pool 객체입니다.

	자세한 것은 \ref thread_pool_sharing 을 참고하십시오.

	\~english
	This is thread pool object for server-sided host module(CNetServer,CLanServer,CLanClient)

	Please refer to \ref thread_pool_sharing for more detail.

	\~chinese
	为了server-sided 主机模型（CNetServer,CLanServer,CLanClient）的thread pool对象。

	详细请参考\ref thread_pool_sharing%。

	\~japanese
	\~

	사용법
	- tp = CThreadPool.Create();
	- tp.SetEventSink(...); // (optinal)
	- tp.SetThreadCount(n); // n = 0~512
	- NC,NS,LC,LS에서 tp를 변수로 참조
	- delete tp;

	TODO: 상기 주석 다듬은 후 다국어화해야!

	*/
	class CThreadPool
	{
	public:
		virtual ~CThreadPool();

		// 원하는 수의 스레드로 갯수를 조절한다.
		// 0~512까지 가능.
		// 이 함수는 즉시 리턴되고, 시간이 지나면서 목표치로 가게 된다.
		// 0이 될 경우 사용자는 CThreadPool.Process를 매번 호출해 주어야 한다.
		virtual void SetDesiredThreadCount(int threadCount) = 0;

		/**
		\~korean
		네트워킹 서버간 통신용 ThreadPool을 생성합니다.

		\~english
		Creates ThreadPool for server communications

		\~chinese
		生成网络服务器之间通信用的ThreadPool。

		\~japanese
		\~
		*/
		static CThreadPool *Create(IThreadPoolEvent* eventSink, int threadCount);

		// zero thread pool을 위한, heartbeat 처리를 한다.
		// zero thread pool인 경우 지속적으로 호출해야 한다.
		// 인자: 최대 대기 시간. 0을 넣으면 무대기 실행.
		// TODO: 위 주석을 정제하여 사용자가 좋아하게 하고, zero thread pool에 대한 피처도 advanced usage 항목에 추가하도록 하자.
		virtual void Process(int timeoutMs) = 0;

	};

	/**  @} */
}

//#pragma pack(pop)
