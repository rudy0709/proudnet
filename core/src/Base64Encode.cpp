#include "stdafx.h"
#include "../include/ByteArrayPtr.h"
#include "Base64.h"
#include "../include/Base64Encode.h"

namespace Proud
{
	void Base64Encoder::Encode(const ByteArray& binary, StringA& outText)
	{
		Base64::Encode<ByteArray>(binary, outText);
	}
	void Base64Encoder::Encode(const uint8_t* binary, int binaryLength, StringA& outText)
	{
		Base64::Encode(binary, binaryLength, outText);
	}
	
	void Base64Encoder::Decode(const StringA& text, ByteArray& outBinary)
	{
		Base64::Decode<ByteArray>(text, outBinary);
	}

	void Base64Encoder::Encode(const ByteArrayPtr& binary, StringA& outText)
	{
		Base64::Encode<ByteArrayPtr>(binary, outText);
	}
	void Base64Encoder::Decode(const StringA& text, ByteArrayPtr& outBinary)
	{
		Base64::Decode<ByteArrayPtr>(text, outBinary);
	}

}
