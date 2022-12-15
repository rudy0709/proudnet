#include "stdafx.h"

#include <stdio.h>
#include <sql.h>
#include <sqlucode.h>

#include "OdbcCommandImpl.h"
#include "OdbcErrorCheck.h"
#include "OdbcHandle.h"
#include "../include/OdbcByteData.h"
#include "../include/OdbcConnection.h"
#include "../include/OdbcRecordset.h"

namespace Proud
{
	COdbcCommandImpl::COdbcCommandImpl()
	{
		m_conn = NULL;
		m_type = OdbcPrepareType_QUERY;
		m_hstmt = new COdbcHandle(OdbcHANDLETYPE_STMT);
		m_paramsMaxCount = 0;
		m_firstExecute = true;
	}

	COdbcCommandImpl::~COdbcCommandImpl()
	{
		m_conn = NULL;

		if (m_hstmt)
		{
			delete m_hstmt;
			m_hstmt = NULL;
		}

		m_params.clear();
	}

	// stored procedure이면 true를 리턴, 일반 쿼리문이면 false.
	bool IsStoredProcedure(const PNTCHAR* str)
	{
		if (str != NULL)
		{
			while (*str)
			{
				if (*str == '{')
					return true;
				else if (*str != ' ')
					return false;

				str++;
			}
		}

		return false;
	}

	// 특정 문자의 갯수를 구한다.
	int GetSpecifiedCharacterCount(const PNTCHAR* str, const PNTCHAR ch)
	{
		int count = 0;

		if (str != NULL)
		{
			while (*str)
			{
				if (*str == ch)
					count++;

				str++;
			}
		}

		return count;
	}

	void COdbcCommandImpl::Prepare(COdbcConnection &conn, const String& query)
	{
		if (!conn.IsOpened())
			throw COdbcException(0, "[COdbcCommand::Prepare] ERROR: Database is not connected.");

		if (query.IsEmpty())
			throw COdbcException(0, "[COdbcCommand::Prepare] ERROR: Query text is empty.");

		OdbcPrepareType type;

		if (IsStoredProcedure(query.GetString()))
			type = OdbcPrepareType_PROC;
		else
			type = OdbcPrepareType_QUERY;

		if (m_hstmt->GetHandle())
		{
			m_hstmt->Close();
		}

		m_conn = &conn;
		m_type = type;

		m_conn->AllocStmtHandle(m_hstmt, m_type);

		m_query = query;

		m_paramsMaxCount = GetSpecifiedCharacterCount(m_query.GetString(), '?');

		m_params.clear();

		m_firstExecute = true;
	}

	void COdbcCommandImpl::AppendParameter(OdbcParamType paramType, const COdbcVariant &value, OdbcSqlDataType sqlType)
	{
		// 유효성 검사
		if (m_params.GetCount() >= m_paramsMaxCount)
			throw COdbcException(0, "[ODBCCommand::AppendParameter] ERROR: Can't put anymore Parameters.");

		if (value.GetType() == OdbcDataType_NULL)
			throw COdbcException(0, "[COdbcCommand::AppendParameter] ERROR: Parameter is NULL.");

		if (paramType != OdbcParamType_INPUT && paramType != OdbcParamType_OUTPUT && paramType != OdbcParamType_INPUT_OUTPUT)
			throw COdbcException(0, StringA::NewFormat("[%s] ERROR: paramType(%d) Error.", __FUNCTION__, paramType));

		// output 용 파라메터는 반드시 외장 데이터이어야 한다.
		if (value.IsRef() == false)
		{
			switch (paramType)
			{
				case OdbcParamType_INPUT:
					break;
				case OdbcParamType_OUTPUT:
					throw COdbcException(0, "[COdbcCommand::AppendOutputParameter] ERROR: Only referenced data is allowed for output parameter.");
				case OdbcParamType_INPUT_OUTPUT:
					throw COdbcException(0, "[COdbcCommand::AppendInputOutputParameter] ERROR: Only referenced data is allowed for input&output parameter.");
			}
		}

		// 파라메터 목록에 추가
		RefCount<CommandParameter> param(new CommandParameter);
		param->m_value = value;
		param->m_paramType = static_cast<SQLSMALLINT>(paramType);
		param->m_indicators = 0;
		param->m_sqlType = sqlType;
		param->m_isNull = false;

		m_params.Add(param);
	}

	void COdbcCommandImpl::SetNullParameter(int paramIndex, bool state)
	{
		if (paramIndex <= 0 || paramIndex > m_params.GetCount())
			throw COdbcException(0, "[COdbcCommand::SetNullParameter] ERROR: paramIndex is out of range.");
		m_params[paramIndex - 1]->m_isNull = state;
	}

	SQLLEN COdbcCommandImpl::Execute(COdbcWarnings* odbcWarnings)
	{
		if (m_conn == NULL)
			throw COdbcException(0, "[COdbcCommand::Execute] ERROR: Connection is not prepared.");

		if (m_conn->IsOpened() == false)
			throw COdbcException(0, "[COdbcCommand::Execute] ERROR: Database is not Connect.");

		// just-in-time stmt handle 생성
		if (m_hstmt->GetHandle() == SQL_NULL_HANDLE)
		{
			m_conn->AllocStmtHandle(m_hstmt, m_type);

			m_firstExecute = true;
		}

		SQLRETURN ret = 0;
		SQLLEN rowCount = 0;

		int paramsCount = static_cast<int>(m_params.GetCount());

		SQLSMALLINT cType = 0;
		SQLSMALLINT sqlType = 0;
		SQLULEN columnSize = 0;
		SQLSMALLINT scale = 0;
		SQLPOINTER valPtr = NULL;
		SQLLEN valLen = 0;

		// 처음 실행하는 경우 Bind parameter 함수들을 호출한다.
		// 파라메터 데이터는 사용자가 제공 즉 외부 참조 형식이거나 내장 형식 중 하나일 것이다.
		if (m_firstExecute)
		{
			for (int loop = 0; loop < paramsCount; loop++)
			{
				CommandParameter *param;
				param = m_params[loop];

				columnSize = 0;
				scale = 0;
				valPtr = param->m_value.GetPtr();
				valLen = 0;

				switch (param->m_value.GetType())
				{
					case OdbcDataType_NULL:
						throw COdbcException(0, "[COdbcCommand::Execute] ERROR: Parameter is NULL.");
					case OdbcDataType_BOOL:
						cType = SQL_C_BIT;
						sqlType = static_cast<SQLSMALLINT>((param->m_sqlType != OdbcSqlDataType_UNKNOWN) ? param->m_sqlType : OdbcSqlDataType_BIT);
						break;
					case OdbcDataType_TCHAR:
						cType = SQL_C_TCHAR;
#if !defined(_PNUNICODE)
						sqlType = (param->m_sqlType != OdbcSqlDataType_UNKNOWN) ? param->m_sqlType : OdbcSqlDataType_CHAR;
#else
						sqlType = static_cast<SQLSMALLINT>((param->m_sqlType != OdbcSqlDataType_UNKNOWN) ? param->m_sqlType : OdbcSqlDataType_WCHAR);
#endif
						columnSize = 1;
						param->m_indicators = 1;
						break;
					case OdbcDataType_SINT8:
						cType = SQL_C_STINYINT;
						sqlType = static_cast<SQLSMALLINT>((param->m_sqlType != OdbcSqlDataType_UNKNOWN) ? param->m_sqlType : OdbcSqlDataType_TINYINT);
						break;
					case OdbcDataType_UINT8:
						cType = SQL_C_UTINYINT;
						sqlType = static_cast<SQLSMALLINT>((param->m_sqlType != OdbcSqlDataType_UNKNOWN) ? param->m_sqlType : OdbcSqlDataType_TINYINT);
						break;
					case OdbcDataType_SINT16:
						cType = SQL_C_SSHORT;
						sqlType = static_cast<SQLSMALLINT>((param->m_sqlType != OdbcSqlDataType_UNKNOWN) ? param->m_sqlType : OdbcSqlDataType_SMALLINT);
						break;
					case OdbcDataType_UINT16:
						cType = SQL_C_USHORT;
						sqlType = static_cast<SQLSMALLINT>((param->m_sqlType != OdbcSqlDataType_UNKNOWN) ? param->m_sqlType : OdbcSqlDataType_SMALLINT);
						break;
					case OdbcDataType_SINT32:
						cType = SQL_C_SLONG;
						sqlType = static_cast<SQLSMALLINT>((param->m_sqlType != OdbcSqlDataType_UNKNOWN) ? param->m_sqlType : OdbcSqlDataType_INTEGER);
						break;
					case OdbcDataType_UINT32:
						cType = SQL_C_ULONG;
						sqlType = static_cast<SQLSMALLINT>((param->m_sqlType != OdbcSqlDataType_UNKNOWN) ? param->m_sqlType : OdbcSqlDataType_INTEGER);
						break;
					case OdbcDataType_SINT64:
						cType = SQL_C_SBIGINT;
						sqlType = static_cast<SQLSMALLINT>((param->m_sqlType != OdbcSqlDataType_UNKNOWN) ? param->m_sqlType : OdbcSqlDataType_BIGINT);
						break;
					case OdbcDataType_UINT64:
						cType = SQL_C_UBIGINT;
						sqlType = static_cast<SQLSMALLINT>((param->m_sqlType != OdbcSqlDataType_UNKNOWN) ? param->m_sqlType : OdbcSqlDataType_BIGINT);
						break;
					case OdbcDataType_FLOAT:
						cType = SQL_C_FLOAT;
						sqlType = static_cast<SQLSMALLINT>((param->m_sqlType != OdbcSqlDataType_UNKNOWN) ? param->m_sqlType : OdbcSqlDataType_FLOAT);
						break;
					case OdbcDataType_DOUBLE:
						cType = SQL_C_DOUBLE;
						sqlType = static_cast<SQLSMALLINT>((param->m_sqlType != OdbcSqlDataType_UNKNOWN) ? param->m_sqlType : OdbcSqlDataType_DOUBLE);
						break;
					case OdbcDataType_STRING:
						cType = SQL_C_TCHAR;
#if !defined(_PNUNICODE)
						sqlType = (param->m_sqlType != OdbcSqlDataType_UNKNOWN) ? param->m_sqlType : OdbcSqlDataType_VARCHAR;
#else
						sqlType = static_cast<SQLSMALLINT>((param->m_sqlType != OdbcSqlDataType_UNKNOWN) ? param->m_sqlType : OdbcSqlDataType_WVARCHAR);
#endif
						valPtr = (SQLPOINTER) ((String*) param->m_value.GetPtr())->GetString();
						if (param->m_paramType == SQL_PARAM_OUTPUT || param->m_paramType == SQL_PARAM_INPUT_OUTPUT)
						{
							valLen = ((String*) param->m_value.GetPtr())->GetLength();
						}
						param->m_indicators = SQL_NTS;
						break;
					case OdbcDataType_BYTE:
						cType = SQL_C_BINARY;
						sqlType = static_cast<SQLSMALLINT>((param->m_sqlType != OdbcSqlDataType_UNKNOWN) ? param->m_sqlType : OdbcSqlDataType_VARBINARY);
						{
							COdbcByteData* byteArray = (COdbcByteData*) param->m_value.GetPtr();
							valPtr = byteArray->GetDataPtr();
							valLen = byteArray->GetDataLength();
							columnSize = byteArray->GetMaxLength();
						}
						param->m_indicators = valLen;
						break;
					case OdbcDataType_TIMESTAMP:
						cType = SQL_C_TIMESTAMP;
						sqlType = static_cast<SQLSMALLINT>((param->m_sqlType != OdbcSqlDataType_UNKNOWN) ? param->m_sqlType : OdbcSqlDataType_TIMESTAMP);
						scale = 6; // precision (microsecond)
						break;
				}

				if (param->m_isNull)
				{
					param->m_indicators = SQL_NULL_DATA;
					valLen = 0;
				}

				if ((ret = SQLBindParameter(
					m_hstmt->GetHandle(),
					(SQLSMALLINT)(loop + 1),
					param->m_paramType,
					cType,
					sqlType,
					columnSize,
					scale,
					valPtr,
					valLen,
					&param->m_indicators))
						!= SQL_SUCCESS)
				{
					CheckError(ret, m_hstmt);
				}
			}

			// SQLPrepareA를 Execute 바로 전에 최초 한번만 호출 하기 때문에 이 부분에서는 병목 문제가 없습니다.
			if ((ret = SQLPrepare(m_hstmt->GetHandle(), (SQLTCHAR*) m_query.GetString(), m_query.GetLength())) != SQL_SUCCESS)
				CheckError(ret, m_hstmt);

			m_firstExecute = false;
		}
		else
		{
			// 이미 쿼리를 실행한 바 있으면 다른 파라메터들은 모두 바인딩된 상태다.
			// 따라서, 문자열 처럼 사용자가 중간에 데이터 주소를 바꾼 경우를 위해 재 바인딩을 일부만 한다.
			for (int loop = 0; loop < paramsCount; loop++)
			{
				CommandParameter *param;
				param = m_params[loop];

				if (param->m_isNull)
				{
					param->m_indicators = SQL_NULL_DATA;
				}
				else
				{
					param->m_indicators = 0;

					switch (param->m_value.GetType())
					{
						case OdbcDataType_TCHAR:
							param->m_indicators = sizeof(PNTCHAR);
							break;
						case OdbcDataType_STRING:
							// 외부 참조 문자열 데이터인 경우 그 사이에 문자열 버퍼 주소가 바뀌었을 수 있다. 따라서 재 바인딩을 한다.
							param->m_indicators = SQL_NTS;
							if (param->m_value.IsRef())
							{
								cType = SQL_C_TCHAR;
#if !defined(_PNUNICODE)
								sqlType = (param->m_sqlType != OdbcSqlDataType_UNKNOWN) ? param->m_sqlType : OdbcSqlDataType_VARCHAR;
#else
								sqlType = static_cast<SQLSMALLINT>((param->m_sqlType != OdbcSqlDataType_UNKNOWN) ? param->m_sqlType : OdbcSqlDataType_WVARCHAR);
#endif
								columnSize = 0;
								scale = 0;
								valPtr = (SQLPOINTER) ((String*) param->m_value.GetPtr())->GetString();
								valLen = 0;

								if (param->m_paramType == SQL_PARAM_OUTPUT || param->m_paramType == SQL_PARAM_INPUT_OUTPUT)
								{
									valLen = ((String*) param->m_value.GetPtr())->GetLength();
								}

								if ((ret = SQLBindParameter(
									m_hstmt->GetHandle(), 
									(SQLSMALLINT)(loop + 1),
									param->m_paramType,
									cType, 
									sqlType, 
									columnSize,
									scale, 
									valPtr, 
									valLen,
									&param->m_indicators)) != SQL_SUCCESS)
								{
									CheckError(ret, m_hstmt);
								}
							}
							break;
						case OdbcDataType_BYTE:
							param->m_indicators = ((COdbcByteData*) param->m_value.GetPtr())->GetDataLength();
							break;
						default:
							break;
					}
				}
			}
		}

		if ((ret = SQLExecute(m_hstmt->GetHandle())) != SQL_SUCCESS)
			CheckError(ret, m_hstmt, odbcWarnings);
		
		if ((ret = SQLRowCount(m_hstmt->GetHandle(), &rowCount)) != SQL_SUCCESS)
			CheckError(ret, m_hstmt);

		return rowCount;
	}

	SQLLEN COdbcCommandImpl::Execute(COdbcRecordset &recordset, COdbcWarnings* odbcWarnings)
	{
		// 쿼리 구문 실행
		SQLLEN rowcount = Execute(odbcWarnings);

		recordset.SetEnvironment(m_conn, m_hstmt, rowcount);

		return rowcount;
	}
}
