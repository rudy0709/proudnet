#pragma once 

#include "pnguid.h"

#if defined(_WIN32) 

// 주의: UE4 project에서는 include하지 말 것! 빌드 오류난다.
#pragma comment(lib,"rpcrt4.lib")
#pragma comment(lib,"winmm.lib")


namespace Proud
{
	/** helper functions for win32 GUID to Proud.Guid. */
	class Win32Guid
	{
	public:
		/** creates a new unique ID. */
		PROUD_API static Guid NewGuid();

		/** gets data from Win32 UUID data. */
		static Guid From(UUID src);

		/** outputs Win32 UUID data. */
		static UUID ToNative(Guid src);
	};
}

#endif // _WIN32