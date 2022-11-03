#include "stdafx.h"
#include "LoopbackHost.h"

namespace Proud
{
	CLoopbackHost::CLoopbackHost(HostID hostID)
	{
		m_HostID = hostID;
		m_backupHostID = HostID_None;
	}

	CLoopbackHost::~CLoopbackHost()
	{
	}

	HostID CLoopbackHost::GetHostID()
	{
		return m_HostID;
	}

}