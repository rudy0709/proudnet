#include "stdafx.h"
// STL header file은 xcode에서는 cpp에서만 include 가능하므로
#include <stack>
#include "NetUtil.h"
namespace Proud
{
	// 여러 모듈에서 공통으로 쓰이는 로직이므로 한 함수로 단일화됨.
	// autoCoalesceInterval  true이면 RTT기반 coal interval이 선택될 것임.
	// recentPingMs 로컬-리모트 RTT
	// manualInterval 사용자가 수동으로 coal interval을 설정한 값
	int NetUtil::SetManualOrAutoCoalesceInterval(bool autoCoalesceInterval, int recentPingMs, int manualInterval)
	{
		if (autoCoalesceInterval)
		{
			// NOTE: 위 값은, 그동안의 네트워크 품질 경험으로 설정해본 것임. RTT와 coalesce interval을 동일하게 맞추면 별꼴을 다 당할 수 있음
			if (recentPingMs < 10)
				return 0;
			else if (recentPingMs < 100)
				return 30;
			else if (recentPingMs < 300)
				return 150;
			else
				return 300;
		}
		else
		{
			return manualInterval;
		}
	}
}