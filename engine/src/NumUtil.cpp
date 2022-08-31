/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <stack>
#include "NumUtil.h"

namespace Proud
{

	CRoundRobinNumberGenerator::CRoundRobinNumberGenerator()
	{
		m_number = 0;
	}

	/** returns one in [0,maxNumber) */
	int CRoundRobinNumberGenerator::Next( int maxNumber )
	{
		if (maxNumber == 0)
			return 0;
		m_number++;
		if (m_number < 0)
			m_number = 0;

		return m_number % maxNumber;
	}

	// 의도적 크래시 유발을 일으키는 내부 함수.
	void CauseAccessViolation()
	{
		// 크래쉬 유발
		int *a = 0;
		*a = 1;
	}
}
