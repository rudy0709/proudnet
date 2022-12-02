/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include "TimeAlarm.h"
#include "../include/Exception.h"

namespace Proud
{
	// 생성자
	CTimeAlarm::CTimeAlarm()
	{
		m_timeToDo = 0;
		m_interval = 0;
	}

	// 초기 interval을 받는다. 이떄 cooltime도 임의 설정된다.
	CTimeAlarm::CTimeAlarm( int64_t alarmInterval )
	{
		m_timeToDo = 0;
		m_interval = alarmInterval;
	}

	bool CTimeAlarm::IsTimeToDo(int64_t currentTime)
	{
		if (m_timeToDo != 0)
		{
			if(currentTime >= m_timeToDo)
			{
				// 너무 오랫동안 isTimeToDo가 호출되지 않은 경우 중복해서 퐈봐봑 work를 하는 사태를 막기 위해.
				m_timeToDo = currentTime + m_interval;
				return true;
			}
			return false;
		} 
		else
		{
			Reset(currentTime);
			return false;
		}
	}

	// interval을 조정한다. 이때, cooltime도 새 interval에 따라 조정된다.
	void CTimeAlarm::SetIntervalMs( int64_t interval )
	{
		if (interval <= 0)
			ThrowInvalidArgumentException();

		// 신구 interval의 비율에 따라 현재 쿨타임 또한 조정한다.
		//m_coolTime = (m_coolTime * interval) / m_interval;
		int64_t oldInterval = m_interval;

		// 마지막에 action을 한 시간 기준으로 새 time-to-do를 설정한다.
		// 이게 빠져 있으면 interval이 짧아질 때 첫 time-to-do가 여전히 긴 값이 되는데,
		// 이것 때문에 클라가 서버와의 재접속 후 ping-pong이 없어서 연결 끊어짐 문제가 발생한다.
		int64_t lastDoTime = m_timeToDo - oldInterval;
		m_timeToDo = lastDoTime + interval;

		// 마무리
		m_interval = interval;
	}

	// 한참동안 TakeElapsedTime를 콜 안했던 상태에서 처음부터 다시 사용하고자 한다면 이걸 콜 하라.
	void CTimeAlarm::Reset(int64_t currentTime)
	{
		// 랜덤값 대신 30%를 넣은 것임.
		m_timeToDo = currentTime + (m_interval * 3 / 10);
	}
}