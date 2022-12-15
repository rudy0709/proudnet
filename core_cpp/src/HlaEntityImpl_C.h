#pragma once 

#include "../include/HlaEntity_C.h"

namespace Proud 
{
	// 엔진 유저에게 내부 구조를 안 알리기 위함
	class CHlaEntityInternal_C
	{		
	public:
		HlaEntityID m_instanceID;
		HlaClassID m_classID;
		CHlaSessionHostImpl_C* m_ownerSessionHost;

		// 이 안에 있는 필드가 로컬에 의해 하나라도 변경되었으면 true.
		// 변경되었으면 noti change가 타 호스트들에게 전송되어야 함을 의미.
		bool m_isDirty;	

		CHlaEntityInternal_C()
		{
			m_isDirty = false;
		}
	};
}