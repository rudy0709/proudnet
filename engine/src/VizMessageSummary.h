#pragma once

#include "../include/MessageSummary.h"

namespace Proud
{
#ifdef VIZAGENT
	// MessageSummary와 별개로 쓴다. m_rmiName이 성능을 먹으므로.
	class VizMessageSummary
	{
	public:
		int m_payloadLength;
		RmiID m_rmiID;
		String m_rmiName;
		EncryptMode m_encryptMode;
		CompressMode m_compressMode;

		VizMessageSummary()
		{
			m_payloadLength = 0;
			m_rmiID = RmiID_None;
			m_encryptMode = EM_None;
			m_compressMode = CM_None;
		}

		VizMessageSummary(const MessageSummary& src)
		{
			m_payloadLength = src.m_payloadLength;
			m_rmiID = src.m_rmiID;
			m_rmiName = src.m_rmiName;
			m_encryptMode = src.m_encryptMode;
			m_compressMode = src.m_compressMode;
		}
	};

	CMessage& operator<<(CMessage& msg, const VizMessageSummary& a);
	CMessage& operator>>(CMessage& msg, VizMessageSummary& a);
	void AppendTextOut(String& text, VizMessageSummary& a);
#endif
}
