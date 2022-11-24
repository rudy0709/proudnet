/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#if defined(_WIN32)

#include "stdafx.h"
#include "DbConfig.h"

namespace Proud
{
	PNGUID guidDbConfigProtocolVersion = { 0x402ca5ed, 0x889, 0x4bbd, { 0xba, 0xc0, 0x9d, 0xdb, 0x92, 0xec, 0xd5, 0x7e } };
	Guid CDbConfig::ProtocolVersion = Guid(guidDbConfigProtocolVersion);
	
	// 지나치게 많으면 되레 비의도된 DB lock 발생이 우려됨.

	//modify by rekfkno1 - DBServerCacheServer 이 쓰레드 세이프 하지않아..일단 1개로 세팅.
	int CDbConfig::ServerThreadCount = 10;

	int64_t CDbConfig::DefaultUnloadAfterHibernateIntervalMs=60*5*1000;
	int64_t CDbConfig::DefaultDbmsWriteIntervalMs=60*2*1000;
	int64_t CDbConfig::FrameMoveIntervalMs=50;
	int64_t CDbConfig::DefaultUnloadRequestTimeoutTimeMs = 10000;

	// 기존에는 4MB였으나 
	// 복수데이터 로드 기능이 추가되면서 메시지 크기가 커질 수 있으므로 늘림.
	int CDbConfig::MesssageMaxLength = 1024 * 1024 * 1024;	// 1 GB
}

#endif // _WIN32
