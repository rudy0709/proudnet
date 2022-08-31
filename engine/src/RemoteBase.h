#pragma once

#include "FinalUserWorkItem.h"
#include "LeanType.h"
#include "../include/ListNode.h"
#include "UseCount.h"
#include <deque>

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
		PROUD_API CHostBase();
		PROUD_API virtual ~CHostBase();

		virtual HostID GetHostID() = 0;
		virtual LeanType GetLeanType() = 0;

		// owner 즉 main 객체의 critsec을 얻습니다.
		virtual CriticalSection& GetOwnerCriticalSection() = 0;

		// HostBase가 loopback 등, 파괴되지는 말아야 하는 경우에 이것이 사용된다. 냅두자.
		void ClearFinalUserWorkItemList() { m_UserTaskQueueUseOnly_finalUserWorkItemList.Clear(); }

		// stale이지만, 상관없다. 통계용이므로.
		int GetFinalUserWotkItemCount_STALE() { return m_UserTaskQueueUseOnly_finalUserWorkItemList.GetCount(); }
		bool CheckMessageOverloadAmount();

		PROUD_API bool MessageOverloadChecking(int64_t currentTime);

	private:
		bool m_overloadPacketState;
		int64_t m_overloadPacketTime;			// Packet이 overload 된 시간

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
			ErrorType m_reason;		// dispose를 하는 이유
			ErrorType m_detail;		// dispose를 할때 detail한 사유
			ByteArray m_comment;	// 클라에서 접속을 해제한 경우 마지막으로 날린 데이터를 의미한다.
			int64_t m_createdTime;	// 이 객체가 생성된 시간
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
	private:


		//////////////////////////////////////////////////////////////////////////
		// 주의: 하기 변수들은 CUserTaskQueue에 의해서만 액세스된다!

		// 이 host가 가진 final user work item 중 가장 오래된 것을 꺼낸다.
		inline bool UserTaskQueueUseOnly_PopFirstUserWorkItem(CFinalUserWorkItem &output)
		{
			// LTCG를 안 쓰는 경우를 위해 inline 화함.

			//AssertIsLockedByCurrentThread();

			if (m_UserTaskQueueUseOnly_finalUserWorkItemList.GetCount() == 0)
				return false;

			m_UserTaskQueueUseOnly_finalUserWorkItemList.RemoveHead(output);

			output.Internal().m_unsafeMessage.m_remoteHostID = GetHostID();

			return true;
		}


		// user message, user defined RMI, local event를 처리하기 위해 모으는 큐
		// 원래는 NC,NS,LC,LS의 remote 각각이 갖고 있던 변수였다. 거기서 이것을 참조하는 곳을 모두 찾아 여기로 옮기자.
		CFinalUserWorkItemList m_UserTaskQueueUseOnly_finalUserWorkItemList;

		// localhost 객체가 user task를 실행중인 동안만 true이다.
		// volatile이 아니더라도 괜찮다. 이 변수는 CUserTaskQueue 안에서 으로 보호된 상태에서 액세스되므로.
		enum  UserTaskQueueUseOnly_WorkState { InWorkingList, InWorkReadyList, NotInWorkList };
		UserTaskQueueUseOnly_WorkState m_UserTaskQueueUseOnly_WorkState;

		// valid only in working list state.
		Proud::Position m_UserTaskQueueUseOnly_iterInWorkingList;

	public:

		//////////////////////////////////////////////////////////////////////////
		// GetLastPing,GetRecentPing과 달리, 몇 초 안에 평균 레이턴시 RTT를 구하는 용도

		// 지금까지 총 측정된 RTT의 총합
		int64_t m_rttTotalTimeMs;
		// 지금까지 총 측정된 RTT 횟수
		int64_t m_rttTestCount;
		// RTT 핑을 보내는 주기
		int64_t m_rttIntervalMs;
		// 마지막으로 핑을 보낸 시간
		int64_t m_rttLastPingMs;
		// 측정을 종료할 시간
		int64_t m_rttEndTimeMs;
		// 사용자가 RTT 측정을 Start한 시간
		int64_t m_rttStartTimeMs;
		// 표준편차 계산을 위해서 모든 핑퐁 레이턴시를 담는다.
		std::deque<int64_t> m_latencies;
		// 현재 측정중인지?
		bool m_isTestingRtt;

		//////////////////////////////////////////////////////////////////////////
		//

		bool m_purgeRequested;

		// 최근 평균 RTT (round-trip latency)
		// 측정된 적이 있으면 1 이상의 값을, 그렇지 않으면 0을 갖는다.
		// Q: 실제 RTT가 <1인 경우는 어떻게 감지하나요? A: 그때 가서 ms or ns로 바꿉시다. RTT가 0이면 측정된 적 없음을 의미하는 것이 통상적입니다.
		int m_recentPingMs;

		//////////////////////////////////////////////////////////////////////////
		// coalesce 기능
		// 파생 클래스에서 설령 이 기능을 안쓰더라도,
		// 중복 코딩과 virtual function 난리법석을 피하는 super class 철학이 차라리 낫다.

		// coalesce interval. m_autoCoalesceInterval에 의해 값이 결정된다.
		int m_coalesceIntervalMs;

		// true이면 coalesce interval이 RTT에 근거해서 변경된다.
		bool m_autoCoalesceInterval;

		int GetCoalesceIntervalMs() { return m_coalesceIntervalMs; }
		PROUD_API void SetManualOrAutoCoalesceInterval(int interval = 0);

		// CRemoteClient_S와 CRemotePeer_C의 runtimePlatform
		RuntimePlatform m_runtimePlatform;
	};

	typedef CFastArray<shared_ptr<CHostBase>, true, false, int> ISendDestArray;

	class RemoteArray : public CFastArray<shared_ptr<CHostBase>, true, false, int> {};

#if 0
	// ndk c++11의 경우 std::shared_ptr::operator==, !=, <, <=, >, >=이 내장되어 있지 않기 때문에
	// shared_ptr<CHostBase>에 대해서 operator==, !=, <, <=, >, >= 선언 & 정의합니다.

	// ==>확실히 내장 안되어 있는지요? 어쩌면, 존재하지만, 뭔가 빠진 것이 아닐까요? 재체크합시다.
	// 아래 구현은 있어도 문제입니다. 같은 역할의 함수가 중복되기 때문에, shared_ptr implementation에 따라 side effect가 있을 수 있습니다. - 사장

	inline bool operator== (const shared_ptr<CHostBase>& a, const shared_ptr<CHostBase>& b)
	{
		return a.get() == b.get();
	}

	inline bool operator!= (const shared_ptr<CHostBase>& a, const shared_ptr<CHostBase>& b)
	{
		return a.get() != b.get();
	}

	inline bool operator< (const shared_ptr<CHostBase>& a, const shared_ptr<CHostBase>& b)
	{
		return a.get() < b.get();
	}

	inline bool operator> (const shared_ptr<CHostBase>& a, const shared_ptr<CHostBase>& b)
	{
		return a.get() > b.get();
	}

	inline bool operator>= (const shared_ptr<CHostBase>& a, const shared_ptr<CHostBase>& b)
	{
		return a.get() >= b.get();
	}

	inline bool operator<= (const shared_ptr<CHostBase>& a, const shared_ptr<CHostBase>& b)
	{
		return a.get() <= b.get();
	}
#endif

	template <typename T>
	inline bool operator== (const shared_ptr<T>& a, const shared_ptr<T>& b)
	{
		return a.get() == b.get();
	}

	template <typename T>
	inline bool operator!= (const shared_ptr<T>& a, const shared_ptr<T>& b)
	{
		return a.get() != b.get();
	}

	template <typename T>
	inline bool operator< (const shared_ptr<T>& a, const shared_ptr<T>& b)
	{
		return a.get() < b.get();
	}

	template <typename T>
	inline bool operator> (const shared_ptr<T>& a, const shared_ptr<T>& b)
	{
		return a.get() > b.get();
	}

	template <typename T>
	inline bool operator>= (const shared_ptr<T>& a, const shared_ptr<T>& b)
	{
		return a.get() >= b.get();
	}

	template <typename T>
	inline bool operator<= (const shared_ptr<T>& a, const shared_ptr<T>& b)
	{
		return a.get() <= b.get();
	}
}
