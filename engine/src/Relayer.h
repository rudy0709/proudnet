/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#pragma once

#include "ReliableUDPFrame.h"
#include "../include/FakeClr.h"
#include "../include/Enums.h"
#include "P2PGroup_C.h"



namespace Proud
{
	using namespace std;

	class CRemotePeer_C;

	// 릴레이 되는 메시지에 들어가는, '릴레이 최종 수신자 리스트'의 각 항목
	struct RelayDest
	{
		HostID m_sendTo;
		// p2p reliable message인 경우 reliable UDP layer에서 사용되는 frame number.
		// 릴레이는 TCP로 이루어지므로 수신측에 reliable하게 가짐. 따라서 수신측은 ack를 보내지 말고, 송신측도 frame을 보낸 후 바로 ack를 받았다는 가정하의 처리를 해야 한다.
		int m_frameNumber;
	};

	class RelayDestList : public CFastArray < RelayDest, true, false, int >
	{
	};

	class CompressedRelayDestList_C
	{
	public:
		CFastMap2<HostID, P2PGroupSubset_C, int> m_p2pGroupList;
		HostIDArray m_includeeHostIDList;			// 어떠한 p2p 그룹에도 등록되어 있지 않은 개별 host id list

		void AddSubset(const HostIDArray& subsetGroupHostID, HostID hostID);	// 제외되어야할 hostid list에 추가한다.
		void AddIndividual(HostID hostID);								// individual list에 추가한다.
		int GetAllHostIDCount();						// 모든 HostID의 갯수 ( 그룹 호스트ID까지 포함됨 )

		CompressedRelayDestList_C();

		// POOLED_LOCAL_VAR의 scope out 시 호출되는 함수
		void ClearAndKeepCapacity()
		{
			// object pool에 의해 재사용되지만, 내용물을 모두 청소할 필요가 있다.
			m_p2pGroupList.Clear(); // CFastMap2이므로 hash 등은 그대로 메모리에 남으므로 heap access 횟수를 줄인다.
			m_includeeHostIDList.Clear();
		}

		// called by CObjectPool.
		void SuspendShrink()
		{
			m_includeeHostIDList.SuspendShrink();
		}
		void OnRecycle() {}
		void OnDrop() {
			ClearAndKeepCapacity();
		}

	};

	// 주의: FastArray에 의해 사용되므로 생성자,파괴자,복사 연산자가 불필요해야 한다!!
	class RelayDest_C
	{
	public:
		shared_ptr<CRemotePeer_C> m_remotePeer;
		int m_frameNumber;
	};

	class RelayDestList_C : public CFastArray < RelayDest_C, true, false, int >
	{
	public:
		void ToSerializable(RelayDestList& ret);
	};
}
