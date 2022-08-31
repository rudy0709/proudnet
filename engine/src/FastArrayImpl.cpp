/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <new>
#include "FastArrayImpl.h"
#include "../include/ByteArray.h"
#ifdef _WIN32
#include <tchar.h>
#endif


namespace Proud
{
	String ByteArray::ToHexString()
	{
		String ret;
		char ele[10];
		for(int i=0;i<GetCount();i++)
		{
			// NDK에서는 wsprintf가 작동 안하므로 sprintf를 쓴다.
			sprintf_s(ele, "%02x", ElementAt(i));
			ret += StringA2T(ele);
		}

		return ret;
	}

#if defined(_WIN32)
	bool ByteArray::FromHexString( String text )
	{
		SetCount(0);

		// 홀수 문자열 즐
		if(text.GetLength()%2 != 0)
			return false;

		for(int i=0;i<text.GetLength()/2;i++)
		{
			PNTCHAR token[3];
			token[0]=text[i*2];
			token[1]=text[i*2+1];
			token[2]=0;

			int v;//byte로 하면 뻑남
#if (_MSC_VER>=1400)
			_stscanf_s(token,_PNT("%02x"),&v);
#else
			Tsscanf(token,_PNT("%02x"),&v);
#endif
			Add((uint8_t)v);
		}

		return true;
	}
#endif

/*	// "new ByteArray" 자체를 성능 강화하기 위함
	// => 이제 이건 쓰지 말자. ByteArrayPtr 자체가 obj-pool을 하니.
	void* ByteArray::LookasideAllocator_Alloc(size_t length)
	{
		return CByteArrayHolderAlloc::Instance().m_allocator->Alloc(length);

	}
	void ByteArray::LookasideAllocator_Free(void* ptr)
	{
		CByteArrayHolderAlloc::Instance().m_allocator->Free(ptr);
	}*/


	ByteArray::~ByteArray()
	{
	}
}
