/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#pragma once

#include "../include/BasicTypes.h"
#include "../include/pnstdint.h"

namespace Proud
{
	/* 쿨타임 체크 용도로 쓰인다.
	일반적 용도의 클래스는 아니다. ProudNet에서 자주 쓰는 유형으로 만들어져 있을 뿐이다. 
	
	일반적 용도: 
	생성자나 SetIntervalMs에서 처리 주기 인터벌 설정
	매번 IsTimeToDo를 호출해서 처리해야 하는 타이밍인지 얻어오고, 필요한 것을 처리 */
	class CTimeAlarm
	{
		int64_t m_timeToDo;
		int64_t m_interval;
	public:
		 PROUD_API CTimeAlarm();
		 PROUD_API CTimeAlarm(int64_t alarmInterval);

		 PROUD_API bool IsTimeToDo(int64_t currentTime);

		 void SetIntervalMs(int64_t interval);

		 void Reset(int64_t currentTime);
	};
}
