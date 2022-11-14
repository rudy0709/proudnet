/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#pragma once

#include "../include/Enums.h"
#include "../include/FakeClr.h"
#include "../include/AddrPort.h"
#include "FreeList.h"
#include "StreamQueue.h"
//#include "TypedPtr.h"
//#include "iocp.h"
#include "SendBrake.h"
#include "SendOpt.h"
//#include "TcpSend.h"
#include "FastList2.h"
#include "FastMap2.h"


namespace Proud
{
	class CSendFragRefs;
	class CStreamQueue;
	class CReceivedMessage;
	class CReceivedMessageList;
	class CFragmentedBuffer;
	class CNetClientImpl;
	class CMessage;

	// TCP는 stream 단위이다. 이를 메시지 단위로 쪼개는 기본 계층을 여기서 처리한다.
	// UDP에서도 coalesce로 인해 1개 이상의 메시지가 1개 datagram에 들어갈 수 있다. 
	// 그래서 거기서도 이것이 쓰인다.
	class CTcpLayer_Common
	{
	public:
		/* TinySplitter는 message payload 길이가 생략되어 1~2바이트 추가 절약을 한다.
		1바이트는 별거 아닌 것 같지만 소량의 통신량을 다수의 피어와 P2P 통신하는 경우에도 효과적임.
		주의: TinySplitter는 (수신된 데이터 길이-현재 read offset)을 근거로
		payload length를 얻기 때문에 메시지 단위가 아닌 TCP stream에서는 사용되지 않아야 한다. */
		enum { Splitter = 0x5713, HasMessageID = 0x5714, TinySplitter = 0x5813, SimpleSplitter = 0x0909 };

		static void AddSplitterButShareBuffer(
			const CSendFragRefs &payload,
			CSendFragRefs &ret,
			CMessage& header,
			bool simplePacketMode = false);
		static void AddSplitterButShareBuffer(
			int messageID, 
			const CSendFragRefs &payload, 
			CSendFragRefs &ret, 
			CMessage& header, 
			bool simplePacketMode);
		static int ExtractMessagesFromStreamAndRemoveFlushedStream(
			CStreamQueue &recvStream,
			CReceivedMessageList &extractedMessageAddTarget,
			HostID senderHostID,
			int messageMaxLength,
			ErrorType& outError,
			bool simplePacketMode = false);
	};

	/* 받은 스트림 혹은 데이터그램에서 1개 이상의 메시지를 추출한다.
	TCP or UDP에서 모두 쓰임. (클래스 이름이 좀 에러)

	스트림의 경우, 현재 수신된 총 누적 스트림이어야.
	socket recv()로부터 받은것만 입력하면 안됨.
	그동안 받은 모든 스트림을 합쳐서 인자로 넣어야 함.
	그리고 메시지로 추출된 것은 스트림에서 빼내야 하고.

	데이터그램의 경우 누적시킬 필요없이 그냥 다 꺼내면 됨.
	데이터그램의 offset 0은 앞서 메시지의 중간을 가리키는 일이 절대 없으므로. */
	class CTcpLayerMessageExtractor
	{
	public:
		// 받은 데이터(스트림 혹은 메시지)
		const uint8_t* m_recvStream;
		// 크기(바이트)
		int m_recvStreamCount;
		// 추출된 메시지가 저장될 output
		CReceivedMessageList* m_extractedMessageAddTarget;
		// 이 메시지를 누구로부터 받았는지?
		HostID m_senderHostID;
		// 추출하는 메시지의 최대 크기
		int m_messageMaxLength;
		//CFastHeap* m_unsafeHeap;

		// 추출에 성공한 메시지들이 차지했던 크기 output.
		// 스트림의 경우 이 값만큼 pop해버리면 된다.
		int m_outLastSuccessOffset;

		// 이 메시지를 누구로부터 받았는가?
		AddrPort m_remoteAddr_onlyUdp;

		CTcpLayerMessageExtractor();
		int Extract(ErrorType &outError, bool simplePacketMode = false);
	};

	typedef CFastMap2<UniqueID, Position, int, UniqueIDTraits> UniqueIDToPacketMap;

	class TcpPacketCtx 
	{
	public:
		// TODO: 추후 이것의 타입을 SendFragRefs로 바꾸자. SendFragRefs는 1개 이상의 ByteArrayPtr or CMessage를 share하는 형식으로 바꾸고. 이러한 수정은 타 언어에도 적용되어야.
		ByteArray m_packet;
		
		// 같은 값을 가진 패킷이 들어올 경우 기존 패킷을 교체함. 스로틀링을 위함.
		UniqueID m_uniqueID;
		
		// 이 송신큐에 들어간 시간. 
		// 오래된 unreliable 메시지인 경우 보내지 말고 폐기하기 위함. 
		// 혼잡 제어=>송신 지연=>메모리 증가=>서버 혹은 모바일폰 다운 예방차.
		int64_t m_enquedTime;

		// 사용자가 unreliable로 보내려던 메시지지만 미 홀펀칭으로 인한 TCP 전송시,
		// 그래도 어떤 의도로 보내려고 했던건지는 알고 있어야 한다. 그래야 10초 패킷 버리기라던지 등을 처리하니까.
		// 아래 변수들은 그러한 목적임.
		MessageReliability m_reliability;
		MessagePriority m_priority;

		inline TcpPacketCtx()
		{
			m_enquedTime = 0; 
			m_reliability = MessageReliability_LAST;
			m_priority = MessagePriority_LAST;
		}
		
		void FromSendOpt(const SendOpt& opt)
		{
			m_uniqueID = opt.m_uniqueID; 
			m_reliability = opt.m_reliability;
			m_priority = opt.m_priority;
		}
	};

	// reliable send 자체는 우선순위가 모순이다. 따라서 아예 지원하지 않는다.
	// 송신 속도 조절기도 포함됨
	class CTcpSendQueue
	{
		CObjectPool<TcpPacketCtx> m_packetPool;

		/* 반드시 list의 형태이어야 함!
		PeekSendBuf에 의해 peek된 것은 send queue critsec으로
		보호되지 않는 상태로 syscall이 됨.
		한편, 사용자가 AddToSendQueue를 써서 이 send queue에 push back을 함.
		즉 여러 스레드에 의해 이 데이터가 접근된다. 이것이 안전하기 위해
		데이터 내용물의 이동이 없는 list이어야 함. */
		typedef CFastList2<TcpPacketCtx*, int> QueueType;
		QueueType m_thinnedQueue;
		QueueType m_nonThinnedQueue;
		// 송신큐에서 빠른 속도로 중복 패킷을 찾아 제거하기 위함
		// thinned queue에 있는 패킷만 가리킨다. non thinned queue는 비해당.
		UniqueIDToPacketMap m_uniqueIDToPacketMap;
	
		// 복사 연산 금지
		CTcpSendQueue(const CTcpSendQueue& src);
		CTcpSendQueue& operator=(const CTcpSendQueue& src);
		
		// 큐의 최상단에 있었던, 앞서 TCP send에서 일부분만 보내졌던 패킷. 어디서부터 잘려있는지는 m_partialSentLength에.
		// 여기로 옮겨진 패킷은 위 queue에서 이미 사라진 상태.
		TcpPacketCtx *m_partialSentPacket;

		// 그 패킷의 보내진 양.
		int m_partialSentLength;

		// 송신큐 들어있는 현재 총량 bytes
		int m_totalLength;
		int m_nonThinnedQueueTotalLength;

		int64_t m_nextNormalizeWorkTime;

		void CheckConsist();

		// TCP 자체가 ack에 의한 sender window slide 기능이 존재->송신량 자연 조절->따라서 불필요함.
		//CRecentReceiveSpeedAtReceiverSide m_recentReceiveSpeedAtReceiverSide;
	public:
		// TCP 자체가 ack에 의한 sender window slide 기능이 존재->송신량 자연 조절->따라서 불필요함.
		//CSendBrake m_sendBrake;
		// TCP 자체가 ack에 의한 sender window slide 기능이 존재->송신량 자연 조절->따라서 불필요함.
		//CAllowedMaxSendSpeed m_allowedMaxSendSpeed;
		// TCP 자체가 ack에 의한 sender window slide 기능이 존재->송신량 자연 조절->따라서 불필요함.
		//CRecentSpeedMeasurer m_sendSpeed;

		CTcpSendQueue();
		~CTcpSendQueue();

		void PushBack_Copy(const CSendFragRefs& sendData, const SendOpt& sendOpt);
		void PeekSendBuf(CFragmentedBuffer& output, int fragCount = INT32_MAX);
		void NormalizePacketQueue();

		int GetLength() { return m_totalLength; }
		int GetTotalLength() { return m_totalLength + m_nonThinnedQueueTotalLength; }

		void PopFront(int length);
		void DoForLongInterval();
	};

	void SetTcpDefaultBehavior_Client(CFastSocket *socket);
	void SetTcpDefaultBehavior_Server(CFastSocket *socket);
	void SetUdpDefaultBehavior_Client(CFastSocket *socket);
	void SetUdpDefaultBehavior_Server(CFastSocket *socket);
	void SetUdpDefaultBehavior_ServerStaticAssigned(CFastSocket *socket);
}