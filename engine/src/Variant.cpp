/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include "../include/variant.h"
#include "../include/ByteArray.h"

namespace Proud
{
	void CVariant::ThrowIfNull() const
	{
		if(IsNull())
			throw Exception("Cannot read Proud.CVariant value whose actual value is Null!");
	}

	template<typename ARRAY_TYPE>
	void VariantToByteArray(const CVariant &input, ARRAY_TYPE &output)
	{
		input.ThrowIfNull();

		output.Clear();
		uint8_t* pBytes = NULL;

		uint32_t lSize = input.m_val.parray->rgsabound->cElements;
		HRESULT hr = SafeArrayAccessData(input.m_val.parray,(void**)&pBytes);

		if(FAILED(hr))
			throw Exception("ToByteArray Failed - SafeArrayAccessData Failed!!");

		output.AddRange(pBytes,lSize);

		SafeArrayUnaccessData(input.m_val.parray);
	}

	void CVariant::ToByteArray(ByteArray &output) const
	{
		VariantToByteArray(*this, output);
	}

	ByteArrayPtr CVariant::ToByteArrayPtr() const
	{
		ByteArrayPtr output;

		output.UseInternalBuffer();
		VariantToByteArray(*this, output);

		return output;
	}

	template<typename ARRAY_TYPE>
	void ByteArrayToVariant(const ARRAY_TYPE &input, CVariant& output)
	{
		if(input.GetCount() < 0) // <= 이었으나 <로 고침
			throw Exception("Input Length under 0!!");

		VARIANT v;
		SAFEARRAY FAR* psa;
		SAFEARRAYBOUND saBound;


		saBound.cElements = (uint32_t)input.GetCount();
		saBound.lLbound = 0;
		psa = SafeArrayCreate(VT_UI1, 1, &saBound);

		VariantInit(&v);
		V_VT(&v) = VT_ARRAY|VT_UI1;
		V_ARRAY(&v) = psa;

		long inputCount = (long)input.GetCount();

		if (inputCount>0)
		{
			uint8_t* arrayData;
			HRESULT ret = SafeArrayAccessData(psa, (void**)&arrayData);
			if (S_OK != ret)
			{
				stringstream ss;
				ss << "Cannot get ptr of COM array! error=" << ret;
				throw Proud::Exception(ss.str().c_str());
			}

			// ikpil.choi 2016-11-10 : memcpy_s 로 변경, destSize(2번째 인자) 값이 항상 올바른 값이여야 합니다.
			//memcpy(arrayData, input.GetData(), inputCount);
			memcpy_s(arrayData, saBound.cElements, input.GetData(), inputCount);

			SafeArrayUnaccessData(psa);
		}

		// 이게 없으면 아래 구문에 의해 누수로 이어질 수 있음. 기존에 있던걸 없애야 하니까.
		output.m_val.Clear();
		output.m_val = v;

		// 안해주면 메모리 누수로 이어짐.
		VariantClear(&v);
	}

	void CVariant::FromByteArray( const ByteArray& input )
	{
		ByteArrayToVariant(input, *this);
	}

	void CVariant::FromByteArray( const ByteArrayPtr input )
	{
		ByteArrayToVariant(input, *this);
	}
}
