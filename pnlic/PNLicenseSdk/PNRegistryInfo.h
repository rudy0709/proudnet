#pragma once

#include <stdint.h>

class CRegInfoEx
{
public:
	uint32_t regKeyValid;	// 레지스트리 유효성
	uint32_t hardwareID;	// 하드웨어 ID (현재 사용안함)
	time_t timeModified;	//레지스트리 변경시간
};