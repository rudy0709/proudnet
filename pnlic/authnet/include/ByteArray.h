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

#include "FastArray.h"
#include "PNString.h"

//#pragma pack(push,8)

namespace Proud
{
	/** \addtogroup util_group
	*  @{
	*/

	/**
	\~korean
	바이트 배열. ProudNet의 용도에 맞게 최적화되어 있다.
	- 네트워크 패킷을 주고 받는 목적이므로 int32로만 사용을 강제한다.
	- ByteArray는 operator new,delete도 오버라이드되어서 fast heap을 접근하고 있다. 따라서	사용시 Proud.CFastHeap의 주의사항 규약을 준수해야 한다.

	\~english TODO:translate needed.

	\~chinese
	Byte 排列。已优化成适用ProudNet用途。
	- 因其目的是为网络数据包之间的交换，因此强制使用int32。
	- BytesArray超过了operator new,delete,因此接近fast heap。所以在使用时要遵守 Proud.CFastHeap%的注意事项规章。

	\japanese
	\~
	\todo translate it.
	 */
	class ByteArray :public CFastArray < uint8_t, false, true, int >
	{
	public:
		typedef CFastArray<uint8_t, false, true, int> Base;
// 		PROUD_API static void* LookasideAllocator_Alloc(size_t length);
// 		PROUD_API static void LookasideAllocator_Free(void* ptr);
        
	public:
//#ifndef __MARMALADE__
//#pragma push_macro("new")
//#undef new
		//DECLARE_NEW_AND_DELETE
//		void* operator new(size_t size)
//		{
//			return LookasideAllocator_Alloc(size);
//		}
//		void operator delete(void* ptr, size_t /*size */)
//		{
//			LookasideAllocator_Free(ptr);
//		}
//#pragma pop_macro("new")
//#endif // __MARMALADE__

		/** 
		\~korean
		기본 생성자

		\~english
		Defalut constructor

		\~chinese
		基本生成者。

		\~japanese
		\~
		 */
		inline ByteArray()
		{
		}

		/** 
		\~korean
		외부 데이터를 복사해오는 생성자
		\param data 복사해올 BYTE배열의 포인터
		\param count count 만큼을 복사한다.

		\~english TODO:translate needed.
		Constructor copies external data 
		\param data
		\param count

		\~chinese
		复制外部数据的生成者。
		\param date 要复制的BYTE排列的指针。
		\param count 复制相当于count的量。 

		\~japanese
		\~
		 */
		inline ByteArray(const uint8_t* data, int count):Base(data,count)
		{
		}

		/** 
		\~korean
		외부 데이터를 복사해오는 생성자
		\param src 복사할 ByteArray 원본

		\~english TODO:translate needed.
		Constructor copies external data
		\param src

		\~chinese
		复制外部数据的生成者。
		\param src 要复制的BytesArray原件。

		\~japanese
		\~
		 */
		inline ByteArray(const ByteArray &src):Base(src)
		{
		}

		virtual ~ByteArray();

// 		BUG PRONE! 따라서 막아버렸다.
		/*
						\~korean
						생성자. 단, 배열 크기를 미리 설정한다.
						\~english
						Constructor. But it must have previously set array size.
						\~
						 */
// 		inline ByteArray(int count):CFastArray<BYTE,true>(count)
// 		{
// 		}

		/** 
		\~korean
		배열을 16진수의 String으로 변환하여 return해준다.
		\return String으로 변환된 16진수 배열 (예 L"FFAB123")

		\~english TODO:translate needed.

		\~chinese
		将排列转换成16进数。
		转换成
		\return String的16进数排列。（例：L”FFAB123”）

		\~japanese
		\~
		*/
		String ToHexString();

		/**
		\~korean
		16진수 배열 String을 16진수로 바꾸어준다.
		\param 16진수 배열 String (예 text = L"FFFFEAB12")
		\return 변환이 성공하면 true, 실패하면 false

		\~english TODO:translate needed.

		\~chinese
		将16进数排列String换成16进数。
		\param 16进数排列String （例：text=L”FFFFEAB12”）
		\return 转换成功为true，否则为false。

		\~japanese
		\~
		*/
		bool FromHexString(String text);
	};
	
	/**  @} */
}

//#pragma pack(pop)

