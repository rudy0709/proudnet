#include "stdafx.h"
#include "../include/Message.h"
#include "CompactFieldMap.h"
#include "marshaler-private.h"

namespace Proud
{
	// fieldName이 가리키는 field의 value를 가져오되, POD type으로 썡으로 가져온다.
	// 잘 가져왔으면 true를 리턴한다.
	// 내부 전용 함수.
	template<typename T>
	bool CompactFieldMap::GetRawField(CompactFieldName fieldName, NetVariantType varType, T* outValue) const
	{
		const FieldMap::CPair *node = m_fields.Lookup(fieldName);
		if (node)
		{
			const NetVariant& value = node->m_value;
			if (value.GetType() == varType)
			{
				*outValue = value; // throwable이지만, 위 if구문으로 체크 후 들어가므로, never throw이다.
				return true;
			}
		}
		return false;
	}

	bool CompactFieldMap::GetBoolField(CompactFieldName fieldName, bool* outValue) const
	{
		return GetRawField<bool>(fieldName, NetVariantType_Bool, outValue);
	}

	void CompactFieldMap::SetField(CompactFieldName fieldType, const NetVariant& value)
	{
		m_fields.SetAt(fieldType, value);
	}

	bool CompactFieldMap::GetInt64Field(CompactFieldName fieldName, int64_t* outValue) const
	{
		return GetRawField<int64_t>(fieldName, NetVariantType_Int64, outValue);
	}

	bool CompactFieldMap::GetInt32Field(CompactFieldName fieldName, int32_t* outValue) const
	{
		return GetRawField<int32_t>(fieldName, NetVariantType_Int32, outValue);
	}

	bool CompactFieldMap::GetStringField(CompactFieldName fieldName, String* outValue) const
	{
		return GetRawField<String>(fieldName, NetVariantType_String, outValue);
	}


	// 갖고 있는 모든 필드의 갯수를 구한다.
	int CompactFieldMap::GetFieldCount() const
	{
		return m_fields.GetCount();
	}

	// msg 객체에 serialize를 한다.
	void Message_Write(CMessage& msg, const CompactFieldMap &fieldMap)
	{
		// 가지고 있는 필드들을 순서대로 저장한다.
		// 하위호환성을 위해, 알 수 없는 타입의 필드가 건너뛸 수 있도록 serialize 후
		// 데이터 크기를 넣는다.
		msg.Write((int)(fieldMap.GetFieldCount()));

		// 각 필드를 serialize한다.
		// ikpil.choi 2016.11.06 : msvc10(2010) 을 위한 코드
		//for (auto field : fieldMap.m_fields)
		//{
		//	Message_Write(msg, field.GetFirst()); // field enum value
		//	msg.WriteVariant(field.GetSecond()); // field value 자체
		//}
		for (CompactFieldMap::FieldMap::iterator it = fieldMap.m_fields.begin(); it != fieldMap.m_fields.end(); ++it)
		{
			Message_Write(msg, it.GetFirst()); // field enum value
			msg.WriteVariant(it.GetSecond()); // field value 자체
		}
	}

	// msg 객체로부터 deserialize를 한다.
	// 잘 가져왔으면 true를 리턴한다.
	bool Message_Read(CMessage& msg, CompactFieldMap &fieldMap)
	{
		// 필드 갯수 가져오기.
		int fieldCount;
		if (!msg.Read(fieldCount))
			return false;

		// 필드들을 가져온다.
		for (int i = 0; i < fieldCount; i++)
		{
			// 필드 이름
			CompactFieldName fieldName;
			if (!Message_Read(msg, fieldName))
			{
				return false;
			}
			// 필드 값 자체
			NetVariant fieldValue;
			if (!msg.ReadVariant(fieldValue))
				return false;

			// 가져온 것을 결과에 취합
			fieldMap.SetField(fieldName, fieldValue);
		}

		return true;
	}

	void AppendTextOut(String &a, const CompactFieldMap &b)
	{
		a += String::NewFormat(_PNT("FieldCount:%d"), b.GetFieldCount());
	}
}
