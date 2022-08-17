#pragma once

#include <sqltypes.h>

#include "../include/OdbcEnum.h"
#include "../include/OdbcVariant.h"
#include "../include/PNString.h"
#include "../include/Ptr.h"
#include "../include/FastArray.h"
#include "../include/FastMap.h"

namespace Proud
{
	class COdbcHandle;
	class COdbcConnection;

	class COdbcRecordsetImpl
	{
	private:
		struct RecordField
		{
			String m_name;
			OdbcSqlDataType m_sqlType;
			SQLULEN m_columnSize;
			// float, double, decimal, numeric, etc.... precision.
			SQLSMALLINT m_scale;
			bool m_nullable;
			COdbcVariant m_value;
			// 데이터의 크기. SQL exec 함수가 실행될때 이 변수가 in,out 참조된다.
			SQLLEN m_indicator;
		};
		typedef RefCount<RecordField> RecordFieldPtr;

		COdbcConnection* m_conn;
		COdbcHandle* m_hstmt;

		// Total Row Count
		SQLLEN m_rowCount;
		SQLSMALLINT m_fieldCount;
		// Row Index
		SQLLEN m_rowIndex;
		bool m_isBof;
		bool m_isEof;

		CFastArray<RecordFieldPtr> m_columns;
		CFastMap<String, RecordFieldPtr> m_fieldNameToValueMap;

		// 한번도 Execute()를 실행한 바 없으면 true.
		// 연이어 Execute를 실행시 Bind를 매번 호출하는 부하를 없애기 위함.
		bool m_firstExecute;

	public:
		COdbcRecordsetImpl();
		~COdbcRecordsetImpl();

		bool Move(SQLSMALLINT fetchType);

		bool NextRecordSet();

		SQLLEN GetRowCount();
		SQLLEN GetCurrentRowIndex();
		SQLSMALLINT GetFieldCount();

		bool IsBof();
		bool IsEof();

		String& GetFieldName(int fieldIndex);

		OdbcSqlDataType GetFieldSqlType(int fieldIndex);
		OdbcSqlDataType GetFieldSqlType(const String& fieldName);

		SQLLEN GetFieldSizeOrNull(int fieldIndex);
		SQLLEN GetFieldSizeOrNull(const String& fieldName);

		SQLULEN GetFieldMaxSize(int fieldIndex);
		SQLULEN GetFieldMaxSize(const String& fieldName);

		SQLSMALLINT GetFieldPrecision(int fieldIndex);
		SQLSMALLINT GetFieldPrecision(const String& fieldName);

		bool GetFieldNullable(int fieldIndex);
		bool GetFieldNullable(const String& fieldName);

		const COdbcVariant& GetFieldValue(int fieldIndex);
		const COdbcVariant& GetFieldValue(const String& fieldName);

		// Recordset object get stmt handle from Connection object or Command object.
		void SetEnvironment(COdbcConnection* conn, COdbcHandle *hstmt, SQLLEN affectedRowCount);

	private:
		void BindRecordset();
		RecordField* GetRecord(int index);
		RecordField* GetRecord(const String& name);
		void ClearAllRecordset();
		void CheckRecordValue(RecordField* record);

		COdbcRecordsetImpl(const COdbcRecordsetImpl& other);
		COdbcRecordsetImpl& operator=(const COdbcRecordsetImpl& other);
	};
}
