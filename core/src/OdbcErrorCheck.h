#pragma once

#include <sqltypes.h>

#include "../include/OdbcException.h"

namespace Proud
{
	class COdbcHandle;
	void CheckError(SQLRETURN ret, COdbcHandle* handle, COdbcWarnings* odbcWarnings = NULL);
}
