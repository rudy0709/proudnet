#pragma once

#include "../include/ReceivedMessage.h"
#include "LocalEvent.h"
#include "../include/NetPeerInfo.h"
#include "FastList2.h"

namespace Proud 
{
	enum FinalUserWorkItemType
	{
		UWI_LocalEvent,
		UWI_RMI,
		UWI_UserMessage,
		UWI_Hla,		// HLA 관련 작업. HLA 관련 작업은 user worker thread에서 해야 함. net io thread에서 하면 성능에 민감한데다 사용자가 원치 않는 시점에서의 HLA 세션 변화가 있으니 즐.
		UWI_LAST,
	};

	FinalUserWorkItemType GetWorkTypeFromMessageHeader(CMessage& msg);

	// ProudNet internal layer를 거쳐, 사용자가 처리해야 하는 task. RMI, local event, user message 등이다.
	class CFinalUserWorkItem
	{
	public:
		// 처리할 작업의 종류
		FinalUserWorkItemType m_type;
		// RMI이거나 user message인 경우 아직 해석하기 전의 데이터
		// MessageType은 빠져있음
		// 수신자, 수신될때 사용된 프로토콜 등이 저장되어 있다.
		// 과거 버전에서 fast heap을 외부 참조하기 때문에 객체 파괴시 main 객체를 잠그고 그 안의 fast heap access를 해야 했지만
		// 이제는 그런 기능이 사라지고 전역 obj-pool 방식을 쓰기 때문에 unsafe가 아님.
		CReceivedMessage m_unsafeMessage; 

		// 종류가 local event인 경우 그것의 type
		LocalEvent m_event;

		CFinalUserWorkItem();
		CFinalUserWorkItem(CReceivedMessage& msg, FinalUserWorkItemType type, bool stripMessageTypeHeader = false);
		CFinalUserWorkItem(LocalEvent& e);
	};

	class CFinalUserWorkItemList:public CFastList2<CFinalUserWorkItem, int, CPNElementTraits<CFinalUserWorkItem> > 
	{
	public:
	};
}