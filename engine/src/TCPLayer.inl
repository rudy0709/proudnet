
#include "SendFragRefs.h"

namespace Proud
{
	/* 이게 있으면 변수 생성하기가 훨씬 편리.
	Q: CSuperSocketPtr이 왜 아닌지?
	A: iocp or epoll에서 받은 user ptr 값이 CSuperSocket의 ptr 값인 경우가 있다.
	이때는 CSuperSocketPtr 객체를 얻어올 방법이 없으므로, 이렇게 한다.
	*/
	inline void CTcpLayer_Common::AddSplitterButShareBuffer(
		const CSendFragRefs &payload,
		CSendFragRefs &ret,
		CMessage& header,
		bool simplePacketMode)
	{
		// 자주 호출된다. 따라서 inline이다.

		// splitter와 data length를 header로 추가한 후 보낸다.
		// splitter가 두 종류인 이유는, messageID를 보낼 필요가 없는 UDP에서는 트래픽 낭비를 하지 않기 위해.
		if (simplePacketMode)
		{
			header.Write((short)SimpleSplitter);  // 이건 garble 하지 않는다. 사용자가 판독할 수 있어야 하므로.
			header.Write((int)payload.GetTotalLength());
		}
		else
		{
			header.Write((short)CTcpLayer_Common::SplitterToRandom(Splitter));
			header.WriteScalar(payload.GetTotalLength());
		}

		ret.Clear(); // 예전에는 성능 떄문에 안했지만, 차칫 bug prone이므로 추가했음.
		ret.Add(header.GetData(), header.GetLength());
		ret.Add(payload);
	};

	inline void CTcpLayer_Common::AddSplitterButShareBuffer(
		int messageID,
		const CSendFragRefs &payload,
		CSendFragRefs &ret,
		CMessage& header,
		bool simplePacketMode)
	{
		// splitter와 data length를 header로 추가한 후 보낸다.
		// splitter가 두 종류인 이유는, messageID를 보낼 필요가 없는 UDP에서는 트래픽 낭비를 하지 않기 위해.
		if (simplePacketMode)
		{
			header.Write((short)SimpleSplitter);
			header.Write(payload.GetTotalLength());
		}
		else
		{
			header.Write((short)CTcpLayer_Common::SplitterToRandom(HasMessageID));
			header.Write(messageID);
			header.WriteScalar(payload.GetTotalLength());
		}

		//clear을 할 필요가 없다.괜한 cost만 먹는다.
		//이 함수를 사용하는 곳에서는 ret를 모두 생성해서 넣기 때문인데...
		//만일 ret를 clear을 하도록 외부에서 사용한다면 이 주석을 풀자.
		ret.Clear(); // 예전에는 성능 떄문에 안했지만, 차칫 bug prone이므로 추가했음.
		ret.Add(header);
		ret.Add(payload);
	}



	inline int CTcpLayer_Common::ExtractMessagesFromStreamAndRemoveFlushedStream(CStreamQueue &recvStream, CReceivedMessageList &extractedMessageAddTarget, HostID senderHostID, int messageMaxLength, ErrorType& outError, bool simplePacketMode /*= false*/)
	{
		CTcpLayerMessageExtractor extractor;

		extractor.m_recvStream = recvStream.GetData();
		extractor.m_recvStreamCount = (int)recvStream.GetLength();
		extractor.m_extractedMessageAddTarget = &extractedMessageAddTarget;
		extractor.m_senderHostID = senderHostID;
		extractor.m_messageMaxLength = messageMaxLength;
		//extractor.m_unsafeHeap = unsafeHeap;

		int addedCount = extractor.Extract(outError, simplePacketMode);

		recvStream.PopFront(extractor.m_outLastSuccessOffset);

		return addedCount;
	}

	inline int CTcpLayerMessageExtractor::Extract(ErrorType &outError, bool simplePacketMode /*= false*/)
	{
		//ATLASSERT(m_unsafeHeap);
		assert(m_messageMaxLength > 0);
		outError = ErrorType_Ok;

		if (m_recvStreamCount == 0)
			return 0;

		int retCount = 0;
		// 메시지 크기, 인식자 등을 파악한다.
		CMessage msg;
		msg.UseExternalBuffer((uint8_t*)m_recvStream, m_recvStreamCount);
		msg.SetLength(m_recvStreamCount);
		msg.SetReadOffset(0);

		int lastSuccessOffset = 0;


		// 더 이상 없을때까지 계속 추가한다.
		while (true)
		{
			short randomSplitter;
			ByteArrayPtr content;
			// 처음에는 splitter로 시작한다.
			if (!msg.Read(randomSplitter))
			{
				m_outLastSuccessOffset = lastSuccessOffset;
				return retCount;
			}

			short splitter = CTcpLayer_Common::RandomToSplitter(randomSplitter);

			// 잘못된 패킷이면 이를 예외 이벤트로 전달하고(C++ 예외 발생을 하지 말자. 디버깅시 혼란만 가중하고
			// fail over에 약하다) 폐기한다. 그리고 스트림을 끝까지 다 처리한 것처럼 간주한다.
			// TCP인 경우라면 상대와의 연결을 끊고, UDP인 경우라면 제3자 해커가 쓰레기 메시지를 보낸
			// 것일 수도 있으므로 무시한다.
			if (
				(splitter != CTcpLayer_Common::Splitter
				&& splitter != CTcpLayer_Common::TinySplitter
				&& splitter != CTcpLayer_Common::HasMessageID
				&& !simplePacketMode)
				||
				(
				splitter != CTcpLayer_Common::SimpleSplitter && simplePacketMode)
				)
			{
				// splitter가 아니면, plain text로 간주해서 sz가 나올때까지 잘 받아 처리한 것으로 간주하자.
				// sz가 안나오면 하나도 처리되지 않은 것으로 간주하자. 성능상 위험해 보이지만, PN 자체가 처음 policy text 주고받을 때 빼고는 plain text가 끼어들 일이 없으므로
				// 성능상 문제 없다.
				msg.SetReadOffset(lastSuccessOffset);

				uint8_t* msgData = msg.GetData();
				int textReadOffset = lastSuccessOffset;
				int msgLength = msg.GetLength();

				for (int textIndex = textReadOffset; textIndex < msgLength; textIndex++)
				{
					if (msgData[textIndex] == '\0') // end of text가 감지되었다. 다 읽은 것으로 간주하고 루프를 계속 돈다.
					{
						lastSuccessOffset = textIndex + 1;
						msg.SetReadOffset(textIndex + 1);  // 이것도 바꿔줘야지.
						goto LoopAgain;	// 루프를 여기서 끝내면 안된다. 받은 message가 추가로 있는지 뒤도 검사해야 하니까.
					}
				}

				// 끝까지 sz가 안나왔다. 아무것도 처리 안된 것으로 간주하고 루프를 끝낸다.
				m_outLastSuccessOffset = lastSuccessOffset;
				return retCount;
			}

			// 메시지 ID
			int messageID; // hasMessageID = true일 때만 유효
			bool hasMessageID;
			hasMessageID = (splitter == CTcpLayer_Common::HasMessageID);
			if (hasMessageID)
			{
				if (!msg.Read(messageID))
				{
					m_outLastSuccessOffset = lastSuccessOffset;
					return retCount;
				}
			}
			else
				messageID = 0;

			// 메시지의 크기를 읽는다.
			int payloadLength;
			if (simplePacketMode)
			{
				if (msg.Read(payloadLength) == false)
				{
					m_outLastSuccessOffset = lastSuccessOffset;
					return retCount;
				}
			}
			else
			{
				if (splitter == CTcpLayer_Common::Splitter || splitter == CTcpLayer_Common::HasMessageID)
				{
					if (msg.ReadScalar(payloadLength) == false)
					{
						m_outLastSuccessOffset = lastSuccessOffset;
						return retCount;
					}
				}
				else // TinySplitter이면
				{
					// 단 1개의 메시지만이 들어있는 UDP packet이다. 따라서 payload length는 그냥 산출하자.
					payloadLength = m_recvStreamCount - msg.GetReadOffset();
				}
			}

			// 메시지의 크기가 잘못된 크기이면 오류 처리를 한다.
			if (payloadLength < 0)
			{
				m_outLastSuccessOffset = msg.GetLength();
				outError = ErrorType_InvalidPacketFormat;
				return -1;
				/*throw new CBadStreamException(); */
			}
			else if (payloadLength > m_messageMaxLength)
			{
				m_outLastSuccessOffset = msg.GetLength();
				outError = ErrorType_TooLargeMessageDetected;
				return -1;
				/*throw new CBadStreamException(); */
			}

			// 메시지 내용을 읽을 공간이 없으면 애시당초 버퍼를 준비하지 않도록 한다. 여기의 실행지점 충돌 횟수가 워낙 많기 때문에
			// 이런것도 최적화 해야 한다.
			if (msg.CanRead(payloadLength) == false)
			{
				m_outLastSuccessOffset = lastSuccessOffset;
				return retCount;
			}

			// 받을 메시지 버퍼를 준비한다.
			// (여기서 Fast heap access! 따라서 느리다.)
			content.UseInternalBuffer();
			content.SetCount(payloadLength);

			// 메시지 내용을 받는다.
			if (msg.Read(content.GetData(), payloadLength) == false)
			{
				m_outLastSuccessOffset = lastSuccessOffset;
				return retCount;
			}

			{
				// 미리 빈 항목을 추가 후 그것의 주소를 얻는다.
				// CReceivedMessage 복사 비용 때문에.
				m_extractedMessageAddTarget->AddTail();
				CReceivedMessage& ri = m_extractedMessageAddTarget->GetTail();

				ri.m_unsafeMessage.ShareFromAndResetReadOffset(content);
				ri.m_hasMessageID = hasMessageID;
				if (ri.m_hasMessageID)
					ri.m_messageID = messageID;

				// 각 ri에 대해 ri.m_remoteAddr_onlyUdp에 채움
				ri.m_remoteHostID = m_senderHostID;
				ri.m_remoteAddr_onlyUdp = m_remoteAddr_onlyUdp;

				/*
				splitter 가 SimpleSplitter 일때에도 아래 조건문을 수행하여야 한다.
				그렇지 않으면 SimplePacKetMode 에서 하나의 메시지 처리를 계속 하게 된다.
				*/
				if (splitter == CTcpLayer_Common::Splitter || splitter == CTcpLayer_Common::SimpleSplitter || splitter == CTcpLayer_Common::HasMessageID)
				{
					retCount++;

					// 메시지를 성공적으로 읽었다. last success offset을 다음 msg read offset으로 바꾸자.
					// (last success offset이라는 변수명 자체가 틀렸다. offset to read first 식의 의미로 바꿔야 할 듯.)
					lastSuccessOffset = msg.GetReadOffset();
				}
				else
				{
					return ++retCount;
				}
			}

		LoopAgain:
			;
		}
	}

	inline void CTcpSendQueue::PushBack_Copy(const CSendFragRefs& sendData, const SendOpt& sendOpt)
	{
		TcpPacketCtx *packet = m_packetPool.NewOrRecycle();
		packet->FromSendOpt(sendOpt);
		packet->m_enquedTime = GetPreciseCurrentTimeMs();

		sendData.ToAssembledByteArray(packet->m_packet);

		// non thinned queue 에 담기
		m_nonThinnedQueue.AddTail(packet);

		m_nonThinnedQueueTotalLength += packet->m_packet.GetCount();

		// 같은 uniqueID를 가지는 것을 찾아서 중복 처리한다. 단 0은 제외!
		// #ifdef ALLOW_UNIQUEID_FOR_TCP
		//		if(packet.m_uniqueID != 0)
		//		{
		//			for(QueueType::iterator i=m_queue.begin();i!=m_queue.end();i++)
		//			{
		//				TcpPacketCtx& e = *i;
		//				if(e.m_uniqueID == packet.m_uniqueID)
		//				{
		//					m_totalLength -= e.m_packet.GetCount();
		//					e = packet;
		//					m_totalLength += e.m_packet.GetCount();
		//					goto L1;
		//				}
		//			}
		//		}
		// #endif // ALLOW_UNIQUEID_FOR_TCP
		//
		//		// 비중복. 그냥 끝에 넣기.
		//		m_queue.AddTail(packet);
		//		m_totalLength += (int)packet->m_packet.GetCount();
		// #ifdef ALLOW_UNIQUEID_FOR_TCP
		// L1:
		// #endif // ALLOW_UNIQUEID_FOR_TCP
		//		CheckConsist();
	}

	// #PacketGarble splitter 범위에 따라 범위 내 난수를 만든다.
	short CTcpLayer_Common::SplitterToRandom(short splitter)
	{
		if (splitter == SimpleSplitter)
			return SimpleSplitter;  // 안 바꾼다. 유저가 판독 가능한 패킷 포맷이어야 하니.

		// 첫 바이트를 채울 랜덤값을 구한다.
		uint8_t rand = NextRandom();
		// SimpleSplitter는 0x0909이다. 첫 바이트가 ascii 범위이다. 따라서 랜덤값을 얻더라도
		// non-ascii로 바꾸어야 한다.
		uint8_t ch0 = NonAscii(rand);

		// 두번째 바이트용 난수
		uint8_t rand2 = NextRandom();
		// 원하는 splitter 값에 대응하는 랜덤 바이트를 만든다.
		// 첫 4비트는 splitter 대응, 나머지 4비트는 그냥 랜덤값.
		uint8_t front = SplitterToFrontBits(splitter);
		uint8_t ch1 = CombineBits(front, rand2);
		int ret;
		// 최종 조합
		ret = ch0 | (int(ch1) << 8);
		return (short)ret;
	}

	// #PacketGarble 난수의 범위에 따라 splitter 값을 리턴한다.
	// 위 함수와 정반대로 작동한다.
	// 비해당이면 0을 리턴.
	short CTcpLayer_Common::RandomToSplitter(short randomSplitter)
	{
		if (randomSplitter == SimpleSplitter)
			return SimpleSplitter;

		// 두번째 바이트만. 첫번쨰 바이트는 그냥 버린다. 단, non-ascii이어야 한다.
		uint8_t ch1 = randomSplitter >> 8;
		if ((ch1 & 0x80) == 0)
			return 0;

		uint8_t front = GetFrontBits(ch1);
		short splitter = FrontBitsToSplitter(front);
		return splitter;
	}
}
