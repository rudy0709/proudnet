/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <new>
#include "../include/RMIContext.h"
#include "../include/NetConfig.h"
#include "../include/ReceivedMessage.h"
#include "../include/Message.h"
//#include "netcore.h"

namespace Proud
{
	// Reliable UDP의 resend starvation을 막으려면 이건 우선순위가 unreliable보다 높아야 한다.
	RmiContext RmiContext::ReliableSend (MessagePriority_High, MessageReliability_Reliable, 0, EM_None);  
	RmiContext RmiContext::UnreliableSend (MessagePriority_Medium, MessageReliability_Unreliable, 0, EM_None);
	
	RmiContext RmiContext::FastEncryptedReliableSend (MessagePriority_High, MessageReliability_Reliable, 0, EM_Fast);  
	RmiContext RmiContext::FastEncryptedUnreliableSend (MessagePriority_Medium, MessageReliability_Unreliable, 0, EM_Fast);
	
	RmiContext RmiContext::SecureReliableSend (MessagePriority_High, MessageReliability_Reliable, 0, EM_Secure);  
	RmiContext RmiContext::SecureUnreliableSend (MessagePriority_Medium, MessageReliability_Unreliable, 0, EM_Secure);

	RmiContext RmiContext::UnreliableS2CRoutedMulticast (MessagePriority_Medium, MessageReliability_Unreliable, CNetConfig::MaxS2CMulticastRouteCount);


	RmiContext GetReliableSendForPN(EncryptMode encryptMode = EM_None) 
	{
		RmiContext ret(MessagePriority_High, MessageReliability_Reliable, 0, encryptMode);
		ret.m_enableP2PJitTrigger = false;
		ret.m_INTERNAL_USE_isProudNetSpecificRmi = true;
		
		return ret;
	}
	RmiContext GetUnreliableSendForPN(EncryptMode encryptMode = EM_None)
	{
		RmiContext ret(MessagePriority_Medium, MessageReliability_Unreliable, 0, encryptMode);
		ret.m_enableP2PJitTrigger = false;
		ret.m_INTERNAL_USE_isProudNetSpecificRmi = true;
		return ret;
	}


	RmiContext g_ReliableSendForPN = GetReliableSendForPN();
	RmiContext g_UnreliableSendForPN = GetUnreliableSendForPN();
	
	RmiContext g_SecureReliableSendForPN = GetReliableSendForPN(EM_None);
	RmiContext g_SecureUnreliableSendForPN = GetUnreliableSendForPN(EM_None);

	void RmiContext::AssureValidation() const
	{
		if (m_reliability == MessageReliability_Unreliable)
		{
			if (m_priority < MessagePriority_High || m_priority > MessagePriority_Low)
			{
				m_priority = MessagePriority_Medium;

				/*char exceptonStr[200];
				sprintf_s(exceptonStr, "RMI messaging cannot have Engine level priority! (m_priority : %d)", (int)m_priority);
				::OutputDebugStringA(exceptonStr);
				
				throw Exception(exceptonStr);*/
			}
		}
	}

}
