#include "stdafx.h"
#include "VizMessageSummary.h"
#include "../include/Marshaler.h"

namespace Proud
{
#ifdef VIZAGENT
	CMessage& operator<<(CMessage& msg, const VizMessageSummary& a)
	{
		msg.Write(a.m_payloadLength);
		msg.Write(a.m_rmiID);
		msg.WriteString(a.m_rmiName);
		msg.Write(a.m_encryptMode);
		return msg;
	}
	CMessage& operator>>(CMessage& msg, VizMessageSummary& a)
	{
		msg.Read(a.m_payloadLength);
		msg.Read(a.m_rmiID);
		msg.ReadString(a.m_rmiName);
		msg.Read(a.m_encryptMode);
		return msg;
	}
	void AppendTextOut(String& text, VizMessageSummary& a)
	{
		String x;
		x.Format(_PNT("Summary: Length=%d,ID=%d,Name=%s,Encrypt=%d"),a.m_payloadLength,a.m_rmiID,a.m_rmiName.GetString(),a.m_encryptMode);

	}
#endif
}
