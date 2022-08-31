#include "stdafx.h"
#include "../include/pnguid-win32.h"
#include <rpcdce.h> // UE4 비호환되므로 이것은 서버에서만 사용하도록 하자.

#if defined(_WIN32)

namespace Proud
{
	Guid Win32Guid::NewGuid()
	{
		Guid ret;
		UuidCreate((UUID*)&ret);
		return ret;
	}

	Guid Win32Guid::From(UUID src)
	{
		Guid ret;
		*(UUID*)&ret = src;
		return ret;
	}

	UUID Win32Guid::ToNative(Guid src)
	{
		UUID ret;
		*(Guid*)&ret = src;
		return ret;
	}

}

#endif // win32
