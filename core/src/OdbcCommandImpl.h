#pragma once

#include <sqlext.h>
#include <sqltypes.h>

#include "../include/FastArray.h"
#include "../include/OdbcEnum.h"
#include "../include/OdbcException.h"
#include "../include/OdbcVariant.h"
#include "../include/PNString.h"
#include "../include/Ptr.h"

namespace Proud
{
	class COdbcHandle;
	class COdbcConnection;
	class COdbcRecordset;

	class COdbcCommandImpl
	{
	private:
		struct CommandParameter
		{
			// value
			COdbcVariant m_value;

			// 파라메터 방향(in or out or inout)
			SQLSMALLINT m_paramType;

			// 문자열 크기나 바이너리 데이터 크기 등을 담는다. 
			// SQL execute 함수 안에서 이 데이터 블럭이 참조 형태로 액세스된다.
			// 양수인 경우 해당 숫자byte만큼 접근한다.
			SQLLEN m_indicators;

			// 필드 타입
			OdbcSqlDataType m_sqlType;
			// true이면 값이 없음을 의미. 
			bool m_isNull;
		};
		typedef RefCount<CommandParameter> CommandParameterPtr;

		COdbcConnection* m_conn;

		COdbcHandle* m_hstmt;

		// Prepare()에 의해 채워짐
		String m_query;

		// proc? 그냥 쿼리문?
		OdbcPrepareType m_type;

		// 사용자가 추가한 파라메터들. 데이터는 내장이거나 외장(사용자가 값을 제공하고 여기서는 참조만) 일 수 있음.
		CFastArray<CommandParameterPtr> m_params;

		// prevent append more parameters. (execute doesn't recognize more parameters. -> no error.)
		// 쿼리구문에서 ?가 있는 갯수
		int m_paramsMaxCount;

		// 한번도 Execute()를 실행한 적 없으면 true.
		// Execute()를 재실행할때 불필요한 처리 skip하고자.
		bool m_firstExecute;

	public:
		COdbcCommandImpl();
		~COdbcCommandImpl();

		void Prepare(COdbcConnection &conn, const String& query);
		void AppendParameter(OdbcParamType paramType, const COdbcVariant &value, OdbcSqlDataType sqlType);
		void SetNullParameter(int paramIndex, bool state);

		SQLLEN Execute(COdbcWarnings* odbcWarnings);
		SQLLEN Execute(COdbcRecordset &recordset, COdbcWarnings* odbcWarnings);

	private:
		// 복사 금지
		COdbcCommandImpl(const COdbcCommandImpl &other);
		COdbcCommandImpl& operator=(const COdbcCommandImpl &other);
	};
}
