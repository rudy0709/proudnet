#pragma once

#if defined(_WIN32)

#include "../include/ProudNetServer.h"

namespace Proud
{
	class CDbConfig
	{
	public:
		static Guid ProtocolVersion;
		static int ServerThreadCount;
		static int64_t DefaultUnloadAfterHibernateIntervalMs;
		static int64_t DefaultDbmsWriteIntervalMs;
		static int64_t FrameMoveIntervalMs;
		static int64_t DefaultUnloadRequestTimeoutTimeMs;
		static int MesssageMaxLength;
	};

	const int64_t OneSecond = 1000;

}

#endif // _WIN32
