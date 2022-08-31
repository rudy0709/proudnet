#pragma once

#include "../include/FakeClr.h"
#include "../include/FastList.h"
#include "../include/FastMap.h"
#include "../include/ListNode.h"
#include "../include/LookasideAllocator.h"
#include "../include/FixedLengthArray.h"
#include "../include/NetConfig.h"
#include "../include/Enums.h"
#include "../include/AddrPort.h"
#include "UniqueID.h"
#include "FilterTag.h"
#include "SendBrake.h"
#include "SendOpt.h"
#include "UnreliableMessageLossMeasurer.h"
#include "FreeList.h"
#include "BufferSegment.h"

//#pragma pack(push,8)

namespace Proud
{
	typedef int32_t PacketIDType; // WORD로 하면 더 효과적일지 모르겠으나 아직은 모험하지 말자.

	// 패킷 조립 과정 결과
	enum AssembledPacketError
	{
		AssembledPacketError_Ok, // frag 소화 끝. 조립 완전히 된 놈이 나왔음
		AssembledPacketError_Assembling,	// frag 소화 끝. 아직 조립중.
		AssembledPacketError_Error,	// 깨진 frag가 나와서 버렸음
	};

	// UDP로 전송되는 datagram의 헤더를 채울 정보
	// 이것 자체가 그대로 저장되는 것이 아니고, WriteScalar 등을 통해 1-2바이트 더 줄어든 데이터로 전송
	struct FragHeader
	{
		// packet splitter. 타 멤버들의 크기와 식별자등이 조립되어 들어간다.
		// 주의: Java때문에 이를 signed로 바꾸지 말것. >>연산자를 쓰는 곳이 있어서다.
		uint16_t splitterFilter;

		// 지금 보내는 datagram에 해당하는 packet의 총 크기.
		// 이 fragment뿐만 아니라 모든 fragment의 총합이다.
		int packetLength;

		// 지금 보내는 datagram 즉 fragment의 packet ID
		PacketIDType packetID;

		// fragment ID. 0이면 맨 앞이다. 최대값은 (fragmentID / MTU) 쯤이다.
		int fragmentID;
	};

	class PacketQueue;
	class CSendFragRefs;
	class CUdpPacketFragBoardOutput;
	class CSuperSocket;

	// unique ID를 가지는 패킷의 정보
	// normalize 즉 송신큐에 중복 의미의 패킷이 아직 전송 안되고 보류중이면 그걸 교체하는 역할을 한다.
	class CUniqueIDPacketInfo
	{
	public:
		// *** TODO: 이 변수는 제거해서 수신자 구별 없이 unique하게 하던지,
		// 수신자 구별할 수 있도록 final send dest hostID의 의미를 가지게 하던지,
		// 결정하자. 지금은 안 쓰이는 변수다.
		// 당연히 타 언어도 같이 수정해야.

		// P2P relay나 routed multicast되는 메시지의 경우(이하 릴레이로 통칭)
		// 릴레이 메시지와 서버에게 주는 메시지가 겹쳐서 hostID 와 unique ID 를 모두 검사해야 한다.
		// 안그러면, 서버한테 받아야 할 unreliable 메시지와 피어가 받아야 할 unreliable 메시지의 unique ID가 같을 경우
		// 둘 중 하나가 증발해 버리는 문제가 생긴다.
		// unique ID는 최종 수신 대상이 다른 경우 서로 영향을 주지 말아야 하는데 말이다.
		// 따라서 unique ID와 relay 2 message가 수신될 최종 클라의 HostID를 넣는다.
		// 릴레이되지 않으면 이 값은 none을 넣어야 한다.
		HostID m_hostID_NOT_USED_YET;

		// unique ID
		UniqueID m_uniqueID;

		CUniqueIDPacketInfo()
		{
			m_hostID_NOT_USED_YET = HostID_None;
		}
	};

	// 사용자가 보내려고 하는 메시지. 아직 MTU 크기로 쪼개지기 전이다.
	class UdpPacketCtx :public CListNode<UdpPacketCtx>
	{
	public:
		// 보낼 패킷. MTU보다 클 수 있다.
		ByteArray m_packet;

		// 이 송신큐에 들어간 시간.
		// 오래된 unreliable 메시지인 경우 보내지 말고 폐기하기 위함.
		// 혼잡 제어=>송신 지연=>메모리 증가=>서버 혹은 모바일폰 다운 예방차.
		int64_t m_enquedTime;

		// 이 패킷의 TTL 값.
		// 인접해 있는 보낼 패킷의 TTL과 서로 다른 경우 서로 다른 sendto()를 호출해야 하므로 이 값을 쓴다.
		int m_ttl;

		// 중복 의미의 패킷을 제거하는 목적으로 쓰이는 unique ID and final relay 2 dest client HostID
		CUniqueIDPacketInfo m_uniqueIDInfo;

		inline UdpPacketCtx() { m_ttl = -1; m_enquedTime = 0; }

		// CFastArray의 동명 함수 주석 참고.
		void SuspendShrink()
		{
			m_packet.SuspendShrink();
		}
		void OnRecycle() {}
		void OnDrop()
		{
			m_packet.Clear();
			m_uniqueIDInfo = CUniqueIDPacketInfo();
		}

	private:
		// operator=를 이용한 복사가 불가능하게 하기 위해
		UdpPacketCtx(const UdpPacketCtx& src);
		UdpPacketCtx& operator=(const UdpPacketCtx& src);

	};

	class CIssueSendResult
	{
	public:
		// 함수 호출 후 true로 세팅되면, caller는 sendto()를 즉시 하라.
		bool m_issueSendNow;

		CIssueSendResult()
		{
			m_issueSendNow = false;
		}
	};

/*	이 클래스는 제거되고, 이 메서드는 CNetCoreImpl로 이동됐다.
	class IUdpPacketFragBoardDg
	{
	public:
		virtual ~IUdpPacketFragBoardDg() {}
		virtual int GetOverSendSuspectingThresholdInBytes() = 0;
	};*/

	class CUniqueIDPacketInfoTraits
	{
	public:
		typedef const CUniqueIDPacketInfo& INARGTYPE;
		typedef CUniqueIDPacketInfo& OUTARGTYPE;

		inline static uint32_t Hash(const CUniqueIDPacketInfo& element) throw()
		{
			return UniqueIDTraits::Hash(element.m_uniqueID) ^ CPNElementTraits<HostID>::Hash(element.m_hostID_NOT_USED_YET);
		}

		inline static bool CompareElements(const CUniqueIDPacketInfo& element1, const CUniqueIDPacketInfo& element2)
		{
			return (element1.m_uniqueID == element2.m_uniqueID && element1.m_hostID_NOT_USED_YET == element2.m_hostID_NOT_USED_YET);
		}

		inline static int CompareElementsOrdered(const CUniqueIDPacketInfo& element1, const CUniqueIDPacketInfo& element2)
		{
			if (element1.m_uniqueID < element2.m_uniqueID)
				return -1;
			else if (element1.m_uniqueID == element2.m_uniqueID)
			{
				if (element1.m_hostID_NOT_USED_YET < element2.m_hostID_NOT_USED_YET)
					return -1;
				else if (element1.m_hostID_NOT_USED_YET == element2.m_hostID_NOT_USED_YET)
					return 0;
				else
					return 1;
			}
			else
				return 1;
		}
	};

	typedef CFastMap2<CUniqueIDPacketInfo, UdpPacketCtx*, int, CUniqueIDPacketInfoTraits> UniqueIDPacketInfoToPacketMap;

	// 보낼 패킷을 frag들로 쪼개는 곳. 송신자에서 씀.
	// coalesce 때문에 dictionary class로 만들었다.
	class CUdpPacketFragBoard
	{
	public:
		// MTU 크기로 아직 쪼개지 않은 패킷들 콜렉션, 각 우선순위들을 모두 갖고 있음
		class CPacketQueue
			: public CListNode<CPacketQueue> // CUdpPacketFragBoard.m_sendReadyList에 등록되기 위한 용도
		{
		public:
			// 1개 우선순위 내의 패킷 모음
			class CPerPriorityQueue
			{
			public:
				/* 일반 message들이 저장되는 곳
				CTcpLayer_Common::AddSplitterButShareBuffer에 의해 추가된 헤더도 여기 들어간다.

				TODO: 단 1개의 message만 들어갔고, 총 크기가 MTU 이하이면, CTcpLayer_Common::AddSplitterButShareBuffer
				에 의해 붙는 헤더 크기도 최적화해야 한다. */
				CListNode<UdpPacketCtx>::CListOwner m_fraggableUdpPacketList;

				// 이미 coalesce & MTU 크기 제한 처리를 해온 message들은 여기 저장된다.
				// 예: relayed message 2, reliable UDP frame
				// 이들은 따로 frag-defrag 처리를 하지 않음으로, 성능 향상 & 추가적인 쪼개짐 오류를 예방한다.
				CListNode<UdpPacketCtx>::CListOwner m_noFraggableUdpPacketList;

				/* 사용자가 packet을 이 queue에 추가하면 일단 아래 두 멤버중 하나에 들어간다. 여기서는 uniqueID 검사를 하지 않은 채 들어간다.  */
				CListNode<UdpPacketCtx>::CListOwner m_checkFraggableUdpPacketList;
				CListNode<UdpPacketCtx>::CListOwner m_checkNoFraggableUdpPacketList;

				// 송신큐에서 빠른 속도로 중복 패킷을 찾아 제거하기 위함
				// thinned queue에 있는 패킷만 가리킨다. non thinned queue는 비해당.
				// NOTE: uniqueID.value!=0
				UniqueIDPacketInfoToPacketMap m_uniqueIDToPacketMap;

				int64_t m_nextNormalizeWorkTime;

				CPacketQueue* m_owner;

				int GetTotalLengthInBytes();
				inline bool IsEmpty() const
				{
					return m_fraggableUdpPacketList.IsEmpty() && m_noFraggableUdpPacketList.IsEmpty() && m_checkFraggableUdpPacketList.IsEmpty() && m_checkNoFraggableUdpPacketList.IsEmpty();
				}

				CPerPriorityQueue()
				{
					m_nextNormalizeWorkTime = 0;
				}
				~CPerPriorityQueue();
			};

			// 패킷 송신 우선순위별로 정렬된다. 0번째 항목은 ProudNet level 전용이다.
			// PacketQueue 객체 자체는 new/delete 횟수가 잦으므로 고정 크기 배열.
			// 단, 바운드 체크를 준수하자.
			CFixedLengthArray<MessagePriority_LAST, CPerPriorityQueue> m_priorities;
		private:
			//static CLookasideAllocator* GetAlloc();

			CPacketQueue(const CPacketQueue& src);
			CPacketQueue& operator=(const CPacketQueue& src);


		public:
			CPacketQueue();
			~CPacketQueue();

			void ClearFragBoardedPackets();
			bool HasPacketAndTimeToSendReached(int64_t currTime, bool calledBySendCompletion);

			// 도마의 상태를 리셋한다. 도마가 매번 비워질 때마다 호출된다.
			inline void ResetFragBoardState()
			{
				m_fragBoardTotalBytes = 0;
				m_destFragID = 0;
				m_globalOffsetInFragBoard = 0;
				m_localOffsetInFragBoard = 0;
				m_srcIndexInFragBoard = 0;

				// 아래 속도 제어를 위한 상태 변수는 초기화하면 안됨!
				//				m_sendBrake = CSendBrake();
				//				m_sendSpeed = CSendSpeedMeasurer();
			}

			void AccumulateSendBrake(int length);

			// 가장 마지막에 패킷이 추가되거나 제거된 시간. coalesce case도 포함해서.
			int64_t m_lastAccessedTime;

			/* coalesce time. 즉, 보낼 패킷이 있음에도 불구하고 패킷 헤더 통신
			낭비를 줄이기 위해 패킷을 일부러 이 시간까지 안보낸다.*/
			int64_t m_nextTimeToCoalescedSend;

			AddrPort m_remoteAddr;
			FilterTag::Type m_filterTag;
			CUdpPacketFragBoard* m_owner;

			int GetTotalCount();

			// 이 함수를 별도 만들어서 성능 가속화.
			inline bool IsEmpty() const
			{
				const CPerPriorityQueue* pPriorities = m_priorities.GetData();  // 바운드체크 피함
				for (int i = 0; i<MessagePriority_LAST; i++)
				{
					if (pPriorities[i].m_fraggableUdpPacketList.GetCount() > 0)
						return false;
					if (pPriorities[i].m_noFraggableUdpPacketList.GetCount() > 0)
						return false;
					if (pPriorities[i].m_checkFraggableUdpPacketList.GetCount() > 0)
						return false;
					if (pPriorities[i].m_checkNoFraggableUdpPacketList.GetCount() > 0)
						return false;
				}

				if (m_fragBoardedPackets.GetCount() > 0)
					return false;
				return true;
			}

			inline bool HasRing0OrRing1Packet()
			{
				if (!m_priorities[MessagePriority_Ring0].IsEmpty() ||
					!m_priorities[MessagePriority_Ring1].IsEmpty())
					return true;

				return false;
			}

			int GetTotalLengthInBytes();
			void PopFragmentOrFullPacket(int64_t curTime, CUdpPacketFragBoardOutput &output);

			void NormalizePacketQueue();
		private:
			void NormalizePacketQueue_Internal(CListNode<UdpPacketCtx>::CListOwner* checkList,
				CListNode<UdpPacketCtx>::CListOwner* neatList,
				UniqueIDPacketInfoToPacketMap* uniqueIDMap,
				int64_t curTime,
				bool removeOldPacket);

		private:
			void CompactFragBoardedPacketForOneSmallMessage();
		public:

			//void PopHigherPriorityFirstWithCoalesce_SLOWTEMP(double curTime,UdpSendOrRecvData &output) const;

			/* 도마 위에 올라간 메시지들.
			- 사용자가 보내려는 1개 이상의 메시지의 집합. MTU보다 크거나 작을 수 있다. (메시지를 일단 도마 위에 얹은 후 MTU보다 크면 중단하는 방식이다)
			- 이들을 1개의 긴 스트림으로 이어붙인 것이 도마이다.
			- 그리고 MTU 이하 단위로 하나씩 뜯어낸 후 헤더(FragHeader)를 붙인다. 그게 fragment다.
			- Fragment가 실제 UDP 소켓으로 전달된다. */
			CFastArray<UdpPacketCtx*, false, true, int> m_fragBoardedPackets;
			PacketIDType m_lastFragmentedPacketID;	// 도마 위에서 썰어지고 있는 메시지 뭉치에 배정될 packet ID

			//도마 위에 올라간 메시지들의 총 길이(byte)
			int m_fragBoardTotalBytes;

			// 다음 pop fragment 함수가 실행될 때 썰려져 나올 fragment가 가질 ID. 0부터 시작한다.
			int m_destFragID;

			// 도마 위에 올라간 메시지들 중에 썰려지고 있는 메시지 안에서 현재까지 썰어진 byte 위치. 도마의 첫번째 메시지들 중 맨 앞을 0으로 기준으로 한다.
			int m_globalOffsetInFragBoard;

			// 도마 위에 올라간 메시지들 중에 썰려지고 있는 메시지 안에서 현재까지 썰어진 byte 위치. 썰리고 있는 메시지의 앞을 0으로 기준으로 한다.
			int m_localOffsetInFragBoard;

			// 도마 위에 올라간 메시지들(총 크기: MTU이하) 중에 썰려지고 있는 메시지의 인덱스. m_fragBoardedPackets 안에 있는 것들 중 말이다.
			int m_srcIndexInFragBoard;

			// 도마 위에서 썰어지지 않고 전송되는 full packet의 packet ID. m_nextFragmentedPacketID과 별개로 지속적으로 증가한다.
			PacketIDType m_lastFullPacketID;

			int m_coalesceIntervalMs;

			CSendBrake m_sendBrake;
		};
	private:
		/* packet queue안에 송신이 되어야 할 packet들이 있는 것들의 목록이다.
		heartbeat에서, 이들을 뒤져서 issue send를 수행하는 목적으로 사용된다.

		send를 할 packet이 CPacketQueue안에 들어오면=>이 목록에 추가
		issue send를 하려고 fragment or full packet을 pop하면=>이 목록에서 제거되거나(텅 비어진 경우) 이 목록의 맨 뒤로 옮겨짐(아직도 남은 경우)
		*/
		CPacketQueue::CListOwner m_sendReadyList;

		bool m_lastPopFragmentSurpressed_ValidAfterPopFragment;

		CSuperSocket* m_owner;

		class AddrPortToQueueMap :public CFastMap2<AddrPort, CPacketQueue*, int>
		{
		public:
			AddrPortToQueueMap()
			{
				SetOptimalLoad_BestLookup(); // 증감폭이 워낙 큰데다 rehash cost가 크기 때문에
			}
			~AddrPortToQueueMap();

			void ClearAndFree();
		};

		AddrPortToQueueMap m_addrPortToQueueMap;

		// 마지막으로 issue send를 해놓은 packet queue
		// 실제 송신 걸어진 속도를 체크하는 목적으로도 사용된다
		// recvfrom()과 달리 send를 수행한 것을 찾을 수 없기 때문에 이렇게 한다.
		CPacketQueue* m_curPacketQueue;

		//void RemoveKeyIfEmpty(PacketQueuePtr pq);
		void AssertConsist();
		void AddToSendReadyListOnNeed(CPacketQueue* packetQueue, int64_t currTime);

	private:
		// 재활용 객체 모음. 즉 객체 풀링용.
		CObjectPool<UdpPacketCtx> m_packetFreeList;
	public:
		//test
		void SetCoalesceInterval(const AddrPort &addr, int interval);

		UdpPacketCtx* NewOrRecyclePacket_();
		void DropPacket_(UdpPacketCtx* packet);

		void Remove(AddrPort key);
		void Clear();

		// 이 함수는 Functor template에 의해 호출된다. 자주 호출되므로 성능을 위해.
		bool IsLastPopFragmentSurpressed_ValidAfterPopFragment() { return m_lastPopFragmentSurpressed_ValidAfterPopFragment; }

		bool m_enableSendBrake; // 이름 그대로. 서버에서는 이걸 false로 할거다.

		CUdpPacketFragBoard(CSuperSocket* owner);
		~CUdpPacketFragBoard();

		//void AddNewPacket(FilterTag::Type filterTag, AddrPort sendTo, const CSendFragRefs &sendData, double addedTime,UdpSendOpt sendOpt);
		void AddNewPacket(
			HostID finalDestHostID,
			FilterTag::Type filterTag,
			AddrPort sendTo,
			const CSendFragRefs &sendData,
			int64_t addedTime,
			SendOpt sendOpt,
			CIssueSendResult& sendResult);

		int GetTotalPacketCountOfAddr(const AddrPort &addr);
		inline bool IsUdpSendBufferPacketEmpty(const AddrPort &addr)
		{
			CPacketQueue* pq = NULL;

			// cache for frequent case
			if (m_addrPortToQueueMap.GetCount() == 0)
				return true;

			if (m_addrPortToQueueMap.TryGetValue(addr, pq))
			{
				return pq->IsEmpty();
			}
			else
				return 0;
		}

		uint32_t GetPacketQueueTotalLength();

		int GetPacketQueueTotalLengthByAddr(AddrPort addr);
		bool PopAnySendQueueFilledOneWithCoalesce(CUdpPacketFragBoardOutput &output, int64_t currentTime, bool calledBySendCompletion);
		void DoForLongInterval(int64_t curTime);
		void DoForShortInterval(int64_t curTime);
		bool HasRing0OrRing1Packet();
		void InitHashTableForClient();
		void SetReceiveSpeedAtReceiverSide(AddrPort dest, int64_t speed, int packetLossPercent, int64_t curTime);
		void AccumulateSendBrake(int length);
		void SetTcpUnstable(int64_t curTime, bool unstable);
		size_t GetAddrPortToQueueMapKeyCount() { return m_addrPortToQueueMap.GetCount(); }

		static void WriteFragHeader(CMessage &msg, FragHeader& header);
		static bool ReadFragHeader(CMessage &msg, FragHeader &header);
	};

	/* send issue를 위해 packet fragger로부터 받은 출력물.
	packet fragger는 0개 이상의 frag를 이 객체에 전달한다. 그리고 전달된 frag들의 파괴 권리를 이 객체가 가진다.
	이 객체가 파괴되기 전에 packet fragger가 파괴되어서는 안된다. */
	class CUdpPacketFragBoardOutput
	{
	public:
		CFragmentedBuffer m_sendFragFrag;
		CMessage m_fragHeader; // CFragmentedBuffer에서 이걸 참조함

		CUdpPacketFragBoard* m_source;
		CFastArray<UdpPacketCtx*, false, true, int> m_owningPackets; // m_sendFragFrag이 참조하고 있을 수 있음. frag board가 계속 갖고 있을 수도 있고. 하여튼 여기에 들어오면 곧 파괴된다.
		AddrPort m_sendTo;
		int m_ttl;

		inline CUdpPacketFragBoardOutput()
		{
			m_source = NULL;
			m_fragHeader.UseInternalBuffer();
		}
		~CUdpPacketFragBoardOutput();
		void ResetForReuse();
	};

	// 조립중인 패킷 1개
	// 주의: new or delete를 쓰지 말고 NewInstance or Drop을 쓸 것! delete는 g_pool 전용!
	class DefraggingPacket
	{
	public:
		CFastArray<bool, false, true, int> m_fragFillFlagList;
		CFastArray<uint8_t, false, true, int> m_assembledData; // ByteArray는 자체적 메모리 관리 기능이 있으므로 안됨
		int m_fragFilledCount;
		int64_t m_createdTime;

		inline DefraggingPacket()
			: m_fragFilledCount(0)
			, m_createdTime(0)
		{
		}

	public:

		static DefraggingPacket* NewInstance();

		void Drop();

		// called by CObjectPool.
		void SuspendShrink()
		{
			m_fragFillFlagList.SuspendShrink();
			m_assembledData.SuspendShrink();
		}

		void OnDrop()
		{
			m_fragFillFlagList.OnDrop();
			m_assembledData.OnDrop();
		}
		void OnRecycle()
		{
			m_fragFillFlagList.OnRecycle();
			m_assembledData.OnRecycle();
		}
	};

	struct DefraggingPacketMap;

	// 1개 주소로부터 도착한 조립중인 패킷들의 집합
	struct DefraggingPacketMap :public CFastMap2<PacketIDType, DefraggingPacket*, int>
	{
	public:
		CUnreliableMessageLossMeasurer m_unreliableMessageLossRatio;

		CRecentSpeedMeasurer m_recentReceiveSpeed;

		DefraggingPacketMap()
		{
			SetOptimalLoad_BestLookup(); // 증감폭이 워낙 큰데다 rehash cost가 크기 때문에
		}

		~DefraggingPacketMap()
		{
			typedef CFastMap2<PacketIDType, DefraggingPacket*, int>::iterator Iter;
			// value는 strong pointer. 다 지우자
			for (Iter i = begin(); i != end(); i++)
			{
				DefraggingPacket* pk = i->GetSecond();
				pk->Drop();
			}
		}
	};

	// 조립이 다 끝난 패킷
	class CAssembledPacket
	{
		DefraggingPacket* m_packetStrongPtr; // string pointer!
	public:
		CAssembledPacket()
		{
			m_packetStrongPtr = NULL;
		}
		~CAssembledPacket()
		{
			if (m_packetStrongPtr)
			{
				m_packetStrongPtr->Drop(); // 객체 풀에 반환
			}
		}

		void TakeOwnership(DefraggingPacket* ptr)
		{
			assert(!m_packetStrongPtr);
			m_packetStrongPtr = ptr;
		}

		inline uint8_t* GetData()
		{
			return m_packetStrongPtr->m_assembledData.GetData();
		}

		inline int GetLength()
		{
			return (int)m_packetStrongPtr->m_assembledData.GetCount();
		}

		AddrPort m_senderAddr;
	};

	// frag들을 받아 패킷으로 조립하는 공간. 수신자에서 씀.
	class CUdpPacketDefragBoard
	{
		CSuperSocket* m_owner;

		// 2중 맵이다. 첫번째는 AddrPort, 두번째는 PacketID이다.
		// 2중 맵으로 굳이 되어있는 이유: AddrPort별 최근 수신 속도 산출을 위해.
		class AddrPortToDefraggingPacketsMap :public CFastMap2<AddrPort, DefraggingPacketMap*, int>
		{
		public:
			AddrPortToDefraggingPacketsMap()
			{
				SetOptimalLoad_BestLookup(); // 증감폭이 워낙 큰데다 rehash cost가 크기 때문에

			}
			~AddrPortToDefraggingPacketsMap();
		};
		AddrPortToDefraggingPacketsMap m_addrPortToDefraggingPacketsMap;

		inline static int GetAppropriateFlagListLength(int packetLength)
		{
			if (packetLength <= 0)
				return 0;

			return (packetLength - 1) / CNetConfig::MtuLength + 1;
		}

		void PruneTooOldDefragBoard();

		// 마지막으로 위 변수를 청소한 시간. 몇 초 간격으로 상기 값을 청소한다.
		int64_t m_recentAssemblyingPacketIDsClearTime;

	public:
		CUdpPacketDefragBoard(CSuperSocket* dg);
		AssembledPacketError PushFragmentAndPopAssembledPacket(
			uint8_t* fragData,
			int fragLength,
			AddrPort senderAddr,
			HostID volatileSrcHostID,
			int64_t currtime,
			CAssembledPacket& output,
			String& outError);

		void DoForLongInterval(int64_t curTime);

		int64_t GetRecentReceiveSpeed(AddrPort src);
		void Remove(AddrPort srcAddr);
		void Clear();
		int GetUnreliableMessagingLossRatioPercent(AddrPort &senderAddr);
	private:
		void DoForLongInterval(DefraggingPacketMap* packets, int64_t curTime);
	};

}

//#pragma pack(pop)
