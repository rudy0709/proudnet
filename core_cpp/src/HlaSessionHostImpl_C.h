#pragma once

#include "../include/CriticalSect.h"
#include "../include/HlaEntity_C.h"

namespace Proud
{
	class CHlaEntityManagerBase_C;
	class CReceivedMessage;
	class IHlaDelegate_C;

	// 넷클섭,랜클섭 자체가 구현하는 클래스. 유저에게는 비노출.
	class IHlaSessionHostImplDelegate_C
	{
	public:
		~IHlaSessionHostImplDelegate_C() {}
		virtual bool AsyncCallbackMayOccur() = 0;
	};

	// HLA 클라 메인 객체. 넷클라,랜클라가 직접 소유하고, 사용한다.
	// HLA 기능이 넷랜클라에 직접 구현되면 코딩이 번거로와지므로 이렇게 별도 객체로 분리했다.
	// 따라서, 사용자가 직접 사용 불가.
	// CNetCoreImpl처럼 다중 상속은 비 C++에서 사용 불가이므로 이렇게 aggregation model로 만들어야.
	class CHlaSessionHostImpl_C
	{
		IHlaSessionHostImplDelegate_C* m_implDg;
		IHlaDelegate_C* m_userDg;
		CFastArray<CHlaEntityManagerBase_C*> m_entityManagerList_NOCSLOCK;
	public:
		CHlaEntities_C m_entities;

		CHlaSessionHostImpl_C(IHlaSessionHostImplDelegate_C* dg);
		~CHlaSessionHostImpl_C(void);

		void HlaSetDelegate( IHlaDelegate_C* dg );
		void HlaAttachEntityTypes(CHlaEntityManagerBase_C* entityManager);

		CHlaEntity_C* HlaCreateEntity(HlaClassID classID, HlaEntityID entityID);
	private:
		void UnregisterEntity(HlaEntityID entityID);
	public:
		void HlaFrameMove();
		void ProcessMessage( CReceivedMessage& receivedMessage );
	private:
		void ProcessMessageType_Appear( CReceivedMessage&  receivedMessage );
		void ProcessMessageType_Disappear( CReceivedMessage&  receivedMessage );
		void ProcessMessageType_NotifyChange( CReceivedMessage&  receivedMessage );
	public:
		void Clear();

	};

	extern const PNTCHAR* HlaUninitErrorText;
}
