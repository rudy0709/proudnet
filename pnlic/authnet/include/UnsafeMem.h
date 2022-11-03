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

//#pragma pack(push,8)

namespace Proud 
{
#if (_MSC_VER>=1400)
#pragma managed(push, off)
#endif

	// 작은 크기의 데이터에 대한 memcpy. memcpy 자체는 내부적으로 SIMD 최적화를 하지만 CPU 상황이라던지 여러가지를 알아보는 코스트가 크다.
	// 오히려 작은 크기는 이게 더 빠름.
	inline void SmallMemcpy(uint8_t*dst, const uint8_t*src, size_t size)  
	{ 
		static const size_t sizetSize = sizeof(size_t);

		size_t i; 
		//BYTE *a = (BYTE *)src; 2009.12.15 : 참조되는곳이 없어서 주석처리함
		size_t pad = size%sizetSize; 
		size_t blsize = size/sizetSize;
		for(i = 0; i < blsize; i++) 
		{ 
			*(size_t*)dst = *(size_t*)src; 
			dst +=sizetSize; 
			src +=sizetSize; 
		} 
		for (i = 0; i < pad; i++) 
		{ 
			*(uint8_t *)dst = *(uint8_t *)src; 
			dst++; 
			src++; 
		} 
	} 

	inline void UnsafeFastMemmove(void *dest,const void* src,size_t length)
	{
		// array.add에서 length=0인 경우가 잦다.
		if(length==0)
			return;

		// memmove_s나 memcpy_s는 memmove, memcpy보다 훨씬 느리다. 여러가지 바운드 체크와 내부적인 zeromem 처리가 있기 때문이다.
		// 따라서 이걸 그냥 쓴다.
		// 추후에는 AMD64,IA64,Itanium,Win32를 위한 어셈 루틴을 따로 만드는 것이 더 나을지도.
		memmove(dest,src,length);
	}

	inline void UnsafeFastMemcpy(void *dest, const void* src, size_t length)
	{
		// array.add에서 length=0인 경우가 잦다.
		if ( length == 0 )
			return;

#if defined(__MARMALADE__) || defined(__ANDROID__) || defined(__arm__) // RVCT를 쓰는 경우 __packed keyword를 쓰면 되지만 gcc를 쓰므로 
		memcpy(dest,src,length);
		// memmove_s나 memcpy_s는 memmove, memcpy보다 훨씬 느리다. 여러가지 바운드 체크와 내부적인 zeromem 처리가 있기 때문이다.
		// 따라서 이걸 그냥 쓴다.
		// 추후에는 AMD64,IA64,Itanium,Win32를 위한 어셈 루틴을 따로 만드는 것이 더 나을지도.
#else
		if ( length <= 256 ) // 32에서 상향. memcpy를 호출하는 조건을 조절해서 성능을 올렸음.
			SmallMemcpy((uint8_t*)dest, (uint8_t*)src, length);
		else
			memcpy(dest, src, length);
#endif
	}

#if (_MSC_VER>=1400)
#pragma managed(pop)
#endif

}

//#pragma pack(pop)