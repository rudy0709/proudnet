#include "stdafx.h"

#include <sql.h>
#include <sqlext.h>
#include <sqlucode.h>
#include <stdio.h>

#include "OdbcConnectionImpl.h"
#include "OdbcErrorCheck.h"
#include "OdbcHandle.h"
#include "../include/OdbcRecordset.h"

namespace Proud
{
	COdbcConnectionImpl::COdbcConnectionImpl()
	{
		m_henv = new COdbcHandle(OdbcHANDLETYPE_ENV);
		m_hdbc = new COdbcHandle(OdbcHANDLETYPE_DBC);
		m_hstmt = new COdbcHandle(OdbcHANDLETYPE_STMT);
	}

	COdbcConnectionImpl::~COdbcConnectionImpl()
	{
		if (m_hstmt)
		{
			delete m_hstmt;
			m_hstmt = NULL;
		}

		if (m_hdbc)
		{
			delete m_hdbc;
			m_hdbc = NULL;
		}

		if (m_henv)
		{
			delete m_henv;
			m_henv = NULL;
		}
	}

	void AllocHandle(COdbcHandle* &henv, COdbcHandle* & hdbc)
	{
		SQLRETURN ret;
		if (henv->GetHandle() == SQL_NULL_HANDLE)
		{
			if ((ret = SQLAllocEnv(&henv->GetHandle())) != SQL_SUCCESS)
				CheckError(ret, henv);
			if ((ret = SQLSetEnvAttr(henv->GetHandle(), SQL_ATTR_CONNECTION_POOLING, (SQLPOINTER) SQL_CP_ONE_PER_HENV, SQL_IS_INTEGER)) != SQL_SUCCESS)
				CheckError(ret, henv);
		}
		if (hdbc->GetHandle() == SQL_NULL_HANDLE)
		{
			if ((ret = SQLAllocConnect(henv->GetHandle(), &hdbc->GetHandle())) != SQL_SUCCESS)
				CheckError(ret, henv);
			if ((ret = SQLSetConnectAttr(hdbc->GetHandle(), SQL_ATTR_AUTOCOMMIT, (SQLPOINTER) SQL_AUTOCOMMIT_ON, 0)) != SQL_SUCCESS)
				CheckError(ret, hdbc);
		}
	}

	void COdbcConnectionImpl::Open(const String& dsn, const String& id, const String& passwd, COdbcWarnings* odbcWarnings)
	{
		if (dsn.GetLength() == 0)
			throw COdbcException(0, "[COdbcConnection::Open] ERROR: DSN length is 0.");

		if (dsn.GetLength() == 0)
			throw COdbcException(0, "[COdbcConnection::Open] ERROR: ID length is 0.");

		if (dsn.GetLength() == 0)
			throw COdbcException(0, "[COdbcConnection::Open] ERROR: Password length is 0.");

		SQLRETURN ret;

		AllocHandle(m_henv, m_hdbc);
		if ((ret = SQLConnect(m_hdbc->GetHandle(), (SQLTCHAR*) dsn.GetString(), dsn.GetLength(), (SQLTCHAR*) id.GetString(), id.GetLength(), (SQLTCHAR*) passwd.GetString(),
				passwd.GetLength())) != SQL_SUCCESS)
			CheckError(ret, m_hdbc, odbcWarnings);
	}

	void COdbcConnectionImpl::Open(const String& connectionString, COdbcWarnings* odbcWarnings)
	{
		if (connectionString.GetLength() == 0)
			throw COdbcException(0, "[COdbcConnection::Open] ERROR: ConnectionString length is 0.");

		SQLRETURN ret;

		AllocHandle(m_henv, m_hdbc);
		if ((ret = SQLDriverConnect(m_hdbc->GetHandle(), NULL, (SQLTCHAR*) connectionString.GetString(), connectionString.GetLength(), NULL, 0, NULL, SQL_DRIVER_NOPROMPT))
				!= SQL_SUCCESS)
			CheckError(ret, m_hdbc, odbcWarnings);
	}

	void COdbcConnectionImpl::Close()
	{
		m_hstmt->Close();
		m_hdbc->Close();
		m_henv->Close();
	}

	void COdbcConnectionImpl::BeginTrans()
	{
		SQLRETURN ret;

		if ((ret = SQLSetConnectAttr(m_hdbc->GetHandle(), SQL_ATTR_AUTOCOMMIT, (SQLPOINTER) SQL_AUTOCOMMIT_OFF, 0)) != SQL_SUCCESS)
			CheckError(ret, m_hdbc);
	}

	void COdbcConnectionImpl::RollbackTrans()
	{
		SQLRETURN ret;

		if ((ret = SQLEndTran(SQL_HANDLE_DBC, m_hdbc->GetHandle(), SQL_ROLLBACK)) != SQL_SUCCESS)
			CheckError(ret, m_hdbc);
		if ((ret = SQLSetConnectAttr(m_hdbc->GetHandle(), SQL_ATTR_AUTOCOMMIT, (SQLPOINTER) SQL_AUTOCOMMIT_ON, 0)) != SQL_SUCCESS)
			CheckError(ret, m_hdbc);
	}

	void COdbcConnectionImpl::CommitTrans()
	{
		SQLRETURN ret;

		if ((ret = SQLEndTran(SQL_HANDLE_DBC, m_hdbc->GetHandle(), SQL_COMMIT)) != SQL_SUCCESS)
			CheckError(ret, m_hdbc);
		if ((ret = SQLSetConnectAttr(m_hdbc->GetHandle(), SQL_ATTR_AUTOCOMMIT, (SQLPOINTER) SQL_AUTOCOMMIT_ON, 0)) != SQL_SUCCESS)
			CheckError(ret, m_hdbc);
	}

	bool COdbcConnectionImpl::IsOpened()
	{
		if (m_hdbc->GetHandle() != SQL_NULL_HDBC)
		{
			SQLLEN isDead = SQL_CD_TRUE;

			SQLGetConnectAttr(m_hdbc->GetHandle(), SQL_ATTR_CONNECTION_DEAD, &isDead, 0, NULL);
			if (isDead == SQL_CD_FALSE)
				return true;
		}
		return false;
	}

	SQLLEN COdbcConnectionImpl::Execute(const String& query, COdbcWarnings* odbcWarnings)
	{
		if (IsOpened() == false)
			throw COdbcException(0, "[COdbcConnection::Execute] ERROR: Database is not Connect.");
		if (query.GetLength() == 0)
			throw COdbcException(0, "[COdbcConnection::Execute] ERROR: Query length is 0.");

		AllocStmtHandle(m_hstmt, OdbcPrepareType_QUERY, odbcWarnings);

		SQLRETURN ret;
		SQLLEN rowCount;

		if ((ret = SQLExecDirect(m_hstmt->GetHandle(), (SQLTCHAR*) query.GetString(), query.GetLength())) != SQL_SUCCESS)
			CheckError(ret, m_hstmt, odbcWarnings);

		if ((ret = SQLRowCount(m_hstmt->GetHandle(), &rowCount)) != SQL_SUCCESS)
			CheckError(ret, m_hstmt, odbcWarnings);

		return rowCount;
	}

	SQLLEN COdbcConnectionImpl::Execute(COdbcConnection* conn, COdbcRecordset &recordset, const String& query, COdbcWarnings* odbcWarnings)
	{
		SQLLEN rowcount = Execute(query, odbcWarnings);

		recordset.SetEnvironment(conn, m_hstmt, rowcount);

		return rowcount;
	}

	void COdbcConnectionImpl::AllocStmtHandle(COdbcHandle* hstmt, OdbcPrepareType type, COdbcWarnings* odbcWarnings)
	{
		if (hstmt == NULL)
			throw COdbcException(0, "[COdbcConnection::AllocStmtHandle] ERROR: Statement Handle is NULL.");

		if (hstmt->GetHandleType() != OdbcHANDLETYPE_STMT)
			throw COdbcException(0, "[COdbcConnection::AllocStmtHandle] ERROR: OdbcHandle type is not OdbcHANDLETYPE_STMT.");

		SQLRETURN ret;
		if (hstmt->GetHandle() == SQL_NULL_HSTMT)
		{
			if ((ret = SQLAllocStmt(m_hdbc->GetHandle(), &hstmt->GetHandle()) != SQL_SUCCESS))
				CheckError(ret, m_hdbc, odbcWarnings);

			switch (type)
			{
				case OdbcPrepareType_QUERY:
					// SQL_CURSOR_STATIC는 앞뒤로 자유롭게 커서를 움직일 수 있는 옵션
					if ((ret = SQLSetStmtAttr(hstmt->GetHandle(), SQL_ATTR_CURSOR_TYPE, (SQLPOINTER*) SQL_CURSOR_STATIC, 0) != SQL_SUCCESS))
						CheckError(ret, hstmt, odbcWarnings);
					break;
				case OdbcPrepareType_PROC:
					// SQL_CURSOR_FORWARD_ONLY는 앞으로만 커서를 움직일 수 있는 옵션
					// MSSQL에서는 Procedure Recordset에 대해 SQL_CURSOR_STATIC을 사용하면 커서 에러가 발생하여 분기함.
					// MSSQL에서 MovePrev가 문제 있으므로 일관성을 위해 MySQL도 SQL_CURSOR_FORWARD_ONLY으로 맞춤.
					if ((ret = SQLSetStmtAttr(hstmt->GetHandle(), SQL_ATTR_CURSOR_TYPE, (SQLPOINTER*) SQL_CURSOR_FORWARD_ONLY, 0) != SQL_SUCCESS))
						CheckError(ret, hstmt, odbcWarnings);
					break;
			}
		}
		else
		{
			SQLCloseCursor(hstmt->GetHandle());
		}
	}

}
