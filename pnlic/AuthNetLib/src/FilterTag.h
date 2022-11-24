#pragma once

#include "../include/pnstdint.h"
#include "../include/Enums.h"

namespace Proud
{
	/* UDP 데이터의 맨 앞단에 붙이는, 송신자 및 수신자의 식별자 HostID를 해싱해서 넣는 작은 데이터.
	Q: 왜 이게 있나요?
	A: A모사의 다단계 인터넷 공유기의 버그로 인해 엉뚱한 내부 주소로 포트 매핑이 되는 경우가 있었습니다.
	이를 해결하려면, 엉뚱한 포트 매핑으로 엄한 단말기가 수신할 경우, 그 단말기는 "이거 나한테 보내진 것 아닌데?"를 판단하여
	그것을 폐기해야 합니다. 그것을 위해 사용됩니다. */
	class FilterTag
	{
	public:
		typedef uint8_t Type; // sender, recver HostID의 하위 4비트들이 조합됨

		static FilterTag::Type CreateFilterTag(HostID srcID, HostID destID);
		static bool ShouldBeFiltered(FilterTag::Type filterTag, HostID srcID, HostID destID);
	};
}
