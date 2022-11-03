#include "stdafx.h"
#include "Base64.h"
#include "../include/ByteArray.h"
#include "libtom/crypto/headers/tomcrypt.h"

namespace Proud
{
	// binary를 text로 변환한다.
	// 본 모듈은 라이선스 인증 기능 등에서만 쓰이기 때문에 StringA로만 쓴다. 어차피 출력 결과물이 다국어일 필요가 없다.
	void Base64::Encode(const ByteArray& binary, StringA &outText)
	{
		unsigned long bufLength = (unsigned long)(binary.GetCount() * 6); // 4가 적은 경우가 있다.
		StrBufA textBuf(outText, bufLength); // 이정도면 충분?
		base64_encode(binary.GetData(), binary.GetCount(), (unsigned char*)textBuf.GetBuf(), (unsigned long*)&bufLength);
	}

	// 위 함수의 반대. base64로 표현된 문자열을 binary로 바꾼다.
	void Base64::Decode(const StringA &text, ByteArray& outBinary)
	{
		ByteArray decodeBinary;
		decodeBinary.SetCount(text.GetLength());
		int bufLength = decodeBinary.GetCount();
		base64_decode((unsigned char*)text.GetString(), text.GetLength(), decodeBinary.GetData(), (unsigned long*)&bufLength);
		decodeBinary.CopyRangeTo(outBinary, 0, bufLength);
	}

}