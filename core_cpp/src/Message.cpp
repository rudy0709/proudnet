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
	const char* ReadOffsetAlignErrorText = "Read offset is not byte aligned!";
	const char* NullAccessErrorText = "Cannot access null data message!";
	const char* SingletonLostErrorTextA = "Make sure that you have no global variable which depends on a singleton.";
	const wchar_t* SingletonLostErrorTextW = L"Make sure that you have no global variable which depends on a singleton.";

	void CMessage::ThrowOnWrongLength(const char* where, int length, int maxLength)
	{
		if (length < 0 || length > maxLength)
		{
			stringstream ss;
			ss << where << " failed! length=" << length << ",max=" << maxLength;
			throw Exception(ss.str().c_str());
		}

	}

	void CMessage::ThrowReadOffsetOutOfRangeException(int offset)
	{
		stringstream ss;
		ss << "Cannot set the read offset out of the range! Length=" << m_msgBuffer.GetCount() << ", Offset=" << offset;
		throw Exception(ss.str().c_str());
	}

	void CMessage::ThrowWrongSplitterException()
	{
		stringstream ss;
		ss << "Expected splitter! offset=" << GetReadOffset() << ",length=" << GetLength();
		throw Exception(ss.str().c_str());
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
		size_t utf8BytesLeft = utf8TextLength; // * sizeof(int8_t)
											   // 남아있는 버퍼의 양
		size_t utf16BytesLeft = utf16Capacity;

		RefCount<CDefaultStringEncoder> e = CDefaultStringEncoder::GetSharedPtr();
		if (e)
		{
			iconv_string_convert(e->m_UTF8toUTF16LEEncoder,
				utf8Src,
				&utf8BytesLeft,
				(char*)utf16Dest.GetData(),
				&utf16BytesLeft);

			utf16Dest.SetCount(utf16Capacity - utf16BytesLeft);
		}
		else
		{
			// 무책임하게 throw를 하는 것도 좀 그렇고...
			// 그냥 빈 문자열을.
			utf16Dest.SetCount(2);
			utf16Dest[0] = 0;
			utf16Dest[1] = 0;
		}
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

		RefCount<CDefaultStringEncoder> e = CDefaultStringEncoder::GetSharedPtr();
		if (e)
		{
			iconv_string_convert(e->m_UTF16LEtoUTF8Encoder,
				(const char*)utf16Src.GetData(),
				&utf16BytesLeft,
				(char*)utf8Buf.GetBuf(),
				&utf8BytesLeft);

			((char*)utf8Buf.GetBuf())[utf8Capacity - utf8BytesLeft] = 0;

			return utf8Str;
		}
		else
		{
			// 이미 converter singleton이 파괴되어 있다.
			// 어쩔 수 없다. 전역변수 파괴자에서 이걸 호출하게 만든 사용자 과실이다.
			return SingletonLostErrorTextA;
		}

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

		RefCount<CDefaultStringEncoder> e = CDefaultStringEncoder::GetSharedPtr();
		if (e)
		{
			iconv_string_convert(e->m_UTF32LEtoUTF16LEEncoder,
				(const char*)utf32Src,
				&utf32BytesLeft,
				(char*)utf16Dest.GetData(),
				&utf16BytesLeft);

			utf16Dest.SetCount(utf16Capacity - utf16BytesLeft);
		}
		else
		{
			// 무책임하게 throw를 하는 것도 좀 그렇고...
			// 그냥 빈 문자열을.
			utf16Dest.SetCount(2);
			utf16Dest[0] = 0;
			utf16Dest[1] = 0;
		}
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

		RefCount<CDefaultStringEncoder> e = CDefaultStringEncoder::GetSharedPtr();
		if (e)
		{
			iconv_string_convert(e->m_UTF16LEtoUTF32LEEncoder,
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
		else
		{
			// 이미 converter singleton이 파괴되어 있다.
			// 어쩔 수 없다. 전역변수 파괴자에서 이걸 호출하게 만든 사용자 과실이다.
			return SingletonLostErrorTextW;
		}
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

		ThrowOnWrongLength("Message.ReadString", byteLength, StringSerializeMaxLength);

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
#elif (WCHAR_LENGTH == 2) // win32
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

			str = StringW2A(wstr.GetString());

			return ret;
		}
#else
#error You must include WCHAR_LENGTH definition!
#endif
	}

	bool CMessage::ReadStringW(Proud::StringW &str)
	{
		int byteLength = 0;

		if (ReadScalar(byteLength) == false)
			return false;

		ThrowOnWrongLength("Message.ReadString", byteLength, StringSerializeMaxLength);

#if (WCHAR_LENGTH == 4)
		{
			// 유닉스에서는 UTF16 문자열을 처리 못하므로 binary data로 일단 가져온다.
			ByteArray temp;
			temp.SetCount(sizeof(short) + byteLength);
			// 0으로 모두 초기화를 하여도 byte byteLength만큼 엎어씌워집니다. 따라서 전부다 초기화하지 않고 맨 마지막 sizeof(short) 부분 2바이트만 0으로 초기화 합니다.
			memset((temp.GetData() + byteLength), 0, sizeof(short));

			bool ret = Read((uint8_t*)temp.GetData(), byteLength);
			if (ret == true)
			{
				str = iconv_utf16LE_to_utf32LE(temp);
			}
			return ret;
		}
#elif (WCHAR_LENGTH == 2) // win32
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
		ThrowOnWrongLength("Message.WriteString", (int)length, StringSerializeMaxLength);

#if (WCHAR_LENGTH == 4)
		{
			// UTF8 -> UTF16LE 변환하여 보냄
			ByteArray temp;
			iconv_utf8_to_utf16LE(str, length, temp);

			const int sendLength = temp.GetCount();

			WriteScalar(sendLength);
			Write(temp.GetData(), sendLength);
		}
#elif (WCHAR_LENGTH == 2) // win32
		{
			const StringW wstr = StringA2W(str);
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
		ThrowOnWrongLength("Message.WriteString", (int)length, StringSerializeMaxLength);

#if (WCHAR_LENGTH == 4)
		{
			// UTF32LE -> UTF16LE 변환하여 보냄
			ByteArray temp;
			iconv_utf32LE_to_utf16LE(str, length, temp);

			const int sendLength = temp.GetCount();

			WriteScalar(sendLength);
			Write(temp.GetData(), sendLength); // null은 비포함하고 시행
		}
#elif (WCHAR_LENGTH == 2) // win32
		{
			const int sendLength = (int)length * sizeof(wchar_t);

			WriteScalar(sendLength);
			Write((uint8_t*)str, sendLength); // null은 비포함하고 시행
		}
#else
#error You must include WCHAR_LENGTH definition!
#endif

	}

}
