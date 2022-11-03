/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <new>
#include "FinalUserWorkItem.h"


namespace Proud 
{
	// 메시지의 맨 껍데기 헤더를 분석해서 RMI, 사용자 정의 메시지, 아니면 뭔지 체크한다.
	Proud::FinalUserWorkItemType GetWorkTypeFromMessageHeader( CMessage& msg )
	{
		CMessage reader;
		reader.UseExternalBuffer(msg.GetData(),msg.GetLength());
		reader.SetLength(msg.GetLength());
		
		MessageType msgType; 
		if(reader.Read(msgType))
		{
			switch((int)msgType)
			{
			case MessageType_Rmi:
				return UWI_RMI;
			case MessageType_UserMessage:
				return UWI_UserMessage;			
			case MessageType_Hla:
				return UWI_Hla;			
			}
		}
		// 못찾겠다꾀꼬리
		return UWI_LAST;
	}

	// Loopback일 경우에 사용합니다.
	CFinalUserWorkItem::CFinalUserWorkItem(CReceivedMessage& msg, FinalUserWorkItemType type, bool stripMessageTypeHeader /*= false*/) :
		m_unsafeMessage(msg)
	{
		// loop back 메시지의 경우 메시지 헤더를 제거해 주어야 한다. (_MessageType enum 제거.)
		if (stripMessageTypeHeader && m_unsafeMessage.m_unsafeMessage.m_msgBuffer.GetCount() >= CMessage::MessageTypeFieldLength)
		{
			m_unsafeMessage.m_unsafeMessage.m_msgBuffer.RemoveRange(0, CMessage::MessageTypeFieldLength);
		}
		m_type = type;
	}

	CFinalUserWorkItem::CFinalUserWorkItem(LocalEvent& e) :m_type(UWI_LocalEvent), m_event(e)
	{

	}

	CFinalUserWorkItem::CFinalUserWorkItem()
	{
		m_type = UWI_LAST;
	}

}