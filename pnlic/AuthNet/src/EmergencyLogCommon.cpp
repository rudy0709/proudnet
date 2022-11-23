/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#if defined(_WIN32)

#include "stdafx.h"
#include <new>
#include "../include/EmergencyLogCommon.h"

#include "EmergencyC2S_common.cpp"
#include "EmergencyS2C_common.cpp"

namespace Proud
{
	const PNGUID guidEmergencyProtocolVersion = { 0xe1b72178, 0xe472, 0x45a4, { 0xaa, 0x3d, 0x9f, 0xe4, 0xcd, 0xb4, 0xc, 0xa0 } };
	const Guid EmergencyProtocolVersion = Guid(guidEmergencyProtocolVersion);
}

#endif // _WIN32
