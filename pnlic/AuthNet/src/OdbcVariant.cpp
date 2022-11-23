#include "stdafx.h"

#include <string.h>

#include "../include/OdbcVariant.h"
#include "../include/OdbcException.h"

namespace Proud
{
	COdbcVariant::COdbcVariant()
	{
		m_type = OdbcDataType_NULL;
		m_ptr = NULL;
		m_isRef = false;
	}

	COdbcVariant::COdbcVariant(bool from)
	{
		m_type = OdbcDataType_BOOL;
		m_val.m_boolVal = from;
		m_ptr = &m_val;
		m_isRef = false;
	}

	COdbcVariant::COdbcVariant(bool* from)
	{
		if (from == NULL)
			throw COdbcException(0, "[COdbcVariant::COdbcVariant] value is NULL.");
		m_type = OdbcDataType_BOOL;
		m_ptr = (void*) from;
		m_isRef = true;
	}

	COdbcVariant::COdbcVariant(PNTCHAR from)
	{
		m_type = OdbcDataType_TCHAR;
		m_val.m_tcharVal = from;
		m_ptr = &m_val;
		m_isRef = false;
	}

	COdbcVariant::COdbcVariant(int8_t from)
	{
		m_type = OdbcDataType_SINT8;
		m_val.m_int8Val = from;
		m_ptr = &m_val;
		m_isRef = false;
	}

	COdbcVariant::COdbcVariant(int8_t* from)
	{
		if (from == NULL)
			throw COdbcException(0, "[COdbcVariant::COdbcVariant] value is NULL.");
		m_type = OdbcDataType_SINT8;
		m_ptr = (void*) from;
		m_isRef = true;
	}

	COdbcVariant::COdbcVariant(int16_t from)
	{
		m_type = OdbcDataType_SINT16;
		m_val.m_int16Val = from;
		m_ptr = &m_val;
		m_isRef = false;
	}

	COdbcVariant::COdbcVariant(int16_t* from)
	{
		if (from == NULL)
			throw COdbcException(0, "[COdbcVariant::COdbcVariant] value is NULL.");
		m_type = OdbcDataType_SINT16;
		m_ptr = (void*) from;
		m_isRef = true;
	}

	COdbcVariant::COdbcVariant(int32_t from)
	{
		m_type = OdbcDataType_SINT32;
		m_val.m_int32Val = from;
		m_ptr = &m_val;
		m_isRef = false;
	}

	COdbcVariant::COdbcVariant(int32_t* from)
	{
		if (from == NULL)
			throw COdbcException(0, "[COdbcVariant::COdbcVariant] value is NULL.");
		m_type = OdbcDataType_SINT32;
		m_ptr = (void*) from;
		m_isRef = true;
	}

	COdbcVariant::COdbcVariant(int64_t from)
	{
		m_type = OdbcDataType_SINT64;
		m_val.m_int64Val = from;
		m_ptr = &m_val;
		m_isRef = false;
	}

	COdbcVariant::COdbcVariant(int64_t* from)
	{
		if (from == NULL)
			throw COdbcException(0, "[COdbcVariant::COdbcVariant] value is NULL.");
		m_type = OdbcDataType_SINT64;
		m_ptr = (void*) from;
		m_isRef = true;
	}

	COdbcVariant::COdbcVariant(uint8_t from)
	{
		m_type = OdbcDataType_UINT8;
		m_val.m_uint8Val = from;
		m_ptr = &m_val;
		m_isRef = false;
	}

	COdbcVariant::COdbcVariant(uint16_t from)
	{
		m_type = OdbcDataType_UINT16;
		m_val.m_uint16Val = from;
		m_ptr = &m_val;
		m_isRef = false;
	}

	COdbcVariant::COdbcVariant(uint16_t* from)
	{
		if (from == NULL)
			throw COdbcException(0, "[COdbcVariant::COdbcVariant] value is NULL.");
		m_type = OdbcDataType_UINT16;
		m_ptr = (void*) from;
		m_isRef = true;
	}

	COdbcVariant::COdbcVariant(uint32_t from)
	{
		m_type = OdbcDataType_UINT32;
		m_val.m_uint32Val = from;
		m_ptr = &m_val;
		m_isRef = false;
	}

	COdbcVariant::COdbcVariant(uint32_t* from)
	{
		if (from == NULL)
			throw COdbcException(0, "[COdbcVariant::COdbcVariant] value is NULL.");
		m_type = OdbcDataType_UINT32;
		m_ptr = (void*) from;
		m_isRef = true;
	}

	COdbcVariant::COdbcVariant(uint64_t from)
	{
		m_type = OdbcDataType_UINT64;
		m_val.m_uint64Val = from;
		m_ptr = &m_val;
		m_isRef = false;
	}

	COdbcVariant::COdbcVariant(uint64_t* from)
	{
		if (from == NULL)
			throw COdbcException(0, "[COdbcVariant::COdbcVariant] value is NULL.");
		m_type = OdbcDataType_UINT64;
		m_ptr = (void*) from;
		m_isRef = true;
	}

	COdbcVariant::COdbcVariant(float from)
	{
		m_type = OdbcDataType_FLOAT;
		m_val.m_floatVal = from;
		m_ptr = &m_val;
		m_isRef = false;
	}

	COdbcVariant::COdbcVariant(float* from)
	{
		if (from == NULL)
			throw COdbcException(0, "[COdbcVariant::COdbcVariant] value is NULL.");
		m_type = OdbcDataType_FLOAT;
		m_ptr = (void*) from;
		m_isRef = true;
	}

	COdbcVariant::COdbcVariant(double from)
	{
		m_type = OdbcDataType_DOUBLE;
		m_val.m_doubleVal = from;
		m_ptr = &m_val;
		m_isRef = false;
	}

	COdbcVariant::COdbcVariant(double* from)
	{
		if (from == NULL)
			throw COdbcException(0, "[COdbcVariant::COdbcVariant] value is NULL.");
		m_type = OdbcDataType_DOUBLE;
		m_ptr = (void*) from;
		m_isRef = true;
	}

	COdbcVariant::COdbcVariant(const PNTCHAR* from)
	{
		if (from == NULL)
			throw COdbcException(0, "[COdbcVariant::COdbcVariant] value is NULL.");
		m_type = OdbcDataType_STRING;
		m_val.m_stringPtr = new String();
		*m_val.m_stringPtr = from;
		m_ptr = m_val.m_stringPtr;
		m_isRef = false;
	}

	COdbcVariant::COdbcVariant(const String& from)
	{
		m_type = OdbcDataType_STRING;
		m_val.m_stringPtr = new String();
		*m_val.m_stringPtr = from;
		m_ptr = m_val.m_stringPtr;
		m_isRef = false;
	}

	COdbcVariant::COdbcVariant(String* from)
	{
		if (from == NULL)
			throw COdbcException(0, "[COdbcVariant::COdbcVariant] value is NULL.");
		m_type = OdbcDataType_STRING;
		m_ptr = (void*) from;
		m_isRef = true;
	}

	COdbcVariant::COdbcVariant(COdbcByteData* from)
	{
		if (from == NULL)
			throw COdbcException(0, "[COdbcVariant::COdbcVariant] value is NULL.");
		m_type = OdbcDataType_BYTE;
		m_ptr = (void*) from;
		m_isRef = true;
	}

	COdbcVariant::COdbcVariant(const TIMESTAMP_STRUCT& from)
	{
		m_type = OdbcDataType_TIMESTAMP;
		m_val.m_timePtr = new TIMESTAMP_STRUCT();
		*m_val.m_timePtr = from;
		m_ptr = m_val.m_timePtr;
		m_isRef = false;
	}

	COdbcVariant::COdbcVariant(TIMESTAMP_STRUCT* from)
	{
		if (from == NULL)
			throw COdbcException(0, "[COdbcVariant::COdbcVariant] value is NULL.");
		m_type = OdbcDataType_TIMESTAMP;
		m_ptr = (void*) from;
		m_isRef = true;
	}

	COdbcVariant::~COdbcVariant()
	{
		Clean();
	}

	COdbcVariant::operator bool() const
	{
		ThrowIfNull();
		return *((bool*) GetPtr());
	}
	COdbcVariant::operator PNTCHAR() const
	{
		ThrowIfNull();
		return *((PNTCHAR*) GetPtr());
	}
	COdbcVariant::operator int8_t() const
	{
		ThrowIfNull();
		return *((int8_t*) GetPtr());
	}
	COdbcVariant::operator int16_t() const
	{
		ThrowIfNull();
		return *((int16_t*) GetPtr());
	}
	COdbcVariant::operator int32_t() const
	{
		ThrowIfNull();
		return *((int32_t*) GetPtr());
	}
	COdbcVariant::operator int64_t() const
	{
		ThrowIfNull();
		return *((int64_t*) GetPtr());
	}
	COdbcVariant::operator uint8_t() const
	{
		ThrowIfNull();
		return *((uint8_t*) GetPtr());
	}
	COdbcVariant::operator uint16_t() const
	{
		ThrowIfNull();
		return *((uint16_t*) GetPtr());
	}
	COdbcVariant::operator uint32_t() const
	{
		ThrowIfNull();
		return *((uint32_t*) GetPtr());
	}
	COdbcVariant::operator uint64_t() const
	{
		ThrowIfNull();
		return *((uint64_t*) GetPtr());
	}
	COdbcVariant::operator float() const
	{
		ThrowIfNull();
		return *((float*) GetPtr());
	}
	COdbcVariant::operator double() const
	{
		ThrowIfNull();
		return *((double*) GetPtr());
	}
	COdbcVariant::operator String() const
	{
		ThrowIfNull();
		return *(String*) GetPtr();
	}
	COdbcVariant::operator COdbcByteData*() const
	{
		ThrowIfNull();
		return (COdbcByteData*) GetPtr();
	}
	COdbcVariant::operator TIMESTAMP_STRUCT() const
	{
		ThrowIfNull();
		return *((TIMESTAMP_STRUCT*) GetPtr());
	}

	COdbcVariant::COdbcVariant(const COdbcVariant& other)
	{
		Copy(other);
	}

	COdbcVariant& COdbcVariant::operator=(const COdbcVariant& other)
	{
		Copy(other);

		return *this;
	}

	// 갖고 있는 데이터를 초기화하되 데이터 형식은 냅둔다.
	void COdbcVariant::Initialize()
	{
		if (IsRef() == false)
		{
			switch (m_type)
			{
				case OdbcDataType_STRING:
					*m_val.m_stringPtr = _PNT("");
					break;
				case OdbcDataType_BYTE:
					memset(m_val.m_binaryPtr->GetDataPtr(), 0, m_val.m_binaryPtr->GetMaxLength());
					break;
				case OdbcDataType_TIMESTAMP:
					memset(m_val.m_timePtr, 0, sizeof(TIMESTAMP_STRUCT));
					break;
				default:
					memset(&m_val, 0, sizeof(m_val));
					break;
			}
		}
	}

	SQLSMALLINT COdbcVariant::GetType() const
	{
		return m_type;
	}

	// COdbcVariant가 내장 혹은 외부참조중인 변수 데이터를 리턴한다.
	// PDT의 경우 데이터 자체를 가리키며 String 등 non PDT인 경우 해당 클래스 인스턴스 자체의 주소를 가리킨다.
	void* COdbcVariant::GetPtr() const
	{
		return m_ptr;
	}

	// 이 variant가 가진 값 데이터가, 사용자가 직접 만든 것을 참조하는 경우 true, 내장이면 false
	bool COdbcVariant::IsRef() const
	{
		return m_isRef;
	}

	// 외부 참조로 입력된 객체를 내부 객체로 바꾸는 함수
	// COdbcVariant 소멸시 객체 파괴를 위해 사용
	void COdbcVariant::ChangeRefToData()
	{
		switch (m_type)
		{
			case OdbcDataType_STRING:
				m_val.m_stringPtr = (String*) m_ptr;
				m_isRef = false;
				break;
			case OdbcDataType_BYTE:
				m_val.m_binaryPtr = (COdbcByteData*) m_ptr;
				m_isRef = false;
				break;
			case OdbcDataType_TIMESTAMP:
				m_val.m_timePtr = (TIMESTAMP_STRUCT*) m_ptr;
				m_isRef = false;
				break;
			default:
				throw COdbcException(0, StringA::NewFormat("[%s] m_type not match.", __FUNCTION__));
		}
	}

	void COdbcVariant::ThrowIfNull() const
	{
		if (GetType() == OdbcDataType_NULL)
			throw COdbcException(0, "[COdbcVariant::ThrowIfNull] COdbcVariant is NULL.");
	}

	// COdbcVariant 객체를 초기화 합니다.
	void COdbcVariant::Clean()
	{
		if (IsRef() == false)
		{
			switch (m_type)
			{
				case OdbcDataType_STRING:
					delete m_val.m_stringPtr;
					break;
				case OdbcDataType_BYTE:
					delete[] m_val.m_binaryPtr->GetDataPtr();
					delete m_val.m_binaryPtr;
					break;
				case OdbcDataType_TIMESTAMP:
					delete m_val.m_timePtr;
					break;
				default:
					break;
			}

			m_type = OdbcDataType_NULL;
			m_ptr = NULL;
			m_isRef = false;
		}
	}

	void COdbcVariant::Copy(const COdbcVariant &other)
	{
		Clean();

		m_type = other.m_type;
		m_isRef = other.IsRef();

		if (m_isRef == true)
		{
			m_ptr = other.m_ptr;
		}
		else
		{
			switch (m_type)
			{
				case OdbcDataType_STRING:
					m_val.m_stringPtr = new String();
					*m_val.m_stringPtr = *other.m_val.m_stringPtr;
					m_ptr = m_val.m_stringPtr;
					break;
				case OdbcDataType_BYTE:
					{
						SQLCHAR* byte = new SQLCHAR[m_val.m_binaryPtr->GetMaxLength()]();
						memcpy(byte, m_val.m_binaryPtr->GetDataPtr(), m_val.m_binaryPtr->GetDataLength());
						m_val.m_binaryPtr = new COdbcByteData(byte, other.m_val.m_binaryPtr->GetDataLength(), other.m_val.m_binaryPtr->GetMaxLength());
					}
					m_ptr = m_val.m_binaryPtr;
					break;
				case OdbcDataType_TIMESTAMP:
					m_val.m_timePtr = new TIMESTAMP_STRUCT();
					*m_val.m_timePtr = *other.m_val.m_timePtr;
					m_ptr = m_val.m_timePtr;
					break;
				default:
					m_val = other.m_val;
					m_ptr = &m_val;
					break;
			}
		}
	}
}
