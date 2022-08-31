/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#pragma once

#include "../include/FakeClr.h"
#include "../include/Enums.h"
#include "../include/AddrPort.h"
#include "../include/Ptr.h"
#include "../include/BasicTypes.h"
#include "../include/ListNode.h"
#include "../include/Singleton.h"
#include "../include/ReceivedMessage.h"
#include "../include/PNSemaphore.h"
#include "enumimpl.h"
//#include "IOCP.h"
//#include "FinalUserWorkItem.h"
#include "ReportError.h"
#include "FastArrayImpl.h"
#include "PtrImpl.h"
#include "Upnp.h"
#include "FastArayPtrImpl.h"
//#include "GlobalTimerThread.h"
#include "IntraTracer.h"
//#include "ThreadPoolImpl.h"
#include "FinalUserWorkItem.h"

#if !defined(_WIN32)
	class CIoReactorEventNotifier;
#endif

namespace Proud
{
	class CTracer;
	class CNetClientImpl;
	class OverlappedResult;
	class CReceivedMessageList;
	class CReceivedMessage;
	class CMessage;
	class CUdpSocket_C;
	class CSuperSocket;

	// Transit state를 할 때마다 초기화되는 변수들의 집합.
	// 언젠가는 CNetClientImpl에 병합하는게 더 나을지도.
	class CNetClientWorker:public CListNode<CNetClientWorker>
	{
	public:
		int64_t m_issueConnectStartTimeMs;	// issue connect를 처음 시작한 시간. 연결 중일때만 유효한 값이다.

		volatile int m_DisconnectingModeHeartbeatCount;
		volatile int64_t m_DisconnectingModeStartTime;
		volatile bool m_DisconnectingModeWarned;
	private:
		void WarnTooLongElapsedTime();

	public:
		void Reset();

		// 서버와의 연결 해제 처리를 할 수 있는 최대 시간.
		// 이 시간이 넘으면 서버에서는 연결 해제를 즉시 인식하지 못하지만 클라이언트는 어쨌거나 나가게 된다.
		int64_t m_gracefulDisconnectTimeout;

		enum State
		{
			// 아래enum들은 순서 지켜야!
			IssueConnect,	// connect함수를 막 호출해야 하는 상황
			Connecting,		// 연결 완료를 기다리는 중
			JustConnected,	// 연결 완료가 막 된 첫 1회
			Connected,		// 연결 되어 있는 상태. 즉 통상 통신하고 있는 상황. 가장 오랜 상태가 이 상태이다.
			Disconnecting,	// 연결 해제를 걸어 놓고 모든 i/o 처리 등이 끝날 때까지 기다리는 중
			Disconnected,	// 연결 해제가 모두 되어서 NetClient를 delete해도 되는 상황
		};
	private:
		// 현재 상태 값.
		// 이 변수를 직접 쓰지 말고 Get/SetState 함수를 쓸 것.
		volatile State m_state_USE_FUNC;
	public:
		void SetState(State newVal);
		State GetState() { return m_state_USE_FUNC; }

		CNetClientImpl *m_owner;

		CNetClientWorker(CNetClientImpl *owner);
		~CNetClientWorker();

		bool ProcessMessage_S2CRoutedMulticast1(
			const shared_ptr<CSuperSocket>& udpSocket,
			CReceivedMessage& ri);
		bool ProcessMessage_S2CRoutedMulticast2(
			const shared_ptr<CSuperSocket>& udpSocket,
			CReceivedMessage& ri);
		bool ProcessMessage_ProudNetLayer(
			const shared_ptr<CSuperSocket>& socket,
			CReceivedMessage& receivedInfo);

		void ProcessMessage_Rmi(CReceivedMessage& receivedInfo, bool &refMessageProcessed);
		void ProcessMessage_UserOrHlaMessage(CReceivedMessage& receivedInfo, FinalUserWorkItemType UWIType, bool &refMessageProcessed);
		bool IsFromRemoteClientPeer(CReceivedMessage& receivedInfo);
		void ProcessMessage_PeerUdp_PeerHolepunch(CReceivedMessage& ri);
		void ProcessMessage_PeerHolepunchAck(CReceivedMessage& ri);
		void ProcessMessage_P2PUnreliablePing(const shared_ptr<CSuperSocket>& socket, CReceivedMessage& ri);
		void ProcessMessage_P2PUnreliablePong(CReceivedMessage& ri);
		void ProcessMessage_ReliableUdp_Frame(
			const shared_ptr<CSuperSocket>& udpSocket,
			CReceivedMessage& ri);
		void ProcessMessage_NotifyClientServerUdpMatched(CMessage &msg);
		void ProcessMessage_UnreliablePong(CMessage &msg);
		void ProcessMessage_ServerHolepunchAck(CReceivedMessage& ri);
		void ProcessMessage_PeerUdp_ServerHolepunchAck(CReceivedMessage& ri);
		void ProcessMessage_RequestStartServerHolepunch(CMessage &msg);
		void ProcessMessage_ConnectServerTimedout(CMessage &msg);
		void ProcessMessage_NotifyStartupEnvironment(const shared_ptr<CSuperSocket>& socket, CMessage &msg);
		void ProcessMessage_NotifyServerConnectSuccess(CMessage &msg);
		void ProcessMessage_NotifyProtocolVersionMismatch(CMessage &msg);
		void ProcessMessage_NotifyServerDeniedConnection(CMessage &msg);
		void ProcessMessage_ReliableRelay2(
			const shared_ptr<CSuperSocket>& socket,
			CMessage &msg);
		void ProcessMessage_UnreliableRelay2(
			const shared_ptr<CSuperSocket>& socket,
			CReceivedMessage& receivedInfo);
		void ProcessMessage_LingerDataFrame2(const shared_ptr<CSuperSocket>& udpSocket, CReceivedMessage& rm);
		void ProcessMessage_P2PReliablePing(CReceivedMessage& ri);
		void ProcessMessage_P2PReliablePong(CReceivedMessage& ri);

		void ProcessMessage_NotifyLicenseMismatch(CMessage &msg);

	private:
		void ProcessReadPacketFailed(void);
	};

	typedef RefCount<CNetClientWorker> CNetClientWorkerPtr;
}
