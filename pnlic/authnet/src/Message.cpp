/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <new>
#include "../include/BasicTypes.h"
#include "../include/AddrPort.h"
#include "../include/Exception.h"
#include "../include/Marshaler.h"
#include "../include/Quantizer.h"
#include "../include/Message.h"
#include "SendFragRefs.h"
#include "Relayer.h"
#include "NetSettings.h"
#include "ReliableUDPFrame.h"
//#include "FastSocket.h"

#if !defined(_WIN32)
    #include <wchar.h>
    #include <stdlib.h>
    #include "../include/sysutil.h"
#endif

#if (WCHAR_LENGTH == 4)
	#include "DefaultStringEncoder.h"
#endif
#include "SendFragRefs.h"


namespace Proud
{
	const int STRING_LENGTH_LIMIT = 1024 * 1024;
    
	inline void AdjustReadOffsetByteAlign(CMessage& msg)
	{
		// 비트 단위 읽기를 하던 중이라 읽기 포인트가 byte align이 아니면 조정한다.
		int octetOffset = msg.m_readBitOffset & 7; // mod 8

		msg.m_readBitOffset &= 0xfffffff8; // mod 8만큼을 빼 버림

		if(octetOffset != 0)
			msg.m_readBitOffset += 8;
	}

	inline void AssureReadOffsetByteAlign(CMessage& msg)
	{
		if((msg.m_readBitOffset & 7) != 0)
		{
			throw Exception("Read offset is not byte aligned!");
		}
	}

	bool CMessage::Read(uint8_t* data, int count)
	{
		// 아예 읽을게 없으면 splitter test 자체도 제낀다.
		if (count == 0)
			return true;

		AdjustReadOffsetByteAlign(*this);

		if (!Read_NoTestSplitter(data, count))
			return false;

		ReadAndCheckTestSplitterOnCase();	

		return true;
	}

	void CMessage::Write(const Guid &b)
	{
		const PNGUID* p = &b;
		Write((const uint8_t*)p, sizeof(*p));
	}

	bool CMessage::Read(Guid &b)
	{
		PNGUID* p = &b;
		return Read((uint8_t*)p, sizeof(*p));
	}

	bool CMessage::CanRead(int count)
	{
		// 아예 읽을게 없으면 splitter test 자체도 제낀다.
		if (count == 0)
			return true;

		int oldReadBitOffset = m_readBitOffset;

		AdjustReadOffsetByteAlign(*this);

		if (!CanRead_NoTestSplitter(count))
		{
			m_readBitOffset = oldReadBitOffset;
			return false;
		}
		ReadAndCheckTestSplitterOnCase();

		m_readBitOffset = oldReadBitOffset;

		return true;
	}

	void CMessage::Write( const AddrPort &b )
	{
		Write(b.m_binaryAddress);
		Write(b.m_port);
	}

	void CMessage::Write( const RelayDest &rd )
	{
		Write(rd.m_frameNumber);
		Write(rd.m_sendTo);
	}

	void CMessage::CopyFromArrayAndResetReadOffset(const uint8_t* src, int srcLength)
	{
		m_msgBuffer.Clear();
		m_msgBuffer.AddRange(src, srcLength);
		m_readBitOffset = 0;
	}

	CMessage::CMessage()
	{
		m_readBitOffset = 0;
		m_enableTestSplitter = CNetConfig::EnableTestSplitter;
		m_bitLengthInOneByte = 0;
		m_isSimplePacketMode = false;
	}

	// 복사 생성자
	CMessage::CMessage( const CMessage &src )
	{
		m_readBitOffset = src.m_readBitOffset;
		m_enableTestSplitter = src.m_enableTestSplitter;
		m_bitLengthInOneByte = src.m_bitLengthInOneByte;
		m_isSimplePacketMode = src.m_isSimplePacketMode;

		m_msgBuffer = src.m_msgBuffer;
	}

	/*void CMessage::ResetWritePointer()
	{
	m_msgBuffer.Clear();
	}*/

	int CMessage::GetWriteOffset()
	{
		return (int)m_msgBuffer.GetCount();
	}

	/* 주의: 수정시 ProudClr의 동명 심볼도 수정해야 한다.
	수정시 MessageTypeFieldLength 도 반드시 수정해야! */
#if _WIN32
	bool CMessage::Read( _MessageType &b )
 	{
 		b = MessageType_None;
 		//modify by rekfkno1 - android버젼 endian때문에 char로 바꾼다.
		int8_t bb;
 		bool r = Read(bb);
 		if (r == true)
			b = (_MessageType)bb;
 		return r;
 	}
#endif
	bool CMessage::Read( MessagePriority &b )
	{
		b = MessagePriority_LAST;
		uint8_t bb;
		bool r = Read(bb);
		if (r == true)
			b = (MessagePriority)bb;
		return r;
	}

	bool CMessage::Read( DirectP2PStartCondition &b )
	{
		b = DirectP2PStartCondition_LAST;
		uint8_t bb;
		bool r = Read(bb);
		if (r == true)
			b = (DirectP2PStartCondition)bb;
		return r;
	}

	bool CMessage::Read( FallbackMethod &b )
	{
		b = FallbackMethod_None;
		uint8_t bb;
		bool r = Read(bb);
		if (r == true)
			b = (FallbackMethod)bb;
		return r;
	}

	bool CMessage::Read( EncryptMode &b )
	{
		b = EM_None;
		uint8_t bb;
		bool r = Read(bb);
		if (r == true)
			b = (EncryptMode)bb;
		return r;
	}

	bool CMessage::Read( ReliableUdpFrameType &b )
	{
		b = ReliableUdpFrameType_None;
		int8_t bb;
		bool r = Read(bb);
		if (r == true)
			b = (ReliableUdpFrameType)bb;
		return r;
	}
#if _WIN32
	bool CMessage::Read( _LocalEventType &b )
	{
		b = LocalEventType_None;
		uint16_t bb;
		bool r = Read(bb);
		if (r == true)
			b = (_LocalEventType)bb;
		return r;
	}
#endif
	bool CMessage::Read( RmiID &b )
	{
		b = RmiID_None;
		uint16_t bb;
		bool r = Read(bb);
		if (r == true)
			b = (RmiID)bb;
		return r;
	}

	bool CMessage::Read( AddrPort &b )
	{
		b = AddrPort::Unassigned;
		return (Read(b.m_binaryAddress) == true &&
			Read(b.m_port) == true);
	}

	// bool CMessage::Read( ByteArray &b )
	// {
	// 	int length;
	// 	if (ReadScalar(length) == false)
	// 		return false;
	// 	
	// 	// 만약 length가 엄청 큰 값이면 에러처리한다
	// 	if (length<0 || length >= CNetConfig::MessageMaxLength)
	// 	{
	// 		throw Exception("Too long array(size=%d) detected in CMessage", length);
	// 	}
	// 	b.SetLength(length);
	// 
	// 	if (GetReadOffset() + b.GetCount() > m_msgBuffer.GetCount())
	// 		return false;
	// 
	// 	m_msgBuffer.CopyRangeTo(b, GetReadOffset(), b.GetCount());
	// 
	// 	m_readBitOffset += b.GetCount()*8;
	// 
	// 	return true;
	// }

	bool CMessage::Read( HostID &b )
	{
		b = HostID_None;
		int x;
		bool r;
		r = Read(x);
		if (r == true)
			b = (HostID)x;
		return r;
	}

	bool CMessage::Read( RelayDestList &relayDestList )
	{
		return ReadArrayT<RelayDest,true,RelayDestList>(*this,relayDestList);
	}
	void CMessage::Write( const RelayDestList &relayDestList )
	{
		WriteArrayT<RelayDest,true,RelayDestList>(*this,relayDestList);
	}

	void CMessage::Write( NamedAddrPort n )
	{
		WriteString(n.m_addr);
		Write(n.m_port);
	}

	bool CMessage::Read( NamedAddrPort &n )
	{
		return ReadString(n.m_addr) && Read(n.m_port);
	}

	bool CMessage::Read( RelayDest &rd )
	{
		return (Read(rd.m_frameNumber) && Read(rd.m_sendTo));
	}

	bool CMessage::Read( CNetSettings &b )
	{
		return Read(b.m_fallbackMethod)  &&
			Read(b.m_serverMessageMaxLength) &&
			Read(b.m_clientMessageMaxLength) &&
			Read(b.m_defaultTimeoutTime) &&
			Read(b.m_directP2PStartCondition) &&
			Read(b.m_overSendSuspectingThresholdInBytes) &&
			Read(b.m_enableNagleAlgorithm) &&
			Read(b.m_encryptedMessageKeyLength) &&
			Read(b.m_fastEncryptedMessageKeyLength) &&
			Read(b.m_allowServerAsP2PGroupMember) &&
			Read(b.m_enableEncryptedMessaging) &&  // 암호화 테스트 youngjunko
			Read(b.m_enableP2PEncryptedMessaging) &&
			Read(b.m_upnpDetectNatDevice) &&
			Read(b.m_upnpTcpAddPortMapping) &&
			Read(b.m_enableLookaheadP2PSend)&&
			Read(b.m_enablePingTest) &&
            Read(b.m_ignoreFailedBindPort) &&
			Read(b.m_emergencyLogLineCount);
	}

	void CMessage::Write( const CNetSettings &b )
	{
		Write(b.m_fallbackMethod);
		Write(b.m_serverMessageMaxLength);
		Write(b.m_clientMessageMaxLength);
		Write(b.m_defaultTimeoutTime);
		Write(b.m_directP2PStartCondition);
		Write(b.m_overSendSuspectingThresholdInBytes);
		Write(b.m_enableNagleAlgorithm);
		Write(b.m_encryptedMessageKeyLength);
		Write(b.m_fastEncryptedMessageKeyLength);
		Write(b.m_allowServerAsP2PGroupMember);
		Write(b.m_enableEncryptedMessaging); // 암호화 테스트 youngjunko
		Write(b.m_enableP2PEncryptedMessaging);
		Write(b.m_upnpDetectNatDevice);
		Write(b.m_upnpTcpAddPortMapping);
		Write(b.m_enableLookaheadP2PSend);
		Write(b.m_enablePingTest);
        Write(b.m_ignoreFailedBindPort);
		Write(b.m_emergencyLogLineCount);
	}

	// void CMessage::WriteMessageContent( const CMessage &msg )
	// {
	// 	Write(msg.m_msgBuffer);
	// }
	// 
	// bool CMessage::ReadMessageContent( CMessage &msg )
	// {
	// 	return Read(msg.m_msgBuffer);
	// }


	// void CMessage::CloneTo( CMessage& dest )
	// {
	// 	dest.m_msgBuffer = ByteArrayPtr::TakeNormalHeapeeArrayAndMakeItUseSafeFastHeap(new ByteArray);
	// 	dest.m_msgBuffer.AddRange(m_msgBuffer.GetData(), m_msgBuffer.GetCount());
	// 	dest.m_readBitOffset = m_readBitOffset;
	// }
	void CMessage::SetReadOffset( int offset )
	{
		if (offset > (int)m_msgBuffer.GetCount())
		{
			throw Exception("Tried to Catch Read Pointer by Extremely Large Offset. Length of Buffer =%d, Offset to Catch=%d", m_msgBuffer.GetCount(), offset);
		}
		m_readBitOffset = offset*8;
	}

	// 이 함수를 호출하면 simple packet mode가 켜지거나 꺼진다.
	// read/write scalar 등을 비활성화하는데 사용된다.
	void CMessage::SetSimplePacketMode(bool isSimplePacketMode)
	{
		m_isSimplePacketMode = isSimplePacketMode;
	}

	// 다른 버퍼 객체와 포인터를 공유하기 시작하면서 읽기 위치를 리셋한다.
	void CMessage::ShareFromAndResetReadOffset( ByteArrayPtr data )
	{
		m_msgBuffer = data;
		m_readBitOffset = 0;
	}

	PROUD_API void CMessage::UseExternalBuffer(uint8_t* buf, int capacity)
	{
// 		if (capacity > CNetConfig::MessageMaxLength)
// 			throw Exception("UseExternalBuffer failed due to too large capacity");

		m_msgBuffer.UseExternalBuffer(buf, capacity);
	}

	// void CMessage::ToByteArray( ByteArray &ret )
	// {
	// 	m_msgBuffer.CopyRangeTo(ret, 0, m_msgBuffer.GetCount());
	// }
	// 
	// ByteArrayPtr CMessage::ToByteArray()
	// {
	// 	ByteArrayPtr ret = ByteArrayPtr::TakeNormalHeapeeArrayAndMakeItUseSafeFastHeap(new ByteArray);
	// 	ToByteArray(ret.GetInternalBufferRef());
	// 
	// 	return ret;
	// }

	void CMessage::UseInternalBuffer()
	{
		/* Q: CMessage ctor를 냅두고 이 함수가 따로 있나요?
		A: global object pool에서 가져오는 과정이 무조건 수행되면 느리잖아요. 
		Q: 그러면, 최초의 write 가 발생하면 JIT로 하면 되잖아요?
		A: Aㅏ..... TODO로 남깁시다 ㅋㅋㅋ */
		m_msgBuffer.UseInternalBuffer();
		m_msgBuffer.SetGrowPolicy(GrowPolicy_HighSpeed);
		m_msgBuffer.SetMinCapacity(CNetConfig::MessageMinLength);
	}

	void CMessage::WriteString(const char* str)
	{
		WriteStringA(str);
	}

	void CMessage::WriteString(const wchar_t* str)
	{
		WriteStringW(str);
	}

	void CMessage::WriteString(const StringA &str)
	{
		WriteStringA(str);
	}

	void CMessage::WriteString(const StringW &str)
	{
		WriteStringW(str);
	}

	//#ifdef _STRING_ // <string>이 include되었으면..

	//#endif

	bool CMessage::ReadString(StringA &str)
	{
		return ReadStringA(str);
	}

	bool CMessage::ReadString(StringW &str)
	{
		return ReadStringW(str);
	}

#if (WCHAR_LENGTH == 4)
	// utf-8 문자열을 utf-16으로 바꾼다.
	// 유닉스에서 주로 쓴다. 윈도에서는 쓸일이 없을지도.
	static void iconv_utf8_to_utf16LE(const char* utf8Src, int utf8TextLength, ByteArray &utf16Dest)
	{
		// UTF16은 1글자당 2 or 4 바이트이다.
		// 따라서 글자수 * 4를 해주어야 절대 모자라지 않는다.
		// 주의! null문자 제외해야함. null 문자는 받는데서 처리함.
		int utf16Capacity = utf8TextLength * 4;

		utf16Dest.SetCount(utf16Capacity);
		if (utf16Capacity == 0)
			return;

        // 남아있는 읽을 양
		size_t utf8BytesLeft = utf8TextLength; // * sizeof(char)
		// 남아있는 버퍼의 양
		size_t utf16BytesLeft = utf16Capacity;

		iconv_string_convert(CDefaultStringEncoder::Instance().m_UTF8toUTF16LEEncoder, 
							 utf8Src,
							 &utf8BytesLeft,
							 (char*)utf16Dest.GetData(),
							 &utf16BytesLeft);

		utf16Dest.SetCount(utf16Capacity - utf16BytesLeft);
	}

	static StringA iconv_utf16LE_to_utf8(ByteArray utf16Src)
	{
		int utf16Size = utf16Src.GetCount(); // utf16 기준 바이트수
		if (utf16Size == 0)
			return StringA();

		assert((utf16Size % 2) == 0);
	
		// NOTE: UTF16의 각 글자의 바이트수는 2 or 4 바이트이다.
		// 한편 UTF8의 각 글자의 바이트수는 1~4바이트이다.
		// 따라서 UTF16의 바이트수를 UTF8로 옮길 경우 절대 모자라지 않는 충분한 크기는 UTF16 size * 2이다.

		int utf8Capacity = utf16Size * 2 + 1; // +1 for null Text
		StringA utf8Str;
		StrBufA utf8Buf(utf8Str, utf8Capacity);

		size_t utf16BytesLeft = utf16Size;
		size_t utf8BytesLeft = utf8Capacity;

		iconv_string_convert(CDefaultStringEncoder::Instance().m_UTF16LEtoUTF8Encoder,
							 (const char*)utf16Src.GetData(),
							 &utf16BytesLeft,
							 (char*)utf8Buf.GetBuf(),
							 &utf8BytesLeft);

		((char*)utf8Buf.GetBuf())[utf8Capacity - utf8BytesLeft] = 0;

		return utf8Str;
	}

	static void iconv_utf32LE_to_utf16LE(const wchar_t *utf32Src, int utf32TextLength, ByteArray &utf16Dest)
	{   
		// UTF16은 1글자당 2 or 4 바이트이다.
		// 따라서 글자수 * 4를 해주어야 절대 모자라지 않는다.
		// 주의! null문자 제외해야함. null 문자는 받는데서 처리함.
		int utf16Capacity = utf32TextLength * 4;

		utf16Dest.SetCount(utf16Capacity);
		if (utf16Capacity == 0)
			return;

        // 남아있는 읽을 양
		size_t utf32BytesLeft = sizeof(wchar_t) * utf32TextLength;
		// 남아있는 버퍼의 양
		size_t utf16BytesLeft = utf16Capacity;

		iconv_string_convert(CDefaultStringEncoder::Instance().m_UTF32LEtoUTF16LEEncoder,
							 (const char*)utf32Src,
							 &utf32BytesLeft,
							 (char*)utf16Dest.GetData(),
							 &utf16BytesLeft);

		utf16Dest.SetCount(utf16Capacity - utf16BytesLeft);
	}

	static StringW iconv_utf16LE_to_utf32LE(ByteArray utf16Src)
	{
		int utf16Size = utf16Src.GetCount(); // utf16 기준 바이트수
		if (utf16Size == 0)
			return StringW();

		assert((utf16Size % 2) == 0);

		// NOTE: UTF16의 각 글자의 바이트수는 2 or 4 바이트이다.
		// 한편 UTF32의 각 글자의 바이트수는 4바이트이다.
		// 따라서 UTF16의 바이트수를 UTF32로 옮길 경우 절대 모자라지 않는 충분한 크기는 UTF16 size * 2이다.

		int utf32Capacity = utf16Size * 2 + 4; // +4 for null Text
		StringW utf32Str;
		StrBufW utf32Buf(utf32Str, utf32Capacity);

		size_t utf16BytesLeft = utf16Size;
		size_t utf32BytesLeft = utf32Capacity;

		iconv_string_convert(CDefaultStringEncoder::Instance().m_UTF16LEtoUTF32LEEncoder,
							 (const char*)utf16Src.GetData(),
							 &utf16BytesLeft,
							 (char*)utf32Buf.GetBuf(),
							 &utf32BytesLeft);

		int null_offset = utf32Capacity - utf32BytesLeft;

		((char*)utf32Buf.GetBuf())[null_offset] = 0;
		((char*)utf32Buf.GetBuf())[null_offset + 1] = 0;
		((char*)utf32Buf.GetBuf())[null_offset + 2] = 0;
		((char*)utf32Buf.GetBuf())[null_offset + 3] = 0;

		return utf32Str;
	}
#elif (WCHAR_LENGTH == 2)

#else
#error You must include WCHAR_LENGTH definition!
#endif

	// 내부 용도. 웬만하면 직접 쓰지 말 것
	bool CMessage::ReadStringA(Proud::StringA &str)
	{
		int byteLength = 0;

		if (ReadScalar(byteLength) == false)
			return false;

		if (byteLength < 0 || byteLength > STRING_LENGTH_LIMIT)
			throw Exception("CMessage.ReadString failed! byteLength=%d > MAX(%d)", byteLength, STRING_LENGTH_LIMIT);

#if (WCHAR_LENGTH == 4)
		{
			// 유닉스에서는 UTF16 문자열을 처리 못하므로 binary data로 일단 가져온다.
			ByteArray temp;
			temp.SetCount(sizeof(short) + byteLength);

			// 0으로 모두 초기화를 하여도 byte byteLength만큼 엎어씌워집니다. 따라서 전부다 초기화하지 않고 맨 마지막 sizeof(short) 부분 2바이트만 0으로 초기화 합니다.
			memset((temp.GetData() + byteLength), 0, sizeof(short));
			bool ret = Read((uint8_t*)temp.GetData(), byteLength);

			// 가져온 결과를 utf8로 변환
			if (ret == true)
			{
				str = iconv_utf16LE_to_utf8(temp);
			}
			return ret;
		}
#elif (WCHAR_LENGTH == 2) // marmalade,win32
		{
			// 글자 수를 계산합니다
			// 이런 식으로 바이트/글자수 구별해서 변수명을 넣어주세요. 다른 곳도요.
			const int characterLength = byteLength / sizeof(wchar_t);

			// 그냥 버퍼에 채우고 끝!
			StringW wstr;
			StrBufW strBuf(wstr, characterLength + 1);
			strBuf[0] = 0;
			strBuf[characterLength] = 0;

			bool ret = Read((uint8_t*)strBuf.GetBuf(), byteLength);

			str = StringW2A(wstr);

			return ret;
		}
#else
#error You must include WCHAR_LENGTH definition!
#endif
	}

	bool CMessage::ReadStringW(Proud::StringW &str) 
	{
		int byteLength = 0;

		if(ReadScalar(byteLength) == false)
			return false;

		if(byteLength < 0 || byteLength > STRING_LENGTH_LIMIT)
			throw Exception("CMessage.ReadString failed! byteLength=%d > MAX(%d)", byteLength, STRING_LENGTH_LIMIT);
#if (WCHAR_LENGTH == 4)
		{
			// 유닉스에서는 UTF16 문자열을 처리 못하므로 binary data로 일단 가져온다.
			ByteArray temp;
			temp.SetCount(sizeof(short) + byteLength);
			// 0으로 모두 초기화를 하여도 byte byteLength만큼 엎어씌워집니다. 따라서 전부다 초기화하지 않고 맨 마지막 sizeof(short) 부분 2바이트만 0으로 초기화 합니다.
			memset((temp.GetData() + byteLength), 0, sizeof(short));

			bool ret = Read((uint8_t*)temp.GetData(), byteLength);
			if( ret == true)
			{
				str = iconv_utf16LE_to_utf32LE(temp);
			}
			return ret;
		}
#elif (WCHAR_LENGTH == 2) // marmalade,win32
		{
			const int characterLength = byteLength / sizeof(wchar_t);
			// 그냥 버퍼에 채우고 끝!
			StrBufW strBuf(str, characterLength + 1);
			strBuf[0] = 0;
			strBuf[characterLength] = 0;

			return Read((uint8_t*)strBuf.GetBuf(), byteLength);
		}
#else
#error You must include WCHAR_LENGTH definition!
#endif
	}

	void CMessage::WriteStringA(const char* str)
	{
		if (str == NULL) return;

		const size_t length = strlen(str);
		if (length > STRING_LENGTH_LIMIT)
			throw Exception("CMessage.WriteString failed! length=%d > MAX(%d)", length, STRING_LENGTH_LIMIT);

#if (WCHAR_LENGTH == 4)
		{
			// UTF8 -> UTF16LE 변환하여 보냄
			ByteArray temp;
			iconv_utf8_to_utf16LE(str, length, temp);

			const int sendLength = temp.GetCount();

			WriteScalar(sendLength);
			Write(temp.GetData(), sendLength);
		}
#elif (WCHAR_LENGTH == 2) // win32, marmalade
		{
			const StringW wstr  = StringA2W(str);
			const int sendLength = wstr.GetLength() * sizeof(wchar_t);

			WriteScalar(sendLength);
			Write((uint8_t*)wstr.GetString(), sendLength);
		}
#else
#error You must include WCHAR_LENGTH definition!
#endif
		
	}

	void CMessage::WriteStringW(const wchar_t* str)
	{
		if (str == NULL) return;

		const size_t length = wcslen(str);
		if (length > STRING_LENGTH_LIMIT)
			throw Exception("CMessage.WriteString failed! length=%d > MAX(%d)", length, STRING_LENGTH_LIMIT);

#if (WCHAR_LENGTH == 4)
		{
			// UTF32LE -> UTF16LE 변환하여 보냄
			ByteArray temp;
			iconv_utf32LE_to_utf16LE(str, length, temp);

			const int sendLength = temp.GetCount();

			WriteScalar(sendLength);
			Write(temp.GetData(), sendLength); // null은 비포함하고 시행
		}
#elif (WCHAR_LENGTH == 2) // win32, marmalade
		{
			const int sendLength = (int)length * sizeof(wchar_t);

			WriteScalar(sendLength);
			Write((uint8_t*)str, sendLength); // null은 비포함하고 시행
		}
#else
	#error You must include WCHAR_LENGTH definition!
#endif

	}

	void CMessage::SetLength( int count )
	{
		if(m_readBitOffset > count*8)
		{
			m_readBitOffset = count*8; // 어쨌거나 크기 조절하기를 시행.
		}

		m_msgBuffer.SetCount(count);
	}

	// bool CMessage::ReadFrameNumberArray( CFastArrayPtr<int,true> &b )
	// {
	// 	b.Clear();
	// 	int count;
	// 	if (Read(count) && count >= 0)
	// 	{
	// 		b.SetLength(count);
	// 		for (int i = 0;i < count;i++)
	// 		{
	// 			int fn;
	// 			if (Read(fn))
	// 				b[i] = fn;
	// 		}
	// 	}
	// 	return true;
	// }

	// void CMessage::WriteFrameNumberArray( const int* arr, int count )
	// {
	// 	Write(count);
	// 	for (int i = 0;i < count;i++)
	// 	{
	// 		Write(arr[i]);
	// 	}
	// }

	/// 읽은 것처럼 구라친다.
	bool CMessage::SkipRead( int count )
	{
		ReadAndCheckTestSplitterOnCase();

		if (m_msgBuffer.IsNull())
			throw Exception("Tried to Read the Data from Null Messages");

		if (GetReadOffset() + count > (int)m_msgBuffer.GetCount())
			return false;
		// memcpy(data,m_msgBuffer.GetData()+m_readOffset,count); 이것만 commented out.
		m_readBitOffset += count*8;

		return true;

	}


	bool CMessage::Read_NoTestSplitter(uint8_t* data, int count)
	{
		if (m_msgBuffer.IsNull())
			throw Exception("Cannot read from Null pointer message!");

		AssureReadOffsetByteAlign(*this);

		if (GetReadOffset() + count > (int)m_msgBuffer.GetCount())
			return false;
		UnsafeFastMemcpy(data, m_msgBuffer.GetData() + GetReadOffset(), count);
		m_readBitOffset += count*8;

		return true;
	}

	bool CMessage::CanRead_NoTestSplitter( int count)
	{
		if (m_msgBuffer.IsNull())
			throw Exception("Cannot read from Null pointer message!");

		int oldReadBitOffset = m_readBitOffset;

		AssureReadOffsetByteAlign(*this);

		if (GetReadOffset() + count > (int)m_msgBuffer.GetCount())
		{
			m_readBitOffset = oldReadBitOffset;
			return false;
		}

		m_readBitOffset = oldReadBitOffset;

		return true;
	}

	void CMessage::ReadAndCheckTestSplitterOnCase()
	{
		if (m_enableTestSplitter)
		{
			uint8_t s;
			if (!Read_NoTestSplitter(&s, sizeof(s)) || s != Splitter)
			{
				throw Exception("CMessage: Test splitter failure(reading=%d,length=%d)", GetReadOffset(), GetLength());
			}
		}
	}

	void CMessage::EnableTestSplitter( bool enable )
	{
		m_enableTestSplitter = enable;
	}

	bool CMessage::ReadWithShareBuffer( CMessage& output, int length )
	{
		if (GetLength() < GetReadOffset() + length)
			return false;

		uint8_t* contentPtr = GetData() + GetReadOffset();
		output.UseExternalBuffer(contentPtr, length);
		output.SetLength(length); // 이게 있어야 함!
		SkipRead(length);

		return true;
	}

	void CMessage::AppendFragments( const CSendFragRefs& fragments )
	{
		for (int i = 0;i < fragments.GetFragmentCount();i++)
			m_msgBuffer.AddRange(fragments[i].GetData(), fragments[i].GetLength());
	}

	void CMessage::AppendByteArray(const uint8_t* fragment, int fragmentLength)
	{
		m_msgBuffer.AddRange(fragment, fragmentLength);
	}

	void CMessage::WriteBits(uint8_t* src, int srcBitLength)
	{
		if(srcBitLength <0 )
			ThrowInvalidArgumentException();

		int srcBitOffset = 0;
		while (srcBitOffset < srcBitLength)
		{
			int done = WriteBitsOfOneByte(src, srcBitLength, srcBitOffset);
			if(done <= 0)
			{
				ThrowBitOperationError(__FUNCTION__);
			}
			srcBitOffset += done;
		}

	}

	int CMessage::WriteBitsOfOneByte(const uint8_t* src, int srcBitLength, int srcBitOffset)
	{
		// 기록할 수 있는 비트 수를 구한다.
		int bitLengthToWrite = PNMIN(8 - srcBitOffset % 8, 8 - m_bitLengthInOneByte);
		bitLengthToWrite = PNMIN(bitLengthToWrite, srcBitLength - srcBitOffset);
		bitLengthToWrite = PNMAX(0, bitLengthToWrite);

		// 기록할 것이 없으면 종료 처리한다.
		if (bitLengthToWrite == 0)
			return 0;

		// 기록 대상의 bit offset이 0이면 새 바이트를 추가한다.
		if (m_bitLengthInOneByte == 0)
			m_msgBuffer.Add(0);

		// 기록 대상의 원래 값을 미리 얻어둔다.
		int result = m_msgBuffer[m_msgBuffer.GetCount() - 1];

		// 기록할 비트를 추출한다. 총 비트에서 기록 대상 외 비트를 제거한 후 왼쪽으로 옮긴다.
		int bits = src[srcBitOffset / 8];

		int bitMask = (1 << bitLengthToWrite) - 1;
		bits = (bits >> (srcBitOffset%8)) & bitMask;
		bits <<= m_bitLengthInOneByte % 8;

		// 결과물을 기록한다.
		result |= bits;
		m_msgBuffer[m_msgBuffer.GetCount() - 1] = (uint8_t)result;
		m_bitLengthInOneByte += bitLengthToWrite;

		if(m_bitLengthInOneByte > 8)
		{
			ThrowBitOperationError(__FUNCTION__);
		}

		if (m_bitLengthInOneByte == 8)
		{
			m_bitLengthInOneByte = 0;
		}
		return bitLengthToWrite;
	}

	bool CMessage::ReadBits(uint8_t* output, int outputBitLength)
	{
		if(outputBitLength <0 )
			ThrowInvalidArgumentException();

		int outputBitOffset = 0;
		while (outputBitOffset < outputBitLength)
		{
			int done = ReadBitsOfOneByte(output, outputBitLength, outputBitOffset);
			if(done == 0)
				return false;   // 만약 더 읽을 것이 없으면 false를 그냥 리턴.
			outputBitOffset += done;
		}

		return true;
	}

	int CMessage::ReadBitsOfOneByte(uint8_t* output, int outputBitLength, int outputBitOffset)
	{
		// 읽을 수 있는 비트 수를 구한다.
		int outputBitOffsetInOneByte = outputBitOffset % 8;
		int bitLengthToRead = PNMIN(8 - outputBitOffsetInOneByte, 8 - m_readBitOffset % 8);
		bitLengthToRead = PNMIN(bitLengthToRead, outputBitLength - outputBitOffset);
		bitLengthToRead = PNMAX(0, bitLengthToRead);

		// 읽을 것이 없으면 종료 처리한다.
		if (bitLengthToRead == 0)
			return 0;

		// 읽기 소스가 있으면 새 바이트를 추가한다.
		if (outputBitOffsetInOneByte == 0)
			output[outputBitOffset / 8] = 0;

		int outputOldValue = output[outputBitOffset / 8];

		// 읽기 소스의 바이트 값을 읽는다.
		int bits = m_msgBuffer[m_readBitOffset / 8];

		// 읽기 소스에서 읽을 만큼의 비트를 걸러내기 위해 비트를 우측으로 옮긴다.
		bits >>= m_readBitOffset % 8;
		int bitMask = (1 << bitLengthToRead) - 1;
		int readResult = bits & bitMask;
		readResult <<= outputBitOffsetInOneByte;

		// 읽은 결과물을 기록할 결과물에 병합한다.
		readResult |= outputOldValue;
		output[outputBitOffset / 8] = (char)readResult;

		m_readBitOffset += bitLengthToRead;

		return bitLengthToRead;
	}

	void CMessage::ThrowBitOperationError( const char* where )
	{
		throw Exception("%s: Bit operation error!", where);
	}

	bool CMessage::ReadScalar( int64_t &a )
	{
		if (m_isSimplePacketMode)
		{
			return Read(a);
		}

		CCompactScalarValue val;
		if (!val.ExtractValue(GetData() + GetReadOffset(), GetLength() - GetReadOffset()))
			return false;
		
		a = val.m_extractedValue;
		SkipRead(val.m_extracteeLength);
		
		return true;
	}

	void CMessage::WriteScalar( int64_t a )
	{
		if (m_isSimplePacketMode)
		{
			return Write(a);
		}

		// 변환된 메모리 블럭 생성
		CCompactScalarValue comp;
		comp.MakeBlock(a);
		// 이를 메시지 객체에 넣기
		Write(comp.m_filledBlock,comp.m_filledBlockLength);
	}

	void CMessage::UninitBuffer()
	{
		m_readBitOffset = 0;
		m_bitLengthInOneByte = 0;
		m_msgBuffer.UninitBuffer();
	}

	CMessageTestSplitterTemporaryDisabler::CMessageTestSplitterTemporaryDisabler( CMessage& msg )
	{
		m_msg = &msg;
		m_oldVal = m_msg->IsTestSplitterEnabled();
		m_msg->EnableTestSplitter(false);
	}

	CMessageTestSplitterTemporaryDisabler::~CMessageTestSplitterTemporaryDisabler()
	{
		m_msg->EnableTestSplitter(m_oldVal);
	}
}
