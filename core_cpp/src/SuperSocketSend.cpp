/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
// STL header file은 xcode에서는 cpp에서만 include 가능하므로
#include <new>
#include <stack>

#include "../include/Message.h"
#include "../include/NetConfig.h"
#include "../include/ReceivedMessage.h"
#include "SendFragRefs.h"

#include "FastSocket.h"
#include "IntraTracer.h"
#include "NetCore.h"
#include "ReportError.h"
#include "Epoll_Reactor.h"
//#include "../UtilSrc/DebugUtil/DebugCommon.h"
//#include "../UtilSrc/DebugUtil/NetworkAnalyze.h"

namespace Proud
{
	/* splitter를 붙이지 않는다. policy file을 전송하거나 이미 splitter가 붙은 '수신미보장 부분'을 재송신하는 데 사용된다.
	수신미보장 부분을 다시 전송할 때에도 사용된다. 그것들은 이미 splitter가 붙어 있으니까.

	Q: shared_from_this 냅두고 왜 this에 대한 shared_ptr을 따로 인자로 받나요?
	A: code profile 결과 shared_from_this에 의한 성능 하락이 커서입니다.
	shared_from_this를 얻어올 때 atomic inc가 발생합니다. 
	하지만 caller에서 이미 this를 액세스하기 전에 this에 대한 shared_ptr가 있었습니다. 
	따라서 atomic inc를 피할 수 있습니다. 이렇게요. */
	void CSuperSocket::AddToSendQueueWithoutSplitterAndSignal_Copy(
		const shared_ptr<CSuperSocket>& param_shared_from_this,  // [1]
		const CSendFragRefs &sendData)
	{
		assert(param_shared_from_this.get() == this);

		// 랜선 끊어짐 시뮬.
		if (m_turnOffSendAndReceive)
		{
			return;
		}

		if (param_shared_from_this->m_socketType == SocketType_WebSocket)
		{
			AddToSendQueueWithoutSplitterAndSignal_Copy_WebSocket(sendData);
		}
		else
		{
		MustTcpSocket();

		CriticalSectionLock sendLock(m_sendQueueCS, true);
		m_sendQueue_needSendLock->PushBack_Copy(sendData, SendOpt());

		// TCP는 항상 
			m_owner->SendReadyList_Add(param_shared_from_this, m_forceImmediateSend);

			m_tcpOnlyLastSentTime = GetPreciseCurrentTimeMs();
		}
	}

	/* TCP 전용.
	AMR 기능이 내장되어 있다. AMR이 활성화되어 있으면 AMR 내 수신미보장 부분에 splitter가 들어간 메시지를 넣는다.
	인자로 주어진 송신 payload는 사본이 떠져서 send queue에 들어간다.
	ACR 과정이 진행중이고 메인 socket이 close된 상태이면 수신미보장 부분에 들어가기만 한다. 
	
	caller에서는 sendOpt에 reliable or unreliable을 넣는데, unreliable을 넣는 경우 이 함수는 AMR에 넣지 않는다. 이유는 코드 내 주석에 있다.
	*/
	void CSuperSocket::AddToSendQueueWithSplitterAndSignal_Copy(
		const shared_ptr<CSuperSocket>& param_shared_from_this,
		const CSendFragRefs &sendData,
		const SendOpt& sendOpt,
		bool simplePacketMode)
	{
		assert(param_shared_from_this.get() == this);

		if (param_shared_from_this->m_socketType == SocketType_WebSocket)
		{
			// websocket은 자체적으로 message oriented이다. 따라서 splitter 등 불필요.
			AddToSendQueueWithoutSplitterAndSignal_Copy_WebSocket(sendData);
		}
		else
		{
		    // 이 overload된 함수는 TCP 전용이어야 한다! UDP 소켓에 대해 이 함수를 호출하면 안되지!
		    MustTcpSocket();

		    // AMR도 이걸로 보호된다. 자세한 것은 ACR 명세서 참고.
		    CriticalSectionLock sendLock(m_sendQueueCS, true);

		    // add message to send queue for each remote host
		    CSendFragRefs sendData2;

		    CMessage header;
		    header.UseInternalBuffer();

		    /*
		    ikpil.choi 2017-04-04 (N3785)
		    언릴라이어블일 경우, ACR 카운팅을 하지 않도록 한다. 이유는 두가지가 있다.
		    1. 언릴라이어블일 경우, 중요 데이터가 아니기 때문에 AMR 에 넣을 필요가 없다.
		    2. 언릴라이어블일 경우, unique number 가 설정 될 수 있는데, unique number에 의해 혼잡제어 루틴이 돌아 간다.
		    이때, AMR의 messageID 카운팅을 하게 되면, sendqueue 속에서 패킷의 messageID가 sendqueue의 앞쪽으로 배치 될 수 있다.
		    이렇게 되면, sendqueue의 패킷에 부여된 messageID가 오름차순으로 정렬 안되는 버그가 발생하여, 클라이언트쪽에서 패킷 유실이 발생한다.
		    */
		    if (NULL != m_acrMessageRecovery && MessageReliability_Unreliable != sendOpt.m_reliability)
		    {
			    // 메시지 ID를 할당한다.
			    int messageID = m_acrMessageRecovery->PopNextMessageIDToSend();

			    // ACR로 복원될 수 있는 메시지이며 이를 위해 수신미보장 부분에 본 메시지를 사본을 저장해 둔다.
			    CTcpLayer_Common::AddSplitterButShareBuffer(messageID, sendData, sendData2, header, simplePacketMode);

			    // TODO: 나중에 senddata와 여기 들어가는 것들을 모두 share 방식으로 바꾸자. 이러면 복사 부하를 대폭 줄인다.
			    m_acrMessageRecovery->Unguarantee_Add(messageID, sendData2); // splitter, messageID가 포함된 것을 저장한다.
		    }
		    else
		    {
			    // message ID가 없이 들어간다. 즉 AMR 비해당.
			    CTcpLayer_Common::AddSplitterButShareBuffer(sendData, sendData2, header, simplePacketMode);
		    }

		    // 랜선 끊어짐 시뮬.
		    // 여기서 검사하는 이유: 랜선 끊어짐 모의 상황에서도 위 AMR 추가는 해야 하니까.
		    if ( m_turnOffSendAndReceive )
		    {
			    return;
		    }

		    // 송신큐에 넣음
		    // 최초 송신 이슈는 10ms마다 하므로(사람이 못느끼며 TCP nagle OFF시 coalesce 효과) iocp post는 불필요.
		    m_sendQueue_needSendLock->PushBack_Copy(sendData2, sendOpt);

		    // TCP는 항상여기에 넣으면서 net worker thread가 깨어나게 해야 한다.
		    // UDP와 달리 coalesce를 위한 delay가 없음.
			m_owner->SendReadyList_Add(param_shared_from_this, m_forceImmediateSend);

			m_tcpOnlyLastSentTime = GetPreciseCurrentTimeMs();
		}
	}

	// UDP로 상대에게 송신한다.
	// fragment가 안되어 있어도 된다. SuperSocket 자체가 fragment를 해주는 기능이 내장되어 있다.
	// param copied.
	// UDP에만 써야 한다. 안그러면 throw 발생.
	void CSuperSocket::AddToSendQueueWithSplitterAndSignal_Copy(
		const shared_ptr<CSuperSocket>& param_shared_from_this,
		HostID finalDestHostID,
		FilterTag::Type filterTag,
		AddrPort sendTo,
		const CSendFragRefs &sendData,
		int64_t addedTime, // caller가 멀티캐스트를 하는 경우 이 함수를 자주 호출한다. 그때 GetPreciseCurrentTimeMs를 매번 호출하는 부하를 막으려고 이 변수가 있다.
		const SendOpt &sendOpt)
	{
		assert(param_shared_from_this.get() == this);

		if (m_socketType != SocketType_Udp)
		{
			this->m_owner->EnqueError(ErrorInfo::From(ErrorType_Unexpected, m_owner->GetVolatileLocalHostID(), _PNT("AddToSendQueueWithSplitterAndSignal_Copy: wrong UDP function call.")));
			return;
		}

		// 랜선 끊어짐 시뮬.
		if (m_turnOffSendAndReceive)
		{
			return;
		}

		if ( m_dropSendAndReceive == false )
		{
			assert(sendTo.IsUnicastEndpoint());

#ifdef TEST_CASE1002
			InterlockedIncrement(&g_CSuperSocket_AddToSendQueueWithSplitterAndSignal_Copy_Count);
			int a = g_CSuperSocket_AddToSendQueueWithSplitterAndSignal_Copy_Count;
#endif
			CIssueSendResult sendResult;
			{
				// 이 함수는 자주 호출되며 syscall을 하지 말아야 한다.
				// 따라서 per-socket lock 없이 바로 send queue lock을 시행.
				// lock order를 거스르지 않으므로 안전 (이해 못하겠으면 공룡책 내 프로세스 챕터 다시 공부하세요)
				CriticalSectionLock sendLock(m_sendQueueCS, true);

				// 패킷 로스 시뮬레이션 (testCase102)
				if ( m_packetTruncatePercent > 0 )
				{
					// Random.Next(100) == 0 ~ 100 , Random.Next(99) + 1 == 1 ~ 100
					int p = m_packetTruncateRandom_needSendLock.Next(99) + 1;

					if ( p <= m_packetTruncatePercent )
						return;
				}

				CSendFragRefs sendData2;
				CSmallStackAllocMessage header;
				CTcpLayer_Common::AddSplitterButShareBuffer(sendData, sendData2, header);


				// 송신큐에 추가한다.
				m_udpPacketFragBoard_needSendLock->AddNewPacket(
					finalDestHostID,
					filterTag,
					sendTo,
					sendData2,
					addedTime,
					sendOpt,
					sendResult);

				// send ready list에 넣고, 
				// UDP는 최초 send issue를 coalesce를 해서 보낸다. 
				// 따라서 어차피 최장 10ms 후 보내므로 그냥 즉시 보내야 함을 발동시키는 신호를 post할 필요 없다.
				// 하지만, ping 등 즉시 보내야 하는 메시지의 경우, syscall이 즉시 일어나게 net worker thread를 깨운다.
				m_owner->SendReadyList_Add(param_shared_from_this, sendResult.m_issueSendNow || m_forceImmediateSend);
			}
		}
	}


	//CSendFragRefs가 CMessage를 인자로 받는 ctor가 있다면, 
	// 그냥 이 함수 자체는 없어도 될 듯. 지워보고 에러 안나면 지우자.
	void CSuperSocket::AddToSendQueueWithSplitterAndSignal_Copy(
		const shared_ptr<CSuperSocket>& param_shared_from_this,
		HostID finalDestHostID,
		FilterTag::Type filterTag,
		AddrPort sendTo,
		CMessage &msg,
		int64_t addedTime, // caller가 멀티캐스트를 하는 경우 이 함수를 자주 호출한다. 그때 GetPreciseCurrentTimeMs를 매번 호출하는 부하를 막으려고 이 변수가 있다.
		const SendOpt &sendOpt)
	{
		assert(param_shared_from_this.get() == this);

		AddToSendQueueWithSplitterAndSignal_Copy(
			param_shared_from_this,
			finalDestHostID,
			filterTag,
			sendTo,
			CSendFragRefs(msg),
			addedTime,
			sendOpt);
	}

	int CSuperSocket::GetOverSendSuspectingThresholdInBytes()
	{
		return m_owner->m_settings.m_overSendSuspectingThresholdInBytes;
	}

	// 여기서 패킷을 보낼 때 수신받을 주소를 key로, 해당 송신 대상에게의 송신큐 총 바이트수를 구한다.
	int CSuperSocket::GetUdpSendQueueLength(const AddrPort &destAddr)
	{
		CriticalSectionLock lock(m_sendQueueCS, true);
		return m_udpPacketFragBoard_needSendLock->GetPacketQueueTotalLengthByAddr(destAddr);
	}

	// 각 수신자별 패킷 큐 자체의 갯수를 구한다.
	int CSuperSocket::GetUdpSendQueueDestAddrCount()
	{
		CriticalSectionLock lock(m_sendQueueCS, true);
		return m_udpPacketFragBoard_needSendLock->GetAddrPortToQueueMapKeyCount();
	}

	// 앞서 overlapped i/o completion 혹은 non-block syscall 후의 결과값을 가지고 나머지 처리를 한 후, caller가 무슨 행동을 해야 하는지를 알려주는 함수.
	CSuperSocket::ProcessType CSuperSocket::GetNextProcessType_AfterSend(const CIoEventStatus& comp, SocketErrorCode& outError)
	{
		Proud::AssertIsLockedByCurrentThread(m_cs);

		ProcessType ret = ProcessType_None;
		outError = SocketErrorCode_Ok;

		//m_lastSendIssuedTime = 0;

		// 송신의 경우 0바이트를 보내는 경우도 있으므로 <=가 아닌 < 비교이다.
		if ( comp.m_completedDataLength < 0 ) // WSAEINTR or EINTR 다루기.pptx
		{
			if ( m_socketType == SocketType_Tcp )
			{
				if ( comp.m_errorCode != SocketErrorCode_Intr )
				{
					// EINTR이 아니라면 TCP가 끊어진 것이므로 이렇게 처리해야.
					ret = ProcessType_CloseSocketAndProcessDisconnecting;
				}
			}
		}
		else
		{
			if ( m_isConnectingSocket )
			{
				// async or non-block으로 connect()를 하고 있는 도중에 이벤트가 왔다.
				// 연결이 성공하거나 실패 처리가 되고 나면 여기 오게 된다.
				outError = m_fastSocket->ConnectExComplete();
				if ((int)outError == ENOTCONN)
				{
					// #iOS_ENOTCONN_issue 그냥 무시하자.
					ret = ProcessType_OnConnectStillInProgress;
				}
				else if ( outError != SocketErrorCode_Ok )
				{
					// 연결 실패
					///m_owner->OnConnectFail(this, err);
					ret = ProcessType_OnConnectFail;
				}
				else
				{
					///m_owner->OnConnectSuccess(this);
					m_isConnectingSocket = false;
					ret = ProcessType_OnConnectSuccess;
				}
			}
			else
			{
				// async or non-block으로 send()를 한 것에 대한 이벤트가 왔다.
				ret = ProcessType_OnMessageSent;
				if ( comp.m_completedDataLength > 0 )	// WSAEINTR case에서 음수인 것은 걸러낸다.
				{
					CriticalSectionLock sendlock(m_sendQueueCS, true);

					// 송신 큐에서 완료된 만큼의 데이터를 제거한다. 그리고 다음 송신을 건다.
					// (최종 completion 상황이라 하더라도 이 과정은 필수!)
					if ( m_socketType == SocketType_Tcp )
					{
						// 앞서 IssueSendOnNeed에서는 peek을 했었다.
						// 여기서 성공한 만큼 앞단을 지우도록 한다.
						m_sendQueue_needSendLock->PopFront(comp.m_completedDataLength);
					}
					else
					{
						assert(comp.m_completedDataLength == m_sendIssuedFragment->m_sendFragFrag.GetLength());// UDP는 all or nothing send니까.

						/* TCP는 peek이기 때문에 상기와 같이 pop을 해줘야 하지만,
						앞서 IssueSendOnNeed에서 호출했던 PopFragmentOrFullPacket 자체가 pop이다.
						즉 보내려는 데이터를 이미 송신큐에서 지웠다. 즉 이미 pop되어 있으며
						pop 한 것은 m_sendIssuedFragment에 옮겨졌다.
						따라서 여기서는 굳이 pop을 할 것이 없다.

						Q: m_sendIssuedFragment를 재송신해야 하는 상황은 없나요? EMSGSIZE같은 것이라도?
						A: 보낼 조건이 못 되어서 시도조차 못하는 경우는 EWOULDBLOCK or IOPENDING 뿐입니다.
						EWOULDBLOCK이 뜰 경우 다 보내거나 아예 못 보낸 후 여기에 추후 오게 되며,
						IOPENDING인 경우도 마찬가지입니다.
						따라서 다시 보낼 이유가 없습니다. */

						// 혼잡제어를 위해
						m_udpPacketFragBoard_needSendLock->AccumulateSendBrake(comp.m_completedDataLength);
					}
				}
			}
		}

		return ret;
	}


	uint32_t CSuperSocket::GetUdpSendQueuePacketCount(const AddrPort &destAddr)
	{
		CriticalSectionLock lock(m_sendQueueCS, true);
		return m_udpPacketFragBoard_needSendLock->GetTotalPacketCountOfAddr(destAddr);
	}

	bool CSuperSocket::IsUdpSendBufferPacketEmpty(const AddrPort &destAddr)
	{
		CriticalSectionLock lock(m_sendQueueCS, true);
		return m_udpPacketFragBoard_needSendLock->IsUdpSendBufferPacketEmpty(destAddr);
	}

	void CSuperSocket::SetCoalesceInterval(AddrPort addr, int interval)
	{
		Proud::AssertIsLockedByCurrentThread(m_sendQueueCS);
		m_udpPacketFragBoard_needSendLock->SetCoalesceInterval(addr, interval);
	}

	int CSuperSocket::GetPacketQueueTotalLengthByAddr(AddrPort addr)
	{
		return m_udpPacketFragBoard_needSendLock->GetPacketQueueTotalLengthByAddr(addr);
	}



}
