#include "stdafx.h"

#include <sql.h>
#include <sqltypes.h>
#include <stdio.h>

#include "OdbcErrorCheck.h"
#include "OdbcHandle.h"
#include "../include/OdbcException.h"
#include "../include/PNString.h"

namespace Proud
{
	void CheckError(SQLRETURN ret, COdbcHandle* handle, COdbcWarnings* odbcWarnings /* = NULL */)
	{
		SQLCHAR stateCode[6] = { 0, };
		SQLINTEGER errNo;
		SQLSMALLINT errMsgLen;
		SQLCHAR errMsg[SQL_MAX_MESSAGE_LENGTH + 1];
		SQLRETURN result;
		int loop = 1;

		switch (ret)
		{
			case SQL_SUCCESS:
				break;
			case SQL_SUCCESS_WITH_INFO:
				if (odbcWarnings != NULL)
					odbcWarnings->Clear();

				while (true)
				{
					result = SQLGetDiagRec(handle->GetHandleType(), handle->GetHandle(), loop++, stateCode, &errNo, errMsg, SQL_MAX_MESSAGE_LENGTH * sizeof(SQLCHAR), &errMsgLen);
					if (result != SQL_SUCCESS)
						break;

					if (odbcWarnings == NULL)
						throw COdbcWarning(ret, StringA::NewFormat("%s [%d:%s]", stateCode, errNo, errMsg));
					else
						odbcWarnings->Add(COdbcWarning(ret, StringA::NewFormat("%s [%d:%s]", stateCode, errNo, errMsg)));
				}
				break;
			default:
				SQLGetDiagRec(handle->GetHandleType(), handle->GetHandle(), 1, stateCode, &errNo, errMsg, SQL_MAX_MESSAGE_LENGTH * sizeof(SQLCHAR), &errMsgLen);
				throw COdbcException(ret, StringA::NewFormat("%s [%d:%s]", stateCode, errNo, errMsg));
		}
	}
}
