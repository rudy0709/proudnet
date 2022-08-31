#pragma once

#include "stdafx.h"
#include "../include/BasicTypes.h"

namespace Proud
{
	class CPnIconv
	{
	public:
		void* m_cd;

		CPnIconv();
		~CPnIconv();

		bool InitializeIconv(const char* src, const char* dest);

		// called by CObjectPool. does nothing.
		void SuspendShrink() {}
		void OnRecycle() {}
		void OnDrop()
		{
			// CPnIconv는 한 종류의 converter에 의해서 계속 핸들이 재사용되는 목적으로 obj-pool을 한다.
			//따라서 여기서 iconv 핸들 제거를 하면 안된다.
		}
	};
}
