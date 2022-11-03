#include "stdafx.h"
#include "HlaSpaceImpl_S.h"


namespace Proud 
{
	CHlaSpace_S::CHlaSpace_S(void)
	{
		m_internal = new CHlaSpaceInternal_S;
	}

	CHlaSpace_S::~CHlaSpace_S(void)
	{
		delete m_internal;
	}

}