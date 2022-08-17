#pragma once
#include "../include/ByteArray.h"

//////////////////////////////////////////////////////////////////////////
// 본 모듈은 사용자에게 노출되는 API가 아니라, 라이선스 인증 기능 등에서 사용되므로,
// src/ 안에 있음.

namespace Proud
{
	// base64 인코딩을 한다. 즉 바이너리를 최대한 작은 크기의 텍스트로 변환.
	class Base64
	{
	public:
		template<typename ByteArrayT>
		inline static void Encode(const ByteArrayT& binary, StringA &outText);

		inline static void Encode(const uint8_t* binary, int binaryLength, StringA &outText);

		template<typename ByteArrayT>
		inline static void Decode(const StringA &text, ByteArrayT& outBinary);
	};

}


#include "Base64.inl"