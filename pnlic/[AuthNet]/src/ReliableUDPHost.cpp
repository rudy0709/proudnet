/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <new>
// xcode는 STL을 cpp에서만 include 가능 하므로
#include <stack>
#include <cstring>
#include <climits>
#include <cstdlib>
#include <cstddef>
#include <iosfwd>

#include "ReliableUDPHost.h"
#include "RemotePeer.h"
#include "NetClient.h"


namespace Proud
{
	// 인공위성 통신에서는 RTT가 매우 길어서 sender window초기값은 3이어야 한다.
	const int SenderWindowInitMaxLength = 3;

	// slow-start threshold는 아무리 낮아도 이 값 이하로 떨어지지 않는다. 
	// sender window가 지나치게 바닥으로 낮아지는 현상을 막기 위함
	const int MinimumSSThreshHold = SenderWindowInitMaxLength + 2;

	// 수신된 스트림 객체 자체를 리턴한다.
	// 성능 좋은 접근을 위함. 여기에서 처리된 부분을 뜯어내는건 유저의 몫이다.
	CStreamQueue* ReliableUdpHost::GetReceivedStream()
	{
		return &m_receivedStream;
	}

	// 객체를 생성하자마자 상대와의 연결이 된 것으로 간주한다. 이미 초기 프레임 값이 서로 맞춰진 셈이니까.
	ReliableUdpHost::ReliableUdpHost( CRemotePeer_C *OwnerRemotePeer, int firstFrameNumber ) 
		:m_sendStream(CNetConfig::StreamGrowBy)
		,m_receivedStream(CNetConfig::StreamGrowBy)
	{
		m_lastReceivedAckNumber = 0;

		m_nextFrameNumberToSend = firstFrameNumber;
		m_expectedFrameNumberToReceive = firstFrameNumber;

		m_lastHeartbeatTimeMs = 0;

		m_streamToSenderWindowLastTimeMs = 0;
		m_delayAckSentLastTimeMs = 0;

		m_receivedFrameCount = 0;
		m_receivedStreamCount = 0;
		m_totalSendStreamLength = 0;
		m_totalReceivedStreamLength = 0;
		m_totalAckFrameCount = 0;
		m_totalFirstSendCount = 0;
		m_totalResendCount = 0;
		m_totalReceivedDataFrameCount = 0;
		m_totalAckFrameReceivedCount = 0;

		m_senderWindowMaxLength = SenderWindowInitMaxLength;
		m_mustSendAck = false;

		m_dupAckReceivedCount = 0;
		m_dupDataReceivedCount = 0;
		m_preventSpuriousRto = false; 
		m_dupDataReceivedCount_LastClearTimeMs = 0;

		m_ownerRemotePeer = OwnerRemotePeer;

		InitSSThresh();
	}

	// 통계 정보를 얻는다.
	void ReliableUdpHost::GetStats( ReliableUdpHostStats &ret )
	{
		ret.m_lastReceivedAckNumber = m_lastReceivedAckNumber;
		ret.m_senderWindowLength = m_senderWindow.GetCount();
		if(m_senderWindow.GetCount() == 0)
		{
			ret.m_senderWindowWidth = 0;
		}
		else
		{
			SenderFrame& head = m_senderWindow.GetHead();
			SenderFrame& tail = m_senderWindow.GetTail();
			ret.m_senderWindowWidth = tail.m_frameNumber - head.m_frameNumber;
		}
		ret.m_senderWindowMaxLength = (int)m_senderWindowMaxLength;

		ret.m_receivedFrameCount = m_receiverWindow.GetCount();
		ret.m_receivedStreamCount = m_receivedStream.GetLength();
		ret.m_totalReceivedStreamLength = m_totalReceivedStreamLength;
		ret.m_totalAckFrameCount = m_totalAckFrameCount;

		ret.m_expectedFrameNumberToReceive = m_expectedFrameNumberToReceive;
		ret.m_nextFrameNumberToSend = m_nextFrameNumberToSend;

		ret.m_sendStreamCount = m_sendStream.GetLength();
		ret.m_senderWindowDataFrameCount = m_senderWindow.GetCount();
		ret.m_totalSendStreamLength = m_totalSendStreamLength;
		ret.m_totalResendCount = m_totalResendCount;
		ret.m_totalFirstSendCount = m_totalFirstSendCount;
		ret.m_totalReceiveDataCount = m_totalReceivedDataFrameCount;

		if(m_senderWindow.GetCount()>0)
			ret.m_senderWindowLastFrame = m_senderWindow.GetTail().m_frameNumber;
		else
			ret.m_senderWindowLastFrame = 0;
	}

	// 매 짧은 순간마다 이것을 호출해야 한다.
	// 적정 주기는 1밀리초이다.
	void ReliableUdpHost::Heartbeat()
	{
		assert(m_senderWindowMaxLength >= 1);
		int64_t currTime = GetPreciseCurrentTimeMs();	

		// 몇 초마다, '받았는데 또 데이터 프레임을 받네 젠장' 카운트를 클리어한다.
		if(currTime - m_dupDataReceivedCount_LastClearTimeMs > 4300) 
		{
			m_dupDataReceivedCount_LastClearTimeMs = currTime;
			m_dupDataReceivedCount = 0;
		}

		/* 만약 relay mode라면, stream에 있는 것을 1개의 long frame으로 만들어 sender window에 옮긴 후, 
		sender window에 있는 것들을 지금 즉시 모두 전송해 버리고, ack를 이미 받은 것처럼 처리해버린다. 
		relay이면 TCP로 전송되므로 혼잡 제어는 걱정 하지 않아도 된다. 즉 여기서 모든 sender window를 한번에 flush해버려도 된다. */
		if(m_ownerRemotePeer->IsRelayedP2P())
		{
			AllStreamToSenderWindow(currTime);

			// relay mode인 경우, reliable udp failure 상태였을 수 있다. 이를 리셋하는 역할을 여기서 해야 한다.
			m_maxResendElapsedTimeMs = 0;
			m_senderWindowMaxLength = SenderWindowInitMaxLength;	// slow start 알고리즘을 처음부터 시작			

			InitSSThresh();		// Sliding Window 관련 초기화 루틴
		}

		// relay mode이면 TCP를 통해 이미 전송 요청이 다 된 상태이므로, 송신할 것이 완전히 비어 있어야 한다.
		if(m_ownerRemotePeer->IsRelayedP2P())
		{
			assert(m_sendStream.GetLength() == 0);	
			assert(m_senderWindow.GetCount() == 0);
		}

		// coalesce타임마다, stream을 sender window로 옮긴다. 단, 혼잡 제어를 위해 sender window의 크기를 제한한다.
		if(m_streamToSenderWindowLastTimeMs == 0 
			|| currTime - m_streamToSenderWindowLastTimeMs > m_ownerRemotePeer->m_owner->m_StreamToSenderWindowCoalesceInterval_USE) // 참고: 여기를 주석 처리하면 성능이 왕창 올라감. 아래 delayed ack는 별 영향을 안줌.
		{
			m_streamToSenderWindowLastTimeMs = currTime;

			while (m_sendStream.GetLength() > 0 && m_senderWindow.GetCount() < (int)m_senderWindowMaxLength)
			{
				int length =  PNMIN(ReliableUdpConfig::GetFrameLength(), (int)m_sendStream.GetLength());

				SenderFrame frame;
				frame.m_type = ReliableUdpFrameType_Data;
				frame.m_frameNumber = m_nextFrameNumberToSend;
				m_nextFrameNumberToSend++;
				frame.m_data.UseInternalBuffer();
				frame.m_data.SetCount(length);
				UnsafeFastMemcpy(frame.m_data.GetData(), m_sendStream.GetData(), length);
				m_senderWindow.AddTail(frame);
				m_sendStream.PopFront(length);
			}
		}

		// sender window에 있는 것들을 first send or resend를 한다
		for (CFastList2<SenderFrame, int, CPNElementTraits<SenderFrame> >::iterator i=m_senderWindow.begin();
			i!=m_senderWindow.end();i++)
		{
			SenderFrame& frame = *i;

			if (frame.m_sentCount == 0)	// 초송신 만족? (주의: 시간으로 검사하지 말자. int 버전으로 추후 바꿀 경우 0일 수 있으니)
			{
				frame.m_lastSentTimeMs = currTime; 
				frame.m_firstSentTimeMs = currTime;
				frame.m_resendCoolTimeMs = GetRetransmissionTimeout();
				frame.m_sentCount++;
				DataFrame_PiggybagAck(frame, currTime);

				//printf("Data frame sent:%d at %d\n", frame.m_frameNumber, GetTickCount());
				m_ownerRemotePeer->m_ToPeerReliableUdp.SendOneFrame(frame); // 초송신 혹은 재송신을 여기서 수행

				m_totalFirstSendCount++;
			}
			else if(frame.m_needFastRetransmit || currTime - frame.m_lastSentTimeMs > frame.m_resendCoolTimeMs)   // 재송신 만족?
			{
				// 최악의 송신 기록을 갱신
				m_maxResendElapsedTimeMs = PNMAX(m_maxResendElapsedTimeMs, currTime - frame.m_firstSentTimeMs);

				if(!frame.m_needFastRetransmit)	// RTO에 의한 재전송인 경우
				{
					frame.m_resendCoolTimeMs *= ReliableUdpConfig::EnlargeResendCoolTimeRatioPercent; // 재송신 횟수가 실패할수록 재송신을 좀 더 느긋하게 보내도록 한다. Exponential Backoff 기법.
					frame.m_resendCoolTimeMs /= 100;

					frame.m_resendCoolTimeMs = PNMIN(frame.m_resendCoolTimeMs, ReliableUdpConfig::MaxResendCoolTimeMs);  // 그래도 느긋함의 한계를 둔다.

					// ssthresh를 갱신한다. 단, 너무 자주 갱신하지는 않는다.
					if (!m_ssThreshValid
						|| currTime - m_ssThreshChangedTimeMs > ReliableUdpConfig::MinThreadholdSlowStartRestartTimeout)
					{
						m_ssThresh = (int)(m_senderWindowMaxLength / 2);
						m_ssThresh = PNMAX(m_ssThresh, MinimumSSThreshHold);			// m_ssThresh 는 최소 2(*Maximum Segment Size) 를 보장하기 위함
						m_ssThreshValid = true;											// ssthreash status 값을 변경
						m_ssThreshChangedTimeMs = currTime;								// 변경된 시간값을 저장(나중에 다시 ss 를 시작하기 위한 최소 임계값(ReliableUdpConfig::MinThreadholdSlowStartRestartTimeout)을 측정하기 위해서
					}

					// 재송신이 발생한다는 뜻은 혼잡 제어가 필요함을 의미. 따라서 sliding window 최대 크기를 감소한다.
					// 어차피 다시 빨리 증가하므로 크게 감해도 될 듯.
					// (근거: 마스터링 TCP/IP 책)
					m_senderWindowMaxLength = SenderWindowInitMaxLength;
				}

				frame.m_needFastRetransmit = false;	// 이게 없으면 fast retransmit을 필요 이상으로 과량 해대니
				frame.m_sentCount++; // 송신 횟수 기록
				frame.m_lastSentTimeMs = currTime;
				DataFrame_PiggybagAck(frame, currTime);

				m_totalResendCount++; // 총 재송신 횟수 기록

				//printf("Data frame resent:%d at %d\n", frame.m_frameNumber,GetTickCount());
				m_ownerRemotePeer->m_ToPeerReliableUdp.SendOneFrame(frame); // 초송신 혹은 재송신을 여기서 수행
			}
		}

		// data frame에 piggybag할 기회도 없었고, delayed ack를 위해 충분히 기다렸다면, ack 정보만 있는 frame을 별도로 보내자.
		// 단, relay mode이라면 예외
		if(currTime - m_delayAckSentLastTimeMs > ReliableUdpConfig::DelayedAckIntervalMs 	// NOTE: 여기를 주석화해도 성능상 별 차이는 없다. 위 first or re-send 쪽을 주석화하면 확 차이남.
			&& m_mustSendAck
			&& !m_ownerRemotePeer->IsRelayedP2P())
		{
			// ack만 들어있는 frame 송신
			ReliableUdpFrame frame;
			frame.m_type = ReliableUdpFrameType_Ack;
			frame.m_ackFrameNumber = m_expectedFrameNumberToReceive;	 
			frame.m_maySpuriousRto = MaySpuriousRto();
			m_mustSendAck = false;
			m_delayAckSentLastTimeMs = currTime;	
			m_ownerRemotePeer->m_ToPeerReliableUdp.SendOneFrame(frame);

			// 통계
			m_totalAckFrameCount++;
		}

		m_lastHeartbeatTimeMs = currTime;
	}

	// 스트림에 송신할 데이터를 추가한다.
	void ReliableUdpHost::Send(const uint8_t* stream, int count)
	{
		m_sendStream.PushBack_Copy(stream, count);
		m_totalSendStreamLength += count;
	}

	void ReliableUdpHost::InitSSThresh()
	{
		m_ssThresh = -1;
		m_ssThreshValid = false;

		m_ssThreshChangedTimeMs = 0;
	}

	// UDP로부터 받은 프레임을 이 객체에 넣는다.
	// 이것은 ack나 데이터 프레임을 구별해서 수신 윈도를 접근한다.
	void ReliableUdpHost::ProcessReceivedFrame( ReliableUdpFrame &frame )
	{
		switch (frame.m_type)
		{
		case ReliableUdpFrameType_Ack:
			ProcessAckFrame(frame);
			break;
		case ReliableUdpFrameType_Data:
			ProcessDataFrame(frame);
			break;
		case ReliableUdpFrameType_None:
			assert(false); // 내부 버그이거나 해킹당한 것임
			break;
		}

	}

	/* 만약 다른 reliable send 경로를 통해 reliable UDP frame을 보낼 방도가 있다면 그쪽으로 이미 성공적으로 보내졌음을
	가정해서 이번에 보낼 frame number를 양보하고 그 번호를 유저에게 건네준다. 
	단, stream에 있는 것들을 모두 sender window로 옮긴 후 시행한다. 게다가 sender window 크기 제한도 무시한다.
	(어차피 TCP로 릴레이되는데 TCP는 혼잡 제어 기능이 있다. 따라서 이래도 안전하다.)

	릴레이 서버를 구현할 때 이 기능이 쓰인다. */
	int ReliableUdpHost::AllStreamToSenderWindowAndYieldFrameNumberForRelayedLongFrame()
	{
		AllStreamToSenderWindow(GetPreciseCurrentTimeMs());

		int ret = m_nextFrameNumberToSend;
		m_nextFrameNumberToSend++;

		return ret;
	}

	// data frame을 보내니, 이참에 ack도 piggybag하자.
	void ReliableUdpHost::DataFrame_PiggybagAck( SenderFrame &frame, int64_t currTime )
	{
		/* piggybag할 때는 용량을 걱정 안해도 된다. 어차피 보내는 것 까짓거 덤으로 더 보내도 된다. 
		하지만. 의도치 않은 fast retransmit이 자주 발생하면 안되니까, 그리고 아직 상대방으로 아무런 data frame도 안 받은 경우
		(예: 사용자가 의도적으로 단방향으로만 메시징하는 앱) 엄한 frame number 0에 대한 ack를 보내면 안된다. 없는건 없는거다.
		그래서 hasAck 플래그가 있는거임. */
		if(m_mustSendAck)
		{
			frame.m_hasAck = true;
			frame.m_ackFrameNumber = m_expectedFrameNumberToReceive;	
			frame.m_maySpuriousRto = MaySpuriousRto();

			m_delayAckSentLastTimeMs = currTime;	// piggybag했으니 
			m_mustSendAck = false; // piggybag했으니
		}
	}

	void ReliableUdpHost::ProcessAckFrame( ReliableUdpFrame & frame )
	{
		m_totalAckFrameReceivedCount++;
		m_lastReceivedAckNumber=frame.m_ackFrameNumber;
		m_preventSpuriousRto = frame.m_maySpuriousRto;	// 수신자(ack를 보내온 측)에서, spurious RTO를 의심했다면, 송신자 또한 RTO를 충분히 길게 잡아야 한다.

#ifdef _DEBUG
		/*		m_ackLog a;
		a.m_number = frame.m_ackFrameNumber;
		a.m_time = m_OwnerRemotePeer->GetCachedTime();
		a.m_pureAck = frame.m_type == ReliableUdpFrameType_Ack;
		m_lastReceivedAckNumbers.Add(a);
		if(m_lastReceivedAckNumbers.GetCount()>10)
		{
		m_lastReceivedAckNumbers.RemoveAt(0);
		}*/
#endif // _DEBUG

		// ack를 보낸 측(즉 data frame의 수신자)에서 보내준 expected frame number 이하 것들은 제거하자. 수신측에게 안전하게 도착한 것들이니.
		int removedCount = RemoveFromSenderWindowBeforeExpectedFrame(frame.m_ackFrameNumber);

		// 성공적으로 상대에게 전달된 data frame 갯수만큼 sliding window 크기를 증가시킨다. slow start 기법.
		if (!m_ssThreshValid || m_senderWindowMaxLength < m_ssThresh)
		{
			// ssthresh 까지는 기하 급수적으로 증가
			// (기하급수인 이유: cwnd를 늘리면 ack가 증가하는 속도가 같이 증가하게 되기 때문)
			m_senderWindowMaxLength += float(removedCount);
		}
		else 
		{
			// ssthresh 크기 이후부터는 ack 를 받아도 완만하게 증가시킨다.
			// 마스터링 TCP/IP에 의하면, 기존 cwnd 크기에 반비례시키면 된다.
			// 증가분이 1 이하인 경우도 누적시켜야 하므로 cwnd가 float 값인 이유가 여기에 있다.
			m_senderWindowMaxLength += float(removedCount) / m_senderWindowMaxLength;
		}

		// 상한선 조절
		if(m_senderWindowMaxLength > 1E+20)
		{
			m_senderWindowMaxLength = 1E+20f;
		}

		// TCP fast retransmit처럼, 몇 번 중복으로 같은 번호의 ack를 받으면 head frame을 즉시 resend한다.
		// 만약 받은 ack 번호에 대응하는 frame을 지울 것이 없다는 것은, 
		if(removedCount == 0)
		{
			m_dupAckReceivedCount++;
		}
		else
		{
			m_dupAckReceivedCount = 0;
		}

		if(m_dupAckReceivedCount > 3)
		{
			// dup ack 상황이다. fast retransmit 신공을 발휘하자.
			m_dupAckReceivedCount = 0;

			if(m_senderWindow.GetCount() > 0)
			{
				SenderFrame& sf = m_senderWindow.GetHead();
				sf.m_needFastRetransmit = true;
			}	

			// fast retransmit이 발생할 때는 cwnd를 반으로 줄여야 한다.
			// (근거: 마스터링 TCP/IP 책)
			m_senderWindowMaxLength /= 2;
			m_senderWindowMaxLength = PNMAX(m_senderWindowMaxLength, SenderWindowInitMaxLength);
		}
	}

	// ack를 받았을 때, ack내 frame number는 상대가 받기를 '기대하는' 것이다. 따라서 그 앞의 것들은 이미 상대가 다 받았다는 뜻.
	// 그것들을 제거한다.
	int ReliableUdpHost::RemoveFromSenderWindowBeforeExpectedFrame( int ackFrameNumber )
	{
		int removedCount = 0;
		while(m_senderWindow.GetCount() > 0)
		{
			const SenderFrame &sf = m_senderWindow.GetHead();

			// 이미 지나간 프레임이라면
			int frameNumberDiff = sf.m_frameNumber - ackFrameNumber;
			if(frameNumberDiff < 0)
			{
				m_senderWindow.RemoveHeadNoReturn();
				removedCount++;
			}
			else 
			{
				// 이제 현재 것이나 미래 것이면 더 이상 탐색 중지. 어차피 정렬된 상태이므로 안전.
				break;
			}
		}

		return removedCount;
	}

	// 수신한 data frame을 처리한다.
	void ReliableUdpHost::ProcessDataFrame( ReliableUdpFrame & frame )
	{
		// 통계
		m_totalReceivedDataFrameCount++;

#ifdef _DEBUG
		/*DataLog a;
		a.m_number = frame.m_frameNumber;
		a.m_time = m_OwnerRemotePeer->GetCachedTime();
		m_lastReceivedDataFrames.Add(a);
		if(m_lastReceivedDataFrames.GetCount()>10)
		{
		m_lastReceivedDataFrames.RemoveAt(0);
		}
		printf("Data frame received: %d at %d\n", a.m_number,GetTickCount());*/
#endif // _DEBUG

		// piggybag된 ack를 처리
		if(frame.m_hasAck)
		{
			ProcessAckFrame(frame);
		}

		// 받은 data frame에 의해 receiver window to stream이 행해지건 안하건 '현재 어디까지의 ack가 됐는지'는 알려주어야 한다.
		// 안한 경우에는 안보내게 했더니 한번 ack loss일어나면 20초 RTO time out 즉 디스로 이어졌기 때문에.
		if(!m_ownerRemotePeer->IsRelayedP2P())			// udp로 받은거라면 ack를 보내줘야한다. relayed에서는 TCP로 전송되므로 ack자체가 불필요.
		{
			m_mustSendAck = true;
		}

		// 이미 stream으로 나간 data frame이 뒷북으로 중첩 도착한 경우(UDP는 그럴 수 있음) 무시한다.
		int diff = frame.m_frameNumber - m_expectedFrameNumberToReceive;
		if(diff < 0)
		{
			m_dupDataReceivedCount++;
			return;
		}

		// receiver window에 추가를.
		if(!ReceiverWindow_AddFrame(frame))
		{
			m_dupDataReceivedCount++; // 이미 받은걸 또 받은거면 이걸 +1
		}

		// 받은 data frame이 모두 연속되는 것들을 stream으로 보낸다. 연속된 것만큼의 next expected number to receive를 갱신한다.
		SequentialReceiverWindowToStream();
	}

	// 받은 data frame을 receiver window에 넣는다.
	// 이미 받은걸 또 받으면 false. 처음 받는 것이라 신규 추가됐으면 true 리턴.
	bool ReliableUdpHost::ReceiverWindow_AddFrame( ReliableUdpFrame &frame )
	{
		// 순서에 맞춰서 receivedFrame에 채워넣는다.
		for (Position rfnode = m_receiverWindow.GetHeadPosition();rfnode;)
		{
			ReliableUdpFrame &RWF = m_receiverWindow.GetAt(rfnode);

			// 이미 같은게 있으면 넘어간다.
			if (RWF.m_frameNumber == frame.m_frameNumber)
			{
				return false;
			}

			int diff = frame.m_frameNumber - RWF.m_frameNumber;
			if (diff < 0)
			{
				m_receiverWindow.InsertBefore(rfnode, frame);

				// 루프끝
				return true;
			}

			m_receiverWindow.GetNext(rfnode);
		}

		// 맨 끝에 삽입한다.
		m_receiverWindow.AddTail(frame);

		return true;
	}

	// 기대했던 프레임 번호와 순차적으로 연속된 프레임들을 모아서 수신 스트림으로 보내준다. 즉 수신 성공 처리를 한다.
	// 동시에 다음에 와야할 frame number값도 갱신한다.
	void ReliableUdpHost::SequentialReceiverWindowToStream()
	{
		// 유저에게 모아서 stream처럼 만들어 보낸다.
		// 그리고 receive queue에서 제거한다.
		while (m_receiverWindow.GetCount() > 0)
		{
			ReliableUdpFrame &rf = m_receiverWindow.GetHead();

			if (rf.m_frameNumber == m_expectedFrameNumberToReceive)
			{
				m_receivedStream.PushBack_Copy(rf.m_data.GetData(), rf.m_data.GetCount());
				m_totalReceivedStreamLength += rf.m_data.GetCount();
				m_receiverWindow.RemoveHeadNoReturn();
				m_expectedFrameNumberToReceive++;
			}
			else
				break;
		}
	}

	// stream에 있는 것들을 모두 sender window에 넣는다. sender window max size를 무시한다.
	// 릴레이로 전환될 때 사용된다.
	void ReliableUdpHost::AllStreamToSenderWindow( int64_t currTime )
	{
		//assert(m_OwnerRemotePeer->IsRelayedP2P()); relayed라 하더라도 C/S가 훨씬 빨라서 릴레이를 타는 경우도 있으니 검사 안함(IsRelayMuchFasterThanDirectP2P)
		if(m_sendStream.GetLength() > 0)
		{
			int length = m_sendStream.GetLength();
			SenderFrame frame;
			frame.m_type = ReliableUdpFrameType_Data;
			frame.m_frameNumber = m_nextFrameNumberToSend;
			frame.m_data.UseInternalBuffer();
			frame.m_data.SetCount(length);
			UnsafeFastMemcpy(frame.m_data.GetData(), m_sendStream.GetData(), length);
			m_senderWindow.AddTail(frame);
			m_sendStream.PopFront(length);

			m_nextFrameNumberToSend++;
		}
		assert(m_sendStream.GetLength()==0);

		for (CFastList2<SenderFrame, int, CPNElementTraits<SenderFrame> >::iterator i=m_senderWindow.begin();
			i!=m_senderWindow.end();i++)
		{
			SenderFrame& frame = *i;
			DataFrame_PiggybagAck(frame, currTime);
			m_ownerRemotePeer->m_ToPeerReliableUdp.SendOneFrame(frame);
		}
		m_senderWindow.Clear();
	}

	// RTO를 얻는다. 고정값을 리턴하거나, RTT 기반으로 충분히 큰 값을 얻는다.
	int64_t ReliableUdpHost::GetRetransmissionTimeout()
	{
		// RTT 기반 아니면 째!
		if(!ReliableUdpConfig::IsResendCoolTimeRelatedToPing)
		{
			return ReliableUdpConfig::FirstResendCoolTimeMs;
		}

		// RTT 구한적 없어도 째!
		int recentPingMs = m_ownerRemotePeer->m_recentPingMs;
		if(recentPingMs <= 0)
		{
			return ReliableUdpConfig::FirstResendCoolTimeMs;
		}

		int64_t ret = recentPingMs * 4 + ReliableUdpConfig::DelayedAckIntervalMs; // RTT보다는 충분히 커야 ack가 올 수 있는데도 resend를 하는 불상사를 막음
		

		// int64_t ret = recentPingMs * 4; 밑에 있을 시 의미가 없어 지는 것 같습니다. 확인 부탁 드립니다.
		// spurious RTO를 송신자가 신고했다면,
		// 1.3초라는 충분히 큰 레이턴시를 더해서 spurious RTO를 방지해 버리자.
		if(m_preventSpuriousRto)	
			ret += 1300;	


		// RTO가 너무 짧으면 패킷 로스 발생이 조금이라도 나면 트래픽이 팍 오름.
		// 따라서 하한선을 둬야.
		// (더하지 않음! 위에서 *4를 했는데 WAN이면 충분히 큰 값이 나오니까)
		ret = PNMAX(ret, ReliableUdpConfig::MinResendCoolTimeMs);				

		// 그래도 상한선은 지키도록 하자. 핑 시간이 잘못 측정된 경우를 위해.
		ret = PNMIN(ret, ReliableUdpConfig::MaxResendCoolTimeMs);

		return ret;
	}

	void ReliableUdpHost::DoForLongInterval()
	{
		// obj-pool는 성능은 좋지만 안 쓰는 잔여 메모리는 주기적으로 정리해 주어야 함.
		m_sendStream.Shrink();
		m_receivedStream.Shrink();
		// 		m_senderWindow.Shrink(); 언젠가, 이것들도 shrink하는 기능이 모바일 버전에서 필요할지도.
		// 		m_receiverWindow.ShrinkOnNeed();
	}

	int64_t ReliableUdpHost::GetMaxResendElapsedTimeMs()
	{
		return m_maxResendElapsedTimeMs;
	}

	// 상대방으로 수신 받는 data frame이 받았던 것이 또 받아지는 경우가 너무 잦으면, spurious RTO를 의심할 수 있다. 이때 true를 리턴한다.
	bool ReliableUdpHost::MaySpuriousRto()
	{
		// 얼마 되지도 않았는데 중복 data frame을 받은 회수가 많다면 의심해야.
		return (m_dupDataReceivedCount > 100) && (m_lastHeartbeatTimeMs - m_dupDataReceivedCount_LastClearTimeMs < 1100);

	}

}