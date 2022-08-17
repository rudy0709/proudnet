#include "stdafx.h"
// xcode는 STL을 cpp에서만 include 가능 하므로
#include <new>
#include <stack>
#include <cstring>
#include <climits>
#include <cstdlib>
#include <cstddef>
#include <iosfwd>

#include "PacketFrag.h"
#include "SendFragRefs.h"
#include "../include/MilisecTimer.h"
//#include "../../include/netinterface.h"
//#include "../include/errorinfo.h"
#include "LookasideAllocatorImpl.h"
#include "STLUtil.h"
#include "../include/IRmiHost.h"
#include "FragHeader.h"
#include "FavoriteLV.h"
#include "PooledObjectAsLocalVar.h"
#include "NetCore.h"


namespace Proud
{

#define PNASSURE(x) { if(!(x)) ShowUserMisuseError(StringA2T(#x)); }

	// 2bit에 사용됨
	const uint8_t FragSplitter = 0x01; 
	const uint8_t FullPacketSplitter = 0x02;

	double CTestStats::TestRecentReceiveSpeed = 0;
	double CTestStats::TestRecentSendSpeedAtReceiverSide = 0;

	// 송신큐가 채워져 있는 아무 항목을 얻는다.
	// 최대한 균등하게 기회를 줄 수 있어야 한다=>Pop을 한 후에는 Pop의 대상이 되는 PacketQueue 항목을 맨 뒤로 옮긴다.
	// calledBySendCompletion: caller가 send completion인가? => send completion에서의 issue는 혼잡 제어의 제약은 받지만 
	// coalesce의 제약을 받지는 말아야 한다.
	bool CUdpPacketFragBoard::PopAnySendQueueFilledOneWithCoalesce(
		CUdpPacketFragBoardOutput &output,
		int64_t currentTime,
		bool calledBySendCompletion )
	{
		AssertIsLockedByCurrentThread(m_owner->m_sendQueueCS);
		m_lastPopFragmentSurpressed_ValidAfterPopFragment = false;
		CPacketQueue* head = m_sendReadyList.GetFirst();
		while (head != NULL)
		{
			// head는 패킷큐가 비어있지 않다.
			if (head->IsEmpty())
			{
				m_curPacketQueue = NULL;
				throw Exception("Consistency failed in Packet Frag Board!");
			}

			assert(head->m_remoteAddr.IsUnicastEndpoint());

			// 패킷 큐가 안 비어있어도 보낼 타임이 아니면 skip하고 다음 것을 돈다.
			// caller가 send completion인 경우: coalesce는 검사 안함, send brake는 검사함
			// caller가 heartbeat인 경우: coalesce는 검사함, send brake는 검사함
			bool hasPacketToSendAndNotSurpressed = head->HasPacketAndTimeToSendReached(currentTime, calledBySendCompletion);

			// 보낼 수 있는 패킷은 있지만 coalesce나 send brake에 의해 surpress된 경우인지를 남긴다.
			m_lastPopFragmentSurpressed_ValidAfterPopFragment = !hasPacketToSendAndNotSurpressed;

			if (hasPacketToSendAndNotSurpressed)
			{
				head->NormalizePacketQueue();

				// 때가 됐다. pop it!
				head->PopFragmentOrFullPacket(currentTime, output);

				// 다음 coalesced send를 할 시간이 갱신되는 조건은, pop 직후 패킷큐가 비어버릴때 뿐이다.
				// 즉, pop을 한 후 empty가 되면 now+coal intv.
				if (head->IsEmpty())
				{
					head->m_nextTimeToCoalescedSend = currentTime + head->m_coalesceIntervalMs;
				}

#ifdef TEST_WANGBA
				if(output.m_fragHeader.inoutInt32PacketLength <= 0)
					OutputDebugString("Cue -D!");
				String txt;
				if(output.m_fragHeader.inSplitter == FullPacketSplitter)
				{
					txt.Format(_PNT("FullPacketSplitter is popped. packetLength=%d FragID=%d PacketID=%d \n"),
						output.m_fragHeader.inoutInt32PacketLength,
						output.m_fragHeader.outDwordFragID,
						output.m_fragHeader.inoutInt32PacketID);
					OutputDebugString(txt);
				}
#endif

				if (output.m_sendFragFrag.GetSegmentCount() == 0)
				{
					throw Exception("Unexpected state in RemoteToPacketSendMap #2!");
				}

				//sendcompletion에서 sendbrake하기 위함.
				m_curPacketQueue = head;

				// send ready list에서 일단 지운다.
				if (head->GetListOwner() != NULL)
					m_sendReadyList.Erase(head);
				head->m_lastAccessedTime = currentTime;

				if (head->IsEmpty())
				{
					//m_map.Remove(head->m_remoteAddr); PacketID reset을 당장 하면 곤란하므로!
				}
				else
				{
					// 패킷 큐가 아직 안비어있으면 send ready list에 넣되 맨 뒤에 넣는다. UDP 소켓에서 보내야 하는 서로 다른 send dest를 위한 fairness를 위해서다.
					m_sendReadyList.PushBack(head);
				}

				AssertConsist();
				
				
				return true;
			}

			// 패킷은 있지만 때가 되지는 않았다. 다음 항목을 뒤지자.
			head = head->GetNext();
		}

		// 아무것도 찾지 못했다. 마무리.
		m_curPacketQueue = NULL;
		return false;
	}

	// 이건 클라에서나 호출함. 서버는 send brake를 안쓰니까.
	void CUdpPacketFragBoard::DoForShortInterval(int64_t curTime)
	{
		assert(CNetConfig::RemoveTooOldUdpSendPacketQueueTimeoutMs > CNetConfig::AssembleFraggedPacketTimeoutMs * 10);  // PacketID가 리셋되어도 충분히 문제없는 시간이어야 하니까

		// 큐에 든게 있지만 앞서 큐에 넣던 순간 과량 송신 상태라(send brake에 의해 통제된 경우) send ready list에 등록되지 못했을 경우, 여기서 필요시 재등록
		for (AddrPortToQueueMap::iterator ipq = m_addrPortToQueueMap.begin();ipq!=m_addrPortToQueueMap.end();ipq++)
		{
			CPacketQueue* p = ipq->GetSecond();

			// 큐에 든게 있지만 앞서 큐에 넣던 순간 과량 송신 상태라 existants에 미등록됐다면 여기서 필요시 재등록
			AddToSendReadyListOnNeed(p, curTime);
		}
	}

	void CUdpPacketFragBoard::DoForLongInterval(int64_t curTime)
	{
		Proud::AssertIsLockedByCurrentThread(m_owner->m_sendQueueCS);
		assert(CNetConfig::RemoveTooOldUdpSendPacketQueueTimeoutMs > CNetConfig::AssembleFraggedPacketTimeoutMs * 10);  // PacketID가 리셋되어도 충분히 문제없는 시간이어야 하니까

		m_packetFreeList.ShrinkOnNeed();

		for (AddrPortToQueueMap::iterator ipq = m_addrPortToQueueMap.begin();ipq!=m_addrPortToQueueMap.end();)
		{
			CPacketQueue* p = ipq->GetSecond();

			p->m_sendBrake.DoForLongInterval(curTime);
#ifdef UPDATE_TEST_STATS
			CTestStats::TestRecentSendSpeedAtReceiverSide = p->m_sendSpeed.GetRecentSpeed();
#endif	
			/* 사용 안된지 너무 오래됐으면 추방 또는 iter to next
			(주의: 여기서는 유령을 없애기 위한 목적이지,
			안쓰이는 것을 없애기 위한 목적이 아니다. overlapped I/O가 아직
			진행중인 것일 수 있으므로 충분한 기간 값을 넣어야만 의미가 있다!) */

			if (curTime - p->m_lastAccessedTime > CNetConfig::RemoveTooOldUdpSendPacketQueueTimeoutMs)
			{
				if(p->GetListOwner() != NULL)
					m_sendReadyList.Erase(p);//p->UnlinkSelf();  // 객체 자체가 파괴되는 것이 아닐 수 있으므로 existant list에서는 제외하도록 한다.
				
				delete p;

				ipq = m_addrPortToQueueMap.erase(ipq);
			}
			else
			{
				ipq++;
			}
		}
	}

	// 	void CUdpPacketFragBoard::Add( AddrPort key, PacketQueuePtr value )
	// 	{
	// 		m_map[key] = value;
	// 	}

	// 	bool CUdpPacketFragBoard::TryGetValue( AddrPort key, PacketQueuePtr& value )
	// 	{
	// 		return m_map.Lookup(key, value);
	// 	}

	// 항목을 제거하되, current selection과 같은 것이면 current selection도 이동한다.
	void CUdpPacketFragBoard::Remove( AddrPort key )
	{
		AddrPortToQueueMap::CPair* pair = m_addrPortToQueueMap.Lookup(key);
		if (pair != NULL)
		{
			CPacketQueue* pq = pair->m_value;
			if(pq->GetListOwner()!= NULL)
				m_sendReadyList.Erase(pq);//pair->m_value->UnlinkSelf(); // existant list에서 제거
			
			delete pq;
			
			m_addrPortToQueueMap.RemoveAtPos(pair);
			m_curPacketQueue = NULL; // 이것도 빠뜨리지 말아야.
		}
	}

	void CUdpPacketFragBoard::Clear()
	{
		for (AddrPortToQueueMap::iterator ipq = m_addrPortToQueueMap.begin();ipq!=m_addrPortToQueueMap.end();)
		{
			CPacketQueue* p = ipq->GetSecond();
			if(p->GetListOwner()!= NULL)
				m_sendReadyList.Erase(p);//->UnlinkSelf();
			
			delete p;

			ipq = m_addrPortToQueueMap.erase(ipq);
		}
		m_curPacketQueue = NULL; // 이것도 빠뜨리지 말아야.
	}


	// 	void CUdpPacketFragBoard::RemoveKeyIfEmpty( PacketQueuePtr pq )
	// 	{
	// 		// 아래 루틴이 막혀있으면 성능상 이익이 온다. 사실 막혀있어야 한다.
	// 		// 만약 UDP 송신 문제가 자꾸 발생하면 아래 루틴을 풀어서 추이를 비교하자.
	// #ifdef SLOW_BUT_PRECISE_QUEUE_MAP
	// 		// 송신큐가 완전히 비워졌으면 맵에서 제거한다.
	// 		// 이렇게 해두어야 UDP 오버랩 송신에서 신속하게 송신큐가 든 리모트 연결을 찾아낼 수 있다.
	// 		if (pq->Count == 0)
	// 		{
	// 			Remove(pq->m_remoteAddr);			
	// 		}
	// #endif
	// 	}

	void CUdpPacketFragBoard::InitHashTableForClient()
	{
		// 클라에서는 hash table 기본값 17은 오히려 성능에 나쁘다. 따라서 이렇게 조절해둔다.
		if (!m_addrPortToQueueMap.InitHashTable(3))
			throw std::bad_alloc();
	}

	int CUdpPacketFragBoard::GetTotalPacketCountOfAddr( const AddrPort &addr )
	{
		CPacketQueue* pq = NULL;
		
		// cache for frequent case
		if(m_addrPortToQueueMap.GetCount() == 0)
			return 0;

		if(m_addrPortToQueueMap.TryGetValue(addr,pq))
		{
			return pq->GetTotalCount();				
		}
		else
			return 0;
	}

	/* 송신할 패킷을 추가한다. 
	finalDestHostID: 패킷의 최종 수신 대상. relay or route 대상인 경우 실제 수신 대상과 다르므로 None을 넣어야 한다. None을 넣는 경우 UniqueID 비교가 무시된다.
	filterTag: UDP는 제3자가 송신자 주소를 속여서 보낼 수 있다. (버그있는 2중 NAT 장치에서 속일 의도가 없어도 같은 현상이 제보된 바 있음.) 따라서 filterTag 값으로 비정상 송신자를 걸러내는 데 쓴다.
	*/
	void CUdpPacketFragBoard::AddNewPacket(
		HostID finalDestHostID,
		FilterTag::Type filterTag,
		AddrPort sendTo,
		const CSendFragRefs &sendData,
		int64_t addedTime,
		SendOpt sendOpt,
		CIssueSendResult& sendResult)
	{
		// unicast만 허용하도록 한다. malware 오인당하면 즐.
		if (!sendTo.IsUnicastEndpoint())
		{
			assert(0);
			return;
		}

		// 0바이트짜리 패킷은 아예 송신을 금지시키자. Socket 내부에서 무슨 짓을 할지 모르니.
		if (sendData.GetTotalLength() <= 0)
			return;

		// 대응하는 송신큐를 찾되 없으면 새로 등록한다.
		CPacketQueue* packetQueue = NULL;
		if (!m_addrPortToQueueMap.TryGetValue(sendTo, packetQueue))
		{
			packetQueue = new CPacketQueue; // note: 객체 파괴는 ~Map에서 함.			
			packetQueue->m_owner = this;
			packetQueue->m_remoteAddr = sendTo;
			packetQueue->m_filterTag = filterTag;
			packetQueue->m_lastAccessedTime = addedTime;

			// coal interval이 0이면 1ms 미만 후에 보내는 것이 아니라, "즉시"보내야 한다.
			// 그래야 LAN환경에서 캐릭터가 덜덜 떨리는 이상 현상이 눈에 안띈다.

			m_addrPortToQueueMap.Add(sendTo, packetQueue);

			if (packetQueue->m_coalesceIntervalMs == 0)
			{
				sendResult.m_issueSendNow = true;
			}

		}

		if (packetQueue->m_remoteAddr != sendTo)
		{
			throw Exception("PacketQueue consistency failed!");
		}

		if (sendOpt.m_priority < 0 || sendOpt.m_priority >= MessagePriority_LAST)
		{
			ThrowInvalidArgumentException();
		}

		// 패킷 우선순위 기능을 강제로 끄도록 만들어져 있는지 체크한다.
		if (!CNetConfig::EnableMessagePriority)
			sendOpt.m_priority = (MessagePriority)0;

		CPacketQueue::CPerPriorityQueue& subQueue = packetQueue->m_priorities[sendOpt.m_priority];

		// uniqueID가 지정된 경우 같은 ID가 지정된 것이 있으면 그것을 제거하고 해당 위치에 넣도록 한다.
		// 성능 저하를 줄이기 위해 0이 지정된 경우 무조건 무시한다.

		// 위 uniqueID에 의한 교체 과정을 하지 않은 경우 새 패킷을 추가한다.
		UdpPacketCtx* sendInfo = NewOrRecyclePacket_(); // RECYCLED! MAY BE DIRTY!

		if (sendOpt.m_fragmentOnNeed)
			subQueue.m_checkFraggableUdpPacketList.PushBack(sendInfo);
		else
			subQueue.m_checkNoFraggableUdpPacketList.PushBack(sendInfo);



		sendInfo->m_uniqueIDInfo.m_uniqueID = sendOpt.m_uniqueID;
		sendInfo->m_uniqueIDInfo.m_hostID_NOT_USED_YET = finalDestHostID;
		sendInfo->m_enquedTime = addedTime;
		sendInfo->m_ttl = sendOpt.m_ttl;

		sendData.CopyTo(sendInfo->m_packet); // copy

#ifdef TEST_WANGBA
		if (sendInfo->m_packet.GetCount() == 0)
			OutputDebugString("Cue B!\n");
#endif

		if (packetQueue->IsEmpty())
		{
			throw Exception("PacketQueue consistency 2 failed!");
		}

		packetQueue->m_lastAccessedTime = addedTime;

		// 아직 existant list에 없으면 넣도록 한다.		
		AddToSendReadyListOnNeed(packetQueue, addedTime);

		AssertConsist();
	}

	bool CUdpPacketFragBoard::HasRing0OrRing1Packet()
	{
		CPacketQueue* queue = m_sendReadyList.GetFirst();

		if(NULL == queue)
			return false;

		return queue->HasRing0OrRing1Packet();
	}


	void CUdpPacketFragBoard::AssertConsist()
	{
#ifdef _DEBUG
		for(CPacketQueue* pq = m_sendReadyList.GetFirst(); pq != NULL; pq = pq->CListNode<CPacketQueue>::GetNext())
		{
			assert(!pq->IsEmpty());
		}
#endif
	}

	CUdpPacketFragBoard::CUdpPacketFragBoard(CSuperSocket* owner)
	{
		m_owner = owner;
		m_lastPopFragmentSurpressed_ValidAfterPopFragment = false;
		m_enableSendBrake = CNetConfig::EnableSendBrake;
		m_curPacketQueue = NULL;
		
		// 이 map 클래스는 신축폭이 매우 크다. 따라서 rehash 역치를 최대한 크게 잡아야 한다.
		// 줄어드는 rehash는 절대 하지 말자.
		m_addrPortToQueueMap.SetOptimalLoad(0.1f,0,2.0f); 

	}

	void CUdpPacketFragBoard::AddToSendReadyListOnNeed( CPacketQueue* packetQueue, int64_t /*currTime*/)
	{
		// HasPacketAndTimeToSendReached는 heartbeat에서 사용되니까 즐
		if(packetQueue->GetListOwner() == NULL && !packetQueue->IsEmpty())
		{
			m_sendReadyList.PushBack(packetQueue); 
		}
	}

	// 수신자 측에서 측정된 실제 수신 속도를 이 호스트(송신자)의 상태값에 저장한다.
	void CUdpPacketFragBoard::SetReceiveSpeedAtReceiverSide(AddrPort dest, int64_t speed, int packetLossPercent, int64_t curTime)
	{
		CPacketQueue* queue = NULL;
		if(m_addrPortToQueueMap.TryGetValue(dest, queue))
		{
			queue->m_sendBrake.SetReceiveQuality(speed, packetLossPercent, curTime);
		}
	}

	uint32_t CUdpPacketFragBoard::GetPacketQueueTotalLength()
	{
		uint32_t retLen = 0;

		for (AddrPortToQueueMap::iterator ipq = m_addrPortToQueueMap.begin(); ipq != m_addrPortToQueueMap.end(); ipq++)
		{
			CPacketQueue* p = ipq->GetSecond();

			retLen += (uint32_t)p->GetTotalLengthInBytes();
		}

		return retLen;
	}

	int CUdpPacketFragBoard::GetPacketQueueTotalLengthByAddr(AddrPort addr)
	{
		CPacketQueue* pq = NULL;
		if(m_addrPortToQueueMap.TryGetValue(addr,pq))
		{
			return pq->GetTotalLengthInBytes();				
		}
		else
			return 0;
	}

	CUdpPacketFragBoard::~CUdpPacketFragBoard()
	{
		m_sendReadyList.UnlinkAll();
		m_addrPortToQueueMap.ClearAndFree(); // 이것부터 먼저 파괴해야 안전하게 packet free list를 제거할 수 있으므로
	}

	UdpPacketCtx* CUdpPacketFragBoard::NewOrRecyclePacket_()
	{
		return m_packetFreeList.NewOrRecycle();
	}

	void CUdpPacketFragBoard::DropPacket_( UdpPacketCtx* packet )
	{
		//packet->m_packet.ClearAndKeepCapacity();
		m_packetFreeList.Drop(packet);
	}

	// send brake가 작동하기 위한 근거 정보를 준다
	void CUdpPacketFragBoard::AccumulateSendBrake( int length )
	{
		Proud::AssertIsLockedByCurrentThread(m_owner->m_sendQueueCS);

		if (m_curPacketQueue != NULL)
		{
			m_curPacketQueue->AccumulateSendBrake(length);
		}
	}

	/* UDP datagram의 헤더를 buffer에 기록한다.
	P2P 피어가 16개 정도 되며, 캐릭터 위치를 7Hz로 멀티캐스트하며, 동남아시아나 브라질 등 인터넷이 저속인 곳에서는
	UDP 과량 사용도 TCP starvation을 유발한다. 따라서 최대한 패킷량을 줄여야 한다.
	이 때문에, 본 함수는 고도로 최적화된다.
	PacketID, Fragment ID, Packet Length는 VLQ 기법을 응용하여 최대한의 바이트를 사용해서 저장되며,
	FragmentID는 불필요한 경우(packet length가 MTU 크기 이하인 경우) 생략해 버린다.

	헤더 구조는 FragHeader 사용량 줄이기.pptx 참고	
	*/
	void CUdpPacketFragBoard::WriteFragHeader(CMessage &msg, FragHeader& header)
	{
		int packetLengthFlag = GetLengthFlag(header.packetLength);
		int packetIDFlag = GetLengthFlag(header.packetID);
		int fragmentIDFlag = GetLengthFlag(header.fragmentID);

		// 이제, 기록한다. 먼저, splitter filter부터 채운다. 이는 1바이트를 차지한다.
		header.splitterFilter |= (packetLengthFlag << 12) | (packetIDFlag << 10) | (fragmentIDFlag << 8);
		msg.Write(header.splitterFilter);

		// 나머지 멤버들을 기록. flag 값에 따라 가변 크기로 저장된다.
		WriteCompressedByFlag(msg, header.packetLength, packetLengthFlag);
		WriteCompressedByFlag(msg, header.packetID, packetIDFlag);
		if (header.packetLength > CNetConfig::MtuLength) // packet length가 MTU length 이하이면, 이것은 그냥 생략하게 만들어 버리자. full packet 자체가 MTU크기 이하라서 fragment ID가 의미 없음
			WriteCompressedByFlag(msg, header.fragmentID, fragmentIDFlag);
	}

	// WriteFragHeader의 반대 역할을 한다. 거기 주석을 참고할 것.
	bool CUdpPacketFragBoard::ReadFragHeader( CMessage &msg, FragHeader &header )
	{
		if(!msg.Read(header.splitterFilter))
			return false;

		int packetLengthFlag = (header.splitterFilter & 0x3000) >> 12;//최상위 2비트부터 4비트까지
		int packetIDFlag = (header.splitterFilter & 0x0c00) >> 10;//최상위 4비트부터 6비트까지
		int fragmentIDFlag = (header.splitterFilter & 0x0300) >> 8;//최상위 6비트부터 8비트까지

		if(!ReadCompressedByFlag(msg, header.packetLength, packetLengthFlag))
			return false;
		if(!ReadCompressedByFlag(msg, header.packetID, packetIDFlag))
			return false;
		if(header.packetLength > CNetConfig::MtuLength)
		{
		if(!ReadCompressedByFlag(msg, header.fragmentID, fragmentIDFlag))
			return false;
		}
		else
			header.fragmentID = 0;

		return true;
	}

	// sendto addr에 대응하는 packet queue가 가진 coalesce interval을 설정한다.
	void CUdpPacketFragBoard::SetCoalesceInterval(const AddrPort &addr, int interval)
	{
		Proud::AssertIsLockedByCurrentThread(m_owner->m_sendQueueCS);
		CPacketQueue* pq = NULL;

		if(m_addrPortToQueueMap.GetCount() == 0)
			return;

		if(m_addrPortToQueueMap.TryGetValue(addr,pq))
		{
			pq->m_coalesceIntervalMs = interval;
		}
	}

	// TCP가 위험한 상황일 때 P2P 혼잡제어를 걸까 말까를 처리
	// NOTE: 클라에서만 한다. 서버는 역할 자체가 P2P 대량 통신을 하는 일이 없으므로.
	void CUdpPacketFragBoard::SetTcpUnstable(int64_t curTime, bool unstable)
	{
		// 모든 수신 대상에 대해 수행

		for (AddrPortToQueueMap::iterator ipq = m_addrPortToQueueMap.begin(); ipq != m_addrPortToQueueMap.end(); ipq++)
		{
			CPacketQueue* p = ipq->GetSecond();
			p->m_sendBrake.SetTcpUnstable(curTime, unstable);
		}
	}

	// MTU 크기 이하까지 packet을 뭉친 후 MTU 크기 직전까지 자른 패킷을 하나 리턴하거나,
	// 1개의 full packet을 얻는다.
	// FragHeader의 Splitter value에 따라 full packet인지 아닌지 확인 가능.
	void CUdpPacketFragBoard::CPacketQueue::PopFragmentOrFullPacket(
		int64_t curTime, 
		CUdpPacketFragBoardOutput &output)
	{
		// frag 안할 패킷이 쌓여있으면 그냥 그것을 우선 주도록 한다. 
		// frag 안할 패킷이 없을 때에만 frag될 수 있는 패킷들을 처리한다.
		for (int iPriority = 0; iPriority < MessagePriority_LAST; iPriority++)
		{
			CPerPriorityQueue& perPriorityQueue = m_priorities[iPriority];
			UdpPacketCtx* headFullPacket = perPriorityQueue.m_noFraggableUdpPacketList.GetFirst();
			if (headFullPacket)
			{
				// rtt 를 받아오기 위해서 여러 함수에 인자 추가가 필요하여 일단 CNetConfig::CleanUpOldPacketIntervalMs 사용.
				if (iPriority != MessagePriority_Ring0 
					&& iPriority != MessagePriority_Ring1 
					&& curTime - headFullPacket->m_enquedTime > CNetConfig::CleanUpOldPacketIntervalMs)
				{
					//  packet frag board에서 오래동안 묵혀진 패킷이라면 버린다.
					perPriorityQueue.m_noFraggableUdpPacketList.Erase(headFullPacket);
					continue;
				}

				output.ResetForReuse();  // 일단 텅 비워버려야. 이미 UDP send completion이 발생했을 터이고 안전하게 output.m_owningPackets도 비워도 됨.
				output.m_sendTo = m_remoteAddr;
				output.m_source = m_owner;
				m_lastFullPacketID++;

				// FragHeader를 싸서 보낸다. 그리고 상황 종료.
				FragHeader fragHeader;
				fragHeader.splitterFilter = 0;
				fragHeader.splitterFilter |= (uint16_t)(FullPacketSplitter << 14);
				fragHeader.packetLength = headFullPacket->m_packet.GetCount();

#ifdef TEST_WANGBA
				if(fragHeader.packetLength <= 0) 
					OutputDebugString("Cue A!\n");

				String txt;
				txt.Format(_PNT("fragHeader.packetLength=%d"), fragHeader.packetLength);
				//OutputDebugString(txt);
#endif

				fragHeader.packetID = m_lastFullPacketID;
				fragHeader.fragmentID = 0;

				// #PacketGarble 보내는 쪽 코드
				// udp 패킷에 최초 4byte가 동일한 값일 경우 일부 공유기에서 받은 패킷을 다시 재전송하는 경우가 있다. 
				// 그러므로, 앞의 4byte(splitter & filterTag) 가 동일한 값이 가지 않도록 해야 한다.
				// 먼저, 값이 계속 변하는 packetID로 헤더 앞단을 XOR 연산을 한다. 받는 쪽에서 defrag 시 다시 packetID로 XOR 연산을 하면 filterTag를 얻게 되고 그것으로 원본 확인을 한다.
				fragHeader.splitterFilter |= (uint16_t)((m_filterTag ^ fragHeader.packetID) & 0x00ff);

				CUdpPacketFragBoard::WriteFragHeader(output.m_fragHeader, fragHeader);
				output.m_sendFragFrag.Add(output.m_fragHeader.GetData(), output.m_fragHeader.GetLength());
				output.m_sendFragFrag.Add(headFullPacket->m_packet.GetData(), (int)headFullPacket->m_packet.GetCount());
				// TTL 채워넣기
				output.m_ttl = headFullPacket->m_ttl;

				output.m_owningPackets.Add(headFullPacket); // 소유권 이양. 이걸 콜 하려면 UDP send completion이 발생했을 터이고 그때는 

				if (headFullPacket->m_uniqueIDInfo.m_uniqueID.m_value != 0)
					perPriorityQueue.m_uniqueIDToPacketMap.RemoveKey(headFullPacket->m_uniqueIDInfo);
				perPriorityQueue.m_noFraggableUdpPacketList.Erase(headFullPacket);//headFullPacket->UnlinkSelf();

				//상황 종료.
				return;
			}
		}

		//assert(CNetConfig::MessageMaxLength < INT_MAX - 10000); // 2GB 딱 채우는 크기는 라운드 오버런으로 곤란

		bool fragBoardWasEmpty = (m_fragBoardedPackets.GetCount() == 0);
		int ttl = -1;

		// 도마가 아직 덜 비워졌으면 생략. 비었다면 풀패킷 큐에서 꺼내서 채운다.
		if (fragBoardWasEmpty)
		{
			ResetFragBoardState();

			m_lastFullPacketID++;

			// m_fragBoardedPackets가 이 안이 아닌 다른 곳에서 채워 지는 경우가 있을 때 문제가 될 수 있다.
			m_lastFragmentedPacketID = m_lastFullPacketID;

			int appendCount = 0;

			// 가장 높은 우선순위의 항목부터 검색해서 MTU 크기 이전까지 도마에 패킷을 올린다.
			for (int iPriority = 0; iPriority < MessagePriority_LAST; iPriority++)
			{
				CPerPriorityQueue& list = m_priorities[iPriority];
				while (list.m_fraggableUdpPacketList.GetCount() > 0)
				{
					UdpPacketCtx *head = list.m_fraggableUdpPacketList.GetFirst();

					if (appendCount == 0)
					{
						// 처음인 경우 무조건 한 패킷은 도마에 올린다. MTU 사이즈보다 당연히 클 수도 있지만 어차피 도마에서 썰린 후 전송될 것임.
						m_fragBoardedPackets.Add(head);

						m_fragBoardTotalBytes += (int)head->m_packet.GetCount();
						ttl = head->m_ttl;

						if (head->m_uniqueIDInfo.m_uniqueID.m_value != 0)
							list.m_uniqueIDToPacketMap.RemoveKey(head->m_uniqueIDInfo);
						list.m_fraggableUdpPacketList.Erase(head);//head->UnlinkSelf();
					}
					else
					{
						// 도마에 올리기 전에 MTU보다 큰지 체크한다.
						// 보내기 전의 소켓 옵션에의 변화가 있어도 도마에 올리는건 더 이상 금지.
						if (m_fragBoardTotalBytes + (int)head->m_packet.GetCount() < CNetConfig::MtuLength
							&& ttl == head->m_ttl
#if !defined(_WIN32)
							// sendmsg의 IOV_MAX제한 때문에 아래와 같은 조건 추가.
							// -3은 헤더를 위한 여유분
							// Mono 내부에서 sendmsg를 사용하므로 이 검사는 C#에도 추가 요망
							&& appendCount < IOV_MAX - 3
#endif
							)
						{
							m_fragBoardedPackets.Add(head);

							m_fragBoardTotalBytes += (int)head->m_packet.GetCount();

							if (head->m_uniqueIDInfo.m_uniqueID.m_value != 0)
								list.m_uniqueIDToPacketMap.RemoveKey(head->m_uniqueIDInfo);
							list.m_fraggableUdpPacketList.Erase(head);//head->UnlinkSelf();
						}
						else
						{
							goto gatherDone;
						}
					}

					appendCount++;
				}
			}
		gatherDone:
			// 주의: 이미 수정된게 또 수정되면 안되므로 여기서 처리해야 한다.
			// 즉, 아래 out-of-scope에서 하면 안됨. 반드시 여기서 해야 됨. 빈 frag board에 추가된 경우에 한해 처리해야 하니까.
			CompactFragBoardedPacketForOneSmallMessage();
		}

		if (m_fragBoardTotalBytes <= 0)
		{
			assert(0);
			ThrowInvalidArgumentException();
		}

		output.ResetForReuse();

		output.m_sendTo = m_remoteAddr;

		// 도마에 올린 것들을 MTU 크기까지 뭉친 한 개를 리턴.
		int currentFragLength = PNMIN(CNetConfig::MtuLength, m_fragBoardTotalBytes - m_globalOffsetInFragBoard);

		output.m_source = m_owner;

		// 헤더 넣기
		FragHeader fragHeader;
		fragHeader.splitterFilter = 0;
		fragHeader.splitterFilter |= (uint16_t)(FragSplitter << 14);
		fragHeader.packetLength = m_fragBoardTotalBytes;

#ifdef TEST_WANGBA
		if(fragHeader.packetLength <=0)
			OutputDebugString("Cue C!\n");
#endif // TEST_WANGBA

		fragHeader.packetID = m_lastFragmentedPacketID;
		fragHeader.fragmentID = m_destFragID;

		// udp 패킷에 최초 4byte가 동일한 값일 경우 일부 공유기에서 받은 패킷을 다시 재전송하는 경우가 있으므로 앞의 4byte(splitter & filterTag) 가 동일한 값이 가지 않도록 
		// 값이 계속 변하는 packetID로 XOR 연산을 하고 defrag 시 다시 packetID로 XOR 연산을 해서 filterTag 를 비교합니다.
		fragHeader.splitterFilter |= (uint16_t)((m_filterTag ^ fragHeader.packetID) & 0x00ff);

		// 헤더부터 issue send를 해야 한다.
		CUdpPacketFragBoard::WriteFragHeader(output.m_fragHeader, fragHeader);
		output.m_sendFragFrag.Add(output.m_fragHeader.GetData(), output.m_fragHeader.GetLength());

		// TTL 채워넣기
		output.m_ttl = ttl;

		// 본문을 채워넣기
		int oldGlobalOffsetInFragBoard = m_globalOffsetInFragBoard;

		while (m_globalOffsetInFragBoard < oldGlobalOffsetInFragBoard + currentFragLength) // MTU 이하인 동안
		{
			UdpPacketCtx *curPacket = m_fragBoardedPackets[m_srcIndexInFragBoard];

			int fragAppendeeLength = PNMIN(
				oldGlobalOffsetInFragBoard + currentFragLength - m_globalOffsetInFragBoard,
				(int)curPacket->m_packet.GetCount() - m_localOffsetInFragBoard);
			if (fragAppendeeLength <= 0)
			{
				assert(0);
				ClearFragBoardedPackets();
				throw Exception("Unexpected at PacketQueue Pop!");
			}

			// 비동기 issue를 할거니까, send frag frag에만 append한다. 복사 최소화를 위해.
			output.m_sendFragFrag.Add(curPacket->m_packet.GetData() + m_localOffsetInFragBoard, fragAppendeeLength);

			m_localOffsetInFragBoard += fragAppendeeLength;
			m_globalOffsetInFragBoard += fragAppendeeLength;

			assert(m_localOffsetInFragBoard <= (int)curPacket->m_packet.GetCount());

			if (m_localOffsetInFragBoard == (int)curPacket->m_packet.GetCount())
			{
				m_srcIndexInFragBoard++;
				m_localOffsetInFragBoard = 0;
			}

			assert((int)((int)output.m_sendFragFrag.GetLength() - sizeof(FragHeader)) <= (int)CNetConfig::MtuLength);
		}

		assert(m_globalOffsetInFragBoard == oldGlobalOffsetInFragBoard + currentFragLength);
		assert(m_globalOffsetInFragBoard <= m_fragBoardTotalBytes);

		// 헤더, 내용을 다 채웠으니 이제 frag ID 증가해도 안전
		m_destFragID++;

		// 끝까지 다 보낸 상태이면 도마에 있는 것들의 소유권을 output에 넘기고, 도마를 비운다. 
		if (m_globalOffsetInFragBoard == m_fragBoardTotalBytes)
		{
			// 소유권을 output 객체가 가지게 한다.
			// 이제 콜러가 갖고 있다가 udp send completion이 발생하면 이 메서드를 또 콜할 것이다. 콜 하면서 m_owningPackets도 다 증발할 것이다.
			output.m_owningPackets.AddRange(m_fragBoardedPackets.GetData(), m_fragBoardedPackets.GetCount());
			m_fragBoardedPackets.ClearAndKeepCapacity();
			// 			if(a)
			// 			{
			// 				assert(m_fragBoardedPackets.GetCapacity() > 0); // clear 호출시 버퍼를 완전히 청소하고 있다면 성능 저하가 크므로 즐
			// 			}
		}

		/* 이작업을 이제 sendcompletion에서 하자.
		int len = output.m_sendFragFrag.GetLength();
		m_sendBrake.Accumulate(len);
		m_sendSpeed.Accumulate(len,curTime);*/

	}

	/*void CUdpPacketFragBoard::PacketQueue::PopHigherPriorityFirstWithCoalesce_SLOWTEMP( double curTime,UdpSendOrRecvData &output ) const
	{
		assert(CNetConfig::MessageMaxLength < INT_MAX - 10000); // 2GB 딱 채우는 크기는 라운드 오버런으로 곤란

		bool fragBoardWasEmpty = (m_fragBoardedPackets.GetCount() == 0);
		if(fragBoardWasEmpty)
		{

			int appendCount = 0;

			// 가장 놓은 우선순위의 항목부터 검색해서 MTU 크기 이전까지 도마에 패킷을 올린다.
			for(int iPriority=0;iPriority<MessagePriority_LAST;iPriority++)
			{
				const ListType& list = m_priorities[iPriority];
				if(list.GetCount()>0)
				{
					const UdpPacketCtx &head = list.GetHead();

					appendCount++;
				}
			}
		}

		output.m_remoteAddr = m_remoteAddr;

		// 도마에 올린 것들을 MTU 크기까지 뭉친 한 개를 리턴.
		int currentFragLength = PNMIN(CNetConfig::MtuLength, m_fragBoardTotalBytes - m_globalOffsetInFragBoard);

		CMessage msg;
		if(output.m_ctx.m_packet.IsNull())
		{
			msg.UseUnsafeInternalBuffer(m_unsafeHeap);		
			output.m_ctx.m_packet = msg.m_msgBuffer;
		}
		else
		{
			// 이미 있는 놈을 재사용한다. 성능 업 효과.
			msg.m_msgBuffer = output.m_ctx.m_packet;
			msg.Clear();
		}

		// 헤더 넣기
		msg.m_msgBuffer.SetMinCapacity(CNetConfig::MtuLength);// 더 적은 frag를 위해서
		msg.Write(FragSplitter);
		msg.Write(m_filterTag);
		msg.WriteScalar(m_fragBoardTotalBytes);
		msg.Write(m_currentPacketID);
		msg.WriteScalar(m_destFragID);

		int msgWritePos0 = msg.GetLength();

		// 본문을 채워넣기
		int t_globalOffsetInFragBoard = m_globalOffsetInFragBoard;
		int oldGlobalOffsetInFragBoard = m_globalOffsetInFragBoard;

		int cnt = 0;
		while(t_globalOffsetInFragBoard < oldGlobalOffsetInFragBoard + currentFragLength && cnt < 100)
		{
			cnt++;
			const ByteArrayPtr &curPacket = m_fragBoardedPackets[m_srcIndexInFragBoard];

			int fragAppendeeLength = PNMIN(oldGlobalOffsetInFragBoard + currentFragLength - t_globalOffsetInFragBoard, curPacket.GetCount() - m_localOffsetInFragBoard);
			if(fragAppendeeLength <= 0)
			{
				assert(0);
				throw Exception("Unexpected at PacketQueue Pop!");
			}

			// 메모리 복사
			msg.Write(curPacket.GetData() + m_localOffsetInFragBoard,fragAppendeeLength);

			t_globalOffsetInFragBoard += fragAppendeeLength;

			assert(m_localOffsetInFragBoard <= curPacket.GetCount());

			if(m_localOffsetInFragBoard == curPacket.GetCount())
			{
			}

			assert(msg.GetLength() - msgWritePos0 <= CNetConfig::MtuLength);
		}

		assert(t_globalOffsetInFragBoard == oldGlobalOffsetInFragBoard + currentFragLength);
		assert(t_globalOffsetInFragBoard <= m_fragBoardTotalBytes);

		// 헤더, 내용을 다 채웠으니 이제 frag ID 증가해도 안전

		int len = output.m_ctx.m_packet.GetCount();
	}*/

	int CUdpPacketFragBoard::CPacketQueue::GetTotalCount()
	{
		int ret = 0;
		CPerPriorityQueue* pPriorities = m_priorities.GetData();  // 바운드체크 피함
		for(int i=0;i<MessagePriority_LAST;i++)
		{
			ret += pPriorities[i].m_fraggableUdpPacketList.GetCount();
			ret += pPriorities[i].m_noFraggableUdpPacketList.GetCount();
			ret += pPriorities[i].m_checkFraggableUdpPacketList.GetCount();
			ret += pPriorities[i].m_checkNoFraggableUdpPacketList.GetCount();
		}

		ret += (int)m_fragBoardedPackets.GetCount();
		return ret;
	}

	CUdpPacketFragBoard::CPacketQueue::CPacketQueue()
	{
		for(int i=0;i<m_priorities.GetCount();i++)
		{
			m_priorities[i].m_owner = this;
		}
		
		m_owner = NULL;
		m_lastAccessedTime = 0;
		m_nextTimeToCoalescedSend = 0;
		m_remoteAddr = AddrPort::Unassigned;
		m_filterTag = 0;

		ResetFragBoardState();

		// 매 호스트 연결시마다 서로 다른 값부터 시작하게 하면 최근 몇초 사이 재접속한 경우 과거 defrag 미완료분과 꼬이는 일이 줄어드니까.
		m_lastFullPacketID = (PacketIDType)(size_t)this;  // 랜덤값의 의미를 가짐
		m_lastFragmentedPacketID = m_lastFullPacketID;

		m_fragBoardedPackets.SetGrowPolicy(GrowPolicy_HighSpeed);

		m_coalesceIntervalMs = CNetConfig::EveryRemoteIssueSendOnNeedIntervalMs;
	}

	// 지금 당장 보내야 할 패킷이 있는지 판별한다.
	// 송신큐가 안 비어있어도, 보낼 타임이 아니면(send brake or coalece에 의해) false를 리턴한다.
	bool CUdpPacketFragBoard::CPacketQueue::HasPacketAndTimeToSendReached( int64_t currTime, bool calledBySendCompletion )
	{
		// 보낼 패킷이 전혀 없으면 
		if(IsEmpty())
			return false;

		// 딜레이가 발생해서는 안되는 ProudNet ping packet 등은 무조건 즉시 보내야 하므로
		if(!m_priorities[MessagePriority_Ring0].IsEmpty() 
			|| !m_priorities[MessagePriority_Ring1].IsEmpty() )
		{
			return true;
		}

		// brake 기능이 켜져있고, brake를 걸어야 하는 상황(udp 송신량이 과다한 경우)라면 송신 큐에 들어 있어도 지연 송신해야 하면 보내지 말아야.
		if (m_owner->m_enableSendBrake && m_sendBrake.BrakeNeeded())
		{
			return false;
		}

		/* send completion에서의 호출이 아니면 heartbeat에서의 호출이다.
		이때는 coalesce에 의한 통제가 된다.
		NOTE: 다음 coalesced send를 할 시간이 갱신되는 조건은, pop 직후 패킷큐가 비어버릴때 뿐이다.
		즉, pop을 한 후 empty가 되면 now+coal intv.
		next time을 여기서는 바꾸면 안된다. reactor model에서는 한 턴에서 여러 
		pop을 수행하게 되는데, 여기서 바꿀 경우 각 턴에서 단 1개의 fragment밖에 못 뽑아오는 
		불상사로 이어지기 때문이다. */
		if(!calledBySendCompletion && currTime < m_nextTimeToCoalescedSend)
			return false;

		// 거부 조건을 모두 지나갔으므로
		return true;
	}

	int CUdpPacketFragBoard::CPacketQueue::GetTotalLengthInBytes()
	{
		int ret = 0;
		for(int i=0;i<MessagePriority_LAST;i++)
		{
			ret += m_priorities[i].GetTotalLengthInBytes();
		}

		if(m_fragBoardTotalBytes > 0)
			ret += m_fragBoardTotalBytes - m_globalOffsetInFragBoard;

		return ret;
	}

	CUdpPacketFragBoard::CPacketQueue::~CPacketQueue()
	{
		ClearFragBoardedPackets();

	}

	void CUdpPacketFragBoard::CPacketQueue::ClearFragBoardedPackets()
	{
		int cnt = m_fragBoardedPackets.GetCount();
		for(int i=0;i<cnt;i++)
			m_owner->DropPacket_(m_fragBoardedPackets[i]);

		m_fragBoardedPackets.ClearAndKeepCapacity();
	}

	void CUdpPacketFragBoard::CPacketQueue::AccumulateSendBrake( int length )
	{
		m_sendBrake.Accumulate(length);
	}

	void CUdpPacketFragBoard::CPacketQueue::CompactFragBoardedPacketForOneSmallMessage()
	{
/*		//3003!!!!!!!!!
		//일단 막았음
		return;
*/







		// packet frag board에 막 올라온 full packet이 단 1개의 MTU크기 이하 메시지면
		if(m_fragBoardedPackets.GetCount() == 1 && m_fragBoardTotalBytes <= CNetConfig::MtuLength)
		{
			// Splitter를 TinySplitter로 바꾸고 Splitter 바로 뒤의 message payload length를 제거하자.			
			UdpPacketCtx* packet = m_fragBoardedPackets[m_srcIndexInFragBoard];

			assert(m_fragBoardTotalBytes == packet->m_packet.GetCount()); // 당연하겠지?

			short randomSplitter;
			int payloadLength;

			CMessage reader;
			reader.UseExternalBuffer((uint8_t*)(packet->m_packet.GetData()), m_fragBoardTotalBytes);
			reader.SetLength(m_fragBoardTotalBytes);
			reader.SetReadOffset(0);
			if(reader.Read(randomSplitter)  // 잘 읽었고
				&& CTcpLayer_Common::RandomToSplitter(randomSplitter) == CTcpLayer_Common::Splitter // splitter 확실하고
				&& reader.ReadScalar(payloadLength)) // message 크기를 잘 얻었으면
			{	
				// 원래 크기: Splitter+Scalar(1~3바이트) => read offset으로 알 수 있음
				// 수정 후 크기: TinySplitter+0바이트
				int diffLength = reader.GetReadOffset() - sizeof(short);
				packet->m_packet.RemoveRange(sizeof(short), diffLength); // 이렇게 하면 memcpy 부하를 줄임. 최적은 아니지만.

				*((short*)(packet->m_packet.GetData())) = CTcpLayer_Common::SplitterToRandom(CTcpLayer_Common::TinySplitter); // Splitter를 TinySplitter로 교체(NOTE: C#에서는 little-endian으로 1바이트씩 교체하도록 하자)

				m_fragBoardTotalBytes -= diffLength;
			}				
		}
	}

	// 송신큐를 검사해서 같은 unique ID를 가지는 메시지를 맨 마지막 것 빼고 제거한다.
	void CUdpPacketFragBoard::CPacketQueue::NormalizePacketQueue()
	{
		int64_t curTime = GetPreciseCurrentTimeMs();
		for (int iPriority = 0; iPriority < MessagePriority_LAST; iPriority++)
		{
			CPerPriorityQueue& list = m_priorities[iPriority];

			if (list.m_nextNormalizeWorkTime == 0)
				list.m_nextNormalizeWorkTime = curTime;

			bool removeOldPacket; // 5초마다 이 값은 true로 세팅된다. 나머지 때는 false.
			if (iPriority >= MessagePriority_High && curTime > list.m_nextNormalizeWorkTime)
			{
				removeOldPacket = true;
				list.m_nextNormalizeWorkTime = curTime + CNetConfig::NormalizePacketIntervalMs;
			}
			else
			{
				removeOldPacket = false;
			}

			NormalizePacketQueue_Internal(&list.m_checkFraggableUdpPacketList, &list.m_fraggableUdpPacketList, &list.m_uniqueIDToPacketMap, curTime, removeOldPacket);
			NormalizePacketQueue_Internal(&list.m_checkNoFraggableUdpPacketList, &list.m_noFraggableUdpPacketList, &list.m_uniqueIDToPacketMap, curTime, removeOldPacket);
		}
	}

	// 송신큐를 검사해서 같은 unique ID를 가지는 메시지를 맨 마지막 것 빼고 제거한다.
	void CUdpPacketFragBoard::CPacketQueue::NormalizePacketQueue_Internal(
		CListNode<UdpPacketCtx>::CListOwner*nonThinnedList,
		CListNode<UdpPacketCtx>::CListOwner* thinnedList,
		UniqueIDPacketInfoToPacketMap* uniqueIDMap,
		int64_t curTime,
		bool removeOldPacket)
	{
		// 아래 오래된 패킷 제거는 5초마다 해야 하지만, unique id 최종것만 남기기는 
		// 매 issue send or nonblock send에서 항상 수행한다.
		// ring 0 or 1가 아니라면 오래된 패킷을 검사하여 버린다.
		if (removeOldPacket)
		{
			// 모든 패킷을 순차 검사하므로 느리다. 그래서 위 조건문을 통해 5초마다 처리한다.
			UdpPacketCtx* pk;
			for (pk = thinnedList->GetFirst(); pk != thinnedList->GetLast();)
			{
				if (pk->m_enquedTime - curTime > CNetConfig::CleanUpOldPacketIntervalMs)
				{
					UdpPacketCtx* erasePacket = pk;
					pk = pk->GetNext();

					thinnedList->Erase(erasePacket);
					m_owner->DropPacket_(erasePacket);
				}
				else
					pk = pk->GetNext();
			}
		}

		// non thinned queue에서 thinned queue로 이동하는 메시지가 unique ID를 가지는 경우
		// 이미 thinned queue에 있을 경우 그것을 교체한다.
		// 없으면 새로 추가.
		while (nonThinnedList->GetCount() > 0)
		{
			// 꺼낸다.
			UdpPacketCtx *checkPacket = nonThinnedList->GetFirst();
			nonThinnedList->Erase(checkPacket);
			bool alreadyExist = false;
			if (checkPacket->m_uniqueIDInfo.m_uniqueID.m_value != 0)
			{
				// unique id 를 사용한다면 map 에서 검사.
				UdpPacketCtx* pk = NULL;
				if (uniqueIDMap->TryGetValue(checkPacket->m_uniqueIDInfo, pk))
				{
					// 같은 정보가 있으면 기존 패킷 삭제 후 새 정보로 map 갱신
					thinnedList->Replace(pk, checkPacket);
					uniqueIDMap->SetAt(checkPacket->m_uniqueIDInfo, checkPacket);

					m_owner->DropPacket_(pk);

					alreadyExist = true;
				}
// 				else
// 					uniqueIDMap->Add(&checkPacket->m_uniqueIDInfo, checkPacket);
			}

			if (!alreadyExist)
			{
				thinnedList->PushBack(checkPacket);
				if (checkPacket->m_uniqueIDInfo.m_uniqueID.m_value != 0)
					uniqueIDMap->Add(checkPacket->m_uniqueIDInfo, checkPacket);

			}
		}
	}

	// 수신한 fragment를 수신큐에 넣은 후 완전히 조립된 1개의 packet을 얻는다.
	// NOTE: 1개의 packet은 1개 이상의 추출 가능한 message들이다.
	AssembledPacketError CUdpPacketDefragBoard::PushFragmentAndPopAssembledPacket( 
		uint8_t* fragData,  // fragment
		int fragLength,  // length of fragment
		AddrPort senderAddr,  // 수신된 UDP datagram에 있던 송신자 주소
		HostID volatileSrcHostID, // 송신자가 갖고 있어야 할 hostID. filter tag 기능을 위함.
		int64_t currtime,  // 현재 시간. 너무 자주 호출되는 함수이므로 이렇게 인자로 넘김.
		CAssembledPacket& output,  // 조립된 full packet은 여기 저장된다.
		String& outError) // 조립 실패시 채워지는 에러 문구
	{
		Proud::AssertIsLockedByCurrentThread(m_owner->m_sendQueueCS);

		CMessage msg;
		msg.UseExternalBuffer(fragData, fragLength);
		msg.SetLength(fragLength);
		msg.SetReadOffset(0);

		FragHeader fragHeader;

		// frag header를 읽는다.
		if (!CUdpPacketFragBoard::ReadFragHeader(msg, fragHeader))
		{
			outError = _PNT("UDP frag header missing!");
			// 이런식으로 무시해도 된다. UDP는 잘못된 패킷이 의레 올 수 있으니까. 대신 경고는 콜백해 주는 것이 좋긴 하다.
			//m_owner->m_owner->EnquePacketDefragWarning(senderAddr, _PNT("UDP frag header missing!"));
			return AssembledPacketError_Error;
		}
		/*if(!msg.Read((BYTE*)&fragHeader.splitterFilter,sizeof(fragHeader.splitterFilter)))
		{
			outError = _PNT("UDP frag header missing!");
			// 이런식으로 무시해도 된다. UDP는 잘못된 패킷이 의레 올 수 있으니까. 대신 경고는 콜백해 주는 것이 좋긴 하다.
			//m_owner->m_owner->EnquePacketDefragWarning(senderAddr, _PNT("UDP frag header missing!"));
			return PacketAssembleResult_Error;
		}*/

		uint8_t readSplitter = (uint8_t)((fragHeader.splitterFilter & 0xc000) >> 14);//최상위 2비트

		if (readSplitter != FragSplitter && readSplitter != FullPacketSplitter)
		{
			outError.Format(_PNT("Cannot identify UDP fragment nor full packet! fragheaderSplitter=%u"), readSplitter);
			return AssembledPacketError_Error;
		}

		// filter tag 체크
		HostID p1 = volatileSrcHostID;
		HostID p2 = m_owner->m_owner->GetVolatileLocalHostID();
		// #PacketGarble 받는 쪽 코드 
		// filterTag 는 packetID 로 XOR연산이 되어 있으므로 원본값으로 다시 돌립니다.
		uint8_t readFiltertag = (uint8_t)(fragHeader.splitterFilter & 0x00ff);//하위 8비트
		readFiltertag ^= (uint8_t)(fragHeader.packetID & 0x000000ff);
		if (FilterTag::ShouldBeFiltered(readFiltertag, p1, p2))
		{
			// m_owner->m_owner->EnqueWarning(senderAddr, _PNT("UDP frag length is wrong #4!")); 이건 쏘지 말도록 하자. 홀펀칭 과정에서 잘못된 호스트가 받는 경우가 으레 있으므로.
			return AssembledPacketError_Assembling;
		}

		// fragment header인 경우에의 정당성 체크
		if (readSplitter == FragSplitter)
		{
			assert(CNetConfig::MtuLength > 0);

			if(fragHeader.packetLength <= 0 || (int)fragHeader.packetLength > m_owner->GetMessageMaxLength() || 
				fragHeader.fragmentID < 0 || fragHeader.fragmentID > fragHeader.packetLength / CNetConfig::MtuLength)
			{
				outError.Format(_PNT("UDP frag length is wrong #1! packetlength=%d, maxlength=%d, fragID=%d"),
								fragHeader.packetLength, m_owner->GetMessageMaxLength(), fragHeader.fragmentID);
				return AssembledPacketError_Error;
			}
		}

		// full packet header인 경우에의 정당성 체크
		if (readSplitter == FullPacketSplitter)
		{
			if(fragHeader.packetLength <= 0 || (int)fragHeader.packetLength > m_owner->GetMessageMaxLength())
			{
				outError = _PNT("UDP full packet length is wrong!");

#ifdef TEST_WANGBA
				String txt;
				txt.Format(_PNT("UDP full packet length is wrong! packetLength=%d"), fragHeader.packetLength);
				OutputDebugString(txt);
#endif // TEST_WANGBA

				//m_owner->m_owner->EnquePacketDefragWarning(senderAddr, _PNT("UDP full packet length is wrong!"));
				return AssembledPacketError_Error;
			}
		}

		// fragID가 마지막을 가리키느냐 여부에 따라 fragLength가 매치되어야 한다.		
		int fragOffset = CNetConfig::MtuLength * fragHeader.fragmentID;
		int desiredFragLength = PNMIN(CNetConfig::MtuLength, (int)fragHeader.packetLength - fragOffset);
		int fragPayloadLength = msg.GetLength() - msg.GetReadOffset();

		if (readSplitter == FragSplitter)
		{
			if (desiredFragLength != fragPayloadLength)
			{
				outError.Format(_PNT("UDP frag length is wrong #2! desiredFragLength=%d, fragPayloadLength=%d"),
					desiredFragLength, fragPayloadLength);
				return AssembledPacketError_Error;
			}
		}

		// addr-to-queue map 항목을 찾거나 없으면 새로 추가.
		// senderAddr, packetID are keys
		DefraggingPacketMap* packets;

		AddrPortToDefraggingPacketsMap::CPair* iPackets = m_addrPortToDefraggingPacketsMap.Lookup(senderAddr);
		if (!iPackets)
		{
			packets = new DefraggingPacketMap;
			packets->m_recentReceiveSpeed.TouchFirstTime(currtime);
			iPackets = m_addrPortToDefraggingPacketsMap.SetAt(senderAddr, packets);
		}
		else
		{
			packets = iPackets->m_value;
		}

		// frag를 받은 경우에 한해,
		if (readSplitter == FragSplitter)
		{
			// board에 없으면 하나 추가. 단, board에 이미 있는 경우 조립중이던 패킷과 크기가 다르면 즐
			DefraggingPacketMap::CPair* iPacket = packets->Lookup(fragHeader.packetID);
			DefraggingPacket* packet;
			if (!iPacket)
			{
				// '조립중인' 패킷이 없으므로 새로 추가
				packet = DefraggingPacket::NewInstance();

				packet->m_assembledData.SetCount(fragHeader.packetLength);
				packet->m_fragFillFlagList.SetCount(GetAppropriateFlagListLength(fragHeader.packetLength));
				packet->m_createdTime = currtime;

				// ikpil.choi 2016-11-02 : bool 코드를 unit8_t 로 변경, 코드 직관성이 떨어지기 때문
				//  memset(packet->m_fragFillFlagList.GetData(), 0, sizeof(bool)* packet->m_fragFillFlagList.GetCount());
				memset_s(packet->m_fragFillFlagList.GetData(), sizeof(uint8_t)* packet->m_fragFillFlagList.GetCount(), 0, sizeof(uint8_t)* packet->m_fragFillFlagList.GetCount());

				// 큐에 추가한다.
				iPacket = packets->SetAt(fragHeader.packetID, packet);
			}
			else
			{
				// '조립중'인 패킷이 있다. 거기다 갱신한다.
				packet = iPacket->m_value;

				// '조립중' 인 패킷에 받은 fragment의 원하는 크기와 서로 다르면 리셋하고 다시 받는다.
				if (packet->m_assembledData.GetCount() != fragHeader.packetLength)
				{
					outError.Format(_PNT("UDP frag length is wrong #3! assembledDataCount=%d, packetLength=%d"),
						packet->m_assembledData.GetCount(), fragHeader.packetLength);
					//m_owner->m_owner->EnquePacketDefragWarning(senderAddr, _PNT("UDP frag length is wrong #3 !"));

					// 				// 롤백 => 이게 왜 있지? 그냥 무시하고 버려도 될 듯 한데?
					// 				if(packets->Count == 0)
					// 				{
					// 					m_addrPortToDefraggingPacketsMap.RemoveAtPos(iPackets);
					// 				}

					// 기 갖고있던 defrag중 상황을 버린다. 옛것이라고 간주된 경우일 수 있으므로.
					packets->RemoveAtPos(iPacket);
					packet->Drop();

					return AssembledPacketError_Error;
				}
			}

			// '조립중' 패킷에 신규 frag를 채우기
			if (!((int)fragHeader.fragmentID < (int)packet->m_fragFillFlagList.GetCount()))
			{
				outError = _PNT("UDP FragID is wrong!");
				//m_owner->m_owner->EnquePacketDefragWarning(senderAddr, _PNT("UDP FragID is wrong!"));
				return AssembledPacketError_Error;
			}

			if (!(fragPayloadLength + fragOffset <= (int)packet->m_assembledData.GetCount()))
			{
				outError = _PNT("UDP Frag Payload Length is wrong!");
				//m_owner->m_owner->EnquePacketDefragWarning(senderAddr, _PNT("UDP Frag Payload Length is wrong!"));
				return AssembledPacketError_Error;
			}

			if (!packet->m_fragFillFlagList[fragHeader.fragmentID])
			{
				packet->m_fragFillFlagList[fragHeader.fragmentID] = true;

				// 카운트 업
				packet->m_fragFilledCount++;
				packets->m_recentReceiveSpeed.Accumulate(fragLength, currtime); // 송신량 카운팅

				// ikpil.choi 2016-11-07 : memcpy_s 로 변경, destSize(2번째 인자) 값이 항상 올바른 값이여야 합니다.
				//UnsafeFastMemcpy(packet->m_assembledData.GetData() + fragOffset, msg.GetData() + msg.GetReadOffset(), fragPayloadLength);
				memcpy_s(packet->m_assembledData.GetData() + fragOffset, packet->m_assembledData.GetCount() - fragOffset, msg.GetData() + msg.GetReadOffset(), fragPayloadLength);
			}

			// 모든 frag를 채운 경우 출력 후 true 리턴하기
			if (packet->m_fragFillFlagList.GetCount() == packet->m_fragFilledCount)
			{
				if (packets->m_unreliableMessageLossRatio.AddPacketID((int)fragHeader.packetID) == false)
				{
					return AssembledPacketError_Assembling;
				}

				packets->m_unreliableMessageLossRatio.UpdateUnreliableMessagingLossRatioVars(fragHeader.packetID);

				output.m_senderAddr = senderAddr;
				output.TakeOwnership(packet);

				// 도마에서도 제거한다.
				packets->RemoveAtPos(iPacket);






				// 참고: 여기서 addrport 대응 항목을 제거하면 잦은 추가제거가 유발되므로 일단은 한동안은 갖고 있는다.

				return AssembledPacketError_Ok;
			}
		}
		else if (readSplitter == FullPacketSplitter)
		{
			assert(fragPayloadLength == msg.GetLength() - msg.GetReadOffset()); // 계산된 값이 같은지 재확인.
			if (fragPayloadLength > 0)
			{
				uint8_t* pPayload = msg.GetData() + msg.GetReadOffset();
				packets->m_recentReceiveSpeed.Accumulate(fragLength, currtime); // 송신량 카운팅

				// 2012.3.16 modify by seungwhan: 패킷 복사 방지 기능 3초간 packetID값을 저장하며, 중복 패킷은 버린다. UDP는 중복패킷이 날아올 수 있음으로 다른 처리는 하지 않는다.
				if (packets->m_unreliableMessageLossRatio.AddPacketID((int)fragHeader.packetID) == false)
				{
					return AssembledPacketError_Assembling;
				}

				packets->m_unreliableMessageLossRatio.UpdateUnreliableMessagingLossRatioVars(fragHeader.packetID);

				output.m_senderAddr = senderAddr;

				// 콜러가 DefraggingPacket을 원하니 맞춰주자.
				DefraggingPacket* packet = DefraggingPacket::NewInstance();
				packet->m_assembledData.SetCount(fragPayloadLength);

				// full packet을 바로 콜러에게 지급한다.
				// ikpil.choi 2016-11-07 : memcpy_s 로 변경, destSize(2번째 인자) 값이 항상 올바른 값이여야 합니다.
				// GetCount() 의 바이트가 fragPayloadLength 와 동등한 byte 인지 어떻게 보장하는가?
				//UnsafeFastMemcpy(packet->m_assembledData.GetData(), pPayload, fragPayloadLength);
				memcpy_s(packet->m_assembledData.GetData(), packet->m_assembledData.GetCount(), pPayload, fragPayloadLength);


				output.TakeOwnership(packet);



				return AssembledPacketError_Ok;
			}
		}



		return AssembledPacketError_Assembling;
	}
	CUdpPacketDefragBoard::CUdpPacketDefragBoard( CSuperSocket* dg )
	{
		m_owner = dg;

		// 이 map 클래스는 신축폭이 매우 크다. 따라서 rehash 역치를 최대한 크게 잡아야 한다.
		m_addrPortToDefraggingPacketsMap.SetOptimalLoad(0.30f,0.05f,8.0f);

		m_recentAssemblyingPacketIDsClearTime = 0;
	}

	void CUdpPacketDefragBoard::PruneTooOldDefragBoard()
	{
		int64_t currTime = GetPreciseCurrentTimeMs();
		for (AddrPortToDefraggingPacketsMap::iterator i = m_addrPortToDefraggingPacketsMap.begin(); i != m_addrPortToDefraggingPacketsMap.end();)
		{
			DefraggingPacketMap* v = i->GetSecond();

			// 두번째 맵에서 제거
			for (DefraggingPacketMap::iterator j = v->begin(); j != v->end();)
			{
				DefraggingPacket* u = j->GetSecond();
				if (currTime - u->m_createdTime > CNetConfig::AssembleFraggedPacketTimeoutMs)
				{
					u->Drop();
					j = v->erase(j);
				}
				else
					j++;
			}

			// 첫번째 맵에서 제거. 단, 충분히 오래되지 않은 것을 제거하면 수신속도 측정 정보가 증발해버리므로 주의.
			if (v->GetCount() == 0 && v->m_recentReceiveSpeed.IsRemovingSafeForCalcSpeed(currTime))
			{
				delete v;

				i = m_addrPortToDefraggingPacketsMap.erase(i);
			}
			else
				i++;
		}
	}

	void CUdpPacketDefragBoard::DoForLongInterval( int64_t curTime )
	{
		Proud::AssertIsLockedByCurrentThread(m_owner->m_sendQueueCS);
		bool clearRecents = false;

		assert(CNetConfig::RecentAssemblyingPacketIDsClearIntervalMs >= 3000);

		// 일정 시간에 한번씩 비워준다.
		if (curTime - m_recentAssemblyingPacketIDsClearTime > CNetConfig::RecentAssemblyingPacketIDsClearIntervalMs)
		{
			clearRecents = true;
			m_recentAssemblyingPacketIDsClearTime = curTime;
		}

		// 최근 수신속도 산출
		for (AddrPortToDefraggingPacketsMap::iterator i = m_addrPortToDefraggingPacketsMap.begin(); i != m_addrPortToDefraggingPacketsMap.end(); i++)
		{
			DefraggingPacketMap* packets = i->GetSecond();

			if (clearRecents)
			{
				packets->m_unreliableMessageLossRatio.ResetUnreliableMessagingLossRatioVars();
			}

			DoForLongInterval(packets, curTime);
		}

		PruneTooOldDefragBoard();
	}

	void CUdpPacketDefragBoard::DoForLongInterval( DefraggingPacketMap* packets, int64_t curTime )
	{
		Proud::AssertIsLockedByCurrentThread(m_owner->m_sendQueueCS);
		packets->m_recentReceiveSpeed.DoForLongInterval(curTime);
#ifdef UPDATE_TEST_STATS
		CTestStats::TestRecentReceiveSpeed = packets->m_sendBrake.GetRecentSendSpeed();
#endif
	}

	// src로부터 온 패킷들의 최근 수신속도
	int64_t CUdpPacketDefragBoard::GetRecentReceiveSpeed( AddrPort src )
	{
		DefraggingPacketMap* packets = NULL;
		if(m_addrPortToDefraggingPacketsMap.Lookup(src, packets))
		{
			return packets->m_recentReceiveSpeed.GetRecentSpeed();
		}

		return 0;
	}

	void CUdpPacketDefragBoard::Remove( AddrPort srcAddr )
	{
		AddrPortToDefraggingPacketsMap::CPair* p = m_addrPortToDefraggingPacketsMap.Lookup(srcAddr);
		if(p)
		{
			DefraggingPacketMap* pks = p->m_value;
			delete pks;
			m_addrPortToDefraggingPacketsMap.RemoveAtPos(p);
		}
	}

	void CUdpPacketDefragBoard::Clear()
	{
		for(AddrPortToDefraggingPacketsMap::iterator i=m_addrPortToDefraggingPacketsMap.begin();i!=m_addrPortToDefraggingPacketsMap.end();)
		{
			DefraggingPacketMap* v = i->GetSecond();

			// 두번째 맵에서 제거
			for(DefraggingPacketMap::iterator j=v->begin();j!=v->end();)
			{
				DefraggingPacket* u = j->GetSecond();
				u->Drop();
				j = v->erase(j);
			}

			delete v;
			i = m_addrPortToDefraggingPacketsMap.erase(i);
		}
	}

	// 없는 sendTo addr에 대해서는 100% 로스가 논리적으로 맞으나, caller에서 fallbackable UDP를 다루는 경우 100% 로스는 잘못된 값이므로 0%로 리턴한다.
	int CUdpPacketDefragBoard::GetUnreliableMessagingLossRatioPercent(AddrPort &senderAddr)
	{
		DefraggingPacketMap* packets;
		if(m_addrPortToDefraggingPacketsMap.Lookup(senderAddr, packets))
		{
			return packets->m_unreliableMessageLossRatio.GetUnreliableMessagingLossRatioPercent();
		}
		
		return 0;	
	}

	int CUdpPacketFragBoard::CPacketQueue::CPerPriorityQueue::GetTotalLengthInBytes()
	{
		int ret = 0;
		UdpPacketCtx* pk;
		for(pk = m_fraggableUdpPacketList.GetFirst();pk;pk=pk->GetNext())
		{
			ret += (int)pk->m_packet.GetCount();
		}

		for(pk = m_noFraggableUdpPacketList.GetFirst();pk;pk=pk->GetNext())
		{
			ret += (int)pk->m_packet.GetCount();
		}

		for (pk = m_checkFraggableUdpPacketList.GetFirst(); pk; pk = pk->GetNext())
		{
			ret += (int)pk->m_packet.GetCount();
		}

		for (pk = m_checkNoFraggableUdpPacketList.GetFirst(); pk; pk = pk->GetNext())
		{
			ret += (int)pk->m_packet.GetCount();
		}

		return ret;
	}

	CUdpPacketFragBoard::CPacketQueue::CPerPriorityQueue::~CPerPriorityQueue()
	{
		// 소유권을 이게 가진 packet 들을 모두 메모리 해제
		// 이게 콜 되는 동안에는 packet free list가 유효해야 한다. 
		while(true)
		{
			UdpPacketCtx* p = m_fraggableUdpPacketList.GetFirst();
			if (!p)
				break;

			m_fraggableUdpPacketList.Erase(p);
			m_owner->m_owner->DropPacket_(p);
		}

		while(true)
		{
			UdpPacketCtx* p = m_noFraggableUdpPacketList.GetFirst();
			if (!p)
				break;

			m_noFraggableUdpPacketList.Erase(p);
			m_owner->m_owner->DropPacket_(p);
		}

		while (true)
		{
			UdpPacketCtx* p = m_checkFraggableUdpPacketList.GetFirst();
			if (!p)
				break;

			m_checkFraggableUdpPacketList.Erase(p);
			m_owner->m_owner->DropPacket_(p);
		}

		while (true)
		{
			UdpPacketCtx* p = m_checkNoFraggableUdpPacketList.GetFirst();
			if (!p)
				break;

			m_checkNoFraggableUdpPacketList.Erase(p);
			m_owner->m_owner->DropPacket_(p);
		}

		m_uniqueIDToPacketMap.Clear();
	}

	CUdpPacketDefragBoard::AddrPortToDefraggingPacketsMap::~AddrPortToDefraggingPacketsMap()
	{
		for (iterator i = begin();i != end();i++)
		{
			DefraggingPacketMap* pks = i->GetSecond();
			if(pks != NULL)
				delete pks;
		}
	}

	CUdpPacketFragBoard::AddrPortToQueueMap::~AddrPortToQueueMap()
	{
		ClearAndFree();
	}

	void CUdpPacketFragBoard::AddrPortToQueueMap::ClearAndFree()
	{
		for (AddrPortToQueueMap::iterator ipq = begin();ipq!=end();)
		{
			CPacketQueue* p = ipq->GetSecond();
			//p->UnlinkSelf();
			delete p;
			ipq = erase(ipq);
		}

		//map_iterate_and_delete_FastHeap(*this, GetRefFastHeap());//이거는 unlinkself가 없어 수정
		Clear();
	}

	CUdpPacketFragBoardOutput::~CUdpPacketFragBoardOutput()
	{
		ResetForReuse();
	}

	void CUdpPacketFragBoardOutput::ResetForReuse()
	{		
		if(m_source)
		{
			intptr_t cnt = m_owningPackets.GetCount();
			UdpPacketCtx** src = m_owningPackets.GetData();
			for(intptr_t i=0;i<cnt;i++)
			{
				m_source->DropPacket_(*(src+i)); // 성능 좀 더 내자고 [] 안씀
			}
		}
		m_owningPackets.Clear();
		m_sendFragFrag.Clear();
		m_sendTo = AddrPort::Unassigned;
		m_source = NULL;
		m_ttl = -1;
		m_fragHeader.SetLength(0);
	}

	DefraggingPacket* DefraggingPacket::NewInstance()
	{
		// 성능에 민감하고 NC or NS 안에서만 쓰이므로 GetUnsafeRef를 쓴다.
		return CClassObjectPool<DefraggingPacket>::GetUnsafeRef().NewOrRecycle();
	}

	void DefraggingPacket::Drop()
	{
		// 내용물 정리 후
		m_fragFilledCount = 0;
		m_createdTime = 0;

		m_fragFillFlagList.ClearAndKeepCapacity();
		m_assembledData.ClearAndKeepCapacity();

		// 풀에 다시 넣기
		CClassObjectPool<DefraggingPacket>::GetUnsafeRef().Drop(this);
	}

}