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

#include "../include/BasicTypes.h"
#include "../include/Exception.h"

namespace Proud
{
	/** 
	CFastArray와 같지만 더 낮은 memory fragmentation 효과가 있다.
	내부에 일정 크기의 내부 배열 데이터 공간을 갖고 있기 때문이다.
	그러나, 제한 크기 이상은 사용 불가능하다. 그리고 객체 자체가 직접 데이터를 가지므로 크기가 크다.
	따라서 로컬 변수 등 지역적 상황에서만 쓰자. */
	template < int InternalDataSize_, typename TYPE, bool TYPE_IN_REF = true, bool RAWTYPE = false, typename INDEXTYPE = intptr_t >
	class CLowFragMemArray :public CFastArray < TYPE, TYPE_IN_REF, RAWTYPE, INDEXTYPE >
	{
		// 위의 의도: 1st 인자는 int지만 (스택에 누가 int64 크기 고정배열이 필요하겠어!) x86 컴파일 호환을 위해 INDEXTYPE이 존재.
		
		/****************************
		주의!!! 아래 변수는 PS4에서 잘 작동하지 않는다! 스택 깨먹음!!
		*/

		// TYPE 배열이면 비의도된 파괴자가 호출되므로 이렇게 byte 블럭을 사용한다.
		uint8_t m_internalData[InternalDataSize_ * sizeof(TYPE)];

		typedef CFastArray < TYPE, TYPE_IN_REF, RAWTYPE, INDEXTYPE > Base;
	public:
		// Construction
		inline CLowFragMemArray()
		{
			this->m_Data = (TYPE*)m_internalData;
			this->m_Capacity = InternalDataSize_;
		}

		// Operations

		// Implementation
	protected:
#if defined(_MSC_VER)       
		__declspec(property(get = GetTypedInternalData)) TYPE* TypedInternalData;
#endif
		
		inline TYPE* GetTypedInternalData() {
			return (TYPE*)m_internalData;
		}
		inline const TYPE* GetTypedInternalData() const {
			return (TYPE*)m_internalData;
		}

		inline bool IsUsingInternalData() const {
			return this->m_Capacity <= InternalDataSize_;
		}
	public:
		~CLowFragMemArray()
		{
			this->SetCount(0);
			this->m_Data = NULL;
		}

		virtual INDEXTYPE GetRecommendedCapacity(INDEXTYPE actualCount)
		{
			if (actualCount > InternalDataSize_)
				ThrowArrayOutOfBoundException();

			return this->m_Capacity;
		}

		virtual void DataBlock_Free(void* data)
		{
			// 하는 일이 없다. 
			// 주의: CFastArray가 m_Data=null 하는 일이 없음을 가정하고 있다!
		}

		virtual void* DataBlock_Alloc(size_t length)
		{
			// 하는 일이 없다. 
			// 주의: CFastArray가 m_Data=null 하는 일이 없음을 가정하고 있다!
			return this->m_Data;
		}

		virtual void* DataBlock_Realloc(void* oldPtr, size_t newLength)
		{
			return this->m_Data;
		}
	};

}
