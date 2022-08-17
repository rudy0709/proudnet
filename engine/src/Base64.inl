#include "stdafx.h"
#include "Base64.h"
#include "../include/ByteArray.h"
//222-원본: #include "libtom/crypto/headers/tomcrypt.h"
#include "../OpenSources/libtomcrypt/headers/tomcrypt.h"

namespace Proud
{
	// binary를 text로 변환한다.
	// 본 모듈은 라이선스 인증 기능 등에서만 쓰이기 때문에 StringA로만 쓴다. 어차피 출력 결과물이 다국어일 필요가 없다.
	// ByteArrayT에는 ByteArray or ByteArrayPtr 등을 쓰자.
	template<typename ByteArrayT>
	inline void Base64::Encode(const ByteArrayT& binary, StringA &outText)
	{
		Encode(binary.GetData(), binary.GetCount(), outText);
	}

	inline void Base64::Encode(const uint8_t* binary, int binaryLength, StringA &outText)
	{
		unsigned long bufLength = (unsigned long)(binaryLength * 6); // 4가 적은 경우가 있다.
		int c;
		{
			StrBufA textBuf(outText, bufLength); // 이정도면 충분?
			c = base64_encode(binary, binaryLength, (unsigned char*)textBuf.GetBuf(), (unsigned long*)&bufLength);
		}

		// 실패하면, 출력값을 청소해버리자.
		// TODO: 이렇게 하지 말고, 명시적으로 에러코드를 콜러에게 주어서, 나머지를 책임감있게 처리하게 하자.
		if (c != CRYPT_OK)
			outText = StringA();
	}	

	// 위 함수의 반대. base64로 표현된 문자열을 binary로 바꾼다.
	// ByteArrayT에는 ByteArray or ByteArrayPtr 등을 쓰자.
	template<typename ByteArrayT>
	inline void Base64::Decode(const StringA &text, ByteArrayT& outBinary)
	{
		outBinary.SetCount(text.GetLength());
		int bufLength = outBinary.GetCount();
		int c = base64_decode((unsigned char*)text.GetString(), text.GetLength(), outBinary.GetData(), (unsigned long*)&bufLength);

		// 실패하면, 출력값을 청소해버리자.
		// TODO: 이렇게 하지 말고, 명시적으로 에러코드를 콜러에게 주어서, 나머지를 책임감있게 처리하게 하자.
		if (c == CRYPT_OK)
			outBinary.SetCount(bufLength);
		else
			outBinary.SetCount(0);
	}

}