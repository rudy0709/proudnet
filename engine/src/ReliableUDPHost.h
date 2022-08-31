/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#pragma once
#include "ReliableUDPFrame.h"
#include "StreamQueue.h"
#include "FastList2.h"
#include "../include/IRmiHost.h"

namespace Proud
{
	class CRemotePeer_C;
	class ReliableUdpFrame;
	class ReliableUdpHostStats;

	/* Reliable UDP의 메인 객체.
	Reliable UDP ver2 개발.pptx 참고.

	Reliable UDP과 유저가 제공하는 UDP 층을 연결하는 방법
	- 먼저 IReliableUdpHostDelegate를 구현한다. 그리고 ReliableUdpHost 객체에 연결한다.
	- UDP에서 받은 것을 처리: UDP frame인지 먼저 확인 후 ReliableUdpHost.TakeReceivedFrame()에 넣는다.
	- UDP로 보내기: ReliableUdpHost에 의해, IReliableUdpHostDelegate의 SendOneFrameToUdp() 호출된다. 이를 구현하라.
	- 일정 시간마다: ReliableUdpHost.FrameMove() 호출하라. DoForLongInterval은 매 몇초마다 콜 해야 하고.
	- relay로 보내는 메시지는: AllStreamToSenderWindowAndYieldFrameNumberForRelayedLongFrame로 그 메시지의 frame number를 얻어야 한다. TCP로 전송되므로 long frame으로서 전송되면 된다.

	Reliable UDP를 통해 메시지 주고받는 방법
	- 먼저 Reliable UDP에 최초 frame number를 지정해야 한다. 이는 생성자에서 한다.
	- reliable UDP의 송신 메인: ReliableUdpHost.Send() 호출하라.
	- reliable UDP의 수신 메인: ReliableUdpHost.PopReceivedStream() 호출하라.

	상대와의 연결 해제 기능은 내장되어 있지 않다. 어차피 relay server를 통해서 중재하니까.
	그러나 상대와의 연결이 정상이 아닌지 여부를 판단하는 방법으로 지속적으로 P2P간 메시지를 주고 받으면서
	최종 도착한 ack의 시간을 체크하는 방법이 있다. 이를 위해
	GetLastAckTimeElapsed()가 있다.

	* 통계 정보 얻기: GetStats() */
	class ReliableUdpHost
	{
		CRemotePeer_C *m_ownerRemotePeer;

		//////////////////////////////////////////////////////////////////////////
		// 데이터 송신을 위해 쓰이는 것들

		// 보낼 stream. sender window로 전달되어 data frame list가 될 것이다.
		CStreamQueue m_sendStream;

		// 송신측 window
		typedef CFastList2<SenderFrame, int, CPNElementTraits<SenderFrame> > SenderFrameList;
		SenderFrameList m_senderWindow;

		// sender window의 최대 크기. 혼잡 제어를 위함. 커질수록 좋은 네트워크 회선을 의미. cwnd or congestion window와 같은 의미다.
		// 절대 1 이상이어야.
		// float인 이유: ssthresh 이후부터는 cwnd가 1 미만 값으로도 증가하기 때문이다.
		// int이면 1미만이 절삭되어서 증가가 제대로 안되는 문제 발생.
		float m_senderWindowMaxLength;

		// 이번에 data frame을 보낼 경우 그 번호
		int m_nextFrameNumberToSend;

		// 마지막으로 heartbeat 함수를 실행했던 시간
		int64_t m_lastHeartbeatTimeMs;

		// stream에서 sender window로 가장 마지막으로 옮긴 시간. coalesce를 위해.
		int64_t m_streamToSenderWindowLastTimeMs;

		// reliable UDP가 너무 오랫동안 resend만 하다가 실패 판정해서 릴레이로 전환되어야 할지 여부 판정에 쓰임.
		int64_t m_maxResendElapsedTimeMs;

		// 같은 값의 ack를 중복해서 연속으로 받은 횟수. duplicated ack 감지 후 fast retransmit을 하는 용도.
		int m_dupAckReceivedCount;

		// 이미 받았던 data frame을 또 받은 경우를 센다.
		// spurious RTO를 감지하기 위함.
		int m_dupDataReceivedCount;
		int64_t m_dupDataReceivedCount_LastClearTimeMs;

		//////////////////////////////////////////////////////////////////////////
		// 데이터 수신을 위해 쓰이는 것들

		// data frame들로부터 stream으로 조립된 결과물. 이 안에 메시지들이 들어있다.
		CStreamQueue m_receivedStream;

		// UDP socket에서 꺼내긴 했지만 아직 스트림으로 조립되지 않고 대기중인 프레임들. 즉 receiver window이다.
		CFastList2<ReliableUdpFrame, int, CPNElementTraits<ReliableUdpFrame> > m_receiverWindow;

		// 이번에 data frame을 받아야 한다면, 그 번호
		int m_expectedFrameNumberToReceive;

		// 마지막으로 ack를 보낸 시간
		int64_t m_delayAckSentLastTimeMs;

		// true이면 언젠가 delayed ack를 쏴야 함
		bool m_mustSendAck;

		//////////////////////////////////////////////////////////////////////////
		// 통계

		// 수신 윈도에 들어있는 프레임 갯수, 아직 뽑아내지 않은 스트림 크기,
		int m_receivedFrameCount, m_receivedStreamCount;

		// 총 ack 받은 횟수
		int m_totalAckFrameCount;

		// 여지껏 보낸 스트림 크기
		int m_totalSendStreamLength;

		//여지껏 받은 스트림 크기
		int m_totalReceivedStreamLength;

		int m_totalResendCount;
		int m_totalFirstSendCount;
		int m_totalReceivedDataFrameCount;
		int m_totalAckFrameReceivedCount;
		int m_lastReceivedAckNumber;
		bool m_preventSpuriousRto;	// true이면, 수신측에서 spurious rto 경고를 보낸 상태이며, 따라서 rtt기반 rto를 충분히 크게 잡도록 한다.

#ifdef _DEBUG
		/*struct m_ackLog
		{
		int m_number;
		double m_time;
		bool m_pureAck;
		};
		CFastArray<m_ackLog> m_lastReceivedAckNumbers;

		struct DataLog
		{
		int m_number;
		double m_time;
		};
		CFastArray<DataLog> m_lastReceivedDataFrames;*/
#endif
		// slow-start threshold. sender window 크기는 매 텀마다 두배씩 증가하지만 이 값을 넘어가면 그때부터는 선형 증가를 한다.
		int				m_ssThresh;

		// ssthresh 값이 정해져 있으면 true
		bool			m_ssThreshValid;

		// 마지막으로 ssthresh가 설정된 시간. 10초마다 갱신되지 못하게 하기 위함.
		int64_t			m_ssThreshChangedTimeMs;

	public:
		ReliableUdpHost(CRemotePeer_C *OwnerRemotePeer, int firstFrameNumber);

		void InitSSThresh();
		void ProcessReceivedFrame(ReliableUdpFrame &frame);
		CStreamQueue* GetReceivedStream();
		void Send(const uint8_t* stream, int count);
		void Heartbeat();


		void DoForLongInterval();
		void GetStats(ReliableUdpHostStats &ret);
		int64_t GetMaxResendElapsedTimeMs();

		int AllStreamToSenderWindowAndYieldFrameNumberForRelayedLongFrame();

	private:
		void ProcessAckFrame( ReliableUdpFrame & frame );
		int RemoveFromSenderWindowBeforeExpectedFrame( int ackFrameNumber );
		void ProcessDataFrame( ReliableUdpFrame & frame );
		bool ReceiverWindow_AddFrame( ReliableUdpFrame &frame );
		void SequentialReceiverWindowToStream();
		void AllStreamToSenderWindowAndPiggybagAck( int64_t currTime );
	private:
		void StreamToSenderWindow(bool ignoreSenderWindowMaxLength);
	public:
		void DataFrame_PiggybagAck( SenderFrame &frame, int64_t currTime );
		int64_t GetRetransmissionTimeout();
		bool MaySpuriousRto();
	};
}
