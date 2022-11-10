/*
ProudNet v1


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

	/** \addtogroup util_group
	*  @{
	*/

	/**
	\~korean
	고정 크기의 배열 클래스.
	- T data[] 형식의 고정 크기 단순 배열을 bounds checking을 하려면 이 클래스로 대체해서 안정성을 도모할 수 있다.

	일반적 용도
	- CAtlArray 를 카피해다 만들었으므로 CAtlArray 와 사용법이 거의 유사하다. 단, static이니 당연하지만, 삽입/제거/청소 등의 기능이 빠져있다.

	주의사항
	- 기존 정적 배열을 CFixedLengthArray 로 바꿀 경우 _countof()나 sizeof()를 쓰는 경우가 있을 터인데, 이러한 부분을 찾아서 수정해야 한다.
	예컨대 _countof()는 항상 1을 리턴할 것이기 때문이다.

	\~english
	Array class with fixed size
	- To 'bounds check' a simple, fixed size array of T data[] format, it is possible to replace with this class to draw stability.

	General usage
	- Has similar usage of CAtlArray since this is copied and made from it, but it does not have insertion/removal/cleanup functions since it is static. (of course...)

	Note
	- If replacing the existing static array with CFixedLengthArray then there can be cases that either _countof() or sizeof() is used. << must modify them correctly.
	This is because of the fact that _countof() will always return 1.

	\~chinese
	固定大小的数组类。
	- 想bounds checking T data[]形式的固定大小单纯数组，用此类代替后可获得更好的稳定性。

	一般用途
	- 因为是复制 CAtlArray%制作的，所以与 CAtlArray%的使用方法基本相似。因为是static所以必定会相似，但里面却没有插入/删除/扫除等功能。

	注意事象
	- 想把之前静态数组替换为 CFixedLengthArray%的话，可能会有用_countof()或者sizeof()，要找到这些并对它们进行修改。
	例如，因为_countof()一直会返回1。

	\~japanese
	\~
	*/
	template<int size,typename TYPE, typename ARG_TYPE = const TYPE&>
	class CFixedLengthArray
	{
	public:

		// 제대로 된 인덱스 번호가 아니면 에러 발생한다.
		inline void CheckValidIndex(int nIndex) const
		{
			if(!(0<=nIndex && nIndex<size) )
				ThrowArrayOutOfBoundException();
		}

		/**
		\~korean
		배열 길이를 구한다.

		\~english
		Gets length of array

		\~chinese
		求数组长度。

		\~japanese
		\~
		*/
		inline int GetSize() const { return size; }

		/**
		\~korean
		배열 길이를 구한다.

		\~english
		Gets length of array

		\~chinese
		求数组长度。

		\~japanese
		\~
		*/
		inline int GetCount() const { return size; }

		/**
		\~korean
		배열이 비었는가?

		\~english
		Is array empty?

		\~chinese
		数组空了吗？

		\~japanese
		\~
		*/
		inline bool IsEmpty() const { return size==0; }

		/**
		\~korean
		배열 항목 접근자

		\~english
		Array item accessor

		\~chinese
		数组项目访问者。

		\~japanese
		\~
		*/
		inline const TYPE& GetAt(int nIndex) const { CheckValidIndex(nIndex); return m_data[nIndex]; }

		/**
		\~korean
		배열 항목 접근자

		\~english
		Array item accessor

		\~chinese
		数组项目访问者。

		\~japanese
		\~
		*/
		inline TYPE& GetAt(int nIndex) { CheckValidIndex(nIndex); return m_data[nIndex]; }

		/**
		\~korean
		배열 항목 접근자

		\~english
		Array item accessor

		\~chinese
		数组项目访问者。

		\~japanese
		\~
		*/
		inline void SetAt(int nIndex, ARG_TYPE newElement) { CheckValidIndex(nIndex); m_data[nIndex]=newElement; }

		/**
		\~korean
		배열 항목 접근자

		\~english
		Array item accessor

		\~chinese
		数组项目访问者。

		\~japanese
		\~
		*/
		inline const TYPE& ElementAt(int nIndex) const {CheckValidIndex(nIndex); return m_data[nIndex]; }

		/**
		\~korean
		배열 항목 접근자

		\~english
		Array item accessor

		\~chinese
		数组项目访问者。

		\~japanese
		\~
		*/
		inline TYPE& ElementAt(int nIndex){CheckValidIndex(nIndex); return m_data[nIndex];}

		/**
		\~korean
		배열 버퍼 객체의 포인터를 얻는다.

		\~english
		Gets pointer of array buffer object

		\~chinese
		获取数组缓存区对象的指针。

		\~japanese
		\~
		*/
		inline const TYPE* GetData() const {return m_data;}

		/**
		\~korean
		배열 버퍼 객체의 포인터를 얻는다.

		\~english
		Gets pointer of array buffer object

		\~chinese
		获取数组缓存区对象的指针。

		\~japanese
		\~
		*/
		inline TYPE* GetData(){return m_data;}

		/**
		\~korean
		배열 항목 접근자

		\~english
		Array item accessor

		\~chinese
		数组项目访问者。

		\~japanese
		\~
		*/
		inline const TYPE& operator[](int nIndex) const{CheckValidIndex(nIndex); return m_data[nIndex];}

		/**
		\~korean
		배열 항목 접근자

		\~english
		Array item accessor

		\~chinese
		数组项目访问者。

		\~japanese
		\~
		*/
		inline TYPE& operator[](int nIndex){CheckValidIndex(nIndex); return m_data[nIndex];}

	private:
		TYPE m_data[size];   // the actual array of data
	};

	/**  @} */

#if (_MSC_VER>=1400)
#pragma managed(pop)
#endif
}

//#pragma pack(pop)
