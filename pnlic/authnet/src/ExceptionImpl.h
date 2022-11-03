#pragma once 

#ifdef _WINDOWS_
#include <comdef.h>

#include "../include/Exception.h"

namespace Proud
{
	void Exception_UpdateFromComError(Exception& e, _com_error& src);
}
#endif
