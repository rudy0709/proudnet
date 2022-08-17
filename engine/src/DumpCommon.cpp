/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/
#include "stdafx.h"
#include "../include/dumpcommon.h"
#include "../include/Enums.h"
#include "DumpC2S_common.cpp"
#include "DumpS2C_common.cpp"
#include "../include/pnguid.h"

namespace Proud {
    
    const PNGUID _dumpProtocolVersion = { 0xe764a875, 0x4fb, 0x441c, { 0x8b, 0x61, 0x1f, 0x61, 0x7b, 0x87, 0xc1, 0xc3 } };

	// PN server dll에서도 이걸 액세스하므로 dllexport를 한다.
	const Guid DumpProtocolVersion = Guid(_dumpProtocolVersion);
}