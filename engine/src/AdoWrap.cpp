#include "stdafx.h"

#if defined(_WIN32)
#include "CriticalSectImpl.h"
#include "../include/AdoWrap.h"
#include "../include/Exception.h"
#include "../include/PnTime.h"

namespace Proud
{
	uint32_t CDbmsAccessTracker::DelayedAccessThresholdMilisec = 500;
	IDbmsAccessEvent* CDbmsAccessTracker::m_DbmsAcessEvent = NULL;

	// ikpil.choi 2017-02-10 : 자연스로운 ADODB::_Connection 풀링을 위한 트릭 추가(N3690)
	// 테스트 결과 ADODB::_Connection의 Open을 시도 하면, 결과와 상관없이 풀링이 시작되며,
	// 마지막 ADODB::_Connection 없어질 때, 풀링이 종료 되는 것을 확인하였습니다. 그러므로, 한개의 객체만 남깁니다.
	class CAdoConnectionInitializer : public CSingleton<CAdoConnectionInitializer>
	{
	public:
		~CAdoConnectionInitializer()
		{
			if (NULL != m_conn.GetInterfacePtr())
			{
				try
				{
					if (m_conn && ADODB::adStateClosed != m_conn->State)
						m_conn->Close();
				}
				catch (_com_error &)
				{
					// 드물게 여기로 오는 경우가 있음.
					// ... 위의 에러일 경우만 흘립니다.
				}
			}
		}

		CAdoConnectionInitializer()
		{
			try
			{
				//m_connectionPool=NULL;
				HRESULT hr = m_conn.CreateInstance(__uuidof(ADODB::Connection));
				if (NULL == m_conn.GetInterfacePtr())
				{
					throw _com_error(hr);
				}

				// 계속 잡고 있을 객체 : Open 결과는 상관 없이 없음을 테스트를 통해 확인하였습니다.
				m_conn->Open(_bstr_t(_PNT("trick")), _PNT(""), _PNT(""), -1);
			}
			catch (_com_error &)
			{
				// ... 위의 에러일 경우만 흘립니다.
			}
		}


	private:
		ADODB::_ConnectionPtr m_conn;
	};

	void CDbmsAccessTracker::SetAdoWrapEvent(IDbmsAccessEvent* pEvent )
	{
		m_DbmsAcessEvent = pEvent;
	}

	CDbmsAccessTracker::~CDbmsAccessTracker()
	{
		m_DbmsAcessEvent = NULL;
	}

	CDbmsAccessTracker::CDbmsAccessTracker()
	{

	}

	_Noreturn void RethrowComErrorAsAdoException(_com_error &e)
	{
		const PNTCHAR* p1 = e.Description();
		if (!p1)
			p1 = _PNT("");

		const PNTCHAR* p2 = e.ErrorMessage();
		if(!p2)
			p2 = _PNT("");

		const PNTCHAR* p3 = e.Source();
		if (!p3)
			p3 = _PNT("");

		Tstringstream part;
		part << p1 << _PNT(" ") << p2 << _PNT(" ") << p3;

		throw AdoException(part.str().c_str(), e);
	}

	AdoException::AdoException( const TCHAR* txt,_com_error &e ) :Exception(txt), m_comError(e)
	{

	}


//#pragma optimize("",off) // 쐤따빡! 여지껏 멀쩡하게 돌아가던 이게 Visual Studio 2005 SP1에서 스택을 긁어먹고 로컬 변수를 깨먹는다. 그래서 막아야 한다.

	/** 이걸 세팅하면 이 소스에 있는 ADO 관련 메서드들 중에서 DB를 억세스하는 메서드가 호출되면
	이게 current thread에 의해 lock됐나 체크하고 lock됐으면 ADODB_RecommendCriticalSectionProc를 실행한다.
	ADODB_RefCriticalSection이 NULL이면 무시한다.

	주의: 예외적으로 SequenceIDGenerator8,SequenceIDGenerator4는 DB 병목을 체크하지 않는다. 이들은 DB 억세스 빈도가 매우 낮은데다
	bunch 갯수를 DB 병목이 0%가 되도록 조절하는 알고리즘이 아직 없기 때문이다. */
	CriticalSection* ADODB_RefCriticalSection = NULL;

	/** NULL이면 경고를 해야 하는 상황에서도 그냥 무시한다. */
	ADODB_RecommendCriticalSectionProc_t ADODB_RecommendCriticalSectionProc = NULL;

#define WARN_COMEXCEPTION_FOR_PROUD(e) WarnComException_ForProud(e,StringA2T(__FILE__).GetString(),__LINE__)

	void WarnComException_ForProud(_com_error &e,const PNTCHAR* file,int line)
	{
#ifdef _DEBUG
		String str;
		const String com_error_message = Win32Guid::From(e.GUID()).GetString();
		str.Format(_PNT("%s(%d): Exception %s\nHRESULT=%d,GUID=%s"),
			file, line,
			(const PNTCHAR*)e.Description(),
			e.Error(),
			com_error_message.GetString());

		NTTNTRACE("%s\n", StringT2A(str).GetString());
#else
		_pn_unused(file);
		_pn_unused(line);
		_pn_unused(e);
#endif
	}

//#pragma optimize("",on)

	long CAdoConnection::Execute(const String& lpszSQL)
	{
		try
		{
			RecommendNoCriticalSectionLock();

			_variant_t noofAffectedRecords;

			CQueryChecker::ExecuteBegin();
			GetInterfacePtr()->Execute(lpszSQL.GetString(), &noofAffectedRecords, -1);
			CQueryChecker::ExecuteEnd(lpszSQL);

			return noofAffectedRecords;
		}
		catch(_com_error &e)
		{
			RethrowComErrorAsAdoException(e);
		}
	}

	long CAdoConnection::Execute(CAdoRecordset& outputRecords, const String& lpszSQL)
	{
		try
		{
			RecommendNoCriticalSectionLock();

			_variant_t noofAffectedRecords;

			CQueryChecker::ExecuteBegin();
			outputRecords = GetInterfacePtr()->Execute(lpszSQL.GetString(), &noofAffectedRecords, -1);
			CQueryChecker::ExecuteEnd(lpszSQL);

			return noofAffectedRecords;
		}
		catch(_com_error &e)
		{
			RethrowComErrorAsAdoException(e);
		}
	}
//#pragma optimize("",off) // 쐤따빡! 여지껏 멀쩡하게 돌아가던 이게 Visual Studio 2005 SP1에서 스택을 긁어먹고 로컬 변수를 깨먹는다. 그래서 막아야 한다.

	CAdoConnection::CAdoConnection(ADODB::_Connection *conn)
	{
		// 최초로 this object가 생성될 때, CAdoConnectionInitializer의 트릭 기능을 켠다.
		CAdoConnectionInitializer::GetSharedPtr();

		//m_connectionPool=NULL;
		if(conn)
			Attach(conn, true);		// AddRef, not Attach
	}

	CAdoConnection::CAdoConnection()
	{
		// 최초로 this object가 생성될 때, CAdoConnectionInitializer의 트릭 기능을 켠다.
		CAdoConnectionInitializer::GetSharedPtr();

		try
		{
			//m_connectionPool=NULL;
			HRESULT hr = CreateInstance(__uuidof(ADODB::Connection));
			if (GetInterfacePtr() == NULL)
			{
				throw _com_error(hr);
			}
		}
		catch (_com_error &e)
		{
			RethrowComErrorAsAdoException(e);
		}
	}

	// CAdoConnection::CAdoConnection( CAdoConnectionPool* connectionPool )
	// {
	//	m_connectionPool=connectionPool;
	//	ADODB::_ConnectionPtr p=connectionPool->GetOne();
	//	*this=p; // 리퍼런스 객체를 addref한다.
	//
	//
	// }
	CAdoConnection::~CAdoConnection()
	{
		//	if(m_connectionPool)
		//	{
		//		// 리퍼런스 객체를 반환한다.
		//		m_connectionPool->ReleaseOne(this);
		//	}
		//	else
		{
			/*
			이걸 굳이 넣는 이유:
			SQL Server OLE DB Connection Pooling을 씀에도 불구하고 단일 스레드에서 잦은 open/close를 한 후에
			엄청난 갯수의 TCP connection이 발생한다. 그러나 SQL Server EM에서의 연결 갯수는 1개로 표시된다.
			이를 해결하려면 명시적으로 Close()를 해줘야 한다고 하는데...
			과연 될까? 끙.

			Transaction 중에 Close 명시적 호출은 exception을 유발한다. 그러나 Close_NoThrow에 의해서
			무시시킨다.

			If you open a Connection object, remember to close it. Do not rely on garbage collection to
			implicitly close your connections.

			If You Explicitly Open It, Explicitly Close It
			Pooling problems are frequently caused by an application that does not clean up
			its connections. Connections are placed in the pool at the time that the connection
			is closed and not before. To avoid this, always explicitly close an object you open.
			If you don't explicitly close it, chances are it won't ever be released to the pool.

			Even if the language you use has effective and reliable garbage collection, an
			instance of an open ADO Connection or Recordset object going out of scope does
			not equate to the Close method of that object being implicitly invoked. You must
			close it explicitly.

			Failing to close an open Connection or Recordset object is probably the single
			most frequent cause of connection creep and the single largest cause of incorrect
			diagnoses of pooling failure.

			*/
			Close_NoThrow();
		}

	}

	void CAdoConnection::OpenEx(const String& connectionString,const PNTCHAR* AppName)
	{
		try
		{
			String connStrEx;
			if(AppName)
			{
				connStrEx.Format(_PNT("%s;Application Name=%s"),connectionString.GetString(),AppName);
			}
			else
				connStrEx=connectionString;

#ifndef _DEBUG
			/* appname이 서로 다른 connection 들은 비록 디버깅을 도와주나
			같은 ADO connection pool에 들어가지 못한다. 즉 connection creep의 가능성을 높인다.
			따라서 껐다. */
			connStrEx=connectionString;
#endif
			String Token = connStrEx;
			Token.Replace(_PNT(" "),_PNT("")); // 공백을 없앤다.
			Token.MakeUpper();



			RecommendNoCriticalSectionLock();

			//드라이버나 프로바이더가 없다면...default 세팅
			if(-1 == Token.Find(_PNT("PROVIDER=")) && -1 == Token.Find(_PNT("DRIVER=")))
				GetInterfacePtr()->PutProvider(_bstr_t(_PNT("sqloledb")));

			GetInterfacePtr()->Open(_bstr_t(connStrEx.GetString()), _PNT(""), _PNT(""), -1);
		}
		catch(_com_error &e)
		{
			RethrowComErrorAsAdoException(e);
		}

	}

	void CAdoConnection::OpenEx( const String& connectionString,const PNTCHAR* AppName,DbmsType Type )
	{
		try
		{
			String connStrEx;
			if (AppName)
			{
				connStrEx.Format(_PNT("%s;Application Name=%s"), connectionString.GetString(), AppName);
			}
			else
				connStrEx = connectionString;

#ifndef _DEBUG
			/* appname이 서로 다른 connection 들은 비록 디버깅을 도와주나
			같은 ADO connection pool에 들어가지 못한다. 즉 connection creep의 가능성을 높인다.
			따라서 껐다. */
			connStrEx = connectionString;
#endif

			RecommendNoCriticalSectionLock();

			if (MsSql == Type)
				GetInterfacePtr()->PutProvider(_bstr_t(_PNT("sqloledb")));

			GetInterfacePtr()->Open(_bstr_t(connStrEx.GetString()), _PNT(""), _PNT(""), -1);
		}
		catch (_com_error &e)
		{
			RethrowComErrorAsAdoException(e);
		}
	}
	void CAdoConnection::Close()
	{
		if(IsOpened())
			GetInterfacePtr()->Close();
	}

	void CAdoConnection::Close_NoThrow()
	{
		try
		{
			Close();
		}
		catch(_com_error &e)
		{
			// 오류가 나더라도 무시
			WARN_COMEXCEPTION_FOR_PROUD(e);
		}
	}

	bool CAdoConnection::IsOpened()
	{
		return ADODB::adStateClosed!=GetInterfacePtr()->State;
	}

	void CAdoRecordset::Open(ADODB::_Connection *conn,ADODB::CursorTypeEnum cursorType,ADODB::LockTypeEnum lockType,const String& lpszSQL)
	{
		try
		{
			RecommendNoCriticalSectionLock();

			CQueryChecker::ExecuteBegin();
			GetInterfacePtr()->Open(lpszSQL.GetString(), conn, cursorType, lockType, -1);
			CQueryChecker::ExecuteEnd(lpszSQL.GetString());
		}
		catch(_com_error &e)
		{
			RethrowComErrorAsAdoException(e);
		}

	}

	void CAdoRecordset::Open(ADODB::_Connection *conn,DbOpenFor openFor,const String& lpszSQL)
	{
		try
		{
			ADODB::CursorTypeEnum cursorType=cursorType=ADODB::adOpenStatic;
			ADODB::LockTypeEnum lockType=ADODB::adLockReadOnly;

			switch(openFor)
			{
			case OpenForRead:
				// 게임 서버에서 주로 사용하는 DB read access 옵션
				// MSDN says: Static cursor... provides a static copy of a set of records for you to use to find data or generate reports; always allows bookmarks and therefore allows all types of movement through the Recordset. Additions, changes, or deletions by other users will not be visible. This is the only type of cursor allowed when you open a client-side Recordset object.
				cursorType=ADODB::adOpenStatic;
				lockType=ADODB::adLockReadOnly;
				// recordset에서 보유하는 record 값들을 client에서 저장하도록 한다.
				GetInterfacePtr()->PutCursorLocation(ADODB::adUseClient);	// snapshot type으로 record를 읽기 떄문에, 이게 빠를 것이다.
				break;
			case OpenForFastRead:
				{
					// 게임 서버에서, 다수 레코드를 빨리 로딩할 때 사용하는 옵션
					// MSDN says: Static cursor... provides a static copy of a set of records for you to use to find data or generate reports; always allows bookmarks and therefore allows all types of movement through the Recordset. Additions, changes, or deletions by other users will not be visible. This is the only type of cursor allowed when you open a client-side Recordset object.
					// thus, use adOpenStatic rather than adOpenForwardOnly.
					cursorType=ADODB::adOpenStatic;
					lockType=ADODB::adLockReadOnly;
					// recordset에서 보유하는 record 값들을 client에서 저장하도록 한다.
					GetInterfacePtr()->PutCursorLocation(ADODB::adUseClient);	// snapshot type으로 record를 읽기 떄문에, 이게 빠를 것이다.
					// cache size를 조정한다.
					long max_open_rows=GetInterfacePtr()->Properties->Item[_PNT("Maximum Open Rows")];
					long cache_size;
					if(max_open_rows==0)
					{
						cache_size=defaultCacheSize;
						GetInterfacePtr()->CacheSize=cache_size;
					}
					else if(max_open_rows>=2)	// max_open_rows==1인 경우 세팅할 가치가 없다.
					{
						// ikpil.choi, min 으로 되어 있어서 PNMIN 으로 변경(종장엔 std::min 으로 변경해야 합니다)
						cache_size = PNMIN(defaultCacheSize,max_open_rows);
						GetInterfacePtr()->CacheSize=cache_size;
					}
				}
				break;
			case OpenForReadWrite:
			case OpenForAppend:
				// 게임 서버에서 주로 사용하는 DB write access 옵션
				cursorType=ADODB::adOpenStatic;
				lockType=ADODB::adLockPessimistic;
				break;
			}
			CQueryChecker::ExecuteBegin();
			GetInterfacePtr()->Open(lpszSQL.GetString(), conn, cursorType, lockType, -1);
			CQueryChecker::ExecuteEnd(lpszSQL.GetString());
		}
		catch(_com_error &e)
		{
			RethrowComErrorAsAdoException(e);
		}


	}

	void CAdoRecordset::Close_NoThrow()
	{
		try
		{
			if(IsOpened())
				Close();
		}
		catch(_com_error &e)
		{
			// 오류가 나더라도 무시
			WARN_COMEXCEPTION_FOR_PROUD(e);
		}
	}

	bool CAdoRecordset::IsOpened()
	{
		return ADODB::adStateClosed!=GetInterfacePtr()->State;
	}

	void CAdoRecordset::CopyFrom(CAdoOfflineRecord &src)
	{
		try
		{
			for (CProperty::const_iterator i = src.begin(); i != src.end(); i++)
			{
				(*this).FieldValues[i.Key.GetString()] = i.Value;
			}
		}
		catch (_com_error &e)
		{
			RethrowComErrorAsAdoException(e);
		}
	}

	void CAdoRecordset::Open(void)
	{
		try
		{
			_variant_t vNull;
			vNull.vt = VT_ERROR;
			vNull.scode = DISP_E_PARAMNOTFOUND;
			//GetInterfacePtr()->Open(vNull,vNull,ADODB::adOpenForwardOnly,ADODB::adLockReadOnly,ADODB::adCmdStoredProc);
			GetInterfacePtr()->Open(vNull,vNull,ADODB::adOpenUnspecified,ADODB::adLockUnspecified,ADODB::adCmdUnspecified);
		}
		catch(_com_error &e)
		{
			RethrowComErrorAsAdoException(e);
		}
	}

	bool CAdoRecordset::GetFieldValue(const String& pszFieldName,String &var)
	{
		try
		{
			CVariant v=FieldValues[pszFieldName];
			if(v.m_val.vt==VT_NULL)
				return false;
			else
			{
				var=(String)v;
				return true;
			}
		}
		catch(_com_error &e)
		{
			RethrowComErrorAsAdoException(e);
		}
	}

	bool CAdoRecordset::GetFieldValue(const String& pszFieldName,UUID &var)
	{
		try
		{
			CVariant v=FieldValues[pszFieldName];
			if(v.m_val.vt==VT_NULL)
				return false;
			else
			{
				var=v;
				return true;
			}
		}
		catch(_com_error &e)
		{
			RethrowComErrorAsAdoException(e);
		}
	}

	bool CAdoRecordset::GetFieldValue(const String& pszFieldName, COleDateTime &var)
	{
		try
		{
			CVariant v = FieldValues[pszFieldName];

			if (v.m_val.vt == VT_NULL)
				return false;
			else
			{
				var = v.m_val;
				return true;
			}
		}
		catch (_com_error &e)
		{
			RethrowComErrorAsAdoException(e);
		}
	}

	CVariant CAdoRecordset::GetFieldValue( const String& pszFieldName )
	{
		try
		{
			CVariant r;
			r.m_val = GetInterfacePtr()->GetCollect(pszFieldName.GetString());
			return r;
		}
		catch(_com_error &e)
		{
			Tstringstream part;
			part << (const TCHAR*)e.Description() << _PNT("(FieldName=") << pszFieldName.GetString() << _PNT(")");
			throw Exception(part.str().c_str());
		}
	}

	CVariant CAdoRecordset::GetFieldValue( int index )
	{
		try
		{
			CVariant r;
			r.m_val=GetInterfacePtr()->GetCollect((long)index);
			return r;
		}
		catch(_com_error &e)
		{
			stringstream part;
			part << StringT2A(e.Description()).GetString() << "(FieldIndex=" << index << ")";
			throw Exception(part.str().c_str());
		}
	}
	// backward compatibility to CRecordset
	void CAdoRecordset::Close()
	{
		if(IsOpened())
			GetInterfacePtr()->Close();
	}

	void CAdoRecordset::OpenForUpdate()
	{
		try
		{
			_variant_t vNull;
			vNull.vt = VT_ERROR;
			vNull.scode = DISP_E_PARAMNOTFOUND;

			GetInterfacePtr()->Open(vNull,vNull,ADODB::adOpenStatic,ADODB::adLockPessimistic,ADODB::adCmdUnspecified);
		}
		catch(_com_error &e)
		{
			RethrowComErrorAsAdoException(e);
		}
	}

	void CAdoRecordset::SetFieldValue( const String& pszFieldName,const CVariant &value )
	{
		//	NTTNTRACE("%s*****\n",StringT2A(pszFieldName));
		//	if(String(pszFieldName)==_PNT("FIELDXX"))
		//	{
		//		int a=1;
		//	}
		try
		{
			GetInterfacePtr()->PutCollect(pszFieldName.GetString(),value.m_val);
		}
		catch(_com_error &e)
		{
			stringstream part;
			part << StringT2A(e.Description()).GetString() << "(FieldName=" << pszFieldName.GetString() << ")";
			throw Exception(part.str().c_str());
		}
	}

	void CAdoRecordset::SetFieldValue( int index,const CVariant &value )
	{
		try
		{
			GetInterfacePtr()->PutCollect((long)index,value.m_val);
		}
		catch(_com_error &e)
		{
			stringstream part;
			part << StringT2A(e.Description()).GetString() << "(FieldIndex=" << index << ")";
			throw Exception(part.str().c_str());
		}
	}

	CAdoRecordset::CAdoRecordset()
	{
		try
		{
			CreateInstance(__uuidof(ADODB::Recordset));
		}
		catch(_com_error &e)
		{
			RethrowComErrorAsAdoException(e);
		}
	}

	_bstr_t CAdoRecordset::GetFieldNames( int index )
	{
		try
		{
			return GetInterfacePtr()->Fields->Item[(long)index]->Name;
		}
		catch(_com_error &e)
		{
			RethrowComErrorAsAdoException(e);
		}
	}

	void CAdoRecordset::Update()
	{
		try
		{
			GetInterfacePtr()->Update();
		}
		catch(_com_error &e)
		{
			GetInterfacePtr()->CancelUpdate();
			RethrowComErrorAsAdoException(e);
		}
	}

	void CAdoRecordset::AddNew()
	{
		try
		{
			GetInterfacePtr()->AddNew();
		}
		catch(_com_error &e)
		{
			RethrowComErrorAsAdoException(e);
		}
	}

	void CAdoRecordset::Delete()
	{
		try
		{
			GetInterfacePtr()->Delete(ADODB::adAffectCurrent);
		}
		catch(_com_error &e)
		{
			RethrowComErrorAsAdoException(e);
		}
	}

	void CAdoRecordset::MoveNext()
	{
		try
		{
			GetInterfacePtr()->MoveNext();
		}
		catch(_com_error &e)
		{
			RethrowComErrorAsAdoException(e);
		}
	}

	void CAdoRecordset::MoveFirst()
	{
		try
		{
			GetInterfacePtr()->MoveFirst();
		}
		catch(_com_error &e)
		{
			RethrowComErrorAsAdoException(e);
		}
	}

	void CAdoRecordset::MovePrevious()
	{
		try
		{
			GetInterfacePtr()->MovePrevious();
		}
		catch(_com_error &e)
		{
			RethrowComErrorAsAdoException(e);
		}
	}

	void CAdoRecordset::MoveLast()
	{
		try
		{
			GetInterfacePtr()->MoveLast();
		}
		catch(_com_error &e)
		{
			RethrowComErrorAsAdoException(e);
		}
	}

	bool CAdoRecordset::MoveNextRecordset( long *recordsAffected /*= NULL*/ )
	{
		try
		{
			CVariant ra2=0L;

			ADODB::_RecordsetPtr recordset = GetInterfacePtr()->NextRecordset(&ra2.m_val);

			if(recordset != NULL)
				Attach(recordset,true);
			else
				return false;

			if(recordsAffected)
				*recordsAffected = ra2;

			return true;
		}
		catch(_com_error &e)
		{
			RethrowComErrorAsAdoException(e);
		}
	}

	void CAdoRecordset::NextRecordset( OUT CAdoRecordset& outRecordset,OUT long *recordsAffected/*=NULL*/ )
	{
		try
		{
			CVariant ra2 = 0L;

			outRecordset = GetInterfacePtr()->NextRecordset(&ra2.m_val);

			if(recordsAffected)
				*recordsAffected = ra2;

		}
		catch(_com_error &e)
		{
			RethrowComErrorAsAdoException(e);
		}
	}
	CAdoCommand::CAdoCommand(ADODB::_Command *conn)
	{
		if(conn)
			Attach(conn,true);		// AddRef, not Attach
	}

	CAdoCommand::CAdoCommand()
	{
		try
		{
			CreateInstance(__uuidof(ADODB::Command));
		}
		catch(_com_error &e)
		{
			RethrowComErrorAsAdoException(e);
		}
	}

	void CAdoCommand::SetParam(int index, const CVariant &value)
	{
		try
		{
			GetInterfacePtr()->Parameters->Item[(long)index]->PutValue(value.m_val);
		}
		catch(_com_error &e)
		{
			RethrowComErrorAsAdoException(e);
		}
	}


	CVariant CAdoCommand::GetParam(int index)
	{
		try
		{
			CVariant r;
			r.m_val=GetInterfacePtr()->Parameters->GetItem((long)index)->GetValue();
			return r;
		}
		catch(_com_error &e)
		{
			RethrowComErrorAsAdoException(e);
		}
	}


	void CAdoCommand::Execute(OUT long *recordsAffected)
	{
		try
		{
			WarnIfParameterHasProblem();
			RecommendNoCriticalSectionLock();

			CVariant ra2=0L;

			CQueryChecker::ExecuteBegin();
			GetInterfacePtr()->Execute(&ra2.m_val, NULL, GetInterfacePtr()->CommandType);
			CQueryChecker::ExecuteEnd((const PNTCHAR*)GetInterfacePtr()->CommandText);

			if(recordsAffected)
				*recordsAffected=ra2;
		}
		catch(_com_error &e)
		{
			RethrowComErrorAsAdoException(e);
		}
	}

	void /*FASTCALL*/ CAdoCommand::Execute( OUT CAdoRecordset& outRecordset,OUT long *recordsAffected/*=NULL*/ )
	{
		try
		{
			WarnIfParameterHasProblem();
			RecommendNoCriticalSectionLock();

			CVariant ra2=0L;

			CQueryChecker::ExecuteBegin();
			outRecordset = GetInterfacePtr()->Execute(&ra2.m_val, NULL, GetInterfacePtr()->CommandType);
			CQueryChecker::ExecuteEnd((const PNTCHAR*)GetInterfacePtr()->CommandText);

			if(recordsAffected)
				*recordsAffected=ra2;
		}
		catch(_com_error &e)
		{
			RethrowComErrorAsAdoException(e);
		}
	}

	void CAdoCommand::Prepare(ADODB::_Connection* connection, const String& storedProcName, ADODB::CommandTypeEnum cmdType)
	{
		try
		{
			GetInterfacePtr()->ActiveConnection = connection;
			GetInterfacePtr()->CommandText = storedProcName.GetString();
			GetInterfacePtr()->CommandType = cmdType;
		}
		catch (_com_error &e)
		{
			RethrowComErrorAsAdoException(e);
		}
	}

	ADODB::_ParameterPtr CAdoCommand::AppendParameter(const String& paramName,
		ADODB::DataTypeEnum paramType,
		ADODB::ParameterDirectionEnum paramDirection)
	{
		try
		{
			ADODB::_ParameterPtr ret;

			ret = GetInterfacePtr()->CreateParameter(paramName.GetString(),
				paramType, paramDirection, -1, _variant_t() );
			GetInterfacePtr()->Parameters->Append(ret);

			return ret;
		}
		catch(_com_error &e)
		{
			RethrowComErrorAsAdoException(e);
		}
	}

	ADODB::_ParameterPtr CAdoCommand::AppendParameter(const String& paramName,
		ADODB::DataTypeEnum paramType,
		ADODB::ParameterDirectionEnum paramDirection,
		const CVariant &defaultValue)
	{
		try
		{
			ADODB::_ParameterPtr ret;
			ret = GetInterfacePtr()->CreateParameter(paramName.GetString(),
				paramType, paramDirection, -1, defaultValue.m_val);
			GetInterfacePtr()->Parameters->Append(ret);
			return ret;
		}
		catch (_com_error &e)
		{
			RethrowComErrorAsAdoException(e);
		}
	}

	ADODB::_ParameterPtr CAdoCommand::AppendParameter(const String& paramName,
		ADODB::DataTypeEnum paramType,
		ADODB::ParameterDirectionEnum paramDirection,
		const String& defaultValue)
	{
		try
		{
			ADODB::_ParameterPtr ret;
			ret=GetInterfacePtr()->CreateParameter(paramName.GetString(),
				paramType, paramDirection, -1, _bstr_t(defaultValue.GetString()) );
			GetInterfacePtr()->Parameters->Append(ret);
			return ret;
		}
		catch(_com_error &e)
		{
			RethrowComErrorAsAdoException(e);
		}
	}

	ADODB::_ParameterPtr CAdoCommand::AppendParameter(
		ADODB::DataTypeEnum paramType,
		ADODB::ParameterDirectionEnum paramDirection,
		const String& defaultValue)
	{
		try
		{
			ADODB::_ParameterPtr ret;
			ret = GetInterfacePtr()->CreateParameter(_bstr_t(),
				paramType, paramDirection, -1, _bstr_t(defaultValue.GetString()));
			GetInterfacePtr()->Parameters->Append(ret);
			return ret;
		}
		catch (_com_error &e)
		{
			RethrowComErrorAsAdoException(e);
		}
	}

	ADODB::_ParameterPtr CAdoCommand::AppendParameter(const String& paramName,
		ADODB::DataTypeEnum paramType,
		ADODB::ParameterDirectionEnum paramDirection,
		const String& defaultValue,
		long length)
	{
		try
		{
			ADODB::_ParameterPtr ret;
			ret = GetInterfacePtr()->CreateParameter(paramName.GetString(),
				paramType, paramDirection, length, _bstr_t(defaultValue.GetString()));
			GetInterfacePtr()->Parameters->Append(ret);
			return ret;
		}
		catch (_com_error &e)
		{
			RethrowComErrorAsAdoException(e);
		}
	}

	ADODB::_ParameterPtr CAdoCommand::AppendInputParameter( const String& paramName, const Guid& value )
	{
		return AppendParameter(paramName, ADODB::adVarWChar, ADODB::adParamInput, value.ToBracketString().GetString(), UUIDVarcharLength);
	}

	ADODB::_ParameterPtr CAdoCommand::AppendReturnValue()
	{
		return AppendParameter(_PNT(""), ADODB::adInteger, ADODB::adParamReturnValue);
	}

	// 파라메터 상태에 문제가 있으면 경고를 뿜는다. 오용을 막기 위함.
	void CAdoCommand::WarnIfParameterHasProblem()
	{
#ifdef _DEBUG
		if (GetInterfacePtr()->CommandType == ADODB::adCmdText)
		{
			int count = GetInterfacePtr()->Parameters->Count;
			for (int i = 0; i < count; i++)
			{
				ADODB::_ParameterPtr p = GetInterfacePtr()->Parameters->Item[(long)i];
				if (p != NULL && p->Size == -1)
				{
					NTTNTRACE("WARNING: You are using '?' in your query text but you are passing varchar with -1 length. "
						"That can cause trying to convert from text or ntext which cannot be done.");
				}
			}
		}
#endif // _DEBUG
	}

	void CAdoOfflineRecord::CopyFrom(CAdoRecordset &src)
	{
		try
		{
			Clear();

			ADODB::FieldsPtr fields=src->Fields;
			int c=fields->GetCount();
			CVariant v;
			for(long i=0;i<c;i++)
			{
				ADODB::FieldPtr field=fields->Item[i];
				String f_n(String((const PNTCHAR*)field->Name).MakeUpper());
				Add(f_n,field->Value);
			}
		}
		catch(_com_error &e)
		{
			RethrowComErrorAsAdoException(e);
		}
	}


	void CAdoOfflineRecordset::IterateAndCopyFrom(CAdoRecordset &source)
	{
		try
		{
			clear();
			IterateAndAppendFrom(source);
		}
		catch(_com_error &e)
		{
			RethrowComErrorAsAdoException(e);
		}
	}

	void CAdoOfflineRecordset::IterateAndAppendFrom(CAdoRecordset &source)
	{
		try
		{
			while(source.IsEOF()==false)
			{
				SetCount(GetCount()+1);
				CAdoOfflineRecord &rf=(*this)[GetCount()-1];
				rf=source;
				source->MoveNext();
			}
		}
		catch(_com_error &e)
		{
			RethrowComErrorAsAdoException(e);
		}
	}

	/** makes a string which can be used to SQL statement */
	String gds(const PNTCHAR* src)
	{
		String ret(src);
		ret.Replace(_PNT("'"),_PNT("''"));
		return ret;
	}

	/** 이 스레드에서 CADODBTemporaryBottleneckCheckDisabler 인스턴스 갯수 */
	__declspec(thread) int tls_CADODBTemporaryBottleneckCheckDisabler_count=0;

	void RecommendNoCriticalSectionLock()
	{
		if(ADODB_RefCriticalSection &&
			Proud::IsCriticalSectionLockedByCurrentThread(*ADODB_RefCriticalSection) &&
			ADODB_RecommendCriticalSectionProc && tls_CADODBTemporaryBottleneckCheckDisabler_count==0)
		{
			String r=_PNT("경고: DB를 억세스하는 중에는 Critical section lock이 걸리면 device burst time이 길어지는 동안 병목 현상이 발생할 수 있습니다.\n");
			//String adr;
			//g_CallStackDumper.GetDumped_ForGameServer(adr);
			//r+=adr;
			ADODB_RecommendCriticalSectionProc(r.GetString());
		}
	}

	CAdoDBTemporaryBottleneckCheckDisabler::CAdoDBTemporaryBottleneckCheckDisabler()
	{
		tls_CADODBTemporaryBottleneckCheckDisabler_count++;
	}

	CAdoDBTemporaryBottleneckCheckDisabler::~CAdoDBTemporaryBottleneckCheckDisabler()
	{
		tls_CADODBTemporaryBottleneckCheckDisabler_count--;
	}

	//CAdoConnectionPool::CAdoConnectionPool( String connectionString )
	//{
	//	m_connectionString=connectionString;
	//}
	//
	//CAdoConnectionPool::~CAdoConnectionPool()
	//{
	//	for(Position i=m_pool.GetHeadPosition();i;)
	//	{
	//		PoolItem &item=m_pool.GetNext(i);
	//
	//		if(item.m_inUse)
	//			throw Exception(_PNT("아직 잔존하는 connection 객체가 사용중이다!"));
	//
	//		try
	//		{
	//			item.m_conn->Close();
	//		}
	//		catch(_com_error &e)
	//		{
	//			// 오류가 나더라도 무시
	//			WARN_COMEXCEPTION_FOR_PROUD(e);
	//		}
	//
	//	}
	//}

	void CQueryChecker::ExecuteBegin()
	{
		m_prevTime = GetPreciseCurrentTimeMs();
	}

	void CQueryChecker::ExecuteEnd(const String& command)
	{
		int64_t delay = GetPreciseCurrentTimeMs() - m_prevTime;

		if (NULL != CDbmsAccessTracker::m_DbmsAcessEvent && delay >= CDbmsAccessTracker::DelayedAccessThresholdMilisec)
		{
			timespec ts;
			timespec_get_pn(&ts, TIME_UTC);

			CDbmsAccessTracker::m_DbmsAcessEvent->OnQueryDelayed(command.GetString(), ts, delay);
		}
	}

	CQueryChecker::CQueryChecker()
	{
		m_prevTime = 0;
	}

	IDbmsAccessEvent::~IDbmsAccessEvent()
	{

	}

//#pragma optimize("",on)
}

#endif //_WIN32
