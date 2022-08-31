/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#ifdef _MSC_VER
#pragma intrinsic (pow)
#endif

#include "stdafx.h"
// xcode는 STL을 cpp에서만 include 가능 하므로
#include <new>
#include <stack>
#include <cstring>
#include <climits>
#include <cstdlib>
#include <cstddef>
#include <iosfwd>
#include <math.h>

#include "../include/NetConfig.h"
#include "../include/numutil.h"
#include "../include/ErrorInfo.h"
#include "../include/IRmiHost.h"
#include "SendBrake.h"

namespace Proud
{
	// 데이터 전송을 했다. 송신한 량을 카운팅한다.
	// 너무 과다하게 보내지면 브레이크 걸리는 근원임.
	void CSendBrake::Accumulate(int byteCount/*, int64_t curTime*/)
	{
		m_sendBrakeGauge += byteCount;
	}

	void CSendBrake::DoForLongInterval(int64_t curTime)
	{
		if (m_lastLongIntervalWorkTime == 0)
		{
			m_lastLongIntervalWorkTime = curTime;
		}

		// 속도 무제한이면 항상 0
		//		if (m_controlledMaxSendSpeed == INT64_MAX)
		//		{
		//			m_sendBrakeGauge = 0;
		//		}
		//		else  //속도 유제한이면
		//		{
		// 지난 시간만큼 send brake gauge를 감소시킨다.
		// 시간은 밀리초 단위이므로 속도를 곱한 후 만들어지는 큰 값을 먼저 구한다. 이 값이 int64_max의 1/1000이 되려면
		// 현존 인터넷보다 훨씬 빠른 인터넷이 나와야 하므로 오버플로우는 걱정 말자.
		int64_t lastInterval = curTime - m_lastLongIntervalWorkTime;

		//지난 시간만큼 보류 양을 감소
		// (속도*밀리초) 연산을 한 후에 /1000을 해야 자리수 잘라먹기를 하지 않는다.
		int64_t decrementValue = (m_controlledMaxSendSpeed * lastInterval) / 1000;
		if (decrementValue >= 0)
		{
			m_sendBrakeGauge = m_sendBrakeGauge - decrementValue;
			if (m_sendBrakeGauge < 0)
				m_sendBrakeGauge = 0;
		}
		else
		{
			// 너무 큰 값이라 음수가 된 것이면 그냥 브레이크 걸지 말자.
			m_sendBrakeGauge = 0;
		}
		//		}

		// 수신속도를 받은 적이 없거나 속도가 무제한이면 제한을 걸지 않는다.
		if (m_recentReceiveSpeed == 0 || m_controlledMaxSendSpeed == INT64_MAX)
		{
			m_doBrake = false;
		}
		else
		{
			if (m_sendBrakeGauge == 0)
				m_doBrake = false;
			else
				m_doBrake = true;
		}

		m_lastLongIntervalWorkTime = curTime;
	}

	CSendBrake::CSendBrake()
	{
		m_sendBrakeGauge = 0;
		m_controlledMaxSendSpeed = INT64_MAX;
		m_recentReceiveSpeed = 0;

		m_lastLongIntervalWorkTime = 0;
		m_lastSetPacketLossValueTime = 0;
		m_lastSetTcpUnstableValueTime = 0;
		m_doBrake = false;
		m_isTcpUnstable = false;
		m_isPacketLossOccurring = false;
	}

	// 수신자로부터 받은 실제 수신 속도와 패킷 로스율을 받아, 최대 허용송신속도를 결정한다.
	void CSendBrake::SetReceiveQuality(int64_t receiveSpeed, int packetLossPercent, int64_t curTime)
	{
		if (m_lastSetPacketLossValueTime == 0)
		{
			m_lastSetPacketLossValueTime = curTime;
		}

		// 이미 TCP가 불안정한 상황으로 제한를 받고 있다면 무시.
		if (m_isTcpUnstable)
			return;

		m_recentReceiveSpeed = receiveSpeed;
		//m_packetLossPercent = packetLossPercent;

		// 값이 갱신되는 시점과 m_controlledMaxSendSpeed 값을 구하는 시점을 같게 하기위해 여기서 계산한다.
		bool isPacketLossOccurring = packetLossPercent > CNetConfig::UdpCongestionControl_MinPacketLossPercent;

		CongestionControl(curTime, &m_lastSetPacketLossValueTime, isPacketLossOccurring);

		m_isPacketLossOccurring = isPacketLossOccurring;
	}

	// TCP가 위험 상황 혹은 아닐때의 처리
	void CSendBrake::SetTcpUnstable(int64_t curTime, bool unstable)
	{
		if (m_lastSetTcpUnstableValueTime == 0)
		{
			m_lastSetTcpUnstableValueTime = curTime;
		}

		// 패킷로스율로 이미 제한을 받고 있다면 무시
		if (m_isPacketLossOccurring)
			return;

		CongestionControl(curTime, &m_lastSetTcpUnstableValueTime, unstable);

		// 이제 처리했으니 값 세팅
		m_isTcpUnstable = unstable;
	}

	// 혼잡 제어
	// control: 혼잡 상황이므로 속도 제어를 켜야 한다면 true
	// curTime: 현재 시간. 이 값을 굳이 인자로 받는 이유는 GetPreciseCurrentTime의 contention을 줄이기 위해서다.
	// lastSetValueTime: 마지막에 CongestionControl를 실행한 시간. 이 값은 자동으로 현재 시간으로 설정된다.
	// doControl: 혼잡제어가 필요한 상황 즉 혼잡 상황이 발생중이면 true이다.
	void CSendBrake::CongestionControl(int64_t curTime, int64_t* lastSetValueTime, bool doControl)
	{
		// 혼잡 제어 기능을 꺼 놨으면, 아래 pow 연산까지 가지 않는다. 일부 SSE가 안되는 VM때문에.
		if (!CNetConfig::EnableSendBrake)
		{
			m_controlledMaxSendSpeed = INT64_MAX;
			return;
		}

		// 혼잡 제어가 발생하고 있으면
		if (doControl)
		{
			// 혼잡 상황. 송신제한 속도는 실제 수신 속도의 80% 로 설정
			m_controlledMaxSendSpeed = (m_recentReceiveSpeed * 8) / 10;

			// m_recentReceiveSpeed는 int64 overflow할 가능성 없지만 최악의 사태를 위해
			if (m_controlledMaxSendSpeed < 0)
			{
				m_controlledMaxSendSpeed = INT64_MAX;
			}

			// 최소 송신속도는 유지하도록 한다.
			m_controlledMaxSendSpeed = PNMAX(CNetConfig::MinSendSpeed, m_controlledMaxSendSpeed);
		}
		else // 혼잡 아니면
		{
			if (m_controlledMaxSendSpeed != INT64_MAX)
			{
				// 혼잡이 없는 상황이라면 현재 송신 제한 속도와 실제 수신 속도 중 큰 값을 선택해서 그 값의 2배로 설정
				m_controlledMaxSendSpeed = PNMAX(m_controlledMaxSendSpeed, m_recentReceiveSpeed);

				/* 대략 주기가 4초, 한번에 4배로 뛰도록 하자.
				pow를 쓰면 로직이 단순해 지지만
				일부 서버 VM에서 SSE 명령어가 크래시를 일으키므로 pow()를 쓰지 않는다. */
				double interval = (double)(curTime - *lastSetValueTime) / 1000;

				// Mac or iOS or android math.h에는 pow template 함수가 없습니다.
				double r = pow(2, interval);

				m_controlledMaxSendSpeed = int64_t(double(m_controlledMaxSendSpeed) * r);

				if (m_controlledMaxSendSpeed < 0)
					m_controlledMaxSendSpeed = INT64_MAX;

				// int64 범위 벗어나지 않도록.
			}
		}
		*lastSetValueTime = curTime;
	}

	void CRecentSpeedMeasurer::DoForLongInterval(int64_t curTime)
	{
		if (m_lastLongIntervalWorkTime == 0)
		{
			m_lastLongIntervalWorkTime = curTime;
			return;
		}

		// 최근 수신속도 산출
		int64_t lastInterval = curTime - m_lastLongIntervalWorkTime;

		// ikpil.choi 2017-01-09 : lastInterval 0일 경우, 정수 나누기를 방어 한다. (N3734)
		if (0 == lastInterval)
			return;

		int64_t lastSpeed = m_lastIntervalTotalBytes / lastInterval;
		m_recentSpeed = LerpInt(m_recentSpeed, lastSpeed, (int64_t)7, (int64_t)10);

		// 청소
		m_lastIntervalTotalBytes = 0;

		m_lastLongIntervalWorkTime = curTime;
	}

	bool CRecentSpeedMeasurer::IsRemovingSafeForCalcSpeed(int64_t curTime)
	{
		return (curTime - m_lastAccumulateTime > CNetConfig::UnreliablePingIntervalMs * 3);
	}
}
