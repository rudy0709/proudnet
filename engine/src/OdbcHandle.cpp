#include "stdafx.h"

#include "OdbcHandle.h"

namespace Proud
{
	COdbcHandle::COdbcHandle(OdbcHandleType handleType)
	{
		m_handle = SQL_NULL_HANDLE;
		m_handleType = handleType;
	}

	COdbcHandle::~COdbcHandle(void)
	{
		Close();
	}

	// for runtime Handle close
	void COdbcHandle::Close()
	{
		if (m_handle != SQL_NULL_HANDLE)
		{
			switch (m_handleType)
			{
			case OdbcHANDLETYPE_ENV:
				SQLFreeEnv(m_handle);
				break;
			case OdbcHANDLETYPE_DBC:
				SQLDisconnect(m_handle);
				SQLFreeConnect(m_handle);
				break;
			case OdbcHANDLETYPE_STMT:
				SQLFreeStmt(m_handle, SQL_DROP);
				break;
			}
			m_handle = SQL_NULL_HANDLE;
		}
	}

	SQLHANDLE & COdbcHandle::GetHandle()
	{
		return m_handle;
	}

	OdbcHandleType& COdbcHandle::GetHandleType()
	{
		return m_handleType;
	}
}
