/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#if defined(_WIN32)

#include "stdafx.h"
#include "../include/Variant.h"
#include "../include/ByteArray.h"

namespace Proud
{
	/** VT_DECIMAL에서 VT_I8로 변환하는 기능은 Windows XP 이상에서나 지원한다고 한다. 결론적으로, BIGINT - int64_t 변환은 못쓴다. 
	ToVariant(), ToLongLong() 메서드는 이 기능을 처리한다. */
	_variant_t ToVariant(LONGLONG x)
	{
		DECIMAL d;
		d.scale=0;
		d.sign=0;
		d.Hi32=0;
		if(x<0)
		{
			d.sign=DECIMAL_NEG;
			d.Lo64=-x;
		}
		else
		{
			d.sign=0;
			d.Lo64=x;
		}
		_variant_t l=d;
		return l;
	}

	_variant_t ToVariant(ULONGLONG x)
	{
		DECIMAL d;
		d.scale=0;
		d.sign=0;
		d.Hi32=0;
		d.Lo64=x;
		_variant_t l=d;
		return l;
	}

	/** Windows 2000 이하에서는 VARIANT에서 LONGLONG으로 전환하는 기능을 제공하지 않는다.
	따라서 이를 수동으로 구현해야 한다. */
	LONGLONG ToLongLong(const _variant_t &r)
	{
		DECIMAL d=r;
		if(d.scale!=0)
			_com_issue_error(DISP_E_OVERFLOW);
		if(d.sign!=0)
			return -LONGLONG(d.Lo64);
		else
			return d.Lo64;
	}

	/** Windows 2000 이하에서는 VARIANT에서 LONGLONG으로 전환하는 기능을 제공하지 않는다.
	따라서 이를 수동으로 구현해야 한다. */
	ULONGLONG ToULongLong(const _variant_t &r)
	{
		DECIMAL d=r;
		if(d.scale!=0)
			_com_issue_error(DISP_E_OVERFLOW);
		if(d.sign!=0)
			return -LONGLONG(d.Lo64);
		else
			return d.Lo64;
	}


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
				throw Proud::Exception("Cannot get ptr of COM array! %d!", ret);
			}

			memcpy(arrayData, input.GetData(), inputCount);
			SafeArrayUnaccessData(psa);
		}

		VariantClear(&output.m_val); // 이게 없으면 아래 구문에 의해 누수로 이어질 수 있음. 기존에 있던걸 없애야 하니까.
		output.m_val = v;
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

#endif // _WIN32
