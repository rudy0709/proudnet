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
#include "NetClient.h"
//#include "P2PGroup_S.h"
#include "ReliableUDP.h"
#include "ReliableUdpHelper.h"
//#include "RemoteClient.h"
//#include "RemotePeer.h"
#include "RmiContextImpl.h"
#include "SendFragRefs.h"
#include "ReceivedMessageList.h"

namespace Proud
{
	int CRemotePeerReliableUdp::AllStreamToSenderWindowAndYieldFrameNumberForRelayedLongFrame()
	{
		return m_host->AllStreamToSenderWindowAndYieldFrameNumberForRelayedLongFrame();
	}

	bool CRemotePeerReliableUdp::EnqueReceivedFrameAndGetFlushedMessages( CMessage &msg, CReceivedMessageList &ret ,ErrorType &outError)
	{
		// 참고: 아래 객체는 사본이므로 나머지 루틴에서 문제 없음.
		ReliableUdpFrame frame;

		int8_t f0;
		if (msg.Read(f0) == false)
			return false;
		frame.m_type = (ReliableUdpFrameType)f0;

		switch (frame.m_type)
		{
		case ReliableUdpFrameType_Ack:
			//puts("ACK FRAME ACK FRAME ACK FRAME ACK FRAME ");
			{
				if(!msg.Read(frame.m_ackFrameNumber))
				{
					return false;
				}
				if(!msg.Read(frame.m_maySpuriousRto))
				{
					return false;
				}
				//
				//				if(msg.GetReadOffset()!=msg.GetLength()) // 이것도 넣어야 할까나?
				//				{
				//					return false;
				//				}
			}

			break;
			//case ReliableUdpFrame::Type_Disconnect:
			//	msg.Read(frame.m_frameID);
			//	msg.Read(frame.m_data);
			//	break;
		case ReliableUdpFrameType_Data:
			//			puts("DATA FRAME DATA FRAME DATA FRAME");
			if(!msg.Read(frame.m_frameNumber))
				return false;
			if(!msg.Read(frame.m_hasAck))
				return false;
			if(frame.m_hasAck)
			{
				if(!msg.Read(frame.m_ackFrameNumber))
					return false;
				if(!msg.Read(frame.m_maySpuriousRto))
					return false;
			}

			frame.m_data.UseInternalBuffer();
			if(!msg.Read(frame.m_data))
				return false;

			break;
		default:
			assert(false);
			//			puts("????????????????????????????????????????");

			break;
		}

		return EnqueReceivedFrameAndGetFlushedMessages(frame, ret, outError);
	}

	bool CRemotePeerReliableUdp::EnqueReceivedFrameAndGetFlushedMessages( ReliableUdpFrame &frame, CReceivedMessageList& ret ,ErrorType &outError)
	{
		ret.Clear();

		m_host->ProcessReceivedFrame(frame);
		CStreamQueue* recvStream = m_host->GetReceivedStream();

		int addedCount = CTcpLayer_Common::ExtractMessagesFromStreamAndRemoveFlushedStream(
			*recvStream, ret, m_owner->m_HostID, m_owner->m_owner->m_settings.m_clientMessageMaxLength, /*m_owner->m_owner->m_unsafeHeap, */outError);

		if (addedCount < 0)
		{
			// 해당 peer와 더 이상 통신하기 어려운 마당이다.
			m_failed = true;
		}

		return true;
	}

	void CRemotePeerReliableUdp::Heartbeat()
	{
		m_host->Heartbeat();
	}

	// 1개의 data or ack frame을 전송한다.
	// ReliableUdpHost에 의해 호출된다.
	void CRemotePeerReliableUdp::SendOneFrame( ReliableUdpFrame &frame )
	{
		MessagePriority priority = g_UnreliableSendForPN.m_priority;

		switch (frame.m_type)
		{
		case ReliableUdpFrameType_Data:
			if(ReliableUdpConfig::HighPriorityDataFrame)
				priority = MessagePriority_Ring1;
			break;
		case ReliableUdpFrameType_Ack:
			// ack는 더 높은 우선으로 쏴야 한다.
			if(ReliableUdpConfig::HighPriorityAckFrame)
				priority = MessagePriority_Ring1;
			break;
		case ReliableUdpFrameType_None:
			break;
		}

		// 가능하면 직빵 UDP로 송신한다.
		if (!m_owner->IsRelayedP2P())
		{
			CSendFragRefs frame2;
			CMessage header; header.UseInternalBuffer();

			CRemotePeerReliableUdpHelper::BuildSendDataFromFrame(frame, frame2, header);
			SendOpt opt = SendOpt(priority, true);
			opt.m_fragmentOnNeed = false;	// reliable UDP 자체가 MSS 단위로 쪼개므로 쓸데없이 frag를 하지 말자. 성능에도 안좋고 행여냐 유지보수하다가 여기서 꼬일 수 있다.
			m_owner->m_ToPeerUdp.SendWithSplitter_Copy(frame2, SendOpt(priority,true));
		}
		else
		{
			// [직빵으로 P2P로 보내지 못하는 경우]
			// TCP를 통해 서버로 릴레이한다.
			// ack는 아예 보낼 필요도 없다. 괜히 쓸데없는 부하만 준다.
			// (참고: 이 직후 resend는 더 이상 하지 않는다. ReliableUdpHost::Heartbeat 참고.)
			switch (frame.m_type)
			{
			case ReliableUdpFrameType_Data:
				{
					CSmallStackAllocMessage header;
					Message_Write(header, MessageType_LingerDataFrame1);
					header.Write(m_owner->m_HostID); // sender host ID
					header.Write(frame.m_frameNumber); // frame number
#ifdef _DEBUG
					CMessageTestSplitterTemporaryDisabler dd(header);
#endif
					header.WriteScalar((int)frame.m_data.GetCount()); // frame length;

					CSendFragRefs sendData;
					sendData.Add(header);
					sendData.Add(frame.m_data.GetData(), (int)frame.m_data.GetCount());

					SendOpt sendOpt = SendOpt::CreateFromRmiContextAndClearRmiContextSendFailedRemotes(g_ReliableSendForPN); // 아래[1]에서 설명.
					m_owner->m_owner->m_remoteServer->Send_ToServer_Directly_Copy(
						m_owner->GetHostID(),
						MessageReliability_Reliable,
						sendData,
						sendOpt,
						m_owner->m_owner->m_simplePacketMode);

					/*[1]에 대해:
					N3785 fix를 한 후에, case 95,96 fail이 떴었다. 종전 코드가 원인이었다. SendOpt(g_UnreliableSendForPN)이었다.
					이것을 reliable로 바꾸고 나서 해결됐다.

					이유: 여기서 하는 일은 RUDP를 resend하기다.
					즉 ACR, AMR 처리가 수반되어야 한다. SuperSocket.AddToSendQueue에서 AMR, ACR 처리가 되어야 한다.
					거기서 이게 켜지게 하려면 reliable 설정이 되어 있어야 한다. 그래야 AMR ID가 매겨진다.
					*/
				}
				//				m_owner->m_owner->m_c2sProxy.P2P_LingerDataFrame1(HostID_Server, RmiContext::ReliableSend,
				//					m_owner->m_HostID, frame.m_frameNumber, frame.m_data.GetInternalBufferRef());
				break;
			case ReliableUdpFrameType_Ack:
				// 릴레이 모드에서는 굳이 ack를 보내지 않는다. 상대방은 ack를 모두 받은 것으로 간주할테니.
				break;
			default:
				assert(false);  // 의도 안한 프레임 타입
				break;
			}
		}
	}

	bool CRemotePeerReliableUdp::IsReliableChannel()
	{
		return m_owner->IsRelayedP2P();
	}

	// param copied
	void CRemotePeerReliableUdp::SendWithSplitter_Copy(const CSendFragRefs &sendData)
	{
		// add message to send queue for each remote host
		CSendFragRefs sendData2;

		// 메시지 헤더, 메시지 크기 등을 감싼다.
		CSmallStackAllocMessage header;
		CTcpLayer_Common::AddSplitterButShareBuffer(sendData, sendData2, header);

		// reliable UDP 의 송신큐에 추가한다.
		// 스트림이므로 아래와 같이 반복 호출도 ok.
		for (int i = 0; i < sendData2.GetFragmentCount(); i++)
		{
			m_host->Send(sendData2[i].m_data, sendData2[i].m_length);
		}
	}

	CRemotePeerReliableUdp::CRemotePeerReliableUdp( CRemotePeer_C *owner )
	{
		m_failed = false;
		m_owner = owner;
	}

	void CRemotePeerReliableUdp::ResetEngine( int frameNumber )
	{
		// 예전건 버리고 새로 만든다.
		m_host.Free();
		m_host.Attach(new ReliableUdpHost(m_owner, frameNumber) );
	}

	Proud::HostID CRemotePeerReliableUdp::TEST_GetHostID()
	{
		return m_owner->m_HostID;
	}

//	int CRemotePeerReliableUdp::GetRecentUnreliablePingMs()
//	{
//		return PNMAX(m_owner->m_recentPingMs, 0);
//	}
}
