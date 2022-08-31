#pragma once

#include "../include/LambdaWrap.h"
#include "../include/ReceivedMessage.h"
#include "LocalEvent.h"
#include "../include/NetPeerInfo.h"
#include "FastList2.h"

namespace Proud
{
	using namespace std;

	class CReferrerHeart;

	enum FinalUserWorkItemType
	{
		UWI_LocalEvent,
		UWI_RMI,
		UWI_UserMessage,
		UWI_Hla,		// HLA 관련 작업. HLA 관련 작업은 user worker thread에서 해야 함. net io thread에서 하면 성능에 민감한데다 사용자가 원치 않는 시점에서의 HLA 세션 변화가 있으니 즐.
		UWI_UserFunction, // 사용자가 RunAsync()로 넣은 함수나 람다식을 실행한다.
		UWI_LAST,
	};

	FinalUserWorkItemType GetWorkTypeFromMessageHeader(CMessage& msg);

	class CFinalUserWorkItem_Internal
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

		// 주의: 실존 객체이므로 조건부 컴파일 구문으로 감싸지 말 것! 찾기 힘든 버그였다!
		RefCount<LambdaBase_Param0<void> > m_userFunction;

		/* Local event 타입인 경우, 여기에 내용물이 채워진다.
		code profile 결과 CFinalUserWorkItem의 복사 비용이 커서 ptr type으로 변경되었다. */
		shared_ptr<LocalEvent> m_event;

		// 쌓인 아이템이 있는데 이미 netCoreHeart가 사라져 있으면 user worker thread에서의 처리를 하지 못한다.
		// 이를 예방하기 위해 this도 이것을 쥔다.
		shared_ptr<CReferrerHeart> m_netCoreReferrerHeart;

		//////////////////////////////////////////////////////////////////////////
		// called by BiasManagedPointer

		void Clear()
		{
			m_event.reset();
			m_type = UWI_LAST;
			m_unsafeMessage.Clear();
			m_userFunction.reset();
			m_netCoreReferrerHeart.reset();
		}

		void SuspendShrink()
		{
			// do nothing
		}

		void OnRecycle()
		{
			m_unsafeMessage.Clear();
		}

		void OnDrop()
		{
			Clear();
		}

		PROUD_API void ModifyForLoopback();
	};

	// ProudNet internal layer를 거쳐, 사용자가 처리해야 하는 task. RMI, local event, user message 등이다.
	class CFinalUserWorkItem :public BiasManagedPointer<CFinalUserWorkItem_Internal, true>
	{
	public:
		inline CFinalUserWorkItem() {}
		CFinalUserWorkItem(LocalEvent& e);

		/* 구 코드를 쉽게 변화하기 위해 추가된 함수.
		내부 실체 내용물이 처음에는 할당조차 안 되어 있는데,
		이를 액세스하려고 하면, 최초 1회 생성을 한 후
		그것의 리퍼런스를 리턴하는 역할을 한다. */
		inline CFinalUserWorkItem_Internal& Internal()
		{
			// just-in-time tombstone creation.
			InitTombstoneIfEmpty();

			return GetTombstone()->m_substance;
		}

		inline bool HasNetCoreHeart() const
		{
			return GetTombstone()
				&& GetTombstone()->m_substance.m_netCoreReferrerHeart;
		}
	};

	typedef CFinalUserWorkItem::Tombstone CFinalUserWorkItem_Tombstone;

	typedef CFastList2<CFinalUserWorkItem, int, CPNElementTraits<CFinalUserWorkItem> > CFinalUserWorkItemList;
}
