#include "stdafx.h"

#include <sql.h>
#include <sqlext.h>
#include <sqlucode.h>
#include <stdio.h>
#include <ctype.h>
#include <utility>

#include "OdbcRecordsetImpl.h"
#include "OdbcErrorCheck.h"
#include "OdbcHandle.h"
#include "../include/OdbcException.h"
#include "../include/OdbcByteData.h"
#include "../include/PNString.h"

//////////////////////////////////////////////////////////////////////////
// 이 모듈에 있는 함수들의 설명은 PIMPL owner 즉 COdbcRecordset 헤더파일에 있음.

namespace Proud
{
	COdbcRecordsetImpl::COdbcRecordsetImpl()
	{
		m_conn = NULL;
		m_hstmt = new COdbcHandle(OdbcHANDLETYPE_STMT);
		m_rowCount = 0;
		m_fieldCount = 0;
		m_rowIndex = -1;
		m_isBof = true;
		m_isEof = true;
		m_firstExecute = true;
	}

	COdbcRecordsetImpl::~COdbcRecordsetImpl()
	{
		m_conn = NULL;

		if (m_hstmt)
		{
			delete m_hstmt;
			m_hstmt = NULL;
		}

		ClearAllRecordset();
	}

	bool COdbcRecordsetImpl::Move(SQLSMALLINT fetchType)
	{
		if (m_conn == NULL)
			throw COdbcException(0, "[COdbcCommand::Execute] ERROR: Connection is not prepared.");

		if (m_fieldCount == 0)
			return false;

		// 각 필드의 정보를 얻어서 바인딩 처리
		BindRecordset();

		SQLRETURN ret;

		// 드디어 메모리로 가져옴
		ret = SQLFetchScroll(m_hstmt->GetHandle(), fetchType, 0);

		if (ret != SQL_SUCCESS && ret != SQL_NO_DATA)
			CheckError(ret, m_hstmt);

		// 가져오고 나서 EOF or BOF인지를 마킹한다.
		switch (fetchType)
		{
			case SQL_FETCH_FIRST:
				if (m_rowCount > 0)
				{
					m_rowIndex = 0;
					m_isBof = false;
					m_isEof = false;
				}
				break;
			case SQL_FETCH_LAST:
				if (m_rowCount > 0)
				{
					m_rowIndex = m_rowCount - 1;
					m_isBof = false;
					m_isEof = false;
				}
				break;
			case SQL_FETCH_NEXT:
				if (ret == SQL_SUCCESS)
				{
					m_rowIndex++;
					m_isBof = false;
				}
				else
				{
					if (m_rowIndex < m_rowCount)
						m_rowIndex++;
					m_isEof = true;
				}
				break;
			case SQL_FETCH_PRIOR:
				if (ret == SQL_SUCCESS)
				{
					m_rowIndex--;
					m_isEof = false;
				}
				else
				{
					if (m_rowCount > 0 && m_rowIndex >= 0)
						m_rowIndex--;
					m_isBof = true;
				}
				break;
		}

		return (ret == SQL_SUCCESS) ? true : false;
	}

	bool COdbcRecordsetImpl::NextRecordSet()
	{
		ClearAllRecordset();

		SQLRETURN ret;

		// the application calls SQLMoreResults to move to the next result set
		ret = SQLMoreResults(m_hstmt->GetHandle());

		// 행 수, 열 수를 얻는다.
		switch (ret)
		{
			case SQL_SUCCESS:
				if ((ret = SQLRowCount(m_hstmt->GetHandle(), &m_rowCount)) != SQL_SUCCESS)
					CheckError(ret, m_hstmt);
				if ((ret = SQLNumResultCols(m_hstmt->GetHandle(), &m_fieldCount)) != SQL_SUCCESS)
					CheckError(ret, m_hstmt);
				return true;
			case SQL_NO_DATA_FOUND:
				break;
			default:
				CheckError(ret, m_hstmt);
				break;
		}

		return false;
	}

	SQLLEN COdbcRecordsetImpl::GetRowCount()
	{
		return m_rowCount;
	}

	SQLLEN COdbcRecordsetImpl::GetCurrentRowIndex()
	{
		return m_rowIndex;
	}

	SQLSMALLINT COdbcRecordsetImpl::GetFieldCount()
	{
		return m_fieldCount;
	}

	bool COdbcRecordsetImpl::IsBof()
	{
		return m_isBof;
	}

	bool COdbcRecordsetImpl::IsEof()
	{
		return m_isEof;
	}

	void StringUpper(PNTCHAR* str)
	{
		if (str != NULL)
		{
			while (*str)
			{
#if !defined(_PNUNICODE)
				*str = toupper(*str);
#else
				*str = towupper(*str);
#endif
				str++;
			}
		}
	}

	// 각 필드 바인딩
	void COdbcRecordsetImpl::BindRecordset()
	{
		SQLRETURN ret;

		SQLSMALLINT cType;
		SQLPOINTER value;

		if (m_firstExecute)
		{
			PNTCHAR columnName[1000];
			SQLSMALLINT nameLength;
			SQLSMALLINT sqlType;
			SQLULEN columnSize;
			SQLSMALLINT scale;
			SQLSMALLINT nullable;

			for (int loop = 0; loop < m_fieldCount; loop++)
			{
				if ((ret = SQLDescribeCol(m_hstmt->GetHandle(), loop + 1, (SQLTCHAR*) columnName, sizeof(columnName), &nameLength, &sqlType, &columnSize, &scale, &nullable))
						!= SQL_SUCCESS)
				{
					CheckError(ret, m_hstmt);
				}

				// case-insensitive 위해
				StringUpper(columnName);

				RecordFieldPtr param(new RecordField);
				param->m_name = columnName;
				param->m_sqlType = (OdbcSqlDataType) sqlType;
				param->m_columnSize = columnSize;
				param->m_scale = scale;
				param->m_nullable = (nullable == SQL_NULLABLE) ? true : false;

				switch (param->m_sqlType)
				{
					case OdbcSqlDataType_CHAR:
					case OdbcSqlDataType_WCHAR:
						cType = SQL_C_TCHAR;
						columnSize = (columnSize + 1) * sizeof(PNTCHAR);
						if (param->m_columnSize > 1)
						{
							String* str = new String();
							param->m_value = str;
							param->m_value.ChangeRefToData();
						}
						else
						{
							param->m_value = '\0';
						}
						break;
					case OdbcSqlDataType_VARCHAR:
					case OdbcSqlDataType_LONGVARCHAR:
					case OdbcSqlDataType_WVARCHAR:
					case OdbcSqlDataType_WLONGVARCHAR:
						cType = SQL_C_TCHAR;
						columnSize = (columnSize + 1) * sizeof(PNTCHAR);
						{
							String* str = new String();
							param->m_value = str;
						}
						param->m_value.ChangeRefToData();
						break;
					case OdbcSqlDataType_BIT:
						cType = SQL_C_BIT;
						param->m_value = true;
						break;
					case OdbcSqlDataType_TINYINT:
						cType = SQL_C_TINYINT;
						param->m_value = (int8_t) 0;
						break;
					case OdbcSqlDataType_SMALLINT:
						cType = SQL_C_SHORT;
						param->m_value = (int16_t) 0;
						break;
					case OdbcSqlDataType_INTEGER:
						cType = SQL_C_LONG;
						param->m_value = (int32_t) 0;
						break;
					case OdbcSqlDataType_BIGINT:
						cType = SQL_C_SBIGINT;
						param->m_value = (int64_t) 0;
						break;
					case OdbcSqlDataType_REAL:
						cType = SQL_C_FLOAT;
						param->m_value = (float) 0.0;
						break;
					case OdbcSqlDataType_FLOAT:
					case OdbcSqlDataType_DOUBLE:
					case OdbcSqlDataType_NUMERIC:
					case OdbcSqlDataType_DECIMAL:
						cType = SQL_C_DOUBLE;
						param->m_value = (double) 0.0;
						break;
					case OdbcSqlDataType_BINARY:
					case OdbcSqlDataType_VARBINARY:
					case OdbcSqlDataType_LONGVARBINARY:
						cType = SQL_C_BINARY;
						{
							SQLCHAR* byte = new SQLCHAR[param->m_columnSize];
							COdbcByteData* byteData = new COdbcByteData(byte, 0, param->m_columnSize);
							param->m_value = byteData;
						}
						param->m_value.ChangeRefToData();
						break;
					case OdbcSqlDataType_DATE:
					case OdbcSqlDataType_TIME:
					case OdbcSqlDataType_TIMESTAMP:
						cType = SQL_C_TIMESTAMP;
						{
							TIMESTAMP_STRUCT* time = new TIMESTAMP_STRUCT();
							param->m_value = time;
						}
						param->m_value.ChangeRefToData();
						break;
					default:
						throw COdbcException(0, StringA::NewFormat("[COdbcRecordsetImpl::BindRecordset] OdbcSqlDataType(%d) is not yet implemented.", param->m_sqlType));
				}

				switch (param->m_value.GetType())
				{
					case OdbcDataType_STRING:
						// 사용자가 Move함수를 호출하기 전까지는 버퍼를 ODBC 모듈에 공유만 해주고 아직 사용자가 사용하지 않기 때문에 ReleaseBuffer를 호출하면 안됩니다.
						// 또 사용자가 Move함수를 호출하고 해당 변수를 사용하든 안하든 BindRecordset()에서 새로 GetBuffer를 호출합니다.
						value = ((String*) param->m_value.GetPtr())->GetBuffer(param->m_columnSize);
						break;
					case OdbcDataType_BYTE:
						value = ((COdbcByteData*) param->m_value.GetPtr())->GetDataPtr();
						break;
					default:
						value = param->m_value.GetPtr();
						break;
				}

				if ((ret = SQLBindCol(m_hstmt->GetHandle(), loop + 1, cType, value, columnSize, &param->m_indicator)) != SQL_SUCCESS)
					CheckError(ret, m_hstmt);

				m_columns.Add(param);

				if (nameLength > 0)
				{
					m_fieldNameToValueMap[param->m_name] = param;
				}
			}

			m_firstExecute = false;
		}
		else
		{
			// 재탕 사용되는 상황이다. string 등 사용자에 의해 버퍼 위치가 바뀐 경우들에 대해서만 재 SQLBindCol을 호출한다.
			RecordFieldPtr param;

			for (int loop = 0; loop < m_fieldCount; loop++)
			{
				param = m_columns[loop];

				switch (param->m_value.GetType())
				{
					case OdbcDataType_STRING:
						cType = SQL_C_TCHAR;
						value = ((String*) param->m_value.GetPtr())->GetBuffer(param->m_columnSize);
						if ((ret = SQLBindCol(m_hstmt->GetHandle(), loop + 1, cType, value, ((param->m_columnSize + 1) * sizeof(PNTCHAR)), &param->m_indicator)) != SQL_SUCCESS)
							CheckError(ret, m_hstmt);
						break;
					default:
						break;
				}
			}
		}
	}

	COdbcRecordsetImpl::RecordField* COdbcRecordsetImpl::GetRecord(int index)
	{
		return ((index > 0) && ((index - 1) < (int) m_columns.GetCount())) ? (m_columns[index - 1]).get() : NULL;
	}

	COdbcRecordsetImpl::RecordField* COdbcRecordsetImpl::GetRecord(const String& name)
	{
		CFastMap<String, RecordFieldPtr>::iterator iter;
		String _name = name;
		// case-insensitive 위해
		StringUpper(_name.GetBuffer());
		_name.ReleaseBuffer();
		iter = m_fieldNameToValueMap.find(_name);
		return (iter != m_fieldNameToValueMap.end()) ? (iter->GetSecond()).get() : NULL;
	}

	String& COdbcRecordsetImpl::GetFieldName(int fieldIndex)
	{
		RecordField* record = GetRecord(fieldIndex);
		if (record)
			return record->m_name;
		else
			throw COdbcException(0, StringA::NewFormat("[COdbcRecordset::GetFieldName] ERROR: fieldIndex[%d] is out of range.", fieldIndex));
	}

	OdbcSqlDataType COdbcRecordsetImpl::GetFieldSqlType(int fieldIndex)
	{
		RecordField* record = GetRecord(fieldIndex);
		if (record)
			return record->m_sqlType;
		else
			throw COdbcException(0, StringA::NewFormat("[COdbcRecordset::GetFieldSqlType] ERROR: fieldIndex[%d] is out of range.", fieldIndex));
	}

	OdbcSqlDataType COdbcRecordsetImpl::GetFieldSqlType(const String& fieldName)
	{
		RecordField* record = GetRecord(fieldName);
		if (record)
			return record->m_sqlType;
		else
		{
			throw COdbcException(0, StringA::NewFormat("[COdbcRecordset::GetFieldSqlType] ERROR: fieldName[%s] is not found.", StringT2A(fieldName).GetString()));
		}
	}

	SQLLEN COdbcRecordsetImpl::GetFieldSizeOrNull(int fieldIndex)
	{
		RecordField* record = GetRecord(fieldIndex);
		if (record)
			return record->m_indicator;
		else
			throw COdbcException(0, StringA::NewFormat("[COdbcRecordset::GetFieldSizeOrNull] ERROR: fieldIndex[%d] is out of range.", fieldIndex));
	}

	SQLLEN COdbcRecordsetImpl::GetFieldSizeOrNull(const String& fieldName)
	{
		RecordField* record = GetRecord(fieldName);
		if (record)
			return record->m_indicator;
		else
			throw COdbcException(0, StringA::NewFormat("[COdbcRecordset::GetFieldSizeOrNull] ERROR: fieldName[%s] is not found.", StringT2A(fieldName).GetString()));
	}

	SQLULEN COdbcRecordsetImpl::GetFieldMaxSize(int fieldIndex)
	{
		RecordField* record = GetRecord(fieldIndex);
		if (record)
			return record->m_columnSize;
		else
			throw COdbcException(0, StringA::NewFormat("[COdbcRecordset::GetFieldMaxSize] ERROR: fieldIndex[%d] is out of range.", fieldIndex));
	}

	SQLULEN COdbcRecordsetImpl::GetFieldMaxSize(const String& fieldName)
	{
		RecordField* record = GetRecord(fieldName);
		if (record != NULL)
			return record->m_columnSize;
		else
			throw COdbcException(0, StringA::NewFormat("[COdbcRecordset::GetFieldMaxSize] ERROR: fieldName[%s] is not found.", StringT2A(fieldName).GetString()));
	}

	SQLSMALLINT COdbcRecordsetImpl::GetFieldPrecision(int fieldIndex)
	{
		RecordField* record = GetRecord(fieldIndex);
		if (record)
			return record->m_scale;
		else
			throw COdbcException(0, StringA::NewFormat("[COdbcRecordset::GetFieldPrecision] ERROR: fieldIndex[%d] is out of range.", fieldIndex));
	}

	SQLSMALLINT COdbcRecordsetImpl::GetFieldPrecision(const String& fieldName)
	{
		RecordField* record = GetRecord(fieldName);
		if (record)
			return record->m_scale;
		else
			throw COdbcException(0, StringA::NewFormat("[COdbcRecordset::GetFieldPrecision] ERROR: fieldName[%s] is not found.", StringT2A(fieldName).GetString()));
	}

	bool COdbcRecordsetImpl::GetFieldNullable(int fieldIndex)
	{
		RecordField* record = GetRecord(fieldIndex);
		if (record)
			return record->m_nullable;
		else
			throw COdbcException(0, StringA::NewFormat("[COdbcRecordset::GetFieldNullable] ERROR: fieldIndex[%d] is out of range.", fieldIndex));
	}

	bool COdbcRecordsetImpl::GetFieldNullable(const String& fieldName)
	{
		RecordField* record = GetRecord(fieldName);
		if (record)
			return record->m_nullable;
		else
			throw COdbcException(0, StringA::NewFormat("[COdbcRecordset::GetFieldNullable] ERROR: fieldName[%s] is not found.", StringT2A(fieldName).GetString()));
	}

	const COdbcVariant& COdbcRecordsetImpl::GetFieldValue(int fieldIndex)
	{
		RecordField* record = GetRecord(fieldIndex);
		if (record)
		{
			CheckRecordValue(record);

			return record->m_value;
		}
		else
		{
			throw COdbcException(0, StringA::NewFormat("[COdbcRecordset::GetFieldValue] ERROR: fieldIndex[%d] is out of range.", fieldIndex));
		}
	}

	const COdbcVariant& COdbcRecordsetImpl::GetFieldValue(const String& fieldName)
	{
		RecordField* field = GetRecord(fieldName);
		if (field != NULL)
		{
			CheckRecordValue(field);

			return field->m_value;
		}
		else
		{
			throw COdbcException(0, StringA::NewFormat("[COdbcRecordset::GetFieldValue] ERROR: fieldName[%s] is not found.", StringT2A(fieldName).GetString()));
		}
	}

	// 쿼리 구문 실행 결과를 이 레코드셋이 가져올 수 있도록 내부 상태를 설정한다.
	// hstmt: command 객체가 갖고 있던 statement handle
	void COdbcRecordsetImpl::SetEnvironment(COdbcConnection* conn, COdbcHandle *hstmt, SQLLEN affectedRowCount)
	{
		m_conn = conn;

		if (m_hstmt->GetHandle() != SQL_NULL_HSTMT)
			m_hstmt->Close();

		// hstmt가 가진 핸들을 여기로 옮겨옴.
		m_hstmt->GetHandle() = hstmt->GetHandle();
		hstmt->GetHandle() = SQL_NULL_HSTMT;

		// 모든 레코드 캐싱 상태를 청소 
		ClearAllRecordset();

		if (affectedRowCount > 0)
		{
			m_rowCount = affectedRowCount;
			m_isBof = false;
			m_isEof = false;
		}

		// 필드 갯수를 얻는다.
		SQLRETURN ret;
		if ((ret = SQLNumResultCols(m_hstmt->GetHandle(), &m_fieldCount)) != SQL_SUCCESS)
			CheckError(ret, m_hstmt);
	}

	void COdbcRecordsetImpl::ClearAllRecordset()
	{
		m_rowCount = 0;
		m_fieldCount = 0;
		m_rowIndex = -1;
		m_isBof = true;
		m_isEof = true;
		m_firstExecute = true;

		m_columns.clear();
		m_fieldNameToValueMap.Clear();
	}

	void COdbcRecordsetImpl::CheckRecordValue(RecordField* record)
	{
		if (record->m_indicator == SQL_NULL_DATA)
			record->m_value.Initialize();

		switch (record->m_value.GetType())
		{
			case OdbcDataType_STRING:
				if (record->m_indicator != SQL_NULL_DATA)
				{
					/* Q: 만약 사용자가 전혀 GetFieldValue를 호출 안함으로 인해 영원히 string 객체에 대해 release buffer를 안해도 문제 없나요?
					 A: 문제 없습니다. 사용 전혀 안하고 이 string 객체가 파괴될테니까요. SetEnvironment에서 청소함.
					 */
					((String*) record->m_value.GetPtr())->ReleaseBuffer();
				}
				break;
			case OdbcDataType_BYTE:
				((COdbcByteData*) record->m_value.GetPtr())->SetDataLength(record->m_indicator);
				break;
			default:
				break;
		}
	}
}
