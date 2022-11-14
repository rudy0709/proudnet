#include "stdafx.h"
#include "FilterTag.h"

namespace Proud
{
	FilterTag::Type FilterTag::CreateFilterTag(HostID srcID, HostID destID)
	{
		FilterTag::Type ret = 0;
		ret |= uint8_t(srcID & 0x0f) << 4; //상위 4비트
		//ret |= BYTE(srcID & 0xff) << 8;
		ret |= uint8_t(destID & 0x0f); // 하위 4비트

		return ret;
	}

	// 받은 UDP 패킷에 들어있는 filter tag으로 맞는 src가 맞는 dest에게 보낸건지 확인.
	// srcID, destID중 하나라도 0이면 무조건 맞다고 가정함.
	// true를 리턴하면 수신한 데이터는 버려야 함.
	bool FilterTag::ShouldBeFiltered(FilterTag::Type filterTag, HostID srcID, HostID destID)
	{
		assert((uint8_t(HostID_None) & 0xff) == 0);

		// srcID,destID,filterTag의 srcID,destID가 0인 경우는 wildcard, 즉 무조건 '통과'를 의미한다.
		//BYTE b1 = (filterTag & 0xff00) >> 8;
		//상위 4비트
		uint8_t b1 = (filterTag & 0xf0) >> 4;
		//하위 4비트
		uint8_t b2 = filterTag & 0x0f;

		uint8_t c1 = srcID & 0x0f;
		uint8_t c2 = destID & 0x0f;

		if (b1 && c1 && b1 != c1)
			return true;

		if (b2 && c2 && b2 != c2)
			return true;

		return false;
	}


}