#include "stdafx.h"
#include "../include/NetVariant.h"
#include "../include/PNString.h"
#include "../include/Message.h"
#include "MessagePrivateImpl.h"

namespace Proud
{
	template<typename T>
	bool Message_ReadVariant_Raw(CMessage& msg, NetVariant& var)
	{
		// 값 본체를 읽는다.
		T v1; // 이렇게 별도 변수로 읽어들이자. 
		if (!msg.Read(v1))
			return false;

		// 읽은 별도 변수를 출력하자.
		var = MOVE_OR_COPY(NetVariant(v1));

		return true;
	}

	bool Message_ReadVariant_String(CMessage& msg, NetVariant& var)
	{
		// 값 본체를 읽는다.
		String v1; // 이렇게 별도 변수로 읽어들이자. 
		if (!msg.ReadString(v1))
			return false;

		// 읽은 별도 변수를 출력하자.
		var = MOVE_OR_COPY(NetVariant(v1.GetString()));

		return true;
	}

	bool Message_ReadVariant_Binary(CMessage& msg, NetVariant& var)
	{
		// 값 본체를 읽는다.
		ByteArray v1; // 이렇게 별도 변수로 읽어들이자. 
		if (!msg.Read(v1))
			return false;

		// 읽은 별도 변수를 출력하자.
		var = MOVE_OR_COPY(NetVariant(v1));

		return true;
	}

	// NetVariant는 암묵적 연산자 오버로딩이 되어버린다.
	// 따라서 이렇게 Read 대신 ReadVariant라는 다른 이름을 쓴다.
	bool CMessage::ReadVariant(NetVariant& var)
	{
		// 타입부터 가져온다.
		NetVariantType type;
		if (!Message_Read(*this, type))
		{
			return false;
		}

		bool indexed;
		if (!this->Read(indexed))
		{
			return false;
		}

#define READ_CASE(TYPE, TYPENAME) \
		case NetVariantType_##TYPENAME: \
			ok = Message_ReadVariant_Raw<TYPE>(*this, var); \
			break;

		// 타입 종류에 맞게 읽는 법을 달리 하자.
		bool ok = false;
		switch (type)
		{
			READ_CASE(int8_t, Int8)
				READ_CASE(uint8_t, Uint8)
				READ_CASE(int16_t, Int16)
				READ_CASE(uint16_t, Uint16)
				READ_CASE(int32_t, Int32)
				READ_CASE(uint32_t, Uint32)
				READ_CASE(int64_t, Int64)
				READ_CASE(uint64_t, Uint64)
				READ_CASE(bool, Bool)
				READ_CASE(float, Float)
				READ_CASE(double, Double)
		case NetVariantType_String:
			ok = Message_ReadVariant_String(*this, var);
			break;
		case NetVariantType_Binary:
			ok = Message_ReadVariant_Binary(*this, var);
			break;
		default:
			// 아직 지원 안하는 타입이다. 그냥 읽기 실패했다고 처리하자.
			ok = false;
			break;
		}

		// 제대로 읽혀졌는지 검사
		if (ok && type != NetVariantType_None)
		{
			assert(var.GetType() != NetVariantType_None);
		}

		var.SetIndexed(indexed);

		// 끝
		return ok;
	}

	template<typename T>
	void WriteVariant_Raw(CMessage& msg, const NetVariant& var)
	{
		// 값 본체를 기록한다.
		T* src = (T*)var.GetBuffer_Internal();
		msg.Write(*src);
	}

	void WriteVariant_RawPtr_String(CMessage& msg, const NetVariant& var)
	{
		// 값 본체를 기록한다.
		msg.WriteString(var);
	}

	void WriteVariant_RawPtr_Binary(CMessage& msg, const NetVariant& var)
	{
		// 값 본체를 기록한다.
		msg.Write(*var.GetBinaryPtr());
	}

	void CMessage::WriteVariant(const NetVariant& var)
	{
		// 타입부터 serialize.
		Message_Write(*this, var.GetType());
		this->Write(var.GetIndexed());

#define WRITE_CASE(TYPE, TYPENAME) \
		case NetVariantType_##TYPENAME: \
			WriteVariant_Raw<TYPE>(*this, var); \
			break;

		// 타입 별 값을 serialize.
		switch (var.GetType())
		{
			WRITE_CASE(int8_t, Int8)
			WRITE_CASE(uint8_t, Uint8)
			WRITE_CASE(int16_t, Int16)
			WRITE_CASE(uint16_t, Uint16)
			WRITE_CASE(int32_t, Int32)
			WRITE_CASE(uint32_t, Uint32)
			WRITE_CASE(int64_t, Int64)
			WRITE_CASE(uint64_t, Uint64)
			WRITE_CASE(bool, Bool)
			WRITE_CASE(float, Float)
			WRITE_CASE(double, Double)
		case NetVariantType_String:
			WriteVariant_RawPtr_String(*this, var);
			break;
		case NetVariantType_Binary:
			WriteVariant_RawPtr_Binary(*this, var);
			break;
		}
	}

	void ThrowInvalidTypeException(NetVariantType type1, NetVariantType type2)
	{
		throw Exception(StringA::NewFormat("Invalid NetVariant Type! expected:%d, real:%d", type1, type2).GetString());
	}

	NetVariant::NetVariant(const PNTCHAR* value)
	{
		m_type = NetVariantType_String;
		m_string = value;
	}

#if defined(_PNUNICODE) || defined(SWIG)
	NetVariant::NetVariant(const char* value)
	{
		m_type = NetVariantType_String;
		m_string = StringA2T(value);
	}
#endif

	NetVariant::NetVariant(const ByteArray& value)
	{
		m_type = NetVariantType_Binary;
		m_binary = value;
	}

	NetVariant::operator String() const
	{
		return m_string;
	}

	NetVariant::operator ByteArray() const
	{
		return m_binary;
	}

	void NetVariant::WriteBinary(const uint8_t* data, int length)
	{
		GetBinaryPtr()->CopyFrom(data, length);
	}

	int NetVariant::GetBinaryLength()
	{
		return (int)GetBinaryPtr()->GetCount();
	}

	int NetVariant::ReadBinary(uint8_t* data, int length)
	{
		int ret = (length > GetBinaryPtr()->GetCount()) ? GetBinaryPtr()->GetCount() : length;

		memcpy_s(data, ret, GetBinaryPtr()->GetData(), ret);

		return ret;
	}

	int NetVariant::CompBinary(const NetVariant& a) const
	{
		int aLength = ((ByteArray)a).GetCount();
		int compLength = (aLength > GetBinaryPtr()->GetCount()) ? GetBinaryPtr()->GetCount() : aLength;

		int compResult = memcmp(GetBinaryPtr()->GetData(), ((ByteArray)a).GetData(), compLength);

		// 메모리가 같으면 메모리의 크기를 다시 비교한다.
		if (compResult == 0)
		{
			if (((ByteArray)a).GetCount() > aLength)
				return 1;
			else if (((ByteArray)a).GetCount() < aLength)
				return -1;
			else
				return 0;
		}

		return compResult;
	}

	void NetVariant::Reset()
	{
		m_type = NetVariantType_None;
	}


	/* 복사 생성자를 별도로 두는 이유는, 컴파일러가 이런 코드를 제대로 컴파일 못할 수 있기 때문이다.
	NetVariant a = "xxx";
	NetVariant b = a; // b,a간 복사는, 무슨 내부 타입으로 복사를 해야 할까? */
	NetVariant::NetVariant(const NetVariant& rhs)
	{
		m_type = rhs.m_type;
		switch (m_type)
		{
		case NetVariantType_String:
			m_string = rhs.m_string;
			break;
		case NetVariantType_Binary:
			rhs.m_binary.CopyTo(m_binary);
			break;
		default:
			// 복사
			memcpy(m_buffer, rhs.m_buffer, sizeof(m_buffer));
			break;
		}
	}

}