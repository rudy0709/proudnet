#include "stdafx.h"

#include "../include/CryptoFast.h"
#include "../include/Message.h"
#include "../include/ByteArray.h"

namespace Proud
{
	CCryptoFastKey::CCryptoFastKey()
	{
	}

	CCryptoFastKey::~CCryptoFastKey()
	{
		Clear();
	}

	int CCryptoFastKey::GetKeyLength() const
	{
		if (KeyExists() == false)
			return -1;
		else
			return m_key.GetCount();
	}

	bool CCryptoFastKey::KeyExists() const
	{
		return (m_key.GetCount() > 0) ? true : false;
	}

	void CCryptoFastKey::Clear()
	{
		m_key.Clear();
	}

	inline uint8_t _FASTROR(uint8_t b)
	{
		return ((b >> 1) | (b << 7));
	}

	inline uint8_t _FASTROL(uint8_t b)
	{
		return ((b << 1) | (b >> 7));
	}

	bool CCryptoFast::ExpandFrom(CCryptoFastKey& outKey, const uint8_t* inputKey, int keyLength)
	{
		if (inputKey == NULL || (keyLength != 64 && keyLength != 128 && keyLength != 256))
			return false;

		if (outKey.KeyExists() == true)
			outKey.Clear();

		outKey.m_key.SetCount(keyLength);

		uint8_t* keyPtr = outKey.m_key.GetData();

		// 키를 변형하여 저장함으로써 XOR 암호화로 인한 Key를 평문을 넣었을 때 생기는 키유출 문제 해결과 함께 암호화의 복잡도 상승
		for (int i = 0; i < keyLength; i++)
		{
			// 각 바이트에 126씩 더함. (키가 ASCII 문자열일 경우 이를 감추는 효과)
			// 1비트 ROR
			*(keyPtr + i) = _FASTROR(inputKey[i] + 0x7E);
		}

		return true;
	}

	int CCryptoFast::GetEncryptSize(int inputLength)
	{
		if (inputLength <= 0)
			return -1;

		// inputLength + checkSum(2)
		return inputLength + 2;
	}

	bool CCryptoFast::Encrypt(
		const CCryptoFastKey& key,
		const uint8_t* input,
		int inputLength,
		uint8_t *output,
		int& outputLength,
		ErrorInfoPtr& errorInfo
		)
	{
		// 키 유효성 검사
		if (key.KeyExists() == false)
		{
			errorInfo = ErrorInfo::From(ErrorType_EncryptFail, HostID_None, _PNT("key for fast encryption does not exist."));
			return false;
		}

		// 평문 유효성 검사
		if (input == NULL)
		{
			errorInfo = ErrorInfo::From(ErrorType_EncryptFail, HostID_None, _PNT("input is NULL."));
			return false;
		}

		if (inputLength <= 0)
		{
			errorInfo = ErrorInfo::From(ErrorType_EncryptFail, HostID_None, _PNT("invalid input length."));
			return false;
		}

		if (output == NULL)
		{
			errorInfo = ErrorInfo::From(ErrorType_EncryptFail, HostID_None, _PNT("output is NULL."));
			return false;
		}

		if (outputLength < GetEncryptSize(inputLength))
		{
			errorInfo = ErrorInfo::From(ErrorType_EncryptFail, HostID_None, _PNT("insufficient output buffer."));
			return false;
		}

		uint16_t checkSum = 0;
		const uint8_t* keyPtr = key.m_key.GetData();
		int keyLength = key.m_key.GetCount();

		// 암호화
		for (int i = 0, keyOffset = 0; i < inputLength; i++, keyOffset++)
		{
			if (keyOffset >= keyLength) keyOffset = 0;

			// XOR 후 1비트 ROR
			output[i] = _FASTROR(input[i] ^ keyPtr[keyOffset]);

			if (i > 0)
				output[i] += output[i - 1]; // 앞 데이터를 더하여 암호 복잡도를 증가시킨다.

			checkSum += input[i];
		}

		// checksum을 저장
		*((uint16_t*)(output + inputLength)) = htons(-checkSum);
		outputLength = inputLength + 2; // 최종 결과는 암호문 + 체크섬(2)

		//errorInfo = ErrorInfo::From(ErrorType_Ok);	//부하 문제가 있어 오류가 발생할때만 생성
		return true;
	}

	bool CCryptoFast::Decrypt(
		const CCryptoFastKey& key,
		const uint8_t* input,
		int inputLength,
		uint8_t *output,
		int& outputLength,
		ErrorInfoPtr& errorInfo
		)
	{
		// 키 유효성 검사
		if (key.KeyExists() == false)
		{
			errorInfo = ErrorInfo::From(ErrorType_DecryptFail, HostID_None, _PNT("key for fast encryption does not exist."));
			return false;
		}

		if (input == NULL)
		{
			errorInfo = ErrorInfo::From(ErrorType_DecryptFail, HostID_None, _PNT("input is NULL."));
			return false;
		}

		if (inputLength < 3)
		{
			errorInfo = ErrorInfo::From(ErrorType_DecryptFail, HostID_None, _PNT("input length is invalid."));
			return false;
		}

		if (output == NULL)
		{
			errorInfo = ErrorInfo::From(ErrorType_DecryptFail, HostID_None, _PNT("output is NULL."));
			return false;
		}

		if (GetEncryptSize(outputLength) < inputLength)
		{
			errorInfo = ErrorInfo::From(ErrorType_DecryptFail, HostID_None, _PNT("output length is invalid."));
			return false;
		}

		uint16_t checkSum;
		const uint8_t* keyPtr = key.m_key.GetData();
		int keyLength = key.m_key.GetCount();

		outputLength = inputLength - 2;	// 암호문의 크기는 입력문에서 체크섬(2)을 제거한 크기
		checkSum = ntohs(*((uint16_t*)&input[outputLength]));

		// 복호화
		for (int i = 0, keyOffset = 0; i < outputLength; i++, keyOffset++)
		{
			if (keyOffset >= keyLength) keyOffset = 0;

			if (i > 0)
				output[i] = input[i] - input[i - 1];	// SUB
			else
				output[i] = input[i];

			// 1비트 ROL 후 XOR
			output[i] = _FASTROL(output[i]) ^ keyPtr[keyOffset];

			checkSum += output[i];
		}

		// 체크섬 검사
		if (checkSum != 0)
		{
			errorInfo = ErrorInfo::From(ErrorType_DecryptFail, HostID_None, _PNT("checksum failure."));
			return false;
		}

		//errorInfo = ErrorInfo::From(ErrorType_Ok);	//부하 문제가 있어 오류가 발생할때만 생성
		return true;
	}

	bool CCryptoFast::EncryptByteArray(
		const CCryptoFastKey& key,
		const ByteArray& input,
		ByteArray& output,
		ErrorInfoPtr& errorInfo
		)
	{
		int inputLength;
		int outputLength;

		inputLength = input.GetCount();
		outputLength = GetEncryptSize(inputLength);
		output.SetCount(outputLength);

		return Encrypt(key, input.GetData(), inputLength, output.GetData(), outputLength, errorInfo);
	}

	bool CCryptoFast::DecryptByteArray(
		const CCryptoFastKey& key,
		const ByteArray& input,
		ByteArray& output,
		ErrorInfoPtr& errorInfo
		)
	{
		bool result;
		int outputLength;

		outputLength = input.GetCount();
		output.SetCount(outputLength);
		result = Decrypt(key, input.GetData(), outputLength, output.GetData(), outputLength, errorInfo);

		if (result == true)
			output.SetCount(outputLength);

		return result;
	}

	bool CCryptoFast::EncryptMessage(
		const CCryptoFastKey& key,
		const CMessage& input,
		CMessage& output,
		int offset,
		ErrorInfoPtr& errorInfo
		)
	{
		int inputLength;
		int outputLength;

		inputLength = input.GetLength() - offset;

		if (inputLength <= 0)
			return false;

		outputLength = GetEncryptSize(inputLength);
		output.SetLength(outputLength);

		return Encrypt(key, input.GetData() + offset, inputLength, output.GetData(), outputLength, errorInfo);
	}

	bool CCryptoFast::DecryptMessage(
		const CCryptoFastKey& key,
		const CMessage& input,
		CMessage& output,
		int offset,
		ErrorInfoPtr& errorInfo
		)
	{
		bool result;
		int outputLength;

		outputLength = input.GetLength() - offset;
		if (outputLength <= 0)
			return false;

		output.SetLength(outputLength);

		result = Decrypt(key, input.GetData() + offset, outputLength, output.GetData(), outputLength, errorInfo);

		if (result == true)
			output.SetLength(outputLength);

		return result;
	}
}
