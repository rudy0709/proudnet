
#pragma once
#ifdef USE_HLA

#include "../include/CriticalSect.h"
#include "../include/HlaEntity_S.h"
#include "../include/FastSet.h"
#include "../include/RecycleNumerFactoryT.h"
#include "FastMap2.h"
#include "FastList2.h"

namespace Proud
{
	class CHlaSpace_S;
	class CHlaEntity_S;
	class CHlaEntityManagerBase_S;
	class CSendFragRefs;
	class CReceivedMessage;
	class IHlaDelegate_S;

	struct SendOpt;

	// 넷클섭,랜클섭 자체가 구현하는 클래스. 유저에게는 비노출.
	class IHlaSessionHostImplDelegate_S
	{
	public:
		~IHlaSessionHostImplDelegate_S() {}
		virtual bool AsyncCallbackMayOccur() = 0;
		virtual int64_t GetTimeMs() = 0; // 현재 시간을 얻음

		// IRmiHost와 같은 모양이어야
		virtual bool Send(const CSendFragRefs &sendData,const SendOpt& sendContext,const HostID *sendTo, int numberOfsendTo ,int &compressedPayloadLength) = 0;
	};

	class IHlaViewer_S
	{
	public:
		virtual ~IHlaViewer_S() {}
		virtual HostID GetHostID() = 0;
		virtual CFastSet<CHlaSpace_S*>& GetViewingSpaces() = 0;
	};

	// HLA 서버 메인 객체. 넷서버,랜서버가 직접 소유하고, 사용한다.
	// HLA 기능이 넷랜서버에 직접 구현되면 코딩이 번거로와지므로 이렇게 별도 객체로 분리했다.
	// 따라서, 사용자가 직접 사용 불가.
	// CNetCoreImpl처럼 다중 상속은 비 C++에서 사용 불가이므로 이렇게 aggregation model로 만들어야.
	class CHlaSessionHostImpl_S
	{
		friend CHlaEntity_S;

		IHlaSessionHostImplDelegate_S* m_implDg;
		IHlaDelegate_S* m_userDg;

		CFastArray<CHlaEntityManagerBase_S*> m_entityManagerList_NOCSLOCK;
		CFastMap2<HostID, IHlaViewer_S*,int> m_viewers;
		CFastMap2<CHlaSpace_S*,int,int> m_spaces;

		// 로컬에서 값을 바꾸어서, 상태 변경이 상대들에게 전송되어야 하는 개체들의 집합
		CFastList2<CHlaEntity_S*,int> m_dirtyEntities;

		CRecycleNumberFactoryT<HlaEntityID> m_entityIDGen;

		void SendAppearMessage(HostID* sendTo, int sendToCount, CHlaEntity_S* entity);
		void SendDisappearMessage(HostID* sendTo, int sendToCount, HlaEntityID entityID);

	public:
		CHlaEntities_S m_entities;


		CHlaSessionHostImpl_S(IHlaSessionHostImplDelegate_S* dg);
		~CHlaSessionHostImpl_S(void);

		void HlaSetDelegate( IHlaDelegate_S* dg );
		void HlaAttachEntityTypes(CHlaEntityManagerBase_S* entityManager);

		void HlaFrameMove();

		void CreateViewer( IHlaViewer_S * rc );
		void DestroyViewer( IHlaViewer_S * rc );
		
		CHlaSpace_S* HlaCreateSpace();
		void HlaDestroySpace(CHlaSpace_S* space);

		CHlaEntity_S* HlaCreateEntity(HlaClassID classID);
		void HlaDestroyEntity(CHlaEntity_S* Entity);
	
		void HlaViewSpace(HostID viewerID, CHlaSpace_S* space, double levelOfDetail);
		void HlaUnviewSpace(HostID viewerID, CHlaSpace_S* space);
		void EntityMoveToSpace( CHlaEntity_S* entity, CHlaSpace_S* space );

		void ProcessMessage( CReceivedMessage& receivedMessage );
	};

	extern const PNTCHAR* HlaUninitErrorText;
}

#endif