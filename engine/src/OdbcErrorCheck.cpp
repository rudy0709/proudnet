#include "stdafx.h"

#include <sql.h>
#include <sqltypes.h>
#include <stdio.h>
#include <sqlucode.h>


#include "OdbcErrorCheck.h"
#include "OdbcHandle.h"
#include "../include/OdbcException.h"
#include "../include/PNString.h"
#include "../include/sysutil.h"

namespace Proud
{
	void CheckError(SQLRETURN ret, COdbcHandle* handle, COdbcWarnings* odbcWarnings /* = NULL */)
	{
		SQLTCHAR stateCode[1000] = { 0, };
		SQLTCHAR errMsg[SQL_MAX_MESSAGE_LENGTH + 1];

		// ikpil.choi 2016-12-28 : 컴파일러가 너무 많으므로, 바이트 사이즈가 다를 경우, 아예 컴파일 안되고 다시 살펴봐야 한다.
		static_assert(sizeof(SQLTCHAR) == sizeof(PNTCHAR), "");

		PNTCHAR *pStateCode = reinterpret_cast<PNTCHAR*>(stateCode);
		PNTCHAR *pErrMSG = reinterpret_cast<PNTCHAR*>(errMsg);

		SQLINTEGER errNo;
		SQLSMALLINT errMsgLen;

		SQLRETURN result;
		int loop = 1;

		switch (ret)
		{
		case SQL_SUCCESS:
			break;

		case SQL_SUCCESS_WITH_INFO: // 처리는 했으나 경고가 좀 있다.
			if (odbcWarnings != NULL)
				odbcWarnings->Clear();

			while (true)
			{
				result = SQLGetDiagRec((SQLSMALLINT)handle->GetHandleType(), handle->GetHandle(), (SQLSMALLINT)(loop++), stateCode, &errNo, errMsg, SQL_MAX_MESSAGE_LENGTH, &errMsgLen);
				if (result != SQL_SUCCESS)
					break; // 경고가 정작 없으므로 무시

				if (odbcWarnings == NULL)
				{
					// 사용자가 경고를 받을 준비가 안 되어 있다. 그냥 경고만 콘솔 출력하고 만다.
					NTTNTRACE(StringA::NewFormat("%s [%d:%s]\n", StringT2A(pStateCode).GetString(), errNo, StringT2A(pErrMSG).GetString()).c_str());
				}
				else
				{
					// 경고 내용을 사용자에게 출력
					odbcWarnings->Add(COdbcWarning(ret, StringA::NewFormat("%s [%d:%s]", StringT2A(pStateCode).GetString(), errNo, StringT2A(pErrMSG).GetString())));
				}
			}
			break;
		default:
			SQLGetDiagRec((SQLSMALLINT)handle->GetHandleType(), handle->GetHandle(), 1, stateCode, &errNo, errMsg, SQL_MAX_MESSAGE_LENGTH, &errMsgLen);

			throw COdbcWarning(ret, StringA::NewFormat("%s [%d:%s]", StringT2A(pStateCode).GetString(), errNo, StringT2A(pErrMSG).GetString()));
			break;
		}
	}
}
