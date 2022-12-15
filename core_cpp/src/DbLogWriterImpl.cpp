/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include "CriticalSectImpl.h"
#include "DbLogWriterImpl.h"
#include "../include/CoInit.h"
#include "../include/sysutil.h"

#ifdef _MSC_VER
#pragma warning(disable:4840)
#endif


namespace Proud
{

	void CDbLogWriterImpl::StaticThreadProc( void* ctx )
	{
		CDbLogWriterImpl* obj = (CDbLogWriterImpl*)ctx;
		obj->ThreadProc();
	}

	void CDbLogWriterImpl::ThreadProc()
	{
		CCoInitializer coi;

		CAdoConnection conn;

		CAdoRecordset rec;
		try
		{
			conn.Open(m_logParameter.m_dbmsConnectionString, m_logParameter.m_dbmsType);
			String query;		
			const PNTCHAR* queryStr;
			switch(m_logParameter.m_dbmsType)
			{
			case MsSql:
				queryStr = _PNT("select top 0 * from %s");
				break;
			case MySql:
				queryStr = _PNT("select * from %s limit 0");
				break;
			default:
				ShowUserMisuseError(_PNT("Unsupported DB type."));
				queryStr = _PNT("select * from %s limit 0");
				break;
			}
			query.Format(queryStr, m_logParameter.m_dbLogTableName);
			rec.Open( conn, OpenForReadWrite, query);
		}
		catch(_com_error &e)
		{
			String txt;
			const PNTCHAR* p2 = e.ErrorMessage();
			if(!p2)
				p2 = _PNT("");
			txt.Format(_PNT("%s %s %s"), (const PNTCHAR*)e.Description(), p2, (const PNTCHAR*)e.Source());		

			AdoException except(txt.GetString(), e);

			m_delegate->OnLogWriterException(except);
			conn.Close();
			return;
		}
		catch(AdoException &e)
		{
			m_delegate->OnLogWriterException(e);
			conn.Close();
			return;
		}

		if(MsSql == m_logParameter.m_dbmsType)
		{		
			do{
				if(m_writeSemaphore.WaitOne(300))
				{
					// 파일에 기록할 것들을 걷어낸다.
					// 걷어낼 데이터를 빨리 걷어낸 후 즉시 mutex unlock을 해야 다른 스레드에서 병목을 최소화한다.
					LogList logList;
					{
						CriticalSectionLock lock(m_CS, true);
						while (m_LogList.GetCount() > 0)
						{
							logList.AddTail(m_LogList.RemoveHead());
						}
					}

					// 걷어낸 것들을 모두 기록한다.
					while (logList.GetCount() > 0)
					{
						RefCount<LogData> logData(logList.RemoveHead());

						try
						{
							rec.AddNew();
							rec.FieldValues[_PNT("LoggerName")] = CVariant(m_logParameter.m_loggerName);
							rec.FieldValues[_PNT("LogText")] = CVariant(logData->m_LogText);

							CProperty prop = logData->m_properties;
							if(prop.GetCount()>0)
							{
								for(int i=0;i<rec->Fields->Count;i++)
								{
									String fieldName=(const PNTCHAR*)(_bstr_t)rec.FieldNames[i];
									fieldName.MakeUpper();
									for(CProperty::const_iterator it = prop.begin(); it != prop.end(); ++it)
									{
										if(fieldName==it.GetKey())
											rec.FieldValues[fieldName]=prop.Fields[fieldName];
									}
								}
							}
							rec.Update();
						}
						catch(_com_error &e)
						{
							String txt;
							const PNTCHAR* p2 = e.ErrorMessage();
							if(!p2)
								p2 = _PNT("");
							txt.Format(_PNT("%s %s %s"), (const PNTCHAR*)e.Description(), p2, (const PNTCHAR*)e.Source());		

							AdoException except(txt.GetString(), e);
							m_delegate->OnLogWriterException(except);
						}
						catch(AdoException &e)
						{
							m_delegate->OnLogWriterException(e);
						}
					}
					Proud::AssertIsNotLockedByCurrentThread(m_CS);
				}
			} while (m_stopThread == false);
		}
		else // mysql에서는 datetime가 default를 지원하지 않으므로 Proc로 처리하자.참고...timestamp는 사용되지만...2037년까지밖에 안된단다...
		{
			try{
				CreateProcedure(conn);
				do{
					if(m_writeSemaphore.WaitOne(300))
					{
						// 파일에 기록할 것들을 걷어낸다.
						// 걷어낼 데이터를 빨리 걷어낸 후 즉시 mutex unlock을 해야 다른 스레드에서 병목을 최소화한다.
						LogList logList;
						{
							CriticalSectionLock lock(m_CS, true);
							while (m_LogList.GetCount() > 0)
							{
								logList.AddTail(m_LogList.RemoveHead());
							}
						}

						// 걷어낸 것들을 모두 기록한다.
						while (logList.GetCount() > 0)
						{
							CAdoCommand cmd;
							RefCount<LogData> logData(logList.RemoveHead());

							String query;
							query.Format(_PNT("pn_Write%s"), m_logParameter.m_dbLogTableName.GetString());

							cmd.Prepare(conn,query);

							cmd.AppendParameter(_PNT("INLOGGERNAME"),ADODB::adVarWChar,ADODB::adParamInput,m_logParameter.m_loggerName.GetString(),m_logParameter.m_loggerName.GetLength());
							cmd.AppendParameter(_PNT("INLOGTEXT"),ADODB::adVarWChar,ADODB::adParamInput,logData->m_LogText.GetString(),logData->m_LogText.GetLength());

							CProperty prop = logData->m_properties;
							if(prop.GetCount()>0)
							{		
								for(int i=0;i<rec->Fields->Count;i++)
								{
									String fieldName=(const PNTCHAR*)(_bstr_t)rec.FieldNames[i];

									fieldName.MakeUpper();
									for(CProperty::const_iterator it = prop.begin(); it != prop.end(); ++it)
									{
										if(fieldName==it.GetKey())
										{
											String ParamName = _PNT("IN");
											ParamName+=fieldName;

											cmd.AppendParameter(ParamName,rec->Fields->Item[(long)i]->Type,ADODB::adParamInput,prop.Fields[fieldName]);
										}
									}
								}
							}
							cmd.Execute();
						}

					}
					Proud::AssertIsNotLockedByCurrentThread(m_CS);
				} while (m_stopThread == false);
			}
			catch(_com_error &e)
			{
				String txt;
				const PNTCHAR* p2 = e.ErrorMessage();
				if(!p2)
					p2 = _PNT("");
				txt.Format(_PNT("%s %s %s"), (const PNTCHAR*)e.Description(), p2, (const PNTCHAR*)e.Source());		

				AdoException except(txt.GetString(), e);
				m_delegate->OnLogWriterException(except);
			}
			catch(AdoException &e)
			{
				m_delegate->OnLogWriterException(e);
			}
		}

		rec.Close();
		conn.Close();
	}

	CDbLogWriterImpl::CDbLogWriterImpl()
		:m_workerThread(StaticThreadProc, this)
		,m_writeSemaphore(0,INT_MAX)
		,m_delegate(0)
	{
		m_stopThread = false;
	}

	CDbLogWriterImpl::~CDbLogWriterImpl()
	{
		// 스레드가 종료할 때까지 기다린다.
		m_stopThread = true;
		m_workerThread.Join();

		for(CFastList2<LogData*,intptr_t>::iterator i = m_LogList.begin();i!=m_LogList.end();i++)
		{
			delete *i;
		}
	}

	void CDbLogWriterImpl::WriteLine( String logText, CProperty* const pProperties )
	{
		CriticalSectionLock lock(m_CS, true);

		LogData* newLog = new LogData;
		newLog->m_LogText = logText;
		if(pProperties)
			newLog->m_properties = *pProperties;
		m_LogList.AddTail(newLog);

		NTTNTRACE("Log: %s\n", StringT2A(logText).GetString());

		// 세마포어 카운트 증가.
		m_writeSemaphore.Release();
	}

	void CDbLogWriterImpl::CreateProcedure(CAdoConnection& conn)
	{
		String strQuery;

		String creatProcedureQuery;
		creatProcedureQuery.Format(_PNT("DROP PROCEDURE IF EXISTS `pn_Write%s`"), m_logParameter.m_dbLogTableName.GetString());

		conn.Execute(creatProcedureQuery);

		String showCulumnsQuery;
		showCulumnsQuery.Format(_PNT("SHOW COLUMNS FROM %s"), m_logParameter.m_dbLogTableName.GetString());
		CAdoRecordset rsGet;
		rsGet.Open(conn, OpenForRead, showCulumnsQuery);

		String strParam;
		String strValues;
		String strType;
		String ColumnName;
		String strParams;
		String strFieldNames;

		//index를 제외키 위해..
		rsGet.MoveNext();

		while (!rsGet.IsEOF())
		{
			bool bInsert = true;
			for (int i = 0; i < rsGet->Fields->Count; i++)
			{
				CVariant fieldValue = rsGet.FieldValues[i];
				String fieldName = (const PNTCHAR*)(_bstr_t)rsGet.FieldNames[i];

				if (fieldName == _PNT("Field"))
				{
					ColumnName = (String)fieldValue;
					ColumnName.MakeUpper();

					if (ColumnName == _PNT("INSERTDATETIME"))
					{
						strValues += _PNT("now()");
					}
					else
					{
						strValues += _PNT("IN");
						strValues += (String)fieldValue;
					}
					strFieldNames += ColumnName;

				}
				else if (fieldName == _PNT("Type"))
				{
					if (ColumnName == _PNT("INSERTDATETIME"))
					{
						bInsert = false;
						break;
					}

					strParams += _PNT("IN IN");
					strParams += ColumnName;
					strParams += _PNT(" ");
					strParams += (String)fieldValue;
				}

			}

			rsGet.MoveNext();

			if (!rsGet.IsEOF())
			{
				if (bInsert)
					strParams += _PNT(",\n");
				strValues += _PNT(",");
				strFieldNames += _PNT(",");
			}
		}

		rsGet.Close();

		////pn_WriteDBLog를 만든다.
		strQuery.Format(_PNT("CREATE PROCEDURE `pn_Write%s`(\n")
			_PNT("%s\n")
			_PNT(")\n")
			_PNT("BEGIN\n")
			_PNT("insert into %s(%s) values(%s);\n")
			_PNT("END"), m_logParameter.m_dbLogTableName.GetString(),
			strParams, m_logParameter.m_dbLogTableName.GetString(),
			strFieldNames.GetString(), strValues.GetString());

		conn.Execute(strQuery);
	}


	CDbLogParameter::CDbLogParameter() :
		// 		m_dbmsConnectionString(_PNT("")),
		// 		m_loggerName(_PNT("")),
		m_dbLogTableName(_PNT("DbLog")),
		m_dbmsType(MsSql)
	{

	}
}