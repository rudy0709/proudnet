#include "stdafx.h"

#include "OdbcCommandImpl.h"
#include "../include/OdbcCommand.h"

namespace Proud
{
	COdbcCommand::COdbcCommand()
	{
		m_pImpl = new COdbcCommandImpl;
	}

	COdbcCommand::~COdbcCommand()
	{
		delete m_pImpl;
	}

	void COdbcCommand::Prepare(COdbcConnection &conn, const String& query)
	{
		m_pImpl->Prepare(conn, query);
	}

	void COdbcCommand::AppendInputParameter(const COdbcVariant &param, OdbcSqlDataType sqlType /* = OdbcSqlDataType_UNKNOWN */)
	{
		m_pImpl->AppendParameter(OdbcParamType_INPUT, param, sqlType);
	}

	void COdbcCommand::AppendOutputParameter(const COdbcVariant &param, OdbcSqlDataType sqlType /* = OdbcSqlDataType_UNKNOWN */)
	{
		m_pImpl->AppendParameter(OdbcParamType_OUTPUT, param, sqlType);
	}

	void COdbcCommand::AppendInputOutputParameter(const COdbcVariant &param, OdbcSqlDataType sqlType /* = OdbcSqlDataType_UNKNOWN */)
	{
		m_pImpl->AppendParameter(OdbcParamType_INPUT_OUTPUT, param, sqlType);
	}

	void COdbcCommand::SetNullParameter(unsigned int paramIndex, bool state)
	{
		m_pImpl->SetNullParameter(paramIndex, state);
	}

	int COdbcCommand::Execute(COdbcWarnings* odbcWarnings /* = NULL */)
	{
		return m_pImpl->Execute(odbcWarnings);
	}

	int COdbcCommand::Execute(COdbcRecordset &recordset, COdbcWarnings* odbcWarnings /* = NULL */)
	{
		return m_pImpl->Execute(recordset, odbcWarnings);
	}
}
