#include "stdafx.h"

#include "OdbcConnectionImpl.h"
#include "../include/OdbcConnection.h"

namespace Proud
{
	COdbcConnection::COdbcConnection()
	{
		m_pImpl = new COdbcConnectionImpl;
	}

	COdbcConnection::~COdbcConnection()
	{
		delete m_pImpl;
	}

	void COdbcConnection::Open(const String& dsn, const String& id, const String& passwd, COdbcWarnings* odbcWarnings /* = NULL */)
	{
		m_pImpl->Open(dsn, id, passwd, odbcWarnings);
	}
	void COdbcConnection::Open(const String& connectionString, COdbcWarnings* odbcWarnings /* = NULL */)
	{
		m_pImpl->Open(connectionString, odbcWarnings);
	}

	void COdbcConnection::Close()
	{
		m_pImpl->Close();
	}

	void COdbcConnection::BeginTrans()
	{
		m_pImpl->BeginTrans();
	}

	void COdbcConnection::RollbackTrans()
	{
		m_pImpl->RollbackTrans();
	}

	void COdbcConnection::CommitTrans()
	{
		m_pImpl->CommitTrans();
	}

	bool COdbcConnection::IsOpened()
	{
		return m_pImpl->IsOpened();
	}

	int COdbcConnection::Execute(const String& query, COdbcWarnings* odbcWarnings /* = NULL */)
	{
		return static_cast<int>(m_pImpl->Execute(query, odbcWarnings));
	}

	int COdbcConnection::Execute(COdbcRecordset &recordset, const String& query, COdbcWarnings* odbcWarnings /* = NULL */)
	{
		COdbcConnection* conn = this;
		return static_cast<int>(m_pImpl->Execute(conn, recordset, query, odbcWarnings));
	}

	void COdbcConnection::AllocStmtHandle(COdbcHandle* hstmt, OdbcPrepareType type, COdbcWarnings* odbcWarnings /* = NULL */)
	{
		m_pImpl->AllocStmtHandle(hstmt, type, odbcWarnings);
	}
}
