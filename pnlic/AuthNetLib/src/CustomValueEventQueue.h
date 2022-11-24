#pragma once

#include "enumimpl.h"

namespace Proud
{
	class IThreadReferrer;

	class CCustomValueEvent
	{
	public:
		// 이 이벤트를 처리하는 주체. IThreadReferrer의 함수가 호출됨.
		IThreadReferrer* m_referrer;
		CustomValueEvent m_customValue;
	};
}
