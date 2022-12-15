#include "stdafx.h"

#include "../include/OdbcException.h"

namespace Proud
{
	COdbcException::COdbcException()
	{
		m_ret = 0;
		chMsg = "";
	}

	COdbcException::COdbcException(const SQLRETURN ret, const StringA& errorString)
	{
		m_ret = ret;
		chMsg = errorString;
	}

	int COdbcException::GetSqlErrorCode()
	{
		return m_ret;
	}

	COdbcException::COdbcException(const COdbcException& other)
	{
		m_ret = other.m_ret;
		chMsg = other.chMsg;
	}

	bool COdbcException::IsConnectionTimeout()
	{
		// msg가 HYT01로 시작하면 타임아웃
		return !strncmp(chMsg.c_str(), "HYT01", 5);
	}

	COdbcException& COdbcException::operator=(const COdbcException& other)
	{
		this->m_ret = other.m_ret;
		this->chMsg = other.chMsg;

		return *this;
	}

	COdbcWarning::COdbcWarning()
			: COdbcException()
	{
	}

	COdbcWarning::COdbcWarning(const SQLRETURN ret, const StringA& errorString)
			: COdbcException(ret, errorString)
	{
	}

	COdbcWarning::COdbcWarning(const COdbcWarning& other)
			: COdbcException(other)
	{
	}

	COdbcWarning& COdbcWarning::operator=(const COdbcWarning& other)
	{
		this->m_ret = other.m_ret;
		this->chMsg = other.chMsg;

		return *this;
	}
}

