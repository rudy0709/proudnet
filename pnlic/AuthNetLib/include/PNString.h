/*
ProudNet v1.7


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

#include <new>
#include <exception>

#include "BasicTypes.h"

#include <exception>
#include <stdio.h>
#include <stdarg.h>
#include "ProcHeap.h"
//#include "CriticalSect.h"
#include "atomic.h"
#include "StringEncoder.h"

#ifdef _MSC_VER // pragma warning은 VC++전용
#pragma warning(push)
#pragma warning(disable:4324)
#endif // __MARMALADE__

//#pragma pack(push,8)

#if (_MSC_VER>=1400)
#pragma managed(push,off)
#endif

namespace Proud
{
	PROUD_API _Noreturn void ThrowInvalidArgumentException();

	/** \addtogroup util_group
	*  @{
	*/

	class AnsiStrTraits
	{
	public:
		typedef char CharType;

		static int SafeStringLen(const CharType* str);

		static void CopyString(CharType* dest, const CharType* src,int length);

		static int StringCompare(const CharType* a,const CharType*b);
		static int StringCompareNoCase(const CharType* a,const CharType*b);
		static int GetFormattedLength(const char* pszFormat, va_list args);
		static int __cdecl Format( char* pszBuffer, size_t nlength, const char* pszFormat, va_list args ) throw();

// 해결 방법을 찾을때 까지 막아놓음.
#if defined(_WIN32) 
		static char* StringUppercase(char* psz, size_t size ) throw();
		static char* StringLowercase(char* psz, size_t size ) throw();
#endif
		static const char* __cdecl StringFindString( const char* pszBlock, const char* pszMatch ) throw();
		static char* __cdecl StringFindString( char* pszBlock, const char* pszMatch ) throw();
		static const char* __cdecl StringFindChar( const char* pszBlock, char chMatch ) throw();
		static int __cdecl StringSpanIncluding( const char* pszBlock, const char* pszSet ) throw();
		static int __cdecl StringSpanExcluding( const char* pszBlock, const char* pszSet ) throw();
		static char* __cdecl CharNext( const char* p ) throw();
		static int __cdecl IsDigit( char ch ) throw();
		static int __cdecl IsSpace( char ch ) throw();

		static char* NullString;
	};

	class UnicodeStrTraits
	{
	public:
		typedef wchar_t CharType;

		static int SafeStringLen(const CharType* str);
		static void CopyString(CharType* dest, const CharType* src,int length);
		static int StringCompare(const CharType* a,const CharType*b);
		static int StringCompareNoCase(const CharType* a,const CharType*b);
		static int GetFormattedLength(const wchar_t* pszFormat, va_list args);
		static int __cdecl Format(wchar_t* pszBuffer, size_t nLength, const wchar_t* pszFormat, va_list args) throw( );

// 해결 방법을 찾을때 까지 막아놓음.
#if defined(_WIN32) 
		static wchar_t* StringUppercase(wchar_t* psz, size_t size) throw( );
		static wchar_t* StringLowercase(wchar_t* psz, size_t size) throw( );
#endif
		static const wchar_t* __cdecl StringFindString(const wchar_t* pszBlock, const wchar_t* pszMatch) throw( );
		static wchar_t* __cdecl StringFindString(wchar_t* pszBlock, const wchar_t* pszMatch) throw( );
		static const wchar_t* __cdecl StringFindChar(const wchar_t* pszBlock, wchar_t chMatch) throw( );
		static int __cdecl StringSpanIncluding(const wchar_t* pszBlock, const wchar_t* pszSet) throw( );
		static int __cdecl StringSpanExcluding(const wchar_t* pszBlock, const wchar_t* pszSet) throw( );
		static wchar_t* __cdecl CharNext(const wchar_t* psz) throw( );
		static int __cdecl IsDigit( wchar_t ch ) throw();
		static int __cdecl IsSpace( wchar_t ch ) throw();

		static wchar_t* NullString;
	};

	/** 
	\~korean
	문자열 클래스
	- 이 클래스를 직접 사용하지 말고 Proud.String, Proud.StringW, Proud.StringA 를 통해 쓰는 것을 권장합니다.
	- 자세한 것은 \ref string 을 참고바랍니다.

	\~english
	Text string class
	- It is recommended not to use this class directly but to use via Proud.String, Proud.StringW and Proud.StringA.
	- Please refer \ref string.

	\~chinese
	字符串类
	- 不要直接使用此类，建议通过 Proud.String, Proud.StringW,  Proud.StringA%使用。
	- 详细请参考\ref string%。

	\~japanese
	\~
	*/
	template<typename XCHAR, typename StringTraits>
	class StringT 
	{
		// copy-on-write 및 shared ptr 형태를 모두 제공한다.
		// 문자열 버퍼 바로 앞단에 위치한다. 
		// NOTE: Tombstone이 가진 문자열 객체는 일단 한번 설정되고 나면 소멸할 때까지 내용물이 절대 바뀌지 말아야 한다.
		// 왜냐하면 여러 변수들이 이 문자열 데이터를 잠금 없이 공유하기 때문이다.
		// 각 변수들은 자기가 가진 문자열 데이터를 변경하고자 하는 경우 
		// 변형하는 문자열 데이터를 만든 후 새 tombstone에 그 내용을 복사해 넣어야 한다.
		class Tombstone
		{
		public:
			// 캐싱된 문자열 길이
			int m_length;

			// 참조 횟수
			PROUDNET_VOLATILE_ALIGN intptr_t m_refCount;

			inline Tombstone()
			{
				m_length = 0;
				m_refCount = 1;
			}

			inline XCHAR* GetBuffer() 
			{
				return (XCHAR*)(this+1);
			}
		};

		// 문자열 클래스는 이 데이터 멤버만을 유일하게 가져야 하며 vtable을 가질 수 없다.
		// 그래야 printf 계열에서 객체 자체를 직접 넣어도 잘 작동하니까.
		// 이 포인터가 가리키는 블럭 바로 앞에 Tombstone이 있다.
		XCHAR* m_strPtr; 

		// 사용자가 이 객체를 zeromem해서 쓰는 경우라도 완충을 하게 하기 위함(F모사)
		void AdjustNullPtr()
		{
			if(m_strPtr == 0)
			{
				m_strPtr = StringTraits::NullString;
			}
		}

		void ShareFrom(const StringT& src)
		{
			AdjustNullPtr();
			if(src.m_strPtr != m_strPtr)
			{
				ReleaseTombstone();
				
				m_strPtr = src.m_strPtr;
				
				if(GetTombstone())
                {
					AtomicIncrementPtr(&GetTombstone()->m_refCount);
                }
			}
		}

		void ReleaseTombstone()
		{
			AdjustNullPtr();
			
			Tombstone* ts = GetTombstone();
			if(ts)
			{
				intptr_t result = AtomicDecrementPtr(&ts->m_refCount);
				if(result == 0)
				{
					CProcHeap::Free(ts);
				}
			}			
			m_strPtr = StringTraits::NullString;
		}

		inline Tombstone* GetTombstone() const
		{
			if(m_strPtr == StringTraits::NullString || !m_strPtr)
				return NULL;

			uint8_t* ptr = (uint8_t*)m_strPtr;
			return (Tombstone*) (ptr-sizeof(Tombstone));
		}

		// strlen: sz제외하고. char의 경우 바이트,wchar_t인 경우 글자수.
		static size_t GetBlockLen( int strLen )
		{
			if(strLen<0)
				ThrowInvalidArgumentException();
			return sizeof(Tombstone) + sizeof(XCHAR) * ((size_t)strLen+1);
		}

		void PrepareCopyOnWrite()
		{
			AdjustNullPtr();

			if (GetTombstone() == NULL)
			{
				// 없으면 새로 만든다.
				size_t blockLen = GetBlockLen(0);
				Tombstone* ts = (Tombstone*)CProcHeap::Alloc(blockLen);
				if(ts == NULL)
					ThrowBadAllocException();

                memset(ts,0,blockLen);
				CallConstructor<Tombstone>(ts);
				m_strPtr = ts->GetBuffer();
			}
			else if(GetTombstone()->m_refCount > 1) // 단일 참조이면 사본 뜨는 것이 무의미하다.
			{
				// 사본
				Tombstone* newTS = (Tombstone*) CProcHeap::Alloc( GetBlockLen(GetLength()));
				if( newTS == NULL )
					ThrowBadAllocException();

				CallConstructor<Tombstone>(newTS);
				newTS->m_length = GetLength();				

				StringTraits::CopyString(newTS->GetBuffer(), GetString(), GetLength()); // sz도 만들어줌.
				
				// 원본 교체
				ReleaseTombstone();

				m_strPtr = newTS->GetBuffer();
			}
		}

		inline void InitVars()
		{
			m_strPtr = StringTraits::NullString;
		}

		inline void ReleaseBufferSetLength(int length)
		{
			length = PNMAX(length, 0);
			length = PNMIN(length, GetLength());

			Tombstone* ts = GetTombstone();
			if (ts != NULL)
			{
				ts->m_length = length;
				ts->GetBuffer()[length] = 0; // sz mark				
			}
		}

		void Truncate( int nNewLength )
		{
			assert( nNewLength <= GetLength() );
			GetBuffer( nNewLength );
			ReleaseBufferSetLength( nNewLength );
		}

		// length: byte(char) or 글자수(wchar_t)
		void SetMaxLength(int length)
		{
			AdjustNullPtr();

			length = PNMAX(length, 0);
			if(length == 0)
			{
				ReleaseTombstone();
			}
			else if(length != GetLength())
			{
				// 다른 변수가 참조 안하고 있으면 그냥 realloc한다.
				Tombstone* ts = GetTombstone();
				if (ts != NULL && ts->m_refCount == 1)
				{
					ts = (Tombstone*)CProcHeap::Realloc(ts, GetBlockLen(length));
					ts->m_length = length;
					m_strPtr = ts->GetBuffer(); // realloc에 의해 ts가 바뀐 경우를 위해
					m_strPtr[length] = 0;	// 크기가 줄어든 경우를 위해 sz마감처리
				}
				else
				{
					// 다른 변수가 참조하고 있으면 원본을 건드리면 안된다. 
					// 따라서 크기 다른 사본 즉 copy on write와 같은 준비를 한다.
					Tombstone* newTS = (Tombstone*)CProcHeap::Alloc(GetBlockLen(length));
					if( newTS == NULL)
						ThrowBadAllocException();

					CallConstructor<Tombstone>(newTS);
					newTS->m_length = length;

					StringTraits::CopyString(newTS->GetBuffer(), GetString(), PNMIN(length,GetLength()));

					// 원본 교체
					ReleaseTombstone();

					m_strPtr = newTS->GetBuffer();
				}
			}
		}

	public:
		/**
		\~korean
		생성자

		\~english
		Constructor

		\~chinese
		生成者

		\~japanese
		\~
		*/
		StringT()
		{
			InitVars();
		}

		/**
		\~korean
		파괴자

		\~english
		Destructor

		\~chinese
		破坏者

		\~japanese
		\~
		*/
		~StringT()
		{
			ReleaseTombstone();
		}

		/**
		\~korean
		src가 가리키는 문자열을 가져온다.

		\~english
		Gets string that pointed by src

		\~chinese
		拿来src所指的字符串。

		\~japanese
		\~
		*/
		inline StringT(const StringT& src)
		{
			InitVars();
			ShareFrom(src);
		}

		/**
		\~korean
		문자열을 가져온다. 

		\~english
		Gets a string

		\~chinese
		拿来字符串。

		\~japanese
		\~
		*/
		StringT(const XCHAR* src, int length = 0)
		{
			InitVars();
			int len = StringTraits::SafeStringLen(src);
			
			if(length > 0)
				len = PNMIN(len, length);
			
			XCHAR* buf = GetBuffer(len);
			StringTraits::CopyString(buf,src,len);
			ReleaseBuffer();
		}

		/**
		\~korean
		빈 문자열인가? 

		\~english
		Is it an empty string ?

		\~chinese
		是空字符串吗?

		\~japanese
		\~
		*/
		inline bool IsEmpty() const
		{
			return GetLength() == 0;
		}

		/** 
		\~korean
		문자열 데이터를 아래 예시처럼 바로 읽을 수 있게 해준다. 
        \code
            Proud::String a = L"abc";
            const wchar_t *b = a; // 객체 a에 있는 문자열 포인터를 b가 받는다.
        \endcode

		\~english
		Let's read text string data directly as shown below 
        \code
            Proud::String a = L"abc";
            const wchar_t *b = a; // Text sting pointer in object a is received by b.
        \endcode

		\~chinese
		让字符串数据像以下例子可以直接读。
\code
            Proud::String a = L"abc";
            const wchar_t *b = a; // 指针b接收在对象a中的字符串。
        \endcode

		\~japanese
		\~
		*/
		inline operator const XCHAR* () const
		{			
			return GetString();
		}

		/** 
		\~korean
		문자열 데이터를 읽을 수 있게 한다. 

		\~english
		Let's read text string data

		\~chinese
		可以使其读取字符串数据。

		\~japanese
		\~
		*/
		inline const XCHAR* GetString() const 
		{
			if(m_strPtr == 0)
				return StringTraits::NullString;

			return m_strPtr;
		}

		/** 
		\~korean
		문자열 데이터를 읽기 및 쓰기를 위해 포인터를 접근할 때 쓰는 메서드입니다.
		- 이 메서드를 쓰고 나서는 ReleaseBuffer로 마감해줘야 합니다. 이를 편하게 하려면
		StrBufA나 StrBufW를 쓰면 됩니다.

		\~english
		Method used to access string pointer to read from and write to string data.
		- After using this method, it must be ended by calling ReleaseBuffer(). To use this easily, use Proud::StrBufA or Proud::StrBufW.

		\~chinese
		为了读取或编写文字列数据从而接近指针时所使用的方法。
		- 用此方法以后得用ReleaseBuffer结束。想进行得便利些的话使用StrBufA或者StrBufW即可。

		\~japanese
		
		\~ 
		\~korean 사용 예시
		\~english Usage example
		\~chinese 使用例子
		\~japanese
		\~

		\code
        Proud::String a = L"abc";
        wchar_t* pa = a.GetBuffer(100);
        _wsprintf(pa, L"%d", 123);
        a.ReleaseBuffer();
        \endcode

		\~
		
		\~korean 더 나은 사용 예시
		\~english Better usage example
		\~chinese 更好的使用例子。
		\~japanese
		\~ \code
		Proud::String a = L"abc";
		Proud::StrBuf pa(a, 100);
		_wsprintf(pa, L"%d", 123);
		\endcode

		\~
		\~korean 읽기 전용 사용 예시
		\~english Example for read-only access
		\~chinese 只读用使用例子。
		\~japanese
		\~
		\code
		Proud::String a = L"abc";
		wchar_t* pa = a;
		\endcode
		*/
		XCHAR* GetBuffer(int length = 0)
		{
			// length는 최소 1글자는 저장할 수 있는 기록 가능한 크기를 만든다.
			// 그래야 사용자가 유효한 문자열 주소를 얻으므로.
			length = PNMAX(length, GetLength());
			length = PNMAX(length, 1);

			if(GetLength() != length)
			{
				// copy-on-write with resize
				// 기존 크기보다 작은 크기를 얻으려고 한다 하더라도, 다른 스레드의 다른 변수가 이 tombstone을 공유하고 있을 경우에 그쪽도 바뀌면 안되므로
				// 이렇게 copy-on-write 처리를 해주어야 함.
				SetMaxLength(length);
			}
			else
			{
				// copy-on-write without resize
				PrepareCopyOnWrite();
			}

			return (XCHAR*)GetString();
		}

		/**
		\~korean
		문자열 길이를 얻습니다. 

		\~english
		Gets length of string.

		\~chinese
		获得字符串长度。

		\~japanese
		\~

		\return length Number of characters if wchar_t is used, number of bytes if char is used, regardless of multibyte or unicode encoding.
		*/
		int GetLength() const
		{
			if(m_strPtr == NULL)
				return 0;

			if(m_strPtr == StringTraits::NullString)
				return 0;

			else 
				return GetTombstone()->m_length;
		}

		/** 
		\~korean
		문자열 길이를 재조정한다. 

		\~english
		Re-modifies the length of text string

		\~chinese
		重新调整字符串长度。

		\~japanese
		\~
		\param length Number of characters if wchar_t is used, number of bytes if char is used, regardless of multibyte or unicode encoding.
		*/
		void SetLength(int length)
		{
			length = PNMIN(length, StringTraits::SafeStringLen(GetString()) );
			SetMaxLength(length);
		}

		/** 
		\~korean
		문자열 뒤에 다른 문자열을 덧붙인다. 

		\~english
		Adds another text string after text string

		\~chinese
		往字符串后面添加其他字符串。

		\~japanese
		\~
		*/
		inline void Append(const StringT& src)
		{
			if(src.GetLength() > 0)
			{
				int oldLen = GetLength();
				XCHAR* buf = GetBuffer(oldLen + src.GetLength());
				StringTraits::CopyString(buf + oldLen, src.GetString(), src.GetLength() );
				ReleaseBufferSetLength(oldLen + src.GetLength());
			}
		}

		/** 
		\~korean
		문자열 뒤에 다른 문자열을 덧붙인다. 

		\~english
		Adds another text string to text string

		\~chinese
		往字符串后面添加其他字符串。

		\~japanese
		\~
		*/
		inline void Append(const XCHAR* src)
		{
			if (src != NULL)
			{
				int srcLen = StringTraits::SafeStringLen(src);
				
				int oldLen = GetLength();
				XCHAR* buf = GetBuffer(oldLen + srcLen);
				StringTraits::CopyString(buf + oldLen, src, srcLen );
				ReleaseBufferSetLength(oldLen + srcLen);
			}
		}

		/** 
		\~korean
		이 객체의 문자열과 src가 가리키는 문자열을 비교한다. 이 객체의 문자열이 더 '사전적 앞'이면 음수가, 같으면 0, 그렇지 않으면 양수가 리턴된다. 

		\~english
		Compare this objective’s string to string that src indicates. If this objective’s string is smaller than src, negative number will be returned (if same, 0 will be returned & if it is bigger than src, positive number will be returned).

		\~chinese
		比较此对象的字符串和src所指的字符串。如果此对象的字符串比src小返回负数，相同返回0，否则返回正数。

		\~japanese
		このオブジェクトの文字列とsrcが示す文字列を比較します。このオブジェクトの文字列がsrcより小さければ陰数、同じければ０、そうでなければ陽数がリターンされます。

		\~
		*/
		inline int Compare(const StringT& src) const
		{
			XCHAR* strPtr = m_strPtr;
			if (strPtr == NULL)
				strPtr = StringTraits::NullString;;

			if(strPtr == src.m_strPtr)
				return 0;

			return StringTraits::StringCompare(GetString(), src.GetString());
		}

		/** 
		\~korean
		이 객체의 문자열과 src가 가리키는 문자열을 비교한다. 이 객체의 문자열이 더 '사전적 앞'이면 음수가, 같으면 0, 그렇지 않으면 양수가 리턴된다. 

		\~english
		Compare this objective’s string to string that src indicates. If this objective’s string is smaller than src, negative number will be returned (if same, 0 will be returned & if it is bigger than src, positive number will be returned).

		\~chinese
		比较此对象的字符串和src所指的字符串。如果此对象的字符串比src小返回负数，相同返回0，否则返回正数。

		\~japanese
		このオブジェクトの文字列とsrcが示す文字列を比較します。このオブジェクトの文字列がsrcより小さければ陰数、同じければ０、そうでなければ陽数がリターンされます。

		\~
		*/
		inline int Compare(const XCHAR* src) const
		{
			return StringTraits::StringCompare(GetString(), src);
		}

		/** 
		\~korean
		이 객체의 문자열과 src가 가리키는 문자열을 비교한다. 이 객체의 문자열이 더 '사전적 앞'이면 음수가, 같으면 0, 그렇지 않으면 양수가 리턴된다. 
		단, 대소문자 구별을 하지 않는다. 

		\~english
		Compare this objective’s string to string that src indicates. If this objective’s string is smaller than src, negative number will be returned (if same, 0 will be returned & if it is bigger than src, positive number will be returned). 
		But, not classify capital and small letter.

		\~chinese
		比较此对象的字符串和src所指的字符串。如果此对象的字符串比src小返回负数，相同返回0，否则返回正数。
		但，不区分大小字母。

		\~japanese
		このオブジェクトの文字列とsrcが示す文字列を比較します。このオブジェクトの文字列がsrcより小さければ陰数、同じければ０、そうでなければ陽数がリターンされます。
		ただし、大小文字の区別をしません。
		\~
		*/
		inline int CompareNoCase(const StringT& src) const
		{
			const XCHAR* p1 = src.m_strPtr;
			const XCHAR* p2 = m_strPtr;
			if(!p1) p1 = StringTraits::NullString;
			if(!p2) p2 = StringTraits::NullString;
			if(p1==p2)
				return 0;

			return StringTraits::StringCompareNoCase(GetString(), src.GetString());
		}

		/** 
		\~korean
		이 객체의 문자열과 src가 가리키는 문자열을 비교한다. 이 객체의 문자열이 더 '사전적 앞'이면 음수가, 같으면 0, 그렇지 않으면 양수가 리턴된다. 
		단, 대소문자 구별을 하지 않는다. 

		\~english
		Compare this objective’s string to string that src indicates. If this objective’s string is smaller than src, negative number will be returned (if same, 0 will be returned & if it is bigger than src, positive number will be returned). 
		But, not classify capital and small letter.

		\~chinese
		比较此对象的字符串和src所指的字符串。如果此对象的字符串比src小返回负数，相同返回0，否则返回正数。
		但，不区分大小字母。

		\~japanese
		このオブジェクトの文字列とsrcが示す文字列を比較します。このオブジェクトの文字列がsrcより小さければ陰数、同じければ０、そうでなければ陽数がリターンされます。
		ただし、大小文字の区別をしません。
		\~
		*/
		inline int CompareNoCase(const XCHAR* src) const
		{
			return StringTraits::StringCompareNoCase(GetString(),src);
		}

		/**
		\~korean
		Format 메소드와 똑같은 기능을 하지만, static 메소드이며 새로운 객체를 리턴한다.

		\~english
		\~chinese
		\~japanese
		\~
		*/
		static inline StringT NewFormat(const XCHAR* pszFormat, ...)
		{
			assert(pszFormat != NULL);

			StringT ret;

			va_list argList;
			va_start(argList, pszFormat);
			ret.FormatV(pszFormat, argList);
			va_end(argList);

			return ret;
		}

		/** 
		\~korean
		printf 처럼 문자열을 만든다. ( \ref string_format  참고) 

		\~english
		Creates text string as printf (Please refer \ref string_format)

		\~chinese
		像printf一样创建字符串。（参考 \ref string_format）

		\~japanese
		\~
		*/
		inline void Format( const XCHAR* pszFormat, ... )
		{
			assert( pszFormat != NULL );

			va_list argList;
			va_start( argList, pszFormat );
			FormatV( pszFormat, argList );
			va_end( argList );
		}

		/** 
		\~korean
		printf 처럼 문자열을 만든다. ( \ref string_format  참고) 

		\~english
		Creates text string as printf (Please refer \ref string_format)

		\~chinese
		像printf一样创建字符串。（参考 \ref string_format）

		\~japanese
		\~
		*/
		void FormatV( const XCHAR* pszFormat, va_list args )
		{
			assert( pszFormat != NULL );

			if(pszFormat == NULL)
            {
				ThrowInvalidArgumentException();
            }

			int nLength = StringTraits::GetFormattedLength(pszFormat, args) + 1;
			
			XCHAR* buf = GetBuffer(nLength);
			StringTraits::Format(buf, nLength, pszFormat, args);
			buf[nLength] = 0;

			ReleaseBuffer();
		}

		/** 
		\~korean
		GetBuffer를 호출했으면 이걸로 마무리를 해줘야 한다. 안그러면 문자열 길이를 정확하게 가지지 못할 수 있다.
		자세한 것은 GetBuffer 설명 참고. 

		\~english
		Once GetBuffer is called then this must end to finalize. Otherwise, the exact length of text string may not be acquired.
		Please refer GetBuffer description

		\~chinese
		呼叫GetBuffer的话要以此结束。否则有可能不能正确拥有字符串长度。
		详细请参考GetBuffer说明。

		\~japanese
		\~
		*/
		inline void ReleaseBuffer()
		{
			ReleaseBufferSetLength(StringTraits::SafeStringLen(GetString()));
		}

		/** 
		\~korean
		문자열 중 chOld가 가리키는 글자를 chNew 로 바꾼다. 

		\~english
		Among text strings, replace the character pointed by chOld with the character pointed by chNew.

		\~chinese
		将字符串中chOld所指的字替换成chNew。

		\~japanese
		\~
		*/
		int Replace( XCHAR chOld, XCHAR chNew )
		{
			int nCount = 0;

			// short-circuit the nop case
			if( chOld != chNew )
			{
				// otherwise modify each character that matches in the string
				bool bCopied = false;
				XCHAR* pszBuffer = const_cast< XCHAR* >( GetString() );  // We don't actually write to pszBuffer until we've called GetBuffer().

				int nLength = GetLength();
				int iChar = 0;
				while( iChar < nLength )
				{
					// replace instances of the specified character only
					if( pszBuffer[iChar] == chOld )
					{
						if( !bCopied )
						{
							bCopied = true;
							pszBuffer = GetBuffer( nLength );
						}
						pszBuffer[iChar] = chNew;
						nCount++;
					}
					iChar = int( StringTraits::CharNext( pszBuffer+iChar )-pszBuffer );
				}
				if( bCopied )
				{
					ReleaseBufferSetLength( nLength );
				}
			}

			return( nCount );
		}

		/** 
		\~korean
		문자열 중 pszOld가 가리키는 문자열을 pszNew 로 바꾼다. 

		\~english
		Among text strings, replace the character pointed by pszOld with the character pointed by pszNew.

		\~chinese
		将字符串中pszOld所指的字替换成pszNew。

		\~japanese
		\~
		*/
		int Replace( const XCHAR* pszOld, const XCHAR* pszNew )
		{
			// can't have empty or NULL lpszOld

			// nSourceLen is in XCHARs
			int nSourceLen = StringTraits::SafeStringLen( pszOld );
			if( nSourceLen == 0 )
				return( 0 );
			// nReplacementLen is in XCHARs
			int nReplacementLen = StringTraits::SafeStringLen( pszNew );

			// loop once to figure out the size of the result string
			int nCount = 0;
			{
				const XCHAR* pszStart = GetString();
				const XCHAR* pszEnd = pszStart+GetLength();
				while( pszStart < pszEnd )
				{
					const XCHAR* pszTarget;
					while( (pszTarget = StringTraits::StringFindString( pszStart, pszOld ) ) != NULL)
					{
						nCount++;
						pszStart = pszTarget+nSourceLen;
					}
					pszStart += StringTraits::SafeStringLen( pszStart )+1;
				}
			}

			// if any changes were made, make them
			if( nCount > 0 )
			{
				// if the buffer is too small, just
				//   allocate a new buffer (slow but sure)
				int nOldLength = GetLength();
				int nNewLength = nOldLength+(nReplacementLen-nSourceLen)*nCount;

				XCHAR* pszBuffer = GetBuffer( PNMAX( nNewLength, nOldLength ) );

				XCHAR* pszStart = pszBuffer;
				XCHAR* pszEnd = pszStart+nOldLength;

				// loop again to actually do the work
				while( pszStart < pszEnd )
				{
					XCHAR* pszTarget;
					while( (pszTarget = StringTraits::StringFindString( pszStart, pszOld ) ) != NULL )
					{
						int nBalance = nOldLength-int(pszTarget-pszBuffer+nSourceLen);
#if (_MSC_VER>=1400)
						memmove_s( pszTarget+nReplacementLen, nBalance*sizeof( XCHAR ), 
							pszTarget+nSourceLen, nBalance*sizeof( XCHAR ) );
						memcpy_s( pszTarget, nReplacementLen*sizeof( XCHAR ), 
							pszNew, nReplacementLen*sizeof( XCHAR ) );
#else
						memmove( pszTarget+nReplacementLen,
							pszTarget+nSourceLen, nBalance*sizeof( XCHAR ) );
						memcpy( pszTarget, 
							pszNew, nReplacementLen*sizeof( XCHAR ) );
#endif
						pszStart = pszTarget+nReplacementLen;
						pszTarget[nReplacementLen+nBalance] = 0;
						nOldLength += (nReplacementLen-nSourceLen);
					}
					pszStart += StringTraits::SafeStringLen( pszStart )+1;
				}
				assert( pszBuffer[nNewLength] == 0 );
				ReleaseBufferSetLength( nNewLength );
			}

			return( nCount );
		}

		/** 
		\~korean
		문자열을 대문자화한다. 

		\~english
		Capitalizes text string

		\~chinese
		将字符串大写字母化。

		\~japanese
		\~
		*/
		inline StringT& MakeUpper()
		{
			int nLength = GetLength();
			XCHAR* pszBuffer = GetBuffer( nLength );
			StringTraits::StringUppercase( pszBuffer, nLength+1 );
			ReleaseBufferSetLength( nLength );

			return( *this );
		}

		/** 
		\~korean
		문자열을 소문자화한다. 

		\~english
		De-capitalize text string

		\~chinese
		将字符串小写字母化。

		\~japanese
		\~
		*/
		inline StringT& MakeLower()
		{
			int nLength = GetLength();
			XCHAR* pszBuffer = GetBuffer( nLength );
			StringTraits::StringLowercase( pszBuffer, nLength+1 );
			ReleaseBufferSetLength( nLength );

			return( *this );
		}

		/** 
		\~korean
		비교 연산자 

		\~english
		Comparison operator

		\~chinese
		比较运算符。

		\~japanese
		\~
		*/
		inline bool operator==(const StringT &src) const
		{
			return Compare(src) == 0;
		}

		inline bool operator<(const StringT& src) const
		{
			return Compare(src) < 0;
		}

		/** 
		\~korean
		비교 연산자 

		\~english
		Comparison operator

		\~chinese
		比较运算符。

		\~japanese
		\~
		*/
		inline bool operator!=(const StringT& src) const
		{
			return Compare(src) != 0;
		}

		/** 
		\~korean
		비교 연산자 

		\~english
		Comparison operator

		\~chinese
		比较运算符。

		\~japanese
		\~
		*/
		inline bool operator==(const XCHAR* src) const
		{
			return Compare(src) == 0;
		}

		/** 
		\~korean
		비교 연산자 

		\~english
		Comparison operator

		\~chinese
		比较运算符。

		\~japanese
		\~
		*/
		inline bool operator!=(const XCHAR* src) const
		{
			return Compare(src) != 0;
		}

		/** 
		\~korean
		문자열 덧붙이기 연산자 

		\~english
		Text string adding operator

		\~chinese
		字符添加串运算符。

		\~japanese
		\~
		*/
		inline StringT& operator+=(const StringT& src)
		{
			Append(src);
			return *this;
		}

		/** 
		\~korean
		문자열 덧붙이기 연산자 

		\~english
		Text string adding operator

		\~chinese
		字符串添加运算符。

		\~japanese
		\~
		*/
		inline StringT& operator+=(const XCHAR* src)
		{
			Append(src);
			return *this;
		}

		/** 
		\~korean
		문자열 복사 대입 연산자 

		\~english
		String copy assignment operator

		\~chinese
		字符串复制运算符。

		\~japanese
		\~
		*/
		inline StringT& operator=(const StringT& src)
		{
			ShareFrom(src);
			return *this;
		}

		/** 
		\~korean
		문자열 복사 대입 연산자 

		\~english
		String copy assignment operator

		\~chinese
		字符串复制运算符。

		\~japanese
		\~
		*/
		inline StringT& operator=(const XCHAR* src)
		{
			StringT r(src);
			ShareFrom(r);
			return *this;
		}

		/** 
		\~korean
		문자열의 iStart 번째 캐릭터부터 검색해서 ch 가 가리키는 글자의 위치를 찾는다. 못찾으면 -1을 리턴한다. 

		\~english
		Finds the location of the character that is pointed by ch through searching from 'iStart'th character of text string. Returns -1 when not found.

		\~chinese
		从字符串的第iStart个角色开始搜索，查找ch所指的字的位置。没有找到的话返回-1。

		\~japanese
		\~
		*/
		int Find( XCHAR ch, int iStart = 0 ) const throw()
		{
			// iStart is in XCHARs
			assert( iStart >= 0 );

			// nLength is in XCHARs
			int nLength = GetLength();
			if( iStart < 0 || iStart >= nLength)
			{
				return( -1 );
			}

			// find first single character
			const XCHAR* psz = StringTraits::StringFindChar( GetString()+iStart, ch );

			// return -1 if not found and index otherwise
			return( (psz == NULL) ? -1 : int( psz-GetString() ) );
		}

		/** 
		\~korean
		문자열의 iStart 번째 캐릭터부터 검색해서 pszSub 가 가리키는 문자열의 첫 글자 위치를 찾는다. 못찾으면 -1을 리턴한다. 

		\~english
		Finds the location of the character that is pointed by pszSub through searching from 'iStart'th character of text string. Returns -1 when not found.

		\~chinese
		从字符串的第iStart个角色开始搜索，查找pszSub所指的字符串的第一个字位置。没有找到的话返回-1。

		\~japanese
		\~
		*/
		int Find( const XCHAR* pszSub, int iStart = 0 ) const throw()
		{
			// iStart is in XCHARs
			assert( iStart >= 0 );

			if(pszSub == NULL)
			{
				return( -1 );
			}
			// nLength is in XCHARs
			int nLength = GetLength();
			if( iStart < 0 || iStart > nLength )
			{
				return( -1 );
			}

			// find first matching substring
			const XCHAR* psz = StringTraits::StringFindString( GetString()+iStart, pszSub );

			// return -1 for not found, distance from beginning otherwise
			return( (psz == NULL) ? -1 : int( psz-GetString() ) );
		}

		/** 
		\~korean
		문자열 안에서 iFirst가 가리키는 곳부터 나머지 글자를 추려서 리턴한다. 

		\~english
		Returns the characters collected from the location where pointed by iFirst to the end in text string.

		\~chinese
		在字符串里面，挑选从iFirst所指的地方开始到剩下的文字后返回。

		\~japanese
		\~
		*/
		inline StringT Mid( int iFirst ) const
		{
			return( Mid( iFirst, GetLength()-iFirst ) );
		}

		/** 
		\~korean
		문자열 안에서 iFirst가 가리키는 곳부터 nCount 만큼의 글자를 추려서 리턴한다. 

		\~english
		Returns the characters collected from the location where pointed by iFirst to the point where the amount of characters become nCount in text string.

		\~chinese
		在字符串里面，从iFirst所指的地方开始挑选到相当于nCount的字后返回。

		\~japanese
		\~
		*/
		StringT Mid( int iFirst, int nCount ) const
		{
			// nCount is in XCHARs

			// out-of-bounds requests return sensible things
			if (iFirst < 0)
				iFirst = 0;
			if (nCount < 0)
				nCount = 0;

			if( iFirst + nCount > GetLength() )
			{
				nCount = GetLength()-iFirst;
			}
			if( iFirst > GetLength() )
			{
				nCount = 0;
			}

			assert( (nCount == 0) || ((iFirst+nCount) <= GetLength()) );

			// optimize case of returning entire string
			if( (iFirst == 0) && ((iFirst+nCount) == GetLength()) )
			{
				return( *this );
			}

			return( StringT( GetString()+iFirst, nCount) );
		}

		/** 
		\~korean
		문자열 내에서 우측으로부터 nCount만큼의 문자열만 추려서 리턴한다. 

		\~english
		Returns the text stings that are collected from RHS to the point where the amount of characters become nCount in text string.

		\~chinese
		在字符串里面，挑选从右侧开始到相当于nCount的字后返回。

		\~japanese
		\~
		*/
		StringT Right( int nCount ) const
		{
			// nCount is in XCHARs
			if (nCount < 0)
				nCount = 0;

			int nLength = GetLength();
			if( nCount >= nLength )
			{
				return( *this );
			}

			return( StringT( GetString()+nLength-nCount, nCount ) );
		}

		/** 
		\~korean
		문자열 내에서 좌측으로부터 nCount만큼의 문자열만 추려서 리턴한다. 

		\~english
		Returns the text stings that are collected from LHS to the point where the amount of characters become nCount in text string.

		\~chinese
		在字符串里面，挑选从左侧开始到相当于nCount的字后返回。

		\~japanese
		\~
		*/
		StringT Left( int nCount ) const
		{
			// nCount is in XCHARs
			if (nCount < 0)
				nCount = 0;

			int nLength = GetLength();
			if( nCount >= nLength )
			{
				return( *this );
			}

			return( StringT( GetString(), nCount ) );
		}

		/** 
		\~korean
		pszToken가 가리키는 구분자들로 구별된 문자열을 뜯어내서 하나씩 리턴한다.
		
        사용 예
        \code
            Proud::String str(L"%First Second#Third");
            Proud::String resToken;
            int curPos = 0;

            resToken= str.Tokenize(L"% #",curPos);
            while (resToken != L"")
            {
                _tprintf_s(L"Resulting token: %s\n", resToken);
                resToken = str.Tokenize(L"% #", curPos);
            };   
        \endcode 

		\~english
		Collects the text strings distinguished by distinguishers pointed by ofpszToken one by one then returns them one by one. 
		
        Usage example
        \code
            Proud::String str(L"%First Second#Third");
            Proud::String resToken;
            int curPos = 0;

            resToken= str.Tokenize(L"% #",curPos);
            while (resToken != L"")
            {
                _tprintf_s(L"Resulting token: %s\n", resToken);
                resToken = str.Tokenize(L"% #", curPos);
            };   
        \endcode

		\~chinese
		拆开pszToken所指的由区分者所区分的字符串后一个个返回。

		使用例子
		\code
            Proud::String str(L"%First Second#Third");
            Proud::String resToken;
            int curPos = 0;

            resToken= str.Tokenize(L"% #",curPos);
            while (resToken != L"")
            {
                _tprintf_s(L"Resulting token: %s\n", resToken);
                resToken = str.Tokenize(L"% #", curPos);
            };   
        \endcode 

		\~japanese
		\~
		*/
        StringT Tokenize( const XCHAR* pszTokens, int& iStart ) const
		{
			assert( iStart >= 0 );

			if(iStart < 0)
			{
				ThrowInvalidArgumentException();
			}

			if( (pszTokens == NULL) || (*pszTokens == (XCHAR)0) )
			{
				if (iStart < GetLength())
				{
					return( StringT( GetString()+iStart ) );
				}
			}
			else
			{
				const XCHAR* pszPlace = GetString()+iStart;
				const XCHAR* pszEnd = GetString()+GetLength();
				if( pszPlace < pszEnd )
				{
					int nIncluding = StringTraits::StringSpanIncluding( pszPlace,
						pszTokens );

					if( (pszPlace+nIncluding) < pszEnd )
					{
						pszPlace += nIncluding;
						int nExcluding = StringTraits::StringSpanExcluding( pszPlace, pszTokens );

						int iFrom = iStart+nIncluding;
						int nUntil = nExcluding;
						iStart = iFrom+nUntil+1;

						return( Mid( iFrom, nUntil ) );
					}
				}
			}

			// return empty string, done tokenizing
			iStart = -1;

			return( StringT() );
		}

		/** 
		\~korean
		문자열 오른쪽 끝에 남아있는 빈 글자들을 잘라내버린다. 

		\~english
		Cuts off the empty characters are left at RHS end of text string

		\~chinese
		剪掉留在字符串右侧的空文字。

		\~japanese
		\~
		*/
		StringT& TrimRight()
		{
			// find beginning of trailing spaces by starting
			// at beginning (DBCS aware)

			const XCHAR* psz = GetString();
			const XCHAR* pszLast = NULL;

			while( *psz != 0 )
			{
				if( StringTraits::IsSpace( *psz ) )
				{
					if( pszLast == NULL )
						pszLast = psz;
				}
				else
				{
					pszLast = NULL;
				}
				psz = StringTraits::CharNext( psz );
			}

			if( pszLast != NULL )
			{
				// truncate at trailing space start
				int iLast = int( pszLast-GetString() );

				Truncate( iLast );
			}

			return( *this );
		}

		/** 
		\~korean
		문자열 왼쪽 끝에 남아있는 빈 글자들을 잘라내버린다. 

		\~english
		Cuts off the empty characters are left at LHS end of text string

		\~chinese
		剪掉留在字符串左侧的空文字。

		\~japanese
		\~
		*/
		StringT& TrimLeft()
		{
			// find first non-space character

			const XCHAR* psz = GetString();

			while( StringTraits::IsSpace( *psz ) )
			{
				psz = StringTraits::CharNext( psz );
			}

			if( psz != GetString() )
			{
				// fix up data and length
				int iFirst = int( psz-GetString() );
				XCHAR* pszBuffer = GetBuffer( GetLength() );
				psz = pszBuffer+iFirst;
				int nDataLength = GetLength()-iFirst;
#if (_MSC_VER>=1400)
				memmove_s( pszBuffer, (GetLength()+1)*sizeof( XCHAR ), 
					psz, (nDataLength+1)*sizeof( XCHAR ) );
#else
				memmove( pszBuffer,
					psz, (nDataLength+1)*sizeof( XCHAR ) );
#endif
				ReleaseBufferSetLength( nDataLength );
			}

			return( *this );
		}

		/** 
		\~korean
		문자열 양쪽 끝에 남아있는 빈 글자들을 잘라내버린다. 

		\~english
		Cuts off the empty characters are left at RHS end and at LHS end of text string

		\~chinese
		剪掉留在字符串两侧的空文字。

		\~japanese
		\~
		*/
		StringT& Trim()
		{
			return( TrimRight().TrimLeft() );
		}

		/** 
		\~korean
		문자열 양쪽 끝에 남아있는 chTarget이 가리키는 글자들을 잘라내버린다. 

		\~english
		Cuts off the characters pointed by chTarget and are left at RHS end and at LHS end of text string

		\~chinese
		剪掉留在字符串两侧chTarget指向的文字。

		\~japanese
		\~
		*/
		StringT& Trim( XCHAR chTarget )
		{
			return( TrimRight( chTarget ).TrimLeft( chTarget ) );
		}

		/** 
		\~korean
		문자열 양쪽 끝에 남아있는 pszTargets이 가리키는 글자들을 잘라내버린다. 

		\~english
		Cuts off the characters pointed by pszTargets and are left at RHS end and at LHS end of text string

		\~chinese
		剪掉留在字符串两侧pszTargets指向的文字。

		\~japanese
		\~
		*/
		StringT& Trim( const XCHAR* pszTargets )
		{
			return( TrimRight( pszTargets ).TrimLeft( pszTargets ) );
		}

		/** 
		\~korean
		문자열 우측 끝에 남아있는 chTarget이 가리키는 글자들을 잘라내버린다. 

		\~english
		Cuts off the characters pointed by chTarget and are left at RHS end of text string 

		\~chinese
		剪掉留在字符串右侧chTarget指向的文字。

		\~japanese
		\~
		*/
		StringT& TrimRight( XCHAR chTarget )
		{
			// find beginning of trailing matches
			// by starting at beginning (DBCS aware)			

			const XCHAR* psz = GetString();
			const XCHAR* pszLast = NULL;

			while( *psz != 0 )
			{
				if( *psz == chTarget )
				{
					if( pszLast == NULL )
					{
						pszLast = psz;
					}
				}
				else
				{
					pszLast = NULL;
				}
				psz = StringTraits::CharNext( psz );
			}

			if( pszLast != NULL )
			{
				// truncate at left-most matching character  
				int iLast = int( pszLast-GetString() );
				Truncate( iLast );
			}

			return( *this );
		}

		/** 
		\~korean
		문자열 우측 끝에 남아있는 pszTargets이 가리키는 글자들을 잘라내버린다. 

		\~english
		uts off the characters pointed by pszTargets and are left at RHS end of text string

		\~chinese
		剪掉留在字符串右侧pszTargets指向的文字。

		\~japanese
		\~
		*/
		StringT& TrimRight( const XCHAR* pszTargets )
		{
			// if we're not trimming anything, we're not doing any work
			if( (pszTargets == NULL) || (*pszTargets == 0) )
			{
				return( *this );
			}

			// find beginning of trailing matches
			// by starting at beginning (DBCS aware)

			const XCHAR* psz = GetString();
			const XCHAR* pszLast = NULL;

			while( *psz != 0 )
			{
				if( StringTraits::StringFindChar( pszTargets, *psz ) != NULL )
				{
					if( pszLast == NULL )
					{
						pszLast = psz;
					}
				}
				else
				{
					pszLast = NULL;
				}
				psz = StringTraits::CharNext( psz );
			}

			if( pszLast != NULL )
			{
				// truncate at left-most matching character  
				int iLast = int( pszLast-GetString() );
				Truncate( iLast );
			}

			return( *this );
		}

		/** 
		\~korean
		문자열 좌측 끝에 남아있는 chTarget이 가리키는 글자들을 잘라내버린다. 

		\~english
		Cuts off the characters pointed by chTarget and are left at LHS end of text string 

		\~chinese
		剪掉留在字符串左侧chTarget指向的文字。

		\~japanese
		\~
		*/
		StringT& TrimLeft( XCHAR chTarget )
		{
			// find first non-matching character
			const XCHAR* psz = GetString();

			while( chTarget == *psz )
			{
				psz = StringTraits::CharNext( psz );
			}

			if( psz != GetString() )
			{
				// fix up data and length
				int iFirst = int( psz-GetString() );
				XCHAR* pszBuffer = GetBuffer( GetLength() );
				psz = pszBuffer+iFirst;
				int nDataLength = GetLength()-iFirst;
#if (_MSC_VER>=1400)
				memmove_s( pszBuffer, (GetLength()+1)*sizeof( XCHAR ), 
					psz, (nDataLength+1)*sizeof( XCHAR ) );
#else
				memmove( pszBuffer,
					psz, (nDataLength+1)*sizeof( XCHAR ) );
#endif
				ReleaseBufferSetLength( nDataLength );
			}

			return( *this );
		}

		/** 
		\~korean
		문자열 좌측 끝에 남아있는 pszTargets이 가리키는 글자들을 잘라내버린다. 

		\~english
		Cuts off the characters pointed by pszTargets and are left at RHS end of text string

		\~chinese
		剪掉留在字符串左侧pszTargets指向的文字。

		\~japanese
		\~
		*/
		StringT& TrimLeft( const XCHAR* pszTargets )
		{
			// if we're not trimming anything, we're not doing any work
			if( (pszTargets == NULL) || (*pszTargets == 0) )
			{
				return( *this );
			}

			const XCHAR* psz = GetString();
			while( (*psz != 0) && (StringTraits::StringFindChar( pszTargets, *psz ) != NULL) )
			{
				psz = StringTraits::CharNext( psz );
			}

			if( psz != GetString() )
			{
				// fix up data and length
				int iFirst = int( psz-GetString() );
				XCHAR* pszBuffer = GetBuffer( GetLength() );
				psz = pszBuffer+iFirst;
				int nDataLength = GetLength()-iFirst;
#if defined(_WIN32)
				Checked::memmove_s( pszBuffer, (GetLength()+1)*sizeof( XCHAR ), 
					psz, (nDataLength+1)*sizeof( XCHAR ) );
#else
                memmove(pszBuffer, psz, (nDataLength+1)*sizeof( XCHAR ));
#endif
				ReleaseBufferSetLength( nDataLength );
			}

			return( *this );
		}

#ifdef _WIN32
#pragma push_macro("new")
#undef new
		// 이 클래스는 ProudNet DLL 경우를 위해 커스텀 할당자를 쓴다.
		// proc heap을 쓴다. 전역 변수로 선언해 쓰기도 하니까.
		DECLARE_NEW_AND_DELETE
#pragma pop_macro("new")
#endif // __MARMALADE__

	};
	
	template<typename XCHAR, typename StringTraits>
	StringT<XCHAR,StringTraits> operator+(const StringT<XCHAR,StringTraits> &a,const StringT<XCHAR,StringTraits> &b)
	{
		StringT<XCHAR,StringTraits> ret = a;
		ret += b;
		return ret;
	}

	template<typename XCHAR, typename StringTraits>
	StringT<XCHAR,StringTraits> operator+(const StringT<XCHAR,StringTraits> &a,const XCHAR* b)
	{
		StringT<XCHAR,StringTraits> ret = a;
		ret += b;
		return ret;
	}

	template<typename XCHAR, typename StringTraits>
	StringT<XCHAR,StringTraits> operator+(const XCHAR* a,const StringT<XCHAR,StringTraits> &b)
	{
		StringT<XCHAR,StringTraits> ret = a;
		ret += b;
		return ret;
	}

	/** 
	\~korean
	CStringT.GetBuffer, CStringT.ReleaseBuffer 의 호출을 자동화한다. 

	\~english
	Automates calling CStringT.GetBuffer, CStringT.ReleaseBuffer

	\~chinese
	自动化 CStringT.GetBuffer,  CStringT.ReleaseBuffer%的呼叫。

	\~japanese
	\~
	*/
	template<typename XCHAR, typename StringTraits>
	class StrBufT
	{
		StringT<XCHAR,StringTraits>* m_obj;
		XCHAR* m_bufPtr;
	public:
		StrBufT(StringT<XCHAR,StringTraits>& src, int maxLength = 0)
		{
			m_obj = &src;
			m_bufPtr = src.GetBuffer(maxLength); 
		}

		~StrBufT()
		{
			m_obj->ReleaseBuffer();
		}

		operator XCHAR*()
		{
			return m_bufPtr;
		}

		XCHAR* GetBuf()
		{
			return m_bufPtr;
		}
	};

	template<typename XCHAR, typename StringTraits>
	class StringTraitsT
	{
	public:
		typedef const StringT<XCHAR,StringTraits>& INARGTYPE;
		typedef StringT<XCHAR,StringTraits>& OUTARGTYPE;

		static uint32_t __cdecl Hash( INARGTYPE str )
		{
			assert( str.GetString() != NULL );
			uint32_t nHash = 0;
			const XCHAR* pch = str;
			while( *pch != 0 )
			{
				nHash = (nHash<<5)+nHash+(*pch);
				pch++;
			}

			return( nHash );
		}

		static bool __cdecl CompareElements( INARGTYPE str1, INARGTYPE str2 )
		{
			return( StringTraits::StringCompare( str1, str2 ) == 0 );
		}

		static int __cdecl CompareElementsOrdered( INARGTYPE str1, INARGTYPE str2 )
		{
			return( StringTraits::StringCompare( str1, str2 ) );
		}
	};

	/** 
	\~korean
	ANSI 캐릭터 문자열 

	\~english
	ANSI character text string

	\~chinese
	ANSI 角色字符串。

	\~japanese
	\~
	*/
	typedef StringT<char,AnsiStrTraits> StringA;
	/** 
	\~korean
	Unicode 캐릭터 문자열 

	\~english
	Unicode character text string

	\~chinese
	Unicode 角色字符串。

	\~japanese
	\~
	*/
	typedef StringT<wchar_t,UnicodeStrTraits> StringW;
    
	/**
	\~korean
	문자열 클래스입니다.
	Windows와 Marmalade에서는 UTF-16을 지원하는 wchar_t 타입 문자열이지만 여타 플랫폼에서는 UTF-8을 지원하는 char 타입 문자열입니다.

	\~english
	It is a string class.
	The string is a wchar_t type that supports UTF-16 at Windows & Marmalade but at the other platforms, it is a char type string that supports UTF-8.

	\~chinese
	字符串 class 。
	虽然在Windows，Marmalade中是支持 UTF-16 的 wchar_t type 的字符串，但是在其他平台中是支持 UTF-8 的 char type 的字符串。

	\~japanese
	文字列クラスです。
	WindowsとMarmaladeではUTF-16を支援するwchar_tタイプ文字列ですが、他のプラットフォームではUTF-8を支援するcharタイプ文字列です。
	*/
#if defined(_UNICODE) // VC++에서만 이것이 켜져 있다. MBS빌드 옵션을 설정하면 꺼지긴 하지만 요즘 시대에 누가 그러겠는가?
	typedef StringW String;
#else
	typedef StringA String;
#endif

	/** 
	\~korean
	ANSI 캐릭터 문자열용 문자열 버퍼 조작을 위한 클래스 

	\~english
	Class for manipulating ANSI character text string buffer

	\~chinese
	为了ANSI角色字符串用字符串操纵buffer的类。

	\~japanese
	\~
	*/
	typedef StrBufT<char,AnsiStrTraits> StrBufA;

	/** 
	\~korean
	Unicode 캐릭터 문자열용 문자열 버퍼 조작을 위한 클래스 

	\~english
	Class for manipulating Unicode character text string bufferUnicode

	\~chinese
	为了Unicode角色字符串用字符串操纵buffer的类。

	\~japanese
	\~
	*/
	typedef StrBufT<wchar_t,UnicodeStrTraits> StrBufW;
    
	typedef StringTraitsT<char,AnsiStrTraits> StringTraitsA;
	typedef StringTraitsT<wchar_t,UnicodeStrTraits> StringTraitsW;

	/** 
	\~korean
	캐릭터 문자열 변환함수

	\~english
	Convert to String Function

	\~chinese
	角色字符串转换函数。

	\~japanese
	\~
	*/

	StringA StringW2A(const wchar_t *src, CStringEncoder* encoder = NULL);		
	StringW StringA2W(const char *src, CStringEncoder* encoder = NULL);

	/* Win32, Marmalade에서는 wchar_t를 애용하지만 unix에서는 char을 애용한다.
	char 문자열이 UTF-8을 처리하기 때문이다. 
	*/
#ifdef _UNICODE
	#define StringT2A StringW2A
	#define StringA2T StringA2W
	#define StringT2W(x, ...) (::Proud::String(x))
	#define StringW2T(x, ...) (::Proud::String(x))
	typedef StringTraitsW StringTraits;
	typedef StrBufW StrBuf;
#else
	#define StringT2A(x, ...) (::Proud::String(x))
	#define StringA2T(x, ...) (::Proud::String(x))
	#define StringT2W StringA2W
	#define StringW2T StringW2A
	typedef StringTraitsA StringTraits;
	typedef StrBufA StrBuf;
#endif


}

// NOTE: CPNElementTraits의 specialized template은 namespace 바깥으로 빼야.
// CFastMap등에서 세번째 인자 안 넣어도 되게 하려고.
template<>
class CPNElementTraits<Proud::StringW> :public Proud::StringTraitsW{};

template<>
class CPNElementTraits<Proud::StringA> :public Proud::StringTraitsA{};

/**  @} */

#if (_MSC_VER>=1400)
#pragma managed(pop)
#endif

//#pragma pack(pop)

#ifdef _MSC_VER 
#pragma warning(pop)
#endif // __MARMALADE__
