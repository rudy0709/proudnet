#include "stdafx.h"

#include "../include/OdbcByteData.h"

namespace Proud
{
	COdbcByteData::COdbcByteData(SQLCHAR* dataPtr, SQLLEN dataLen, SQLULEN maxLen)
	{
		this->m_dataPtr = dataPtr;
		this->m_dataLength = dataLen;
		this->m_maxLength = maxLen;
	}

	SQLCHAR* COdbcByteData::GetDataPtr()
	{
		return m_dataPtr;
	}

	SQLLEN COdbcByteData::GetDataLength()
	{
		return m_dataLength;
	}

	void COdbcByteData::SetDataLength(SQLLEN dataLen)
	{
		this->m_dataLength = dataLen;
	}

	SQLULEN COdbcByteData::GetMaxLength()
	{
		return m_maxLength;
	}
}
