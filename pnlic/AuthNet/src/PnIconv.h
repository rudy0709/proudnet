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
	};
}
