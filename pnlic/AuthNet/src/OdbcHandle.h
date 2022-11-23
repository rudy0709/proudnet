#pragma once

#include <sql.h>
#include <sqltypes.h>

namespace Proud
{
	enum OdbcHandleType
	{
		OdbcHANDLETYPE_ENV = SQL_HANDLE_ENV,
		OdbcHANDLETYPE_DBC = SQL_HANDLE_DBC,
		OdbcHANDLETYPE_STMT = SQL_HANDLE_STMT
	};

	// ODBC handle wrapper
	class COdbcHandle
	{
	private:
		SQLHANDLE m_handle;
		OdbcHandleType m_handleType;

	public:
		COdbcHandle(OdbcHandleType handleType);
		~COdbcHandle();

		void Close();

		SQLHANDLE& GetHandle();
		OdbcHandleType& GetHandleType();

	private:
		COdbcHandle(const COdbcHandle& other);
		COdbcHandle& operator=(const COdbcHandle& other);
	};
}
