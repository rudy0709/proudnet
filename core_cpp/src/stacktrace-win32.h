// 본 소스 파일의 저작권은 http://stackoverflow.com/questions/5693192/win32-backtrace-from-c-code 에서 이 소스를 답변에 첨부한 사람에게 있습니다.

#pragma once

#include "../include/PNString.h"
#include "../include/FastArray.h"

namespace Proud
{
	void GetStackTrace_Win32(CFastArray<String>& output);
}