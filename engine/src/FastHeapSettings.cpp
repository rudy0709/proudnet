#include "stdafx.h"
#include "../include/FastHeapSettings.h"
#include "../include/FastHeap.h"

namespace Proud
{
	CFastHeapSettings::CFastHeapSettings()
	{
		m_pHeap = NULL;
		m_accessMode = FastHeapAccessMode_MultiThreaded;
		m_debugSafetyCheckCritSec = NULL;
	}
}
