/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/
#pragma once

#include <cstdint>

namespace Proud
{
	// UDP 송신을 과량으로 발생시키면, 혼잡 붕괴가 일어난다.
	// 이때 TCP도 멀쩡하던 것이 끊어지기도 함 (retransmission timeout)
	// 이 클래스는 혼잡 상황을 감지해서 송신 속도를 제어하는 역할을 한다.
	// 송신자에서 쓰임.
	// (스로틀링 기능 노트.pptx)
	class CSendBrake
	{
	private:
		// 송신을 할 때마다 증가, 시간이 지날 때마다 지정된 속도만큼 감소.
		// send brake를 할지 말지를 판단하는 단서가 되는 값.
		int64_t m_sendBrakeGauge;

		// 최종 계산된, "최대 허락되는 송신 속도"
		// 혼잡 상황시=>실제 수신자가 받고 있는 수신 속도에 근거해서 낮아짐.
		// 원활 상황시=>시간이 지날수록 높아짐.
		int64_t m_controlledMaxSendSpeed;

		// 수신자 측에서 보내준, 실제 자기가 수신한 속도와 패킷 로스율
		int64_t m_recentReceiveSpeed;
		//int m_packetLossPercent;

		// true이면, 송신이 너무 빠르니 억제하라.
		// 매 heartbeat마다 갱신된다.
		bool m_doBrake;
		// TCP 핑퐁이 매우 랙이 심한, 즉 패킷 로스가 일어나고 있는 상황이면 true
		// 필요 이유: P2P 패킷 로스가 전혀 없는데도 C/S TCP가 위험한 상황 때문
		// (재현법: P2P가 5km 이내이고 서버와 100km 이상 거리에서 P2P 통신량이 한계에 근접)
		bool m_isTcpUnstable;

		// 마지막 체크업에서 패킷 로스가 심각하게 일어나고 있는 상황인지?
		bool m_isPacketLossOccurring;

		int64_t m_lastLongIntervalWorkTime;
		// SetValue를 호출했던 시간
		int64_t m_lastSetPacketLossValueTime;
		// UDP가 unstable해졌던 시간
		int64_t m_lastSetTcpUnstableValueTime;

	public:
		CSendBrake();

		void Accumulate(int byteCount/*, int64_t curTime*/);

		void DoForLongInterval(int64_t curTime);
		bool BrakeNeeded() { return m_doBrake; }
		void SetReceiveQuality(int64_t receiveSpeed, int packetLossPercent, int64_t curTime);

		void SetTcpUnstable(int64_t curTime, bool unstable);
		void CongestionControl(int64_t curTime, int64_t* lastSetValueTime, bool doControl);

		int64_t GetMaxSendSpeed() { return m_controlledMaxSendSpeed; }
	};

	// 송신자가 보낸 UDP 패킷들을 실제로 수신하고 있는 속도를 측정하는 역할.
	// 송신자가 혼잡 상황에서 혼잡 제어를 위한 단서로 쓰임.
	// 수신자에서 쓰임.
	class CRecentSpeedMeasurer
	{
		// 최근 송수신속도 (바이트 단위)
		int64_t m_recentSpeed;
		int64_t m_lastIntervalTotalBytes;
		int64_t m_lastLongIntervalWorkTime;
		int64_t m_lastAccumulateTime;

	public:
		CRecentSpeedMeasurer()
		{
			m_lastIntervalTotalBytes = 0;
			m_lastLongIntervalWorkTime = 0;
			m_recentSpeed = 0;
			m_lastAccumulateTime = 0;
		}

		bool IsRemovingSafeForCalcSpeed(int64_t curTime);

		void TouchFirstTime(int64_t curTime)
		{
			m_lastAccumulateTime = curTime;
		}

		void Accumulate(int byteCount, int64_t curTime)
		{
			m_lastIntervalTotalBytes += byteCount;
			m_lastAccumulateTime = curTime;
		}
		void DoForLongInterval(int64_t curTime);

		int64_t GetRecentSpeed()
		{
			return m_recentSpeed;
		}
	};
}