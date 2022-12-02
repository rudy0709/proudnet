#pragma once

#include <sqltypes.h>

#include "../include/OdbcEnum.h"
#include "../include/OdbcException.h"
#include "../include/PNString.h"

namespace Proud
{
	class COdbcHandle;
	class COdbcRecordset;
	class COdbcConnection;

	class COdbcConnectionImpl
	{
	private:
		COdbcHandle* m_henv;
		COdbcHandle* m_hdbc;
		COdbcHandle* m_hstmt;

	public:
		COdbcConnectionImpl();
		~COdbcConnectionImpl();

		void Open(const String& dsn, const String& id, const String& passwd, COdbcWarnings* odbcWarnings);
		void Open(const String& connectionString, COdbcWarnings* odbcWarnings);

		void Close();

		void BeginTrans();

		void RollbackTrans();

		void CommitTrans();

		bool IsOpened();

		SQLLEN Execute(const String& query, COdbcWarnings* odbcWarnings);

		SQLLEN Execute(COdbcConnection* conn, COdbcRecordset &recordset, const String& query, COdbcWarnings* odbcWarnings);

		void AllocStmtHandle(COdbcHandle* hstmt, OdbcPrepareType type, COdbcWarnings* odbcWarnings);

	private:
		COdbcConnectionImpl(const COdbcConnectionImpl& other);
		COdbcConnectionImpl& operator=(const COdbcConnectionImpl& other);
	};
}
