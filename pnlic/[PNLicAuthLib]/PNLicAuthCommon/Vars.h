#pragma once

//DNS 연결이 막혀있는 외국 게임서버에서 인증키 설치가 안되기 때문에 도메인으로 접속 시도 후에 실패하면 ip로 재접속 시도
//static String g_serverIp = L"175.125.93.117";


#include "../../[AuthNetLib]/include/ProudNetCommon.h"

using namespace std;
using namespace Proud;

extern Proud::String g_serverIp;
extern Proud::String g_serverHostName;
extern int g_port;
extern Proud::Guid g_protocolVersion;
//extern const char* g_ProudNetVersion;