/*
ProudNet HERE_SHALL_BE_EDITED_BY_BUILD_HELPER


이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의 : 저작물에 관한 위의 명시를 제거하지 마십시오.


This program is soley copyrighted by Nettention.
Any use, correction, and distribution of this program are subject to the terms and conditions of the License Agreement.
Any violated use of this program is prohibited and will be cause of immediate termination according to the License Agreement.

** WARNING : PLEASE DO NOT REMOVE THE LEGAL NOTICE ABOVE.


此程序的版权归Nettention公司所有。
与此程序的修改、使用、发布相关的事项要遵守此程序的所有权者的协议。
不遵守协议时要原则性的禁止擅自使用。
擅自使用的责任明示在与此程序所有权者的合同书里。

** 注意：不要移除关于制作物的上述明示。


このプログラムの著作権はNettentionにあります。
このプログラムの修正、使用、配布に関する事項は本プログラムの所有権者との契約に従い、
契約を遵守しない場合、原則的に無断使用を禁じます。
無断使用による責任は本プログラムの所有権者との契約書に明示されています。

** 注意：著作物に関する上記の明示を除去しないでください。

*/
#pragma once

#include "FavoriteLV.h"
#include "PooledObjects_C.h"

namespace Proud
{
	inline CSendFragRefs::CSendFragRefs()
	{
		m_fragArray = CClassObjectPool<FragArray>::GetUnsafeRef().NewOrRecycle();
		Array().SetCount(0);
	}

	inline CSendFragRefs::CSendFragRefs(const CMessage &msg)
	{
		m_fragArray = CClassObjectPool<FragArray>::GetUnsafeRef().NewOrRecycle();
		Array().SetCount(0);

		Add(msg.GetData(), msg.GetLength());
	}

	inline CSendFragRefs::~CSendFragRefs()
	{
		CClassObjectPool<FragArray>::GetUnsafeRef().Drop(m_fragArray);
	}

	// 프로필러 결과 최적화가 중요한 함수
	inline void CSendFragRefs::ToAssembledMessage(CMessage &ret) const
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
			// ikpil.choi 2016-11-07 : memcpy_s 로 변경, destSize(2번째 인자) 값이 항상 올바른 값이여야 합니다.
			//UnsafeFastMemcpy(ret.GetData() + offset, (*m_fragArray)[i].GetData(), fragLen);
			memcpy_s(ret.GetData() + offset, ret.GetLength() - offset, (*m_fragArray)[i].GetData(), fragLen);
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
	inline void CSendFragRefs::CloneButBufferShared(CSendFragRefs &ret)
	{
		(*m_fragArray).CopyTo(*ret.m_fragArray);
	}

	template<typename ByteArrayT>
	inline void CSendFragRefs_ToByteArrayT(const CSendFragRefs& src, ByteArrayT& ret)
	{
		ret.SetCount(src.GetTotalLength());

		int offset = 0;
		intptr_t srcCount = src.Array().GetCount();
		uint8_t* retData = ret.GetData();
		const CFrag* srcData = src.GetFragmentData();

		for (intptr_t i = 0; i < srcCount; i++)
		{
			int fragLen = srcData[i].GetLength();
			// ikpil.choi 2016-11-07 : memcpy_s 로 변경, destSize(2번째 인자) 값이 항상 올바른 값이여야 합니다.
			// ret.GetCount() 가 1 byte 의 카운트인지 어떻게 보장 할 것인가?
			//UnsafeFastMemcpy(retData + offset, srcData[i].GetData(), fragLen);
			memcpy_s(retData + offset, ret.GetCount() - offset, srcData[i].GetData(), fragLen);
			offset += fragLen;
		}
	}

	inline void CSendFragRefs::CopyTo(ByteArray &ret) const
	{
		CSendFragRefs_ToByteArrayT<ByteArray>(*this, ret);
	}

	inline void CSendFragRefs::CopyTo(ByteArrayPtr &ret) const
	{
		CSendFragRefs_ToByteArrayT<ByteArrayPtr>(*this, ret);
	}


	// void CSendFragRefs::AddRangeToByteArray( ByteArrayPtr target ) const
	// {
	//	if (GetFragmentCount() == 0)
	//		return;
	//
	//	if (target.IsNull())
	//		target.UseInternalBuffer();
	//
	//	for (int i = 0;i < GetFragmentCount();i++)
	//	{
	//		if (((*this)[i]).GetData() != NULL)
	//		{
	//			target.AddRange(((*this)[i]).GetData(), ((*this)[i]).GetLength());
	//		}
	//	}
	// }
	//
	// void CSendFragRefs::AddRangeToByteArray( ByteArray &target ) const
	// {
	//	for (int i = 0;i < GetFragmentCount();i++)
	//	{
	//		if (((*this)[i]).GetData() != NULL)
	//		{
	//			target.AddRange(((*this)[i]).GetData(), ((*this)[i]).GetLength());
	//		}
	//	}
	// }

	inline int CSendFragRefs::GetTotalLength() const
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

	template<typename ByteArrayT>
	inline void CSendFragRefs::ToAssembledByteArray(ByteArrayT &output) const
	{
		CSendFragRefs_ToByteArrayT(*this, output);
	}
}
