#include "stdafx.h"

#include <sql.h>

#include "OdbcHandle.h"
#include "OdbcRecordsetImpl.h"
#include "../include/OdbcRecordset.h"

namespace Proud
{
	COdbcRecordset::COdbcRecordset()
	{
		m_pImpl = new COdbcRecordsetImpl;
	}

	COdbcRecordset::~COdbcRecordset()
	{
		delete m_pImpl;
	}

	bool COdbcRecordset::MoveFirst()
	{
		return m_pImpl->Move(SQL_FETCH_FIRST);
	}

	bool COdbcRecordset::MoveLast()
	{
		return m_pImpl->Move(SQL_FETCH_LAST);
	}

	bool COdbcRecordset::MoveNext()
	{
		return m_pImpl->Move(SQL_FETCH_NEXT);
	}

	bool COdbcRecordset::MovePrev()
	{
		return m_pImpl->Move(SQL_FETCH_PRIOR);
	}

	bool COdbcRecordset::NextRecordSet()
	{
		return m_pImpl->NextRecordSet();
	}

	SQLLEN COdbcRecordset::GetRowCount()
	{
		return m_pImpl->GetRowCount();
	}

	SQLLEN COdbcRecordset::GetCurrentRowIndex()
	{
		return m_pImpl->GetCurrentRowIndex();
	}

	SQLSMALLINT COdbcRecordset::GetFieldCount()
	{
		return m_pImpl->GetFieldCount();
	}

	bool COdbcRecordset::IsBof()
	{
		return m_pImpl->IsBof();
	}

	bool COdbcRecordset::IsEof()
	{
		return m_pImpl->IsEof();
	}

	String COdbcRecordset::GetFieldName(int fieldIndex)
	{
		return m_pImpl->GetFieldName(fieldIndex);
	}

	OdbcSqlDataType COdbcRecordset::GetFieldSqlType(int fieldIndex)
	{
		return m_pImpl->GetFieldSqlType(fieldIndex);
	}

	OdbcSqlDataType COdbcRecordset::GetFieldSqlType(const String& fieldName)
	{
		return m_pImpl->GetFieldSqlType(fieldName);
	}

	SQLLEN COdbcRecordset::GetFieldSizeOrNull(int fieldIndex)
	{
		return m_pImpl->GetFieldSizeOrNull(fieldIndex);
	}

	SQLLEN COdbcRecordset::GetFieldSizeOrNull(const String& fieldName)
	{
		return m_pImpl->GetFieldSizeOrNull(fieldName);
	}

	SQLULEN COdbcRecordset::GetFieldMaxSize(int fieldIndex)
	{
		return m_pImpl->GetFieldMaxSize(fieldIndex);
	}

	SQLULEN COdbcRecordset::GetFieldMaxSize(const String& fieldName)
	{
		return m_pImpl->GetFieldMaxSize(fieldName);
	}

	SQLSMALLINT COdbcRecordset::GetFieldPrecision(int fieldIndex)
	{
		return m_pImpl->GetFieldPrecision(fieldIndex);
	}

	SQLSMALLINT COdbcRecordset::GetFieldPrecision(const String& fieldName)
	{
		return m_pImpl->GetFieldPrecision(fieldName);
	}

	bool COdbcRecordset::GetFieldNullable(int fieldIndex)
	{
		return m_pImpl->GetFieldNullable(fieldIndex);
	}

	bool COdbcRecordset::GetFieldNullable(const String& fieldName)
	{
		return m_pImpl->GetFieldNullable(fieldName);
	}

	const COdbcVariant& COdbcRecordset::GetFieldValue(int fieldIndex)
	{
		return m_pImpl->GetFieldValue(fieldIndex);
	}

	const COdbcVariant& COdbcRecordset::GetFieldValue(const String& fieldName)
	{
		return m_pImpl->GetFieldValue(fieldName);
	}

	void COdbcRecordset::SetEnvironment(COdbcConnection* conn, void *hstmt, const SQLLEN &rowcount)
	{
		m_pImpl->SetEnvironment(conn, (COdbcHandle *) hstmt, rowcount);
	}
}
