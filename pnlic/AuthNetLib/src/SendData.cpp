/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <new>
#include "SendFragRefs.h"
#include "../include/Marshaler.h"
#include "PooledObjects.h"

namespace Proud
{
	CSendFragRefs::CSendFragRefs()
	{
		m_fragArray = CClassObjectPool<FragArray>::Instance().NewOrRecycle();
		Array().SetCount(0);
	}

	CSendFragRefs::CSendFragRefs(const CMessage &msg)
	{
		m_fragArray = CClassObjectPool<FragArray>::Instance().NewOrRecycle();
		Array().SetCount(0);

		Add(msg.GetData(), msg.GetLength());
	}

	CSendFragRefs::~CSendFragRefs()
	{
		CClassObjectPool<FragArray>::Instance().Drop(m_fragArray);
	}

	void CSendFragRefs::AppendFrag_ShareBuffer( CSendFragRefs &appendee )
	{
		int count = appendee.Array().GetCount();
		for (int i = 0; i < count; i++)
		{
			Add((*appendee.m_fragArray)[i]);
		}
	}

	// 프로필러 결과 최적화가 중요한 함수
	void CSendFragRefs::ToAssembledMessage( CMessage &ret ) const
	{
		ret.UninitBuffer();
		ret.UseInternalBuffer();

		// 미리 크기 쟁여놓고 복사를 해버리게 해야 더 빠름
		int totalLength = GetTotalLength();
		ret.SetLength(totalLength);

		intptr_t cnt = (*m_fragArray).GetCount();
		int offset = 0;

		for (int i = 0; i < cnt; i++)
		{
			int fragLen = (*m_fragArray)[i].GetLength();
			UnsafeFastMemcpy(ret.GetData()+offset,(*m_fragArray)[i].GetData(),fragLen);
			offset += fragLen;
		}
	}

	// 프로필러 결과 위의함수만 쓰면됨 아래는 복사가 일어나므로 쓰지말것....
	/*CMessage CSendFragRefs::ToAssembledMessage() const
	{
		CMessage ret;
		ToAssembledMessage(ret);
		return ret;
	}*/


	// m_data를 복사하되 이것이 가진 내부 데이터까지 복사하지는 않는다.
	// 다층 layer를 다룰 때 유용한 함수.
	void CSendFragRefs::CloneButBufferShared( CSendFragRefs &ret )
	{
		(*m_fragArray).CopyTo(*ret.m_fragArray);
	}

	template<typename T>
	void CSendFragRefs_ToByteArrayT(const CSendFragRefs& src, T& ret) 
	{
		ret.SetCount(src.GetTotalLength());

		int offset = 0;
		intptr_t srcCount = src.Array().GetCount();
		uint8_t* retData = ret.GetData();
		const CSendFragRefs::CFrag* srcData = src.GetFragmentData();

		for (intptr_t i = 0; i < srcCount; i++)
		{
			int fragLen = srcData[i].GetLength();
			UnsafeFastMemcpy(retData+offset, srcData[i].GetData(), fragLen);
			offset += fragLen;
		}
	}

	void CSendFragRefs::CopyTo( ByteArray &ret ) const
	{
		CSendFragRefs_ToByteArrayT<ByteArray>(*this, ret);
	}

	void CSendFragRefs::CopyTo( ByteArrayPtr &ret ) const
	{
		CSendFragRefs_ToByteArrayT<ByteArrayPtr>(*this, ret);
	}


	// void CSendFragRefs::AddRangeToByteArray( ByteArrayPtr target ) const
	// {
	// 	if (GetFragmentCount() == 0)
	// 		return;
	// 
	// 	if (target.IsNull())
	// 		target.UseInternalBuffer();
	// 
	// 	for (int i = 0;i < GetFragmentCount();i++)
	// 	{
	// 		if (((*this)[i]).GetData() != NULL)
	// 		{
	// 			target.AddRange(((*this)[i]).GetData(), ((*this)[i]).GetLength());
	// 		}
	// 	}
	// }
	// 
	// void CSendFragRefs::AddRangeToByteArray( ByteArray &target ) const
	// {
	// 	for (int i = 0;i < GetFragmentCount();i++)
	// 	{
	// 		if (((*this)[i]).GetData() != NULL)
	// 		{
	// 			target.AddRange(((*this)[i]).GetData(), ((*this)[i]).GetLength());
	// 		}
	// 	}
	// }

	int CSendFragRefs::GetTotalLength() const
	{
		int ret = 0;
		intptr_t cnt = Array().GetCount(); // cache
		const CFrag* pData = Array().GetData(); // skip bound check
		for (intptr_t i = 0; i < cnt; i++)
		{
			ret += (int)pData[i].GetLength();
		}

		return ret;
	}

	void CSendFragRefs::ToAssembledByteArray( ByteArray &output ) const
	{
		CSendFragRefs_ToByteArrayT(*this, output);
	}

	void AppendTextOut(String& output, const CSendFragRefs& v)
	{
		String t;
		t.Format(_PNT("Data(%d bytes)\n"), v.GetTotalLength());
		output += t;
	}

}
