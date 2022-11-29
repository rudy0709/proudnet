#include "stdafx.h"
#include "Vars.h"
#include "../../../version/PNVersion.h"

PNGUID uuid = { 0xabf9ab5, 0xd8b6, 0x4af4, { 0xae, 0x55, 0x38, 0x12, 0x21, 0x9b, 0x34, 0x6c } };

//DNS 연결이 막혀있는 외국 게임서버에서 인증키 설치가 안되기 때문에 도메인으로 접속 시도 후에 실패하면 ip로 재접속 시도

String g_serverIp = _PNT("52.78.194.36");  //2022.11.10 : _PNT("52.78.40.135");
String g_serverHostName = _PNT("auth2.nettention.com");
int g_port = 80;
Guid g_protocolVersion = Guid(uuid);
const char* g_ProudNetVersion = Proud::g_versionText/*"HERE_SHALL_BE_EDITED_BY_BUILD_HELPER"*/;
