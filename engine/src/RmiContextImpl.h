#pragma once

#include "../include/RMIContext.h"

namespace Proud
{
	PROUD_API extern RmiContext g_ReliableSendForPN, g_UnreliableSendForPN;
	PROUD_API extern RmiContext g_SecureReliableSendForPN,g_SecureUnreliableSendForPN;
}
