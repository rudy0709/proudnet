#pragma once

#include "../include/NetVariant.h"
#include "FastMap2.h"
#include "CompatibleMode.h"

namespace Proud
{
	class CMessage;
	class CompactFieldMap;

	PROUD_API void Message_Write(CMessage& msg, const CompactFieldMap &fieldMap);
	PROUD_API bool Message_Read(CMessage& msg, CompactFieldMap &fieldMap);

	// 프넷 내부에서 주고받는 RMI나 Message는 이제 하위 호환성을 갖추어야 한다.
	// Google Play, AppStore들은 잦은 앱 업데이트를 부정한다. 이 때문에 서로 다른 버전의 클라이언트가 서버에 접속하더라도 잘 되어야 한다.
	// 따라서 메시지 구조가 가급적 하위 호환성을 갖추어야 한다.
	// 따라서 모든 메시지는 필수적으로 이것을 가지며, 여기에 하위 호환성을 잘 지킬 수 있도록 key-value map을 활용하도록 하자.
	// JSON 생각하면 된다. 하위호환성에 효과적인.
	// 이 클래스는 프넷 내부에서만 쓰인다. 유저에게 비노출.
	class CompactFieldMap
	{
		friend PROUD_API void Message_Write(CMessage& msg, const CompactFieldMap &fieldMap);
		friend PROUD_API bool Message_Read(CMessage& msg, CompactFieldMap &fieldMap);

		// field name to field value map. 단, field name은 문자열 대신 enum이 쓰인다.
		typedef CFastMap2<CompactFieldName, NetVariant, int> FieldMap;
		FieldMap m_fields;
	public:
		PROUD_API int GetFieldCount() const;

		PROUD_API bool GetBoolField(CompactFieldName fieldName, bool* outValue) const;
		PROUD_API void SetField(CompactFieldName fieldName, const NetVariant& value);
		PROUD_API bool GetInt64Field(CompactFieldName fieldName, int64_t* outValue) const;
		PROUD_API bool GetInt32Field(CompactFieldName fieldName, int32_t* outValue) const;
		PROUD_API bool GetStringField(CompactFieldName fieldName, String* outValue) const;
	private:
		template<typename T>
		bool GetRawField(CompactFieldName fieldName, NetVariantType varType, T* outValue) const;
	public:
		/* #TODO string, binary 등을 다룰 때는 ctor,dtor,copy operator, move operator를 다루도록 하자.

		*/
	};
}
