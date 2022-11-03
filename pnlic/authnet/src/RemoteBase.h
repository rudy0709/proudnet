#pragma once 

#include "FinalUserWorkItem.h"
#include "LeanType.h"
#include "../include/ListNode.h"
#include "UseCount.h"
//#include "../UtilSrc/DebugUtil/NetworkAnalyze.h"

namespace Proud
{
	class CUserTaskQueue;

	/* 모든 remote 및 local host를 가리키는 클래스들의 베이스 클래스.
	NetCore와의 밀접한 관계를 가진다.
	
	remote client, peer 등의 공통 기능을 모조리 가지는 슈퍼클래스다. 
	(과거에는 program to interface였으나 이것이 오히려 중복코드가 많아, 
	차라리 중복코드를 모두 모아 한 클래스로 가는 것이 유지보수가 더 편하다는 것이 밝혀졌음.)
	http://www.slideshare.net/noerror/ndc11-8209615 

	hostID를 식별할 수 있어야 한다.
	user task queue 기능을 내장한다.
	tag를 가지고 있다.
	*/
	class CHostBase
	{
		friend class CUserTaskQueue;
	public:
		class CNode :public CListNode < CNode >
		{
		public:
			CHostBase* m_owner;
			CNode(CHostBase* owner)
			{
				m_owner = owner;
			}
		};
	private:
		// CUserTaskQueue에 자기가 등록될 때 이것이 등록된다.
		// CListNode의 특성상 ITaskSubject가 CListNode 자체가 될 수 없기에 이런 꼼수를 썼다.
		CNode m_taskSubjectNode;

	public:
		/* 초기값은 0이다. 
		garbaged가 되면 -1 된다. 0이 되면 safe to delete다. 
    garbage collector에서는 0일때만 검사하는데, stale이더라도 항상 >0이므로 문제 없다.
		main unlock 상태에서 이것이 비파괴 보장하고자 하면 IncreaseUseCount()를 하면 된다.
		그러면 항상 >0인 상태가 된다.
    단, increase를 하기 전에는 반드시 main lock이 된 상태이어야 한다.    

		Q: 이게 있어야 해요?
		A: main unlock 상태에서 this가 파괴되지 않게 보장해야 하는 일부 루틴이 있기 때문이다. */
		volatile int m_useCount;

		CHostBase();
		virtual ~CHostBase();
		virtual HostID GetHostID() = 0;
		virtual LeanType GetLeanType() = 0;
		
		// 이거 없으면 P2P leave event를 제대로 처리 못함. LanRemotePeer에 대해서도 마찬가지.
		// 오버라이드 가능. 오버라이드하면 base function call을 잊지 말자.
		virtual bool IsDisposeSafe()
		{
			// 다른 스레드에서 사용중이지 않아야 한다.
			// 얻는 값이 stale이더라도 괜찮다. 
			// garbage 선언되기 전에는 항상 >0이기 때문에 stale이 문제되려면 0인 때가 과거에 있어야 하는데 그런 떄가 없다.

 			if (m_useCount > 0)
 				return false;
			return true;
		}

		bool IsTaskRunning();

		void OnSetTaskRunningFlag(bool val);
		bool PopFirstUserWorkItem(CFinalUserWorkItem &output);
	
		bool IsInUserTaskQueue();
		bool IsFinalReceiveQueueEmpty();

		void ClearFinalUserWorkItemList() { m_finalUserWorkItemList.Clear(); }
		int GetFinalUserWotkItemCount() { return m_finalUserWorkItemList.GetCount(); }
		bool CheckMessageOverloadAmount();

		bool MessageOverloadChecking(int64_t currentTime);

	private:
		bool m_overloadPacketState;
		int64_t m_overloadPacketTime;             // Packet이 overload 된 시간

		// 디버깅용.
		AddrPort m_tcpAddrPort_debug;
	public:
		// 이 remote 객체의 hostID
		HostID m_HostID;
		
		// 이 host 객체가 가지고 있는 사용자 정의 데이터
		void* m_hostTag;

		// 이 Host가 garbaged 상태이면 true이다.
		// 대부분의 경우, garbaged 상태인 경우 수신이나 이벤트 처리를 하지 않는다.
		bool m_garbaged;

		class CDisposeWaiter
		{
		public:
			ErrorType m_reason;  // dispose를 하는 이유
			ErrorType m_detail;	 // dispose를 할때 detail한 사유
			ByteArray m_comment;		// 클라에서 접속을 해제한 경우 마지막으로 날린 데이터를 의미한다.
			int64_t m_createdTime;    // 이 객체가 생성된 시간
			SocketErrorCode m_socketErrorCode; // 문제 발생시 추적을 위함

			inline CDisposeWaiter()
			{
				m_reason = ErrorType_Ok;
				m_socketErrorCode = SocketErrorCode_Ok;
				m_createdTime = 0;
			}
		};

		// remote가 왜 나가졌는지에 대한 기록을 담는 구조체
		CHeldPtr<CDisposeWaiter> m_disposeWaiter; // 평소에는 null이지만 일단 세팅되면 모든 스레드에서 종료할 때까지 기다린다.

		// user message, user defined RMI, local event를 처리하기 위해 모으는 큐
		// 원래는 NC,NS,LC,LS의 remote 각각이 갖고 있던 변수였다. 거기서 이것을 참조하는 곳을 모두 찾아 여기로 옮기자.
		CFinalUserWorkItemList m_finalUserWorkItemList;

		// localhost 객체가 user task를 실행중인 동안만 true이다.
		// volatile이 아니더라도 괜찮다. 이 변수는 CUserTaskQueue 안에서 으로 보호된 상태에서 액세스되므로.
		bool m_userTaskRunning;

		bool m_purgeRequested;

		// 최근 평균 RTT (round-trip latency)
		// 측정된 적이 있으면 1 이상의 값을, 그렇지 않으면 0을 갖는다.
		// Q: 실제 RTT가 <1인 경우는 어떻게 감지하나요? A: 그때 가서 ms or ns로 바꿉시다. RTT가 0이면 측정된 적 없음을 의미하는 것이 통상적입니다.
		int m_recentPingMs;

/*
		// 3003 버그 잡기 위해!
		MessageAnalyzeLog m_MessageAnalyzeLog;
*/



		//////////////////////////////////////////////////////////////////////////
		// coalesce 기능
		// 파생 클래스에서 설령 이 기능을 안쓰더라도, 
		// 중복 코딩과 virtual function 난리법석을 피하는 super class 철학이 차라리 낫다.

		// coalesce interval. m_autoCoalesceInterval에 의해 값이 결정된다.
		int m_coalesceIntervalMs;

		// true이면 coalesce interval이 RTT에 근거해서 변경된다.
		bool m_autoCoalesceInterval;

		int GetCoalesceIntervalMs() { return m_coalesceIntervalMs; }
		void SetManualOrAutoCoalesceInterval(int interval = 0);
	};

	typedef CFastArray<CHostBase*, false, true, int> ISendDestArray;

}