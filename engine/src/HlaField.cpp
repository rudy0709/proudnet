#include "stdafx.h"
#include "../include/HlaField.h"
#if !defined(_WIN32)
    #include <new>
#endif

namespace Proud
{
	// 필드 그룹에 바뀐게 있으면 필드그룹 ID를 시리얼라이즈하고 변경 플래그 리셋을.
	// 아무것도 안하면 false 리턴.
	// 함수 이름이 이모양인건 엔진 유저가 알 바 아니므로.
	bool CHlaFieldGroup::DoSomething( HlaFieldGroupID fgID, CMessage& output )
	{
		if(m_valueChanged_INTERNAL)
		{
			output.Write(fgID);
			m_valueChanged_INTERNAL = false;

			return true;
		}

		return false;
	}

}
