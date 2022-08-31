#pragma once

namespace Proud
{
	// CompactFieldMap에서 사용되는, 필드의 이름.
	// 내부 메시지이고 네트워크 통신량도 고려해야 한다. 따라서 CompactFieldMap은 string name이 직접 쓰이지 않고, enum을 쓰자.
	// 주의: 이 enum은 하위호환성을 위해 한번 정의하면 함부로 옮기지 말자.
	enum CompactFieldName
	{
		// 보내는 쪽에서의 로컬 시간. value는 int64이다.
		FieldType_LocalTime,
		// P2P피어의 Platform, value는 int이다
		FieldType_RuntimePlatform,
		// 서버의 외부주소 즉 public 주소
		FieldType_ServerAddrAtClient,
	};
}
