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
#include "../include/MilisecTimer.h"
#include "FreeList.h"
#include "StreamQueue.h"
//#include "TypedPtr.h"
//#include "iocp.h"
#include "SendBrake.h"
#include "SendOpt.h"
#include "FastList2.h"
#include "FastMap2.h"
#include "ReceivedMessageList.h"
#include "BufferSegment.h"


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

	private:
		// 매우 단순한 난수 알고리즘. 난수 복잡도보다도 얻는 속도가 절대적으로 중요하다.
		// 따라서 여러 스레드에서 이걸 호출하면 같은 값이 연속해서 나오거나 두번 건너뛴 값이 나오겠지만, 개의치 말자.
		// 속도가 더 중요하니까 atomic op으로 시간 크게 깎아먹지 말자. 수십배 느려질거다.
		// 초기값 25이고 매번 117씩 더하는데 오버플로되어서 다시 작은 값이 될거다.
		// 이 랜덤값은 금방 0~255 범위를 다 채운다. 이를 테스트하는 프로그램은 
		// Test/SimpleRandomNumberTest 에 있으므로 참고하세요.
		static volatile uint8_t m_randomNumber;
		inline static uint8_t NextRandom() {
			m_randomNumber += 117;
			return m_randomNumber;
		}

		// 영문,숫자 범위 외 글자로 변환하다. garble의 시작이다.
		inline static uint8_t NonAscii(uint8_t ch)
		{
			return ch | 0x80; 
		}

		// 상위 4비트를 채운 상태로 리턴한다.
		inline static uint8_t SplitterToFrontBits(short splitter)
		{
			switch (splitter)
			{
			case Splitter:
				return 0x90; // 1001xxxx. 모든 고정값에는 최상위비트를 설정하여 non-ascii화한다.
			case HasMessageID:
				return 0xa0; // 1010xxxx
			case TinySplitter:
				return 0xb0; // 1011xxxx
			default:
				assert(0);
				return 0; // 비해당
			}
		}

		// 상위 4비트에는 splitter로부터 얻은 고정값을, 하위 4비트에는 랜덤값을 넣는다.
		inline static uint8_t CombineBits(uint8_t frontBits, uint8_t random)
		{
			return frontBits | (random & 0x0f);
		}

		// 첫 4비트만 구한다.
		inline static uint8_t GetFrontBits(uint8_t ch)
		{
			return ch & 0xf0;
		}

		// 입력받은 첫 4비트에서 Splitter enum 값을 얻는다. 해당하는게 없으면 0을 리턴한다.
		inline static short FrontBitsToSplitter(uint8_t ch)
		{
			switch (ch)
			{
			case 0x90:
				return Splitter;
			case 0xa0:
				return HasMessageID;
			case 0xb0:
				return TinySplitter;
			default:
				return 0; // 비해당
			}
		}
	public:

		/* header는 heap access를 줄이기 위해 이 함수를 호출하는데서
		stack alloc을 해둔 초기화된 객체이어야 한다.
		함수 내부에서 임시로 쓰는 변수이므로 사용 후 폐기 가능. */
		inline static void AddSplitterButShareBuffer(
			const CSendFragRefs &payload,
			CSendFragRefs &ret,
			CMessage& header,
			bool simplePacketMode = false);

		// 위 함수와 같되, messageID 인자를 추가로 받는다.
		// Splitter는 HasMessageID라는 다른 것이 들어가게 된다.
		// 연결 유지 기능에서 auto message recovery 기능에 쓰임.
		inline static void AddSplitterButShareBuffer(
			int messageID,
			const CSendFragRefs &payload,
			CSendFragRefs &ret,
			CMessage& header,
			bool simplePacketMode);

		// 리턴값: 추출되어 추가된 항목의 갯수. 만약 잘못된 스트림 데이터가 입력된 것이라면 -1가 리턴된다.
		// 예외 발생가능 함수
		static int ExtractMessagesFromStreamAndRemoveFlushedStream(
			CStreamQueue &recvStream,
			CReceivedMessageList &extractedMessageAddTarget,
			HostID senderHostID,
			int messageMaxLength,
			ErrorType& outError,
			bool simplePacketMode = false);

		inline static short SplitterToRandom(short splitter);
		inline static short RandomToSplitter(short randomSplitter);
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

		inline CTcpLayerMessageExtractor()
		{
			//m_unsafeHeap = NULL;
			m_recvStream = NULL;
			m_recvStreamCount = 0;
			m_extractedMessageAddTarget = NULL;
			m_senderHostID = HostID_None;
			m_messageMaxLength = 0;
			m_outLastSuccessOffset = 0;
		}

		/* 스트림으로부터 메시지들을 추려낸다.
		추려낸 결과는 m_extractedMessageAddTarget 에 채워진다.
		- 메서드 호출 전에 멤버 변수들을 다 채워야 한다.
		- policy text는 skip하고 메시지를 추출한다.
		\return 추출한 메시지 갯수. -1이면 잘못된 스트림 데이터가 있음을 의미한다. */
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

		// called by object pool classes.
		inline void SuspendShrink()
		{
			m_packet.SuspendShrink();
		}

		// called by object pool classes.
		void OnRecycle() {}

		void OnDrop()
		{
			m_packet.OnDrop();
			
			m_uniqueID = UniqueID();
			m_enquedTime = 0;
			m_reliability = MessageReliability_LAST;
			m_priority = MessagePriority_LAST;
		}
	};

	/* 본 명세는 
	http://waterwall:8090/pages/viewpage.action?pageId=3049009 
	에 있습니다. 
	
	reliable send 자체는 우선순위가 모순이다. 따라서 아예 지원하지 않는다.
	송신 속도 조절기도 포함됨
	*/
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

		inline void PushBack_Copy(const CSendFragRefs& sendData, const SendOpt& sendOpt);


		// length만큼 보낼 데이터들을 fragmented send buffer(WSABUF)에 포인터 리스트로서 채운다.
		// fragCount는 output에 집어넣을 fragment의 개수이다.
		inline void PeekSendBuf(CFragmentedBuffer& output, int fragCount = INT32_MAX)
		{
			output.Clear();

			NormalizePacketQueue();

			// 보내다 만것을 우선으로!
			if (m_partialSentPacket != NULL)
			{
				uint8_t* ptr = m_partialSentPacket->m_packet.GetData() + m_partialSentLength;

				int len = (int)m_partialSentPacket->m_packet.GetCount() - m_partialSentLength;
				assert(len > 0);

				output.Add(ptr, len);
				--fragCount;
			}

			// 나머지를 채우기
			for (QueueType::iterator i = m_thinnedQueue.begin(); i != m_thinnedQueue.end(); ++i)
			{
				if (fragCount <= 0)
					break;
				TcpPacketCtx* e = *i;
				output.Add(e->m_packet.GetData(), (int)e->m_packet.GetCount());
				--fragCount;
			}
		}

		void NormalizePacketQueue();

		int GetLength() { return m_totalLength; }
		int GetTotalLength() { return m_totalLength + m_nonThinnedQueueTotalLength; }

		// length만큼 패킷 큐에서 제거한다.
		// 최우선: 보내다 만거, 차우선: 패킷 큐 상단
		// 패킷 큐에서 제거 후 남은건 partial sent packet으로 옮긴다. 그리고 offset도 변경.
		// 최종 처리 후 m_totalLength로 변경됨.
		void PopFront(int length)
		{
			if (length < 0)
				ThrowInvalidArgumentException();

			if (length == 0)
				return;

			if (m_partialSentPacket != NULL)
			{
				// 지우고 나서도 남아있으면
				if (m_partialSentLength + length < (int)m_partialSentPacket->m_packet.GetCount())
				{
					m_partialSentLength += length;
					m_totalLength -= length;
					return;
				}

				// partial sent packet 청소
				int remainder = (int)m_partialSentPacket->m_packet.GetCount() - m_partialSentLength;
				m_totalLength -= remainder;
				length -= remainder;

				//m_partialSentPacket->m_packet.ClearAndKeepCapacity();
				m_packetPool.Drop(m_partialSentPacket);
				m_partialSentPacket = NULL;
				m_partialSentLength = 0;
			}

			// queue에서 length만큼 다 지우되 남은 것들은 partial sent packet으로 이송.
			while (length > 0 && m_thinnedQueue.GetCount() > 0)
			{
				TcpPacketCtx *headPacket = m_thinnedQueue.RemoveHead();

				if (headPacket->m_uniqueID.m_value != 0)
				{
					m_uniqueIDToPacketMap.RemoveKey(headPacket->m_uniqueID);
				}

				if ((int)headPacket->m_packet.GetCount() <= length)
				{

					m_totalLength -= (int)headPacket->m_packet.GetCount();
					length -= (int)headPacket->m_packet.GetCount();
					//headPacket->m_packet.ClearAndKeepCapacity();
					m_packetPool.Drop(headPacket);
				}
				else
				{
					m_partialSentPacket = headPacket;
					m_partialSentLength = length;

					m_totalLength -= length;
					length = 0;
				}
			}

			CheckConsist();
		}

		void DoForLongInterval()
		{
			m_packetPool.ShrinkOnNeed();
		}
	};

	void SetTcpDefaultBehavior_Client(const shared_ptr<CFastSocket>& socket);
	void SetTcpDefaultBehavior_Server(const shared_ptr<CFastSocket>& socket);
	void SetUdpDefaultBehavior_Client(const shared_ptr<CFastSocket>& socket);
	PROUD_API void SetUdpDefaultBehavior_Server(const shared_ptr<CFastSocket>& socket);
	PROUD_API void SetUdpDefaultBehavior_ServerStaticAssigned(const shared_ptr<CFastSocket>& socket);

}

#include "TCPLayer.inl"
