#pragma once
#ifdef USE_HLA
#include "../include/HlaEntity_S.h"

namespace Proud
{
	// 엔진 유저에게 내부 구조를 안 알리기 위함
	class CHlaEntityInternal_S
	{
	public:
		HlaEntityID m_instanceID;
		HlaClassID m_classID;
		CHlaSessionHostImpl_S* m_ownerSessionHost;
		CHlaSpace_S* m_enteredSpace; // 소속된 space. 무소속시 NULL.

		// 이 안에 있는 필드가 로컬에 의해 하나라도 변경되었으면 true.
		// 변경되었으면 noti change가 타 호스트들에게 전송되어야 함을 의미.
		bool m_isDirty;
		Proud::Position m_iterInDirtyList; // 위 값이 true일때만 유효

		CHlaEntityInternal_S()
		{
			m_isDirty = false;
			m_enteredSpace = NULL;
		}
	};
}
#endif
