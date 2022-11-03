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

#include <sqltypes.h>
#include <sys/types.h>

#include "OdbcByteData.h"
#include "OdbcEnum.h"
#include "PNString.h"
#include "pnstdint.h"

namespace Proud
{
	// *********************** WARNING ***********************
	// 이 클래스는 프라우드넷 ODBC API 내부에서 사용되는 클래스입니다.
	// 절대 이 클래스를 이용하여 코딩하지 마시기 바랍니다.
	// This class is used internally by ProudNet ODBC API.
	// Please never use this class.
	// *******************************************************

	class COdbcVariant
	{
	private:
		OdbcDataType m_type;

		// 변수 데이터를 내장할 경우 여기에 내용물이 저장된다.
		union OdbcUnionValue
		{
			// PDT type들
			bool m_boolVal;
			TCHAR m_tcharVal;
			int8_t m_int8Val;
			int16_t m_int16Val;
			int32_t m_int32Val;
			int64_t m_int64Val;
			uint8_t m_uint8Val;
			uint16_t m_uint16Val;
			uint32_t m_uint32Val;
			uint64_t m_uint64Val;
			float m_floatVal;
			double m_doubleVal;

			// non-PDT type들
			String* m_stringPtr;
			COdbcByteData* m_binaryPtr;
			TIMESTAMP_STRUCT* m_timePtr;
		} m_val;

		// 데이터의 주소값.
		void* m_ptr;

		// 사용자가 갖고 있는 변수를 참조하는 데이터이면 true, 위 내장된 데이터를 가리키면 false
		bool m_isRef;

	public:
		COdbcVariant();
		COdbcVariant(bool from);
		COdbcVariant(bool* from);
		COdbcVariant(TCHAR from);
		COdbcVariant(int8_t from);
		COdbcVariant(int8_t* from);
		COdbcVariant(int16_t from);
		COdbcVariant(int16_t* from);
		COdbcVariant(int32_t from);
		COdbcVariant(int32_t* from);
		COdbcVariant(int64_t from);
		COdbcVariant(int64_t* from);
		COdbcVariant(uint8_t from);
		COdbcVariant(uint16_t from);
		COdbcVariant(uint16_t* from);
		COdbcVariant(uint32_t from);
		COdbcVariant(uint32_t* from);
		COdbcVariant(uint64_t from);
		COdbcVariant(uint64_t* from);
		COdbcVariant(float from);
		COdbcVariant(float* from);
		COdbcVariant(double from);
		COdbcVariant(double* from);
		COdbcVariant(const TCHAR* from);
		COdbcVariant(const String& from);
		COdbcVariant(String* from);
		COdbcVariant(COdbcByteData* from);
		COdbcVariant(const TIMESTAMP_STRUCT& from);
		COdbcVariant(TIMESTAMP_STRUCT* from);
		~COdbcVariant();

		operator bool() const;
		operator TCHAR() const;
		operator int8_t() const;
		operator int16_t() const;
		operator int32_t() const;
		operator int64_t() const;
		operator uint8_t() const;
		operator uint16_t() const;
		operator uint32_t() const;
		operator uint64_t() const;
		operator float() const;
		operator double() const;
		operator String() const;
		operator COdbcByteData*() const;
		operator TIMESTAMP_STRUCT() const;

		COdbcVariant(const COdbcVariant& other);
		COdbcVariant& operator=(const COdbcVariant& other);

		void Initialize();
		SQLSMALLINT GetType() const;
		void* GetPtr() const;
		bool IsRef() const;
		void ChangeRefToData();

	private:
		void ThrowIfNull() const;
		void Clean();
		void Copy(const COdbcVariant &other);

		// unsigned char*는 BYTE로 판단
		COdbcVariant(const unsigned char* from);
	};

}
