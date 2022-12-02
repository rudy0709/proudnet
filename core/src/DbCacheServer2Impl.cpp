/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include "../include/AdoWrap.h"
#include "../include/coinit.h"
#include "../include/Exception.h"
#include "../include/ProudNetServer.h"
#include "../include/threadutil.h"
#include "DbCacheServer2Impl.h"
#include "DbConfig.h"
#include "RmiContextImpl.h"
#include "AsyncWork.h"
#include "DBCacheJob.h"
#include "FavoriteLV.h"
#include "quicksort.h"
#include "CriticalSectImpl.h"
#include "ExceptionImpl.h"

#ifdef _MSC_VER
#pragma warning(disable:4840)
#endif

namespace Proud
{
	const PNTCHAR* g_notSameUUIDErrorText = _PNT("Root Node is not valid! UUID, OwnerUUID, RootUUID is not same.");

	class IDbCacheNullDelegate : public IDbCacheServerDelegate2
	{
	public:
		void OnError(ErrorInfo*) {}
		void OnWarning(ErrorInfo*) {}
		void OnException(const Exception&) {}
	};
	IDbCacheNullDelegate g_nullDelegate;

	CDbCacheServer2StartParameter::CDbCacheServer2StartParameter()
	{
		m_externalNetWorkerThreadPool = NULL;
		m_externalUserWorkerThreadPool = NULL;
		m_dbWorkThreadCount = 10;
		// default 30
		m_commandTimeoutSec = 30;
		m_allowExceptionEvent = true;
		m_serializeRecursiveUpdateWork = false;
		m_autoDeisolateData = true; 
		m_delegate = &g_nullDelegate;

		m_useUtf8mb4Encoding = false;
	}

	const double AutoObliterateInterval=60 * 60 * 3; // 3시간

	CDbCacheServer2* CDbCacheServer2::New()
	{
		return new CDbCacheServer2Impl;
	}

	CDbCacheServer2Impl::CDbCacheServer2Impl(void) : m_frameMoveWorker(0)
	{
		m_saveToDbmsInterval=CDbConfig::DefaultDbmsWriteIntervalMs;
		m_unloadedHibernateDuration=CDbConfig::DefaultUnloadAfterHibernateIntervalMs;
		m_keptValForSetDefaultTimeoutTimeMs = -1;
		m_dbmsType = MsSql;
		m_allowNonExclusiveAccess = false;
		m_dbJobQueue = NULL;
		m_pdbCacheWorker = NULL;
		m_serializeRecursiveUpdateWork = false;
		m_autoDeisolateData = true;
		m_unloadRequestTimeoutTimeMs = CDbConfig::DefaultUnloadRequestTimeoutTimeMs;
		
	}

	CDbCacheServer2Impl::~CDbCacheServer2Impl(void)
	{
		Stop();
	}

	intptr_t CDbCacheServer2Impl::GetCachingDataTreeCount()
	{
		CriticalSectionLock lock(m_startOrStopCS, true);

		return m_loadedDataList.GetCount();
	}

	void CDbCacheServer2Impl::Start(CDbCacheServer2StartParameter &params)
	{
		CriticalSectionLock lock(m_startOrStopCS, true);

		//modify by rekfkno1 - free를 안할수도 있으므로 수정.
		//if(m_lanServer)
		//{
		//	ThrowInvalidArgumentException();
		//}

		//mysql이라는 글자가 들어가 있음 mysql이다..
		//이래도 되는 이유는 driver지정할때 어차피 mysql이 들어가기 때문...
		//커넥션 스트링에 mysql이라는 글자가 안들어가도 된다면 다른 방법을 사용하자.
		String strFind = params.m_DbmsConnectionString;
		strFind.MakeUpper();
		if ( -1 != strFind.Find(_PNT("MYSQL")) )
		{
			m_dbmsType = MySql;
		}

		m_dbmsConnectionString = params.m_DbmsConnectionString;
		m_useUtf8mb4Encoding = params.m_useUtf8mb4Encoding;

		//DBMS 초기 접속을 하나 해둔다.
		// ** 주의 ** Start가 실패할 경우 시작 전의 상태로 확실하게 롤백해야 한다! 유지보수시 주의할 것.
		m_initialDbmsConnection.Open(params.m_DbmsConnectionString, m_dbmsType);

		//modify by rekfkno1 - 모두 upper로 바꾼다.
		for ( int i=0; i < (int)params.m_tableNames.GetCount(); i++ )
		{
			params.m_tableNames[i].m_rootTableName.MakeUpper();

			for ( int j=0; j < (int)params.m_tableNames[i].m_childTableNames.GetCount(); j++ )
			{
				params.m_tableNames[i].m_childTableNames[j].MakeUpper();
			}
		}

		// table name list들을 검사하고 필요한 Stored Procedure를 생성한다.
		//CheckTableNames안에서 OnError를 호출할수 있기 때문에 delegate가 먼저 세팅 되어야 한다. 
		m_delegate = params.m_delegate;
		CheckTableNames(params.m_tableNames);

		m_tableNames = params.m_tableNames;
		m_dbmsConnectionString=params.m_DbmsConnectionString;
		m_authenticationKey = params.m_authenticationKey;

		m_allowNonExclusiveAccess = params.m_allowNonExclusiveAccess;
		m_saveToDbmsWorkingCount=0;
		m_commandTimeoutTime = params.m_commandTimeoutSec;
		// 		if(m_timer != NULL)
		// 			m_timer.Free();
		// 
		// 		m_timer.Attach(CMilisecTimer::New(params.m_useTimerType));
		// 		m_timer->Start();

		//GetPreciseCurrentTimeMs()=m_timer->GetTimeMs(); 

		if ( m_dbJobQueue != NULL )
		{
			m_dbJobQueue->m_listJob.Clear();
			delete m_dbJobQueue;
		}

		m_dbJobQueue = new CDbCacheJobQueue();

		{
			int threadCount = PNMAX(params.m_dbWorkThreadCount, 1);
			threadCount = PNMIN(threadCount, 512);
			m_dbmsPoolString.Format(_PNT("; Min Pool Size=%d; Max Pool Size=512; Pooling=true;"), threadCount);

			m_pdbCacheWorker = new CDBCacheWorker(threadCount, this);
			m_pdbCacheWorker->Start();
		}

		m_serializeRecursiveUpdateWork = params.m_serializeRecursiveUpdateWork;

		m_autoDeisolateData = params.m_autoDeisolateData; 

		m_dbmsConnectionString += m_dbmsPoolString;


		m_asyncLoadWorks.Attach(new CAsyncWorksOwner);

		// 서버를 시작한다.
		if ( !m_netServer )
		{
			m_netServer.Attach(CNetServer::Create());
			m_netServer->AttachProxy(&m_s2cProxy);
			m_netServer->AttachStub(this);
			m_netServer->SetEventSink(this);
			m_netServer->SetMessageMaxLength(CDbConfig::MesssageMaxLength, CDbConfig::MesssageMaxLength); // 서버간 통신이므로 1개의 메시지라도 대량의 전송이 가능해야 한다.
		}

		// 이전에 m_netServer가 생성되기 전에 void CDbCacheServer2Impl::SetDefaultTimeoutTimeSec(double newValInSec)를 호출한 적이 있어서 
		// m_keptValForSetDefaultTimeoutTimeMs에 -1 디폴트 값이 저장되어 있다면
		// m_netServer가 시작되고 난 직후인 지금 DefaultTimeoutTime에 keep해둔 값을 넣어준다.
		if (m_keptValForSetDefaultTimeoutTimeMs != -1)
			m_netServer->SetDefaultTimeoutTimeMs(m_keptValForSetDefaultTimeoutTimeMs);

		CStartServerParameter p1;
		p1.m_protocolVersion = CDbConfig::ProtocolVersion;
		p1.m_serverAddrAtClient = params.m_serverIP;
		p1.m_localNicAddr = params.m_localNicAddr;
		p1.m_tcpPorts.Add(params.m_tcpPort);
		p1.m_threadCount = CDbConfig::ServerThreadCount;

		p1.m_externalNetWorkerThreadPool = params.m_externalNetWorkerThreadPool;
		p1.m_externalUserWorkerThreadPool = params.m_externalUserWorkerThreadPool;
		p1.m_allowExceptionEvent = params.m_allowExceptionEvent;

		// 메인 타이머 루프 함수를 발동한다.
		int64_t time = CDbConfig::FrameMoveIntervalMs;
		assert(time > 10);

		//m_frameMoveWorker.Attach(CTimerQueue::Instance().NewTimer(StaticFrameMoveFunction,this,(DWORD)time));
		//m_frameMoveWorker = new CTimerThread((VOID NTAPI )CDbCacheServer2Impl::StaticFrameMoveFunction,(DWORD)time,this);
		m_frameMoveWorker = new CTimerThread(StaticFrameMoveFunction2, (uint32_t)time, this);
		m_frameMoveWorker->m_useComModel = true;
		m_frameMoveWorker->Start();

		m_netServer->Start(p1);
	}

	void CDbCacheServer2Impl::Stop()
	{
		CriticalSectionLock sslock(m_startOrStopCS, true);

		if ( m_netServer == NULL || m_frameMoveWorker == NULL )
			return;

		// 네트웍 엔진 종료
		m_netServer->Stop();

		WaitForPendedWorkFinished();

		// DBMS write 처리중인 스레드가 모두 종료할 때까지 기다린다.
		// 이제 QUWI를 콜 할 가능성이 있는 스레드가 전혀 없다. 따라서 QUWI 진행중인 스레드 갯수가 0이 되면 종료해도 안전.
		while ( m_saveToDbmsWorkingCount > 0 )
		{
			Sleep(1);

		}
		// 메인 루프 스레드 자체의 종료를 대기한다.
		//m_frameMoveWorker.Free();
		if ( m_frameMoveWorker )
		{
			m_frameMoveWorker->Stop();
			delete m_frameMoveWorker;
			m_frameMoveWorker = NULL;
		}

		CriticalSectionLock clk(m_cs, true);

		m_initialDbmsConnection.Close();

		for ( int i=0; i < m_tableNames.GetCount(); i++ )
		{
			m_tableNames[i].m_childTableNames.clear();
		}

		m_tableNames.clear();

		m_remoteClients.Clear();

		// 로딩된 객체들
		m_loadedDataList.Clear();
		m_loadedDataListBySession.Clear();
		m_asyncLoadWorks.Free();
		m_isolatedDataList.Clear();

		// DBMS에서 제거되어야 하는 Loadee(Gamer or Zone) 객체
		// m_dbmsDeleteeList.Clear();

		if ( m_pdbCacheWorker )
		{
			m_pdbCacheWorker->Stop();
			delete m_pdbCacheWorker;
			m_pdbCacheWorker = NULL;
		}

		if ( m_dbJobQueue )
		{
			m_dbJobQueue->m_listJob.Clear();
			delete m_dbJobQueue;
			m_dbJobQueue = NULL;
		}
	}


	void CDbCacheServer2Impl::OnClientJoin(CNetClientInfo *clientInfo)
	{
		// 접속해온 클라이언트에 대한 객체를 생성한다.
		CriticalSectionLock lock(m_cs, true);

		HostID clientID = clientInfo->m_HostID;
		CDbRemoteClientPtr_S newObj(new CDbRemoteClient_S);
		newObj->m_HostID=clientID;
		m_remoteClients.Add(clientID, newObj);
	}

	void CDbCacheServer2Impl::OnClientLeave(CNetClientInfo *clientInfo, ErrorInfo* /*errorInfo*/, const ByteArray& /*comment*/)
	{
		// 나가는 대응 객체를 제거한다.
		CriticalSectionLock lock(m_cs, true);

		HostID clientID = clientInfo->m_HostID;
		CDbRemoteClientPtr_S delObj;
		if ( m_remoteClients.TryGetValue(clientID, delObj) )
		{
			UnloadLoadeeByLoaderHostID(delObj->m_HostID);

			m_remoteClients.Remove(clientID);
		}

		// 격리 기능은 어떤 데이터를 어떠한 디비캐시 클라도 쓰지 말라는 의도로 있는 기능이다.
		// 그런데 그것을 요청했던 디비캐시 클라가 나갔을 경우, 그 데이터를 접근하려는 의도의 게임서버는 전혀 없음을 의미
		// 따라서 격리 해제를 해도 안전.
		// (글쎄 항상 그럴지는 모르곘지만 일단은 부작용 없는 이 방식이 더 나을 듯)
		if (m_autoDeisolateData)
		{
			for ( IsolateDataList2_S::iterator i = m_isolatedDataList.begin(); i != m_isolatedDataList.end(); )
			{
				if ( i->GetSecond()->m_requester == clientID )
				{
					i = m_isolatedDataList.erase(i);
				}
				else
					++i;
			}
		}
	}

	Proud::CDbRemoteClientPtr_S CDbCacheServer2Impl::GetRemoteClientByHostID(HostID clientID)
	{
		CDbRemoteClientPtr_S output;
		if ( !m_remoteClients.TryGetValue(clientID, output) )
			return CDbRemoteClientPtr_S();
		else
			return output;
	}

	// 대응하는 디비캐시 클라 찾기.
	Proud::CDbRemoteClientPtr_S CDbCacheServer2Impl::GetAuthedRemoteClientByHostID(HostID clientID)
	{
		CDbRemoteClientPtr_S output=GetRemoteClientByHostID(clientID);
		if ( output && output->IsLoggedOn() )
			return output;
		else return CDbRemoteClientPtr_S();
	}

	// root table 즉 LD의 최상의 노드의 type인지?
	bool CDbCacheServer2Impl::IsValidRootTable(String strRoot)
	{
		intptr_t rootTableCount = m_tableNames.GetCount();

		for ( intptr_t i=0; i < rootTableCount; ++i )
		{
			if ( m_tableNames[i].m_rootTableName == strRoot )
				return true;
		}
		return false;
	}


	void StaticFrameMoveFunction2(void* context)
	{
		CDbCacheServer2Impl* server=(CDbCacheServer2Impl*)context;
#ifdef _DEBUG
		SetThreadName(::GetCurrentThreadId(), "DbCacheServerFrameMove");  // 매우 느린 메서드이므로 릴리즈 빌드에서 즐

#endif
		try
		{
			//server->GetPreciseCurrentTimeMs() = server->m_timer->GetTimeMs();

			server->Heartbeat();
		}
		catch ( Exception &e )
		{
			server->m_delegate->OnException(e);
		}
		catch ( std::exception &e )
		{
			Exception ew(e);
			server->m_delegate->OnException(ew);
		}
		catch ( _com_error &e )
		{
			Exception ext; Exception_UpdateFromComError(ext, e);
			server->m_delegate->OnException(ext);
		}
		// 		catch (...) 사용자 콜백이 이 구간에서는 없으므로 처리 대상이 아님
		// 		{
		// 			server->m_delegate->OnUnhandledException();
		// 		}
	}

	void CDbCacheServer2Impl::Heartbeat()
	{
		EnquePendedDbmsWorksForActiveLoadees();
		//EnquePendedDbmsWorksForDeletees();
	}

	DWORD WINAPI CDbCacheServer2Impl::StaticSaveToDbms(void* context)
	{
#ifdef _DEBUG
		//SetThreadName(GetCurrentThreadId(),"ProudDB SaveToDbms");
#endif
		SaveToDbmsContext* ctx=(SaveToDbmsContext*)context;

#ifdef EXPOSE_CRASH_THAN_CONCEAL_EXCEPTION // 사용자가 핸들링하지 않은 에러는 차라리 unhandled exception으로 이어지게 해서 미니덤프를 남기는 것이 conceal보다 훨씬 문제해결을 빨리 해낸다.
		try
		{
#endif // EXPOSE_CRASH_THAN_CONCEAL_EXCEPTION
			ctx->m_server->SaveToDbms(ctx);
#ifdef EXPOSE_CRASH_THAN_CONCEAL_EXCEPTION
		}
		catch (void *e)
		{
			Exception ew(e);
			ctx->m_server->m_delegate->OnException(ew);
		}
		catch ( Exception &e )
		{
			ctx->m_server->m_delegate->OnException(e);
		}
		catch (std::exception &e)
		{
			Exception ew(e);
			ctx->m_server->m_delegate->OnException(ew);
		}
		catch (_com_error &e)
		{
			Exception ew(e);
			ctx->m_server->m_delegate->OnException(ew);
		}
#endif // EXPOSE_CRASH_THAN_CONCEAL_EXCEPTION
		//  		catch (...) 
		//  		{
		//             Exception ew;
		//             ew.m_exceptionType = ExceptionType_Unhandled;
		//  			ctx->m_server->m_delegate->OnException(ew);
		//  		}

		InterlockedDecrement(&ctx->m_server->m_saveToDbmsWorkingCount); // 이것 때문에 catch (...)가 필요

		delete ctx;

		return 0;
	}

	void CDbCacheServer2Impl::SaveToDbms(SaveToDbmsContext* ctx)
	{
		AssertIsNotLockedByCurrentThread(m_cs);
		NTTNTRACE("CDbCacheServer2Impl::SaveToDbms [0]\n");
		Guid loadeeGuid=ctx->m_exclusiveLoadeeGuid;

		uint32_t currentThreadID = ::GetCurrentThreadId();
		while (true)
		{
			// 더 이상 guid에서 저장할 것이 없을 때까지 모조리 저장 쿼리를 실행한다.
			// 단, CS unlock 상태에서 진행한다.
			CriticalSectionLock lock(m_cs, true);

			CLoadedData2Ptr_S loadee = GetLoadedDataByRootGuid(loadeeGuid);
			if ( !loadee )
			{
				// 이 과정은 객체가 deletee가 되었을 때를 위해 필요하다.
				loadee=ctx->m_deleteeData;
				if ( !loadee )
					return;
			}

			if ( loadee->m_dbmsWritePendList.GetCount() == 0 )
			{
				// 주의: 이 라인은 1회의 cs lock 중에만 해야 한다. 안그러면 두 스레드가 동시 양보로 인한 유령 잔존의 문제가 남는다.
				loadee->m_dbmsWritingThreadID=0;
				return;
			}

			// 만약 타 스레드에서 이 루틴을 실행중이면 양보하고 나가버린다. 그렇지 않으면 점유를 선언한다.
			if ( loadee->m_dbmsWritingThreadID != currentThreadID && loadee->m_dbmsWritingThreadID != 0 )
			{
				return;
			}
			else
			{
				// 주의: 이 라인은 1회의 cs lock 중에만 해야 한다. 안그러면 두 스레드가 동시 양보로 인한 유령 잔존의 문제가 남는다.
				loadee->m_dbmsWritingThreadID=currentThreadID;
			}

			DbmsWritePend2 pend = loadee->m_dbmsWritePendList.RemoveHead();
			assert(pend.m_changePropNode.m_node->UUID != Guid());

			// Unilateral Update 를 하면서 access 를 false 로 바꾸도록 수정.
			// 			//modify by rekfkno1 - ado 동작에 들어가기전에 access를 false로 바꾼다.
			// 			if(pend.m_DoneNotify)
			// 			{
			// 				if(pend.m_changePropNodeArray.GetCount() == 0)
			// 				{
			// 					CPropNodePtr updateNode = loadee->m_data.GetNode(pend.m_changePropNode.m_node.UUID);
			// 
			// 					if(updateNode)
			// 						updateNode->ClearSoftWorkIssued();
			// 				}
			// 			}

			lock.Unlock(); // 이제 DB에 기록할 것이므로 cs unlock을 한다.


			NTTNTRACE("CDbCacheServer2Impl::SaveToDbms [1]: %s\n", StringT2A(TypeToString(pend.m_changePropNode.m_type)).GetString());
			//#ifdef EXPOSE_CRASH_THAN_CONCEAL_EXCEPTION // 사용자가 핸들링하지 않은 에러는 차라리 unhandled exception으로 이어지게 해서 미니덤프를 남기는 것이 conceal보다 훨씬 문제해결을 빨리 해낸다.
			try
			{
				//#endif // EXPOSE_CRASH_THAN_CONCEAL_EXCEPTION
				String strQuery;
				switch ( pend.m_changePropNode.m_type )
				{
					case DWPNPT_AddPropNode:
					{
						// ***************
						// TODO: 이거 고치자!!!!!!!!!!!!!
						// select top 0 * 을 때려놓고서는 정작 아래에서는 insert record를 하는 SP를 호출하고 있다. 뭐하자는거야!!!
						CAdoConnection conn;
						conn.Open(m_dbmsConnectionString, m_dbmsType);
						SetAdditionalEncodingSettings(conn);

						CAdoRecordset rs;
						if ( MsSql == m_dbmsType )
							rs.Open(conn, OpenForRead, String::NewFormat(_PNT("select top 0 * from %s"), pend.m_changePropNode.m_node->m_INTERNAL_TypeName.GetString()));
						else
							rs.Open(conn, OpenForRead, String::NewFormat(_PNT("select * from %s limit 0"), pend.m_changePropNode.m_node->m_INTERNAL_TypeName.GetString()));

						CAdoCommand cmd;
						cmd->CommandTimeout = m_commandTimeoutTime;
						strQuery.Format(_PNT("pn_Add%sData"), pend.m_changePropNode.m_node->m_INTERNAL_TypeName.GetString());
						cmd.Prepare(conn, strQuery);
						if ( MsSql == m_dbmsType )
						{
							cmd.AppendReturnValue();
						}

						String strParam;
						for ( int n=0; n < rs->Fields->Count; ++n )
						{
							String strFieldName = (const PNTCHAR*)(_bstr_t)rs.FieldNames[n];
							strParam.Format(_PNT("in%s"), strFieldName.GetString());
							strFieldName.MakeUpper();

							if ( strFieldName == _PNT("ROOTUUID") )
								cmd.AppendInputParameter(_PNT("inRootUUID"), pend.m_changePropNode.m_node->RootUUID);
							else if ( strFieldName == _PNT("OWNERUUID") )
								cmd.AppendInputParameter(_PNT("inOwnerUUID"), pend.m_changePropNode.m_node->OwnerUUID);
							else if ( strFieldName == _PNT("UUID") )
								cmd.AppendInputParameter(_PNT("inUUID"), pend.m_changePropNode.m_node->UUID);
							else
							{
								ADODB::DataTypeEnum fieldType = rs->Fields->Item[(long)n]->Type;
								cmd.AppendParameter(strParam, fieldType, ADODB::adParamInput, pend.m_changePropNode.m_node->Fields[strFieldName]);
							}
						}

						rs.Close();
						cmd.Execute();
						conn.Close();
					}
						break;
					case DWPNPT_RequestUpdatePropNode:
					{
						CAdoConnection conn;
						conn.Open(m_dbmsConnectionString, m_dbmsType);
						SetAdditionalEncodingSettings(conn);

						CAdoRecordset rs;
						if ( MsSql == m_dbmsType )
							rs.Open(conn, OpenForAppend, String::NewFormat(_PNT("select * from %s where uuid='%s'"), pend.m_changePropNode.m_node->m_INTERNAL_TypeName.GetString(), pend.m_changePropNode.m_node->UUID.ToString().GetString()));
						else
							rs.Open(conn, OpenForAppend, String::NewFormat(_PNT("select * from %s where uuid='%s'"), pend.m_changePropNode.m_node->m_INTERNAL_TypeName.GetString(), pend.m_changePropNode.m_node->UUID.ToBracketString().GetString()));

						if ( rs.IsEOF() )
						{
							throw Exception("Update-UUID Matching Record Not Found");
						}

						CLoadedData2_S::DbmsSave(rs, *pend.m_changePropNode.m_node, true);

						rs.Update();
						conn.Close();
					}
						break;

					case DWPNPT_MovePropNode:
					{
						CAdoConnection conn;
						long affected = 0;
						conn.Open(m_dbmsConnectionString, m_dbmsType);
						SetAdditionalEncodingSettings(conn);

						// Unlock 상태이다 PendingList에 의존해서 DBMS에 Write에 한다.
						// 여기서는 데이터를 가져오거나 조작하는 것들 해서는 절대 안된다.
						// 언제 Unload 될지 모른다.
						CPropNodePtr srcNode = pend.m_changePropNode.m_node;
						CPropNodePtr destNode = pend.m_changePropNodeArray[0].m_node;

						// MoveNode RootUUID, OwnerUUID 변경
						if ( MsSql == m_dbmsType )
						{
							affected = conn.Execute(
								String::NewFormat(_PNT("update %s set RootUUID = '%s', OwnerUUID = '%s' where UUID = '%s'"),
								srcNode->GetStringTypeName(), destNode->RootUUID.ToString(), destNode->UUID.ToString(), srcNode->UUID.ToString()));
						}
						else
						{
							affected =conn.Execute(
								String::NewFormat(_PNT("update %s set RootUUID = '%s', OwnerUUID = '%s' where UUID = '%s'"),
								srcNode->GetStringTypeName(), destNode->RootUUID.ToBracketString(), destNode->UUID.ToBracketString(), srcNode->UUID.ToBracketString()));
						}

						if ( affected != 1 )
							throw Exception("Update-UUID Not Found");


						//하위 노드 DB적용. 
						if ( pend.m_changePropNodeArray.GetCount() > 1 && srcNode->RootUUID != destNode->RootUUID )
						{
							// srcNode->RootUUID != destNode->RootUUID 비교 구문 전에 Remove를 해버리면 다음 노드의 RootUUID와 비교하는 일이 발생한다.
							pend.m_changePropNodeArray.RemoveAt(0);
							ChangeDBRootUUID(conn, &pend.m_changePropNodeArray, destNode->RootUUID);
						}

						conn.Close();
					}
						break;

					case DWPNPT_RemovePropNode:
					{
						CAdoConnection conn;
						conn.Open(m_dbmsConnectionString, m_dbmsType);
						SetAdditionalEncodingSettings(conn);

						CAdoCommand cmd;
						cmd->CommandTimeout = m_commandTimeoutTime;
						strQuery.Format(_PNT("pn_Remove%sData"), pend.m_changePropNode.m_node->m_INTERNAL_TypeName);
						cmd.Prepare(conn, strQuery);
						if ( MsSql == m_dbmsType )
						{
							cmd.AppendReturnValue();
						}

						String strFmt;
						strFmt.Format(_PNT("in%sGuid"), pend.m_changePropNode.m_node->m_INTERNAL_TypeName);
						cmd.AppendInputParameter(strFmt, pend.m_changePropNode.m_node->UUID);

						cmd.Execute();
						conn.Close();
					}
						break;

					case DWPNPT_SetValueIf:
					{
						//중간에 바뀐것과 검사해야 하기때문에 여기서
						//얻어서 여기서 모든것을 처리해야 한다.
						if ( !pend.m_DoneNotify )
							throw Exception("SetValueIf doneNotify not Found");

						lock.Lock();

						CPropNodePtr node = loadee->m_data.GetNode(pend.m_changePropNode.m_node->UUID);

						if ( !node )
						{
							lock.Unlock();
							throw Exception("setValue final work not found updateData");
						}

						CDbWriteDoneNonExclusiveSetValueNotify* noti = (CDbWriteDoneNonExclusiveSetValueNotify*)pend.m_DoneNotify.get();

						CVariant var = node->GetField(noti->GetPropertyName());

						if ( var.IsNull() )
						{
							lock.Unlock();
							throw Exception("PropertyName Invalid");
						}

						CVariant newVar;
						if ( noti->IsCompareSuccess(var, newVar) )
						{
							node->ClearSoftWorkIssued();
							pend.m_changePropNode.m_node = node;
							pend.m_changePropNode.m_node->SetField(noti->GetPropertyName(), newVar);

							lock.Unlock();

							CAdoConnection conn;
							conn.Open(m_dbmsConnectionString, m_dbmsType);
							SetAdditionalEncodingSettings(conn);

							CAdoRecordset rs;
							if ( MsSql == m_dbmsType )
								rs.Open(conn, OpenForAppend, String::NewFormat(_PNT("select * from %s where uuid='%s'"), pend.m_changePropNode.m_node->m_INTERNAL_TypeName, pend.m_changePropNode.m_node->UUID.ToString()));
							else
								rs.Open(conn, OpenForAppend, String::NewFormat(_PNT("select * from %s where uuid='%s'"), pend.m_changePropNode.m_node->m_INTERNAL_TypeName, pend.m_changePropNode.m_node->UUID.ToBracketString()));

							if ( rs.IsEOF() )
								throw Exception("SetValueIf-UUID Matching Record Not Found");

							CLoadedData2_S::DbmsSave(rs, *pend.m_changePropNode.m_node, true);

							rs.Update();
							conn.Close();
						}
						else
						{
							lock.Unlock();
							throw Exception("Compare Failed");
						}
					}
						break;
					case DWPNPT_ModifyValue:
					{
						//중간에 바뀐것과 검사해야 하기때문에 여기서
						//얻어서 여기서 모든것을 처리해야 한다.
						if ( !pend.m_DoneNotify )
							throw Exception("ModifyValue doneNotify not Found");

						lock.Lock();

						CPropNodePtr node = loadee->m_data.GetNode(pend.m_changePropNode.m_node->UUID);

						if ( !node )
						{
							lock.Unlock();
							throw Exception("ModifyValue final work not found updateData");
						}

						CDbWriteDoneNonExclusiveModifyValueNotify* noti = (CDbWriteDoneNonExclusiveModifyValueNotify*)pend.m_DoneNotify.get();

						CVariant var = node->GetField(noti->GetPropertyName());

						if ( var.IsNull() )
						{
							lock.Unlock();
							throw Exception("PropertyName Invalid");
						}

						CVariant newVar;
						if ( noti->Operation(var, newVar) )
						{
							node->ClearSoftWorkIssued();
							pend.m_changePropNode.m_node = node;
							pend.m_changePropNode.m_node->SetField(noti->GetPropertyName(), newVar);

							lock.Unlock();

							CAdoConnection conn;
							conn.Open(m_dbmsConnectionString, m_dbmsType);
							SetAdditionalEncodingSettings(conn);

							CAdoRecordset rs;
							if ( MsSql == m_dbmsType )
								rs.Open(conn, OpenForAppend, String::NewFormat(_PNT("select * from %s where uuid='%s'"), pend.m_changePropNode.m_node->m_INTERNAL_TypeName, pend.m_changePropNode.m_node->UUID.ToString()));
							else
								rs.Open(conn, OpenForAppend, String::NewFormat(_PNT("select * from %s where uuid='%s'"), pend.m_changePropNode.m_node->m_INTERNAL_TypeName, pend.m_changePropNode.m_node->UUID.ToBracketString()));

							if ( rs.IsEOF() )
								throw Exception("ado ModifyValue failed");

							CLoadedData2_S::DbmsSave(rs, *pend.m_changePropNode.m_node, true);

							rs.Update();
							conn.Close();
						}
						else
						{
							lock.Unlock();
							throw Exception("Operation Failed");
						}
					}
						break;
					case DWPNPT_RequestUpdatePropNodeList:
					{
						SaveToDbms_Internal(pend.m_changePropNodeArray, pend.m_transaction);
					}
						break;

					case DWPNPT_UnilateralUpdatePropNode:
					{
						lock.Lock();

						CPropNodePtr node = loadee->m_data.GetNode(pend.m_changePropNode.m_node->UUID);

						/*
							노드가 존재하지 않을 수 있다.
							해당 case 에 대한 요청 후 DB 에 기록되기 전에 해당 노드의 remove 가 Unilateral 로 요청된 경우,
							pendList 해당 case 를 처리할 때 로컬에는 이미 해당 노드가 제거된 상태일 수 있다.
							그러므로 node 가 로컬에 없을 경우 Exception 을 발생시키는 것이 아니라 무시하도록 한다.
							*/
						if ( !node )
						{
							// DB 에 기록 하지 않았지만 그래도 기록했다는 노티는 보내주도록 해야 한다. 
							// 즉 맨 아래 DoneNotify 어쩌고 처리를 해야 한다. 
							//따라서 continue 대신 break.
							break;

							// 							lock.Unlock();
							// 							throw Exception("UnilateralUpdatePropNode final work not found updateData.");
						}

						/*
							ModifyValue, SetValueIf 에서 처리된 데이터의 경우 DB 에 기록하지 않기 위해.
							ModifyValue, SetValueIf 에 대한 요청이 DB 에 반영되기 전에 UnilateralUpdate 요청이 들어올 경우 데이터 불일치가 일어날 수 있다.
							ModifyValue, SetValueIf 는DB 에 기록하는 시점에서 Cache 된 최신 데이터를 이용한다.
							ModifyValue, SetValueIf 요청이 DB 에 기록되기 전에 UnilateralUpdate 요청이 들어온 경우 ModifyValue, SetValueIf 를 처리하면서
							Cache 된 최신 데이터를 이용하기 때문에 UnilateralUpdate 에 대한 적용사항이 반영되어 처리된다.
							아래에서 break 해주지 않을 경우 DB 에 ModifyValue, SetValueIf 요청에 대한 기록이 되지 않는다.
							UnilateralUpdate 에 대한 변경이 해당 변경사항을 덮어 씌어 버린다.
							*/
						if ( node->IsSoftWorkIssued() == false )
						{
							lock.Unlock();
							break;
						}

						// DB 에 Unilateral에 의한 Update 를 하기전에 SoftWorkIssued 를 Clear 한다.
						// Clear 하지 않을 시 UnilateralUpdate 후에 RequestRecursiveUpdateData 요청 시 OnAccessError 가 발생한다.
						node->ClearSoftWorkIssued();

						// 노드를 갱신 시켜 주어야 한다.
						// 갱신을 시켜주지 않을 경우, pendlist 에 UnilateralUpdate 에 대한 요청이 또 잡혀 있을경우 그 수정사항은 반영되지 않는다.
						*pend.m_changePropNode.m_node = *node;

						lock.Unlock();

						CAdoConnection conn;
						conn.Open(m_dbmsConnectionString, m_dbmsType);
						SetAdditionalEncodingSettings(conn);

						CAdoRecordset rs;
						if ( MsSql == m_dbmsType )
							rs.Open(conn, OpenForAppend, String::NewFormat(_PNT("select * from %s where uuid='%s'"), pend.m_changePropNode.m_node->m_INTERNAL_TypeName, pend.m_changePropNode.m_node->UUID.ToString()));
						else
							rs.Open(conn, OpenForAppend, String::NewFormat(_PNT("select * from %s where uuid='%s'"), pend.m_changePropNode.m_node->m_INTERNAL_TypeName, pend.m_changePropNode.m_node->UUID.ToBracketString()));

						if ( rs.IsEOF() )
							throw Exception("Update-UUID Matching Record Not Found");

						CLoadedData2_S::DbmsSave(rs, *pend.m_changePropNode.m_node, true);

						rs.Update();
						conn.Close();
					}
						break;
					case DWPNPT_UnilateralUpdatePropNodeList:
					{
						lock.Lock();


						for ( int i = 0; i < pend.m_changePropNodeArray.GetCount(); )
						{
							CPropNodePtr node = loadee->m_data.GetNode(pend.m_changePropNodeArray[i].m_node->UUID);

							/*
								노드가 존재하지 않을 수 있다.
								add, Update 에 대한 Unilateral 요청 후 DB 에 기록되기 전에 해당 노드의 remove 가 Unilateral 로 요청된 경우,
								pendList 의 add, update 를 처리할 때 로컬에는 이미 해당 노드가 제거된 상태일 수 있다.
								그러므로 node 가 로컬에 없을 경우 Exception 을 발생시키는 것이 아니라 무시하도록 한다.
								*/
							if ( !node )
							{
								if ( pend.m_changePropNodeArray[i].m_type != DWPNPT_RemovePropNode )
								{
									pend.m_changePropNodeArray.RemoveAt(i);
								}
								else
								{
									// DWPNPT_RemovePropNode 일 경우.
									i++;
								}

								continue;

								// 								lock.Unlock();
								// 								throw Exception("UnilateralUpdatePropNodeList final work not found updateData.");
							}

							/*
							이곳에 들어오는 node 의 type 은 add 또는 update 이다.
							update 일때만 체크하여 remove 해주어야 한다.
							*/
							if ( pend.m_changePropNodeArray[i].m_type == DWPNPT_UpdatePropNode && node->IsSoftWorkIssued() == false )
							{
								/*
								ModifyValue, SetValueIf 에서 처리된 데이터의 경우 DB 에 기록하지 않기 위해.
								ModifyValue, SetValueIf 에 대한 요청이 DB 에 반영되기 전에 UnilateralUpdate 요청이 들어올 경우 데이터 불일치가 일어날 수 있다.
								ModifyValue, SetValueIf 는DB 에 기록하는 시점에서 Cache 된 최신 데이터를 이용한다.
								ModifyValue, SetValueIf 요청이 DB 에 기록되기 전에 UnilateralUpdate 요청이 들어온 경우 ModifyValue, SetValueIf 를 처리하면서
								Cache 된 최신 데이터를 이용하기 때문에 UnilateralUpdate 에 대한 적용사항이 반영되어 처리된다.
								아래에서 remove 해주지 않을 경우 DB 에 ModifyValue, SetValueIf 요청에 대한 기록이 되지 않는다.
								UnilateralUpdate 에 대한 변경이 해당 변경사항을 덮어 씌어 버린다.
								*/
								pend.m_changePropNodeArray.RemoveAt(i);
							}
							else
							{
								// DB 에 Unilateral에 의한 Update 를 하기전에 SoftWorkIssued 를 Clear 한다.
								// Clear 하지 않을 시 UnilateralUpdate 후에 RequestRecursiveUpdateData 요청 시 OnAccessError 가 발생한다.
								node->ClearSoftWorkIssued();

								// 노드를 갱신 시켜 주어야 한다.
								// 갱신을 시켜주지 않을 경우, pendlist 에 UnilateralUpdate 에 대한 요청이 또 잡혀 있을경우 그 수정사항은 반영되지 않는다.
								*pend.m_changePropNodeArray[i].m_node = *node;

								// for() 에서 i 값 증가시 array 내의 모든 item 을 다 확인 하지 않는다.
								i++;
							}
						}

						lock.Unlock();

						SaveToDbms_Internal(pend.m_changePropNodeArray, pend.m_transaction);
					}
						break;
					default:
						throw Exception("bad write pend Type!");
				}

				if ( pend.m_DoneNotify )
				{
					pend.m_DoneNotify->Success(pend.m_dbClientID, *pend.m_changePropNode.m_node, pend.m_changePropNodeArray);
				}

				if ( m_netServer )
					m_s2cProxy.NotifyDbmsWriteDone(pend.m_dbClientID, g_ReliableSendForPN, pend.m_changePropNode.m_type, pend.m_changePropNode.m_node->UUID);

				m_delegate->OnDbmsWriteDone(pend.m_changePropNode.m_type);

				//#ifdef EXPOSE_CRASH_THAN_CONCEAL_EXCEPTION // 사용자가 핸들링하지 않은 에러는 차라리 unhandled exception으로 이어지게 해서 미니덤프를 남기는 것이 conceal보다 훨씬 문제해결을 빨리 해낸다.
			}
			catch ( AdoException &e )
			{
				if ( pend.m_DoneNotify )
				{
					pend.m_DoneNotify->Failed(pend.m_dbClientID, StringA2T(e.what()),
						e.m_comError.Error(), String((const PNTCHAR*)e.m_comError.Source()));
				}

				m_delegate->OnException(e);
			}
			catch ( Exception &e )
			{
				if ( pend.m_DoneNotify )
				{
					pend.m_DoneNotify->Failed(pend.m_dbClientID, StringA2T(e.what()));
				}

				m_delegate->OnException(e);
			}
			catch ( _com_error &e )
			{
				if ( pend.m_DoneNotify )
				{
					pend.m_DoneNotify->Failed(pend.m_dbClientID, String((const PNTCHAR*)e.Description()),
						e.Error(), String((const PNTCHAR*)e.Source()));
				}

				Exception ext; Exception_UpdateFromComError(ext, e);
				m_delegate->OnException(ext);
			}
			/*catch(std::exception &e)
			{
			if(pend.m_DoneNotify)
			{
			pend.m_DoneNotify->Failed(pend.m_dbClientID,StringA2T(e.what()));
			}

			Proud::Exception w(e);
			m_delegate->OnException(w);
			}*/
			//#endif // EXPOSE_CRASH_THAN_CONCEAL_EXCEPTION
			//             catch (...)
			//             {
			// 				if(pend.m_DoneNotify)
			// 				{
			// 					pend.m_DoneNotify->Failed(pend.m_dbClientID,_PNT("unexpected"));
			// 				}
			// 
			//                 if(CNetConfig::CatchUnhandledException)
			//                 {
			//                     assert(0);
			//                     Proud::Exception w;
			//                     w.m_exceptionType = ExceptionType_Unhandled;
			//                     m_delegate->OnException(w); // 어쨋거나 이것도 처리해야한다.
			//                 }
			//                 else
			//                     throw;
			//             }
		}
	}

	void CDbCacheServer2Impl::SaveToDbms_Internal(DbmsWritePropNodeListPendArray& saveArray, bool transaction)
	{
		CAdoConnection conn;
		conn.Open(m_dbmsConnectionString, m_dbmsType);
		SetAdditionalEncodingSettings(conn);

		int nCount = (int)saveArray.GetCount();
		String strQuery;

		CriticalSectionLock recursiveLock(m_recursiveUpdateLock, false);
		if ( m_serializeRecursiveUpdateWork )
			recursiveLock.Lock();

		if ( transaction )
			conn.BeginTrans();

		try
		{
			for ( int i=0; i < nCount; ++i )
			{
				switch ( saveArray[i].m_type )
				{
					case DWPNPT_AddPropNode:
					{
						CAdoRecordset rs;
						if ( MsSql == m_dbmsType )
							rs.Open(conn, OpenForRead, String::NewFormat(_PNT("select top 0 * from %s"), saveArray[i].m_node->m_INTERNAL_TypeName));
						else
							rs.Open(conn, OpenForRead, String::NewFormat(_PNT("select * from %s limit 0"), saveArray[i].m_node->m_INTERNAL_TypeName));

						CAdoCommand cmd;
						cmd->CommandTimeout = m_commandTimeoutTime;
						strQuery.Format(_PNT("pn_Add%sData"), saveArray[i].m_node->m_INTERNAL_TypeName);
						cmd.Prepare(conn, strQuery);
						//modify by rekfkno1 - 어차피 순서대로 proc를 만드므로 순서대로 넣자.
						if ( MsSql == m_dbmsType )
							cmd.AppendReturnValue();

						String strParam;
						for ( int n=0; n < rs->Fields->Count; ++n )
						{
							String strFieldName = (const PNTCHAR*)(_bstr_t)rs.FieldNames[n];
							strParam.Format(_PNT("in%s"), strFieldName);
							strFieldName.MakeUpper();

							if ( strFieldName == _PNT("ROOTUUID") )
								cmd.AppendInputParameter(_PNT("inRootUUID"), saveArray[i].m_node->RootUUID);
							else if ( strFieldName == _PNT("OWNERUUID") )
								cmd.AppendInputParameter(_PNT("inOwnerUUID"), saveArray[i].m_node->OwnerUUID);
							else if ( strFieldName == _PNT("UUID") )
								cmd.AppendInputParameter(_PNT("inUUID"), saveArray[i].m_node->UUID);
							else
								cmd.AppendParameter(strParam, rs->Fields->Item[(long)n]->Type, ADODB::adParamInput, saveArray[i].m_node->Fields[strFieldName]);
						}

						rs.Close();
						cmd.Execute();
					}
						break;
					case DWPNPT_RemovePropNode:
					{
						CAdoCommand cmd;
						cmd->CommandTimeout = m_commandTimeoutTime;
						strQuery.Format(_PNT("pn_Remove%sData"), saveArray[i].m_node->m_INTERNAL_TypeName);
						cmd.Prepare(conn, strQuery);
						if ( MsSql == m_dbmsType )
							cmd.AppendReturnValue();

						String strFmt;
						strFmt.Format(_PNT("in%sGuid"), saveArray[i].m_node->m_INTERNAL_TypeName);
						cmd.AppendInputParameter(strFmt.GetString(), saveArray[i].m_node->UUID);

						cmd.Execute();
					}
						break;
					case DWPNPT_UpdatePropNode:
					{
						CAdoRecordset rs;
						if ( MsSql == m_dbmsType )
							rs.Open(conn, OpenForAppend, String::NewFormat(_PNT("select * from %s where uuid='%s'"), saveArray[i].m_node->m_INTERNAL_TypeName, saveArray[i].m_node->UUID.ToString()));
						else
							rs.Open(conn, OpenForAppend, String::NewFormat(_PNT("select * from %s where uuid='%s'"), saveArray[i].m_node->m_INTERNAL_TypeName, saveArray[i].m_node->UUID.ToBracketString()));

						if ( rs.IsEOF() )
							break;

						CLoadedData2_S::DbmsSave(rs, *saveArray[i].m_node, true);

						rs.Update();
					}
						break;
				}
			}
		}
		catch ( Exception &e )
		{
			if ( transaction )
				conn.RollbackTrans();

			throw e;
		}
		catch ( std::exception &e )
		{
			if ( transaction )
				conn.RollbackTrans();

			throw Proud::Exception(e);
		}
		catch ( _com_error &e )
		{
			if ( transaction )
				conn.RollbackTrans();

			Exception ext; Exception_UpdateFromComError(ext, e);
			throw Proud::Exception(ext);
		}

		if ( transaction )
		{
			conn.CommitTrans();
		}

		conn.Close();
	}


	void CDbCacheServer2Impl::SetDbmsWriteIntervalMs(int64_t val)
	{
		CriticalSectionLock lock(m_cs, true);
		if ( val < 100 )
		{
			ThrowInvalidArgumentException();
		}
		m_saveToDbmsInterval=val;
	}

	void CDbCacheServer2Impl::SetUnloadedDataHibernateDurationMs(int64_t val)
	{
		CriticalSectionLock lock(m_cs, true);
		if ( val < 0 )
		{
			ThrowInvalidArgumentException();
		}
		m_unloadedHibernateDuration=val;
	}

	void CDbCacheServer2Impl::WaitForPendedWorkFinished()
	{
		// 모든 미처리 DBMS 기록을 즉시 수행하도록 지정한다. 
		{
			CriticalSectionLock lock2(m_cs, true);
			for ( LoadedDataList2_S::iterator i=m_loadedDataList.begin(); i != m_loadedDataList.end(); i++ )
			{
				CLoadedData2Ptr_S gd=i->GetSecond();
				if ( gd->m_lastModifiedTime > 0 )
				{
					gd->m_lastModifiedTime=min(gd->m_lastModifiedTime, GetPreciseCurrentTimeMs() - m_saveToDbmsInterval - OneSecond);
				}
			}
		}
		// 모든 미처리 기록이 다 기록될 때까지 대기

		while ( 1 )
		{
			{
				bool waitMore=false;

				CriticalSectionLock lock2(m_cs, true);
				for ( LoadedDataList2_S::iterator i=m_loadedDataList.begin(); i != m_loadedDataList.end(); i++ )
				{
					CLoadedData2Ptr_S gd=i->GetSecond();
					if ( gd->m_dbmsWritePendList.GetCount() > 0 )
					{
						waitMore=true;
						break;
					}

				}

				if ( !waitMore )
					break;
			}
			Sleep(50);
		}

		// pended read work가 모두 끝날때까지 기다려야.
		while ( 1 )
		{
			if ( m_asyncLoadWorks->IsFinished() )
				break;

			Sleep(50);
		}

	}

	// 각 data tree를 순회하면서, DB save work가 있으면 그것들을 비동기 작업시작을 지시한다.
	// 아무도 안쓰는 data tree 상태에서 일정 시간이 지난 것들을 메모리에서 제거하는 역할도 한다.
	void CDbCacheServer2Impl::EnquePendedDbmsWorksForActiveLoadees()
	{
		// DBMS write 비동기 큐를 처리한다.
		CriticalSectionLock lock(m_cs, true);

		int64_t nowTime = GetPreciseCurrentTimeMs();

		int64_t loadTimeout = m_unloadRequestTimeoutTimeMs;

		// 각 loadee에 대해...
		for ( LoadedDataList2_S::iterator i=m_loadedDataList.begin(); i != m_loadedDataList.end(); )
		{
			CLoadedData2Ptr_S ld=i->second;
			CIsolateRequestPtr isolateRequest;
			m_isolatedDataList.TryGetValue(i->GetFirst(), isolateRequest);

			// 독점권 이양 요청의 타임아웃 여부 검사.
			// 순서대로 들어가므로 head의 시간만 검사하면 된다.
			while ( ld->m_unloadRequests.GetCount() > 0 )
			{
				CLoadRequestPtr& req = ld->m_unloadRequests.GetHead();
				if ( nowTime - req->m_requestTimeMs > loadTimeout )
				{
					// 타임아웃 처리 
					CFailInfo_S info;
					info.m_uuid = ld->m_data.GetRootUUID();
					info.m_reason = ErrorType_TimeOut;
					ProcessLoadDataFail(req, info);

					// 목록에서 뺀다.
					ld->m_unloadRequests.RemoveHead();
				}
				else // head가 아직 타임아웃이 아니면 뒤에 있는 놈들도 무조건 아니다.
					break;
			}


			// 최근 갱신 후 충분한 시간이 지났으면 DBMS에 flush한다.
			if ( ld->m_lastModifiedTime && nowTime - ld->m_lastModifiedTime > m_saveToDbmsInterval )
			{
				// per-loadee work item으로서 추가를 한다.
				// 단, 이미 해당 loadee 에 대해서 처리를 하는 중이면 queue user work item을 하지 않는다.
				InterlockedIncrement(&m_saveToDbmsWorkingCount);

				ld->m_lastModifiedTime = 0;
				SaveToDbmsContext* saveToDbmsContext=new SaveToDbmsContext;
				saveToDbmsContext->m_exclusiveLoadeeGuid=i->GetFirst();
				saveToDbmsContext->m_server=this;

				//QueueUserWorkItem(StaticSaveToDbms,saveToDbmsContext,WT_EXECUTEDEFAULT);
				m_dbJobQueue->PushJob(CDbCacheJobQueue::JobType_SaveToDbmsContext, saveToDbmsContext);
			}

			bool eraseIter=false;

			// 오랫동안 휴면 상태 혹은 격리 처리된 객체를 DBMS에 즉시 기록을 하면서 unload한다.
			// 조건: unload 후 타임 아웃 및 객체에 대한 비동기 쿼리 실행이 모두 끝나기
			if ( (ld->m_hibernateStartTimeMs != 0 && nowTime - ld->m_hibernateStartTimeMs > m_unloadedHibernateDuration)
				|| (isolateRequest != NULL ) // 격리조치된 경우
				)
			{
				// 아직 기록 타임이 아니더라도 기록할게 남아있으면 당장 기록하도록 강제한다.
				if ( ld->m_lastModifiedTime != 0 )
				{
					ld->m_lastModifiedTime = nowTime - m_saveToDbmsInterval - OneSecond;
				}
				// 이미 기록이 다 마친 상태이고 DBMS 기록 상태도 더 이상 아니면 제거(complete unload)하도록 한다.
				if ( ld->m_lastModifiedTime == 0 && ld->m_dbmsWritePendList.GetCount() == 0 && ld->m_dbmsWritingThreadID == 0 )
				{
					eraseIter=true;
				}
			}

			if ( eraseIter )
			{			
				NTTNTRACE("%s: Loadee hybernate to unload\n", __FUNCTION__);

				// 격리된 데이터인 경우 RequestIsolateData 요청에 대한 응답을 보낸다.
				if ( isolateRequest != NULL )
				{
					ByteArray isolateDataBlock;
					ld->m_data.ToByteArray(isolateDataBlock);
					m_s2cProxy.NotifyIsolateDataSuccess(
						isolateRequest->m_requester,
						g_ReliableSendForPN,
						isolateRequest->m_session,
						i->GetFirst(), 
						isolateDataBlock
						);
				}

				// 로드 목록에서 제거
				i = m_loadedDataList.erase(i);
			}	
			else
			{
				++i;
			}
		}
	}

	//void CDbCacheServer2Impl::EnquePendedDbmsWorksForDeletees()
	//{
	//       // DBMS write 비동기 큐를 처리한다.
	//       // 단, loadee 자체를 DBMS delete를 하는 케이스를 처리한다.
	//       CriticalSectionLock lock(m_cs,true);

	//       while(m_dbmsDeleteeList.GetCount()>0)
	//       {
	//           CLoadedData2Ptr_S ld=m_dbmsDeleteeList.begin()->second;

	//           // per-loadee work item으로서 추가를 한다.
	//           // 단, 이미 해당 loadee 에 대해서 처리를 하는 중이면 queue user work item을 하지 않는다.
	//           int32_t oldCount=InterlockedIncrement(&m_saveToDbmsWorkingCount);

	//           SaveToDbmsContext* saveToDbmsContext=new SaveToDbmsContext;
	//           saveToDbmsContext->m_deleteeData = ld;
	//           saveToDbmsContext->m_server=this;

	//           QueueUserWorkItem(StaticSaveToDbms,saveToDbmsContext,WT_EXECUTEDEFAULT);
	//           m_dbmsDeleteeList.erase(m_dbmsDeleteeList.begin());
	//       }
	//}

	void CDbCacheServer2Impl::SetDefaultTimeoutTimeMs(int newValInMs)
	{
		// m_netServer가 생성되기 전에 이 함수를 호출한 것이라면
		// m_keptValForSetDefaultTimeoutTimeMs에 DefaultTimeoutTime으로 지정해주려는 값을 보관한다.
		// m_netServer가 시작된 상태라면 바로 지정해준다.

		if (m_netServer)
			m_netServer->SetDefaultTimeoutTimeMs(newValInMs);
		else
			m_keptValForSetDefaultTimeoutTimeMs = newValInMs;
	}

	void CDbCacheServer2Impl::SetDefaultTimeoutTimeSec(double newValInSec)
	{
		int newValInMs = DoubleToInt(newValInSec * 1000);
		SetDefaultTimeoutTimeMs(newValInMs);
	}

	void CDbCacheServer2Impl::SetUnloadRequestTimeoutTimeMs(int64_t timeoutMs) 
	{
		if (timeoutMs < 0)
		{
			stringstream ss;
			ss << "invalid argument : " << timeoutMs;
			throw Exception(ss.str().c_str());
		}

		// Unload Request Timeout 제한 수정
		m_unloadRequestTimeoutTimeMs = timeoutMs;

		CriticalSectionLock lock(m_startOrStopCS, true);

		// 모든 클라이언트에게 통보한다.
		if ( m_netServer != NULL )
		{
			int clientCount = m_netServer->GetClientCount();
			if (clientCount > 0)
			{
				// NotifyDataUnloadRequested를 통해서도 통보하기때문에
				// 늦게 들어온 몇몇 클라이언트가 수신을 못하더라도 문제는 없다.
				CFastArray<HostID> clients;
				clients.SetCount(clientCount);
				clientCount = m_netServer->GetClientHostIDs(clients.GetData(), clientCount);

				// 최신의 값만이 유효하므로 UniqueID를 사용하고,
				// 그 값은 서버가 구동 중인 동안 불변하는 m_unloadRequestTimeoutTimeMs의 주소값으로 한다.
				RmiContext rmictx = g_ReliableSendForPN;
				rmictx.m_uniqueID = (int64_t) & m_unloadRequestTimeoutTimeMs;
				m_s2cProxy.NotifyUnloadRequestTimeoutTimeMs(clients.GetData(), 
															clientCount, 
															rmictx, 
															(int64_t)m_unloadRequestTimeoutTimeMs);
			}
		}
	}

	int64_t CDbCacheServer2Impl::GetUnloadRequestTimeoutTimeMs() const
	{
		return m_unloadRequestTimeoutTimeMs;
	}

	void CDbCacheServer2Impl::CheckTableNames(CFastArray<CCachedTableName>& dbCache2Tables)
	{
		// 적어도 하나 이상이여야 한다.
		if ( dbCache2Tables.GetCount() == 0 )
		{
			ErrorInfoPtr err =  ErrorInfo::From(ErrorType_Unexpected, HostID_Server, _PNT("Table not have!"));
			OnError(err);
			return;
		}

		intptr_t rootTableCount = dbCache2Tables.GetCount();

		// Root - Root 중복 체크				
		for ( intptr_t i=0; i < rootTableCount; i++ )
		{
			for ( intptr_t j = 0; j < rootTableCount; j++ )
			{
				if ( dbCache2Tables[i].m_rootTableName == dbCache2Tables[j].m_rootTableName && i != j )
				{
					ErrorInfoPtr err =  ErrorInfo::From(ErrorType_Unexpected, HostID_Server, _PNT("No table name must be exist in both rootTableNames and childTableNames!"));
					OnError(err);
					return;
				}
			}
		}

		// Root - Child 중복 체크
		for ( intptr_t i=0; i < rootTableCount; i++ )
		{
			for ( intptr_t j=0; j < rootTableCount; j++ )
			{
				intptr_t childTableCount = dbCache2Tables[j].m_childTableNames.GetCount();

				for ( intptr_t k=0; k < childTableCount; k++ )
				{
					if ( dbCache2Tables[i].m_rootTableName == dbCache2Tables[j].m_childTableNames[k] )
					{
						ErrorInfoPtr err =  ErrorInfo::From(ErrorType_Unexpected, HostID_Server, _PNT("No table name must be exist in both rootTableNames and childTableNames!"));
						OnError(err);
						return;
					}
				}
			}

		}

		// Create Root Table Store Procedure
		for ( CFastArray<CCachedTableName>::iterator itr = dbCache2Tables.begin(); itr != dbCache2Tables.end(); ++itr )
		{
			String rootTableName = (*itr).m_rootTableName;

			CreateProcedure(rootTableName, &((*itr).m_childTableNames));
		}

		// Create Child Table Store Procedure
		for ( int i=0; i < dbCache2Tables.GetCount(); i++ )
		{
			for ( int j = 0; j < dbCache2Tables[i].m_childTableNames.GetCount(); j++ )
			{
				String childTableName = dbCache2Tables[i].m_childTableNames[j];

				CreateProcedure(childTableName);
			}
		}
	}

	void CDbCacheServer2Impl::CreateProcedure(const String& tableName, CFastArray<String>* childTableNames)
	{
		String strQuery;
		CAdoRecordset rs;
		String strFieldNames;
		CFastArray<String> strTypeNames;

		// m_initialDbmsConnection.BeginTrans(); DCL에 웬 transaction?

		if ( MsSql == m_dbmsType )
		{
			// 테이블 검사및 필요 필드 검사
			m_initialDbmsConnection.Execute(String::NewFormat(_PNT("Select top 0 UUID,OwnerUUID,RootUUID from %s"), tableName));

			if ( childTableNames != NULL && childTableNames->GetCount() != 0 )
			{
				//pn_GetAll이 있다면 지운다.
				strQuery.Format(_PNT("if exists(select * from dbo.sysobjects where id = object_id(N'[dbo].[pn_Get%sAllData]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)\n")
					_PNT("drop procedure [dbo].[pn_Get%sAllData]"),
					tableName, tableName);

				m_initialDbmsConnection.Execute(strQuery);

				String childSelects;
				CFastArray<String>::iterator it = (*childTableNames).begin();
				for ( ; it != (*childTableNames).end(); ++it )
				{
					String selectExp;
					selectExp.Format(_PNT("select * from %s with(nolock) where RootUUID=@RootGuid\n"), *it);

					childSelects.Append(selectExp);
				}

				childSelects.Append(_PNT("return 0;\nEND\n"));

				//pn_GetAll을 만든다
				//최상위 로드 데이터의 uuid로 검색하는걸 만든다.
				strQuery.Format(_PNT("CREATE PROCEDURE [dbo].[pn_Get%sAllData]\n")
					_PNT("@RootGuid uniqueidentifier\n")
					_PNT("AS\n")
					_PNT("BEGIN\n")
					_PNT("SET NOCOUNT ON;\n"),
					tableName);

				strQuery.Append(childSelects);

				m_initialDbmsConnection.Execute(strQuery);
			}

			//pn_Remove가 있다면 지운다.
			strQuery.Format(_PNT("if exists(select * from dbo.sysobjects where id = object_id(N'[dbo].[pn_Remove%sData]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)\n")
				_PNT("drop procedure [dbo].[pn_Remove%sData]"),
				tableName, tableName);

			m_initialDbmsConnection.Execute(strQuery);

			//pn_Remove를 만든다.
			strQuery.Format(_PNT("CREATE PROCEDURE [dbo].[pn_Remove%sData]\n")
				_PNT("@%sGuid uniqueidentifier\n")
				_PNT("AS\n")
				_PNT("BEGIN\n")
				_PNT("SET NOCOUNT ON;\n")
				_PNT("delete from %s where UUID=@%sGuid\n")
				_PNT("return 0;\n")
				_PNT("END"),
				tableName, tableName, tableName, tableName);

			m_initialDbmsConnection.Execute(strQuery);

			//pn_Add가 있다면 지운다.
			strQuery.Format(_PNT("if exists(select * from dbo.sysobjects where id = object_id(N'[dbo].[pn_Add%sData]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)\n")
				_PNT("drop procedure [dbo].[pn_Add%sData]"),
				tableName, tableName);

			m_initialDbmsConnection.Execute(strQuery);

			//mssql 2000을 위해변경.
			rs.Open(m_initialDbmsConnection, OpenForRead, String::NewFormat(
				_PNT("select INFORMATION_SCHEMA.COLUMNS.COLUMN_NAME as ColumnName,INFORMATION_SCHEMA.COLUMNS.DATA_TYPE as ColumnTypeName,syscolumns.length as Length ")
				_PNT("from INFORMATION_SCHEMA.COLUMNS,syscolumns where ")
				_PNT("INFORMATION_SCHEMA.COLUMNS.TABLE_NAME ='%s' and ")
				_PNT("OBJECT_ID('%s') = syscolumns.id and ")
				_PNT("INFORMATION_SCHEMA.COLUMNS.COLUMN_NAME = syscolumns.name"),
				tableName, tableName));

			String strParam;
			String strValues;
			String strType;
			String ColumnName;
			String strParams;

			while ( !rs.IsEOF() )
			{
				for ( int i=0; i < rs->Fields->Count; i++ )
				{
					CVariant fieldValue = rs.FieldValues[i];
					String fieldName=(const PNTCHAR*)(_bstr_t)rs.FieldNames[i];

					if ( fieldName == _PNT("ColumnName") )
					{
						ColumnName = (String)fieldValue;
						strFieldNames += ColumnName;
					}
					else if ( fieldName == _PNT("ColumnTypeName") )
					{
						strParam.Format(_PNT("@%s%s "), tableName, ColumnName);
						strValues += strParam;
						strType = (String)fieldValue;
						strParam += strType;
					}
					else if ( fieldName == _PNT("Length") )
					{
						int nLength = (int)fieldValue;

						if ( strType == _PNT("varchar") ||
							strType == _PNT("nchar") ||
							strType == _PNT("binary") ||
							strType == _PNT("char") ||
							strType == _PNT("nvarchar") ||
							strType == _PNT("varbinary") )
						{
							String strTemp;
							if ( -1 != nLength )
								strTemp.Format(_PNT("(%d)"), nLength);
							else
								strTemp = _PNT("(MAX)");

							strParam += strTemp;
						}

						strParams += strParam;
					}
				}

				rs.MoveNext();

				if ( !rs.IsEOF() )
				{
					strParams += _PNT(",\n");
					strValues += _PNT(",");
					strFieldNames += _PNT(",");
				}
			}

			rs.Close();

			////pn_Add를 만든다.
			strQuery =String(_PNT("CREATE PROCEDURE [dbo].[pn_Add")) +
				tableName +
				String(_PNT("Data]\n")) +
				strParams +
				String(_PNT("\n")
				_PNT("AS\n")
				_PNT("BEGIN\n")
				_PNT("SET NOCOUNT ON;\n")
				_PNT("if exists (select UUID from ")) +
				tableName +
				String(_PNT(" where UUID=@")) +
				tableName +
				String(_PNT("UUID)\n")
				_PNT("begin\n")
				_PNT("return -1;\n")
				_PNT("end\n")
				_PNT("insert into ")) +
				tableName +
				String(_PNT("(")) +
				strFieldNames +
				String(_PNT(") values (")) +
				strValues +
				String(_PNT(")\n")
				_PNT("return 0;\n")
				_PNT("END"));


			m_initialDbmsConnection.Execute(strQuery);
			strFieldNames = _PNT("");

		}
		else //mysql
		{
			// 테이블 검사및 필요 필드 검사

			m_initialDbmsConnection.Execute(String::NewFormat(_PNT("Select UUID,OwnerUUID,RootUUID from %s limit 0"), tableName));

			if ( childTableNames != NULL && childTableNames->GetCount() != 0 )
			{
				//pn_GetAll이 있다면 지운다.
				m_initialDbmsConnection.Execute(String::NewFormat(_PNT("DROP PROCEDURE IF EXISTS `pn_Get%sAllData`"), tableName));

				//pn_GetAll을 만든다.
				//최상위 로드 데이터의 uuid로 검색하는걸 만든다.
				strQuery.Format(_PNT("CREATE PROCEDURE `pn_Get%sAllData`(\n")
					_PNT("IN inRootGuid varchar(38)\n")
					_PNT(")\n")
					_PNT("BEGIN\n"),
					tableName);

				String childSelects;
				CFastArray<String>::iterator it = (*childTableNames).begin();
				for ( ; it != (*childTableNames).end(); ++it )
				{
					String selectExp;
					selectExp.Format(_PNT("select * from %s where RootUUID=inRootGuid;\n"), *it);
					childSelects.Append(selectExp);
				}

				childSelects.Append(_PNT("END\n"));

				strQuery.Append(childSelects);

				m_initialDbmsConnection.Execute(strQuery);
			}

			m_initialDbmsConnection.Execute(String::NewFormat(_PNT("DROP PROCEDURE IF EXISTS `pn_Remove%sData`"), tableName));

			//pn_Remove를 만든다.
			strQuery.Format(_PNT("CREATE PROCEDURE `pn_Remove%sData`(\n")
				_PNT("IN in%sGuid varchar(38)\n")
				_PNT(")\n")
				_PNT("BEGIN\n")
				_PNT("delete from %s where UUID=in%sGuid;\n")
				_PNT("END\n"),
				tableName, tableName, tableName, tableName);

			m_initialDbmsConnection.Execute(strQuery);

			//pn_Add가 있다면 지운다.
			m_initialDbmsConnection.Execute(String::NewFormat(_PNT("DROP PROCEDURE IF EXISTS `pn_Add%sData`"), tableName));

			//pn_Add를 위해 필드명,필드타입명,length를 얻어온다.
			rs.Open(m_initialDbmsConnection, OpenForRead, String::NewFormat(_PNT("SHOW COLUMNS FROM %s"), tableName));

			String strParam;
			String strValues;
			String strType;
			String ColumnName;
			String strParams;

			while ( !rs.IsEOF() )
			{
				for ( int i=0; i < rs->Fields->Count; i++ )
				{
					CVariant fieldValue = rs.FieldValues[i];
					String fieldName=(const PNTCHAR*)(_bstr_t)rs.FieldNames[i];

					if ( fieldName == _PNT("Field") )
					{
						ColumnName = (String)fieldValue;
						strFieldNames += ColumnName;
						strValues += _PNT("in");
						strValues+=(String)fieldValue;
					}
					else if ( fieldName == _PNT("Type") )
					{
						strParams += _PNT("IN in");
						strParams += ColumnName;
						strParams += _PNT(" ");
						strParams += (String)fieldValue;
					}

				}

				rs.MoveNext();

				if ( !rs.IsEOF() )
				{
					strParams += _PNT(",\n");
					strValues += _PNT(",");
					strFieldNames += _PNT(",");
				}
			}

			rs.Close();

			////pn_Add를 만든다.
			strQuery = String(_PNT("CREATE PROCEDURE `pn_Add")) +
				tableName +
				String(_PNT("Data`(\n")) +
				strParams +
				String(_PNT("\n")
				_PNT(")\n")
				_PNT("BEGIN\n")
				_PNT("set @result = 0;\n")
				_PNT("select count(UUID) into @result from ")) +
				tableName +
				String(_PNT(" where UUID=inUUID;\n")
				_PNT("if(@result <> 0) then\n")
				_PNT("select -1;\n")
				_PNT("else\n")
				_PNT("insert into ")) +
				tableName +
				String(_PNT("(")) +
				strFieldNames +
				String(_PNT(") values(")) +
				strValues +
				String(_PNT(");\n")
				_PNT("select 0;\n")
				_PNT("end if;\n")
				_PNT("END"));


			m_initialDbmsConnection.Execute(strQuery);
			strFieldNames = _PNT("");
		}

		//m_initialDbmsConnection.CommitTrans();
	}

	void CDbCacheServer2Impl::OnWarning(Proud::ErrorInfo *errorInfo)
	{
		NTTNTRACE("DB cache server에서 경고 발생: %s\n", StringT2A(errorInfo->ToString()));

		m_delegate->OnWarning(errorInfo);
	}

	void CDbCacheServer2Impl::OnError(ErrorInfo *errorInfo)
	{
		NTTNTRACE("DB cache server에서 오류 발생: %s\n", StringT2A(errorInfo->ToString()));

		m_delegate->OnError(errorInfo);

	}

	void CDbCacheServer2Impl::OnException(const Exception &e)
	{
		NTTNTRACE("DB cache server에서 Exception 발생: %s\n", e.what());

		m_delegate->OnException(e);
	}

	// Root UUID를 키값으로 해서 빠른 속도로 loaded data tree를 찾는다.
	Proud::CLoadedData2Ptr_S CDbCacheServer2Impl::GetLoadedDataByRootGuid(const Guid &guid)
	{
		if ( guid == Guid() )
			return CLoadedData2Ptr_S();

		CriticalSectionLock lock(m_cs, true);

		CLoadedData2Ptr_S output;
		if ( m_loadedDataList.TryGetValue(guid, output) )
			return output;
		else
			return CLoadedData2Ptr_S();
	}

	Proud::CLoadedData2Ptr_S CDbCacheServer2Impl::GetLoadedDataBySessionGuid(const Guid &guid)
	{
		if ( guid == Guid() )
			return CLoadedData2Ptr_S();

		CriticalSectionLock lock(m_cs, true);

		LoadedDataList2_S::iterator itr = m_loadedDataListBySession.find(guid);

		if ( itr != m_loadedDataListBySession.end() )
			return itr->GetSecond();

		return CLoadedData2Ptr_S();
	}

	// DB에서 LD를 로딩한다. 이 함수는 no lock 상태에서 수행되어야 한다.
	bool CDbCacheServer2Impl::LoadDataFromDbms(String roottableName, Guid rootUUID, CLoadedData2Ptr_S &outData)
	{
		String searhString;
		String comment;

		if ( MsSql == m_dbmsType )
		{
			searhString.Format(_PNT("uuid='%s'"), rootUUID.ToString());
		}
		else
		{
			searhString.Format(_PNT("uuid='%s'"), rootUUID.ToBracketString());
		}

		return LoadDataFromDbms(roottableName, searhString, outData, comment);
	}

	// root table을 로딩한다.
	bool CDbCacheServer2Impl::LoadDataFromDbms(String roottableName,
		String searchString,
		CLoadedData2Ptr_S &outData,
		String& comment)
	{
		AssertIsNotLockedByCurrentThread(m_cs);
		CLoadedData2Ptr_S loadedData(new CLoadedData2_S(this));
		try
		{
			CAdoConnection conn;

			// 측정 결과, 처음에는 여기가 2초 이상 걸리지만, ADO 자체의 connection pool기능 덕택에(Proud.CDbCacheServer2Impl.m_initialDbmsConnection가 있어서)
			// 이후부터는 매우 빨리 이 함수가 끝난다. 따라서 여기서 처음에는 2초가 걸려도 문제시 될 것 없다.
			conn.Open(m_dbmsConnectionString, m_dbmsType);
			SetAdditionalEncodingSettings(conn);

			// 정보 로딩
			CAdoRecordset rs;

			rs.Open(conn, OpenForRead, String::NewFormat(_PNT("select * from %s where %s"), roottableName, searchString));

			CPropNodePtr rootNode = CPropNodePtr(new CPropNode(roottableName));
			CLoadedData2_S::DbmsLoad(rs, *rootNode);

			if ( rootNode->UUID == Guid() || rootNode->OwnerUUID == Guid() || rootNode->RootUUID == Guid() )
				throw Exception("UserDefine Record has NULL GUID!");

			// Root 테이블은 모든 UUID가 같아야한다.   
			if ( rootNode->UUID == rootNode->OwnerUUID &&
				rootNode->OwnerUUID == rootNode->RootUUID )
			{
				loadedData->m_data.InsertChild(CPropNodePtr(), rootNode);
				loadedData->m_data._INTERNAL_ClearChangeNode();
			}
			else
				throw Exception(g_notSameUUIDErrorText);

			rs.Close();

			// Child Table이 없는 경우 바로 리턴한다.
			if ( HasNoChildTable(roottableName) )
			{
				outData = loadedData;
				return true;
			}

			// root Node과 관련된 모든 하위노드들을 로딩한다.
			if ( !LoadDataForChildTablesFromDbms(conn, roottableName, rootNode->UUID, loadedData->GetData()) )
				return false;

			// 마무리
			outData = loadedData;
		}
		catch ( _com_error &e )
		{
			Exception ext; Exception_UpdateFromComError(ext, e);
			comment = (const PNTCHAR*)e.Description();
			m_delegate->OnException(ext);
			return false;
		}
		catch ( Exception &e )
		{
			m_delegate->OnException(e);
			return false;
		}

		return true;
	}

	// DB2C2S
	DEFRMI_DB2C2S_RequestDbCacheClientLogon(CDbCacheServer2Impl)
	{
		rmiContext;

		CriticalSectionLock lock(m_cs, true);

		// 클라 객체 찾기
		CDbRemoteClientPtr_S rc = GetRemoteClientByHostID(remote);
		if ( !rc || rc->IsLoggedOn() )
			return true;

		if ( authenticationKey != m_authenticationKey )
			m_s2cProxy.NotifyDbCacheClientLogonFailed(remote, g_ReliableSendForPN, ErrorType_InvalidSessionKey);

		// 인증 완료 처리를 한다.
		rc->m_logonTime = GetPreciseCurrentTimeMs();

		m_s2cProxy.NotifyDbCacheClientLogonSuccess(remote, g_ReliableSendForPN);

		return true;
	}

	DEFRMI_DB2C2S_RequestAddData(CDbCacheServer2Impl)
	{
		rmiContext;
		ownerUUID;

		CriticalSectionLock lock(m_cs, true);

		CDbRemoteClientPtr_S rc = GetAuthedRemoteClientByHostID(remote);
		if ( !rc )
		{
			return true;
		}

		if ( CheckIsolateDataList(rootUUID) )
		{
			m_s2cProxy.NotifyAddDataFailed(remote, g_ReliableSendForPN, tag, ErrorType_PermissionDenied, _PNT("Data in the IsolateDataList"), 0, _PNT(""), blocked);
			return true;
		}

		CLoadedData2Ptr_S loadData = GetLoadedDataByRootGuid(rootUUID);
		if ( !loadData || loadData->GetLoaderClientHostID() != remote )
		{
			m_s2cProxy.NotifyAddDataFailed(remote, g_ReliableSendForPN, tag, ErrorType_InvalidPacketFormat, _PNT("No Exclusive Client"), 0, _PNT(""), blocked);
			return true;
		}

		CPropNode addData(_PNT(""));
		addData.FromByteArray(addDataBlock);

		//!!!!주의 !!!존재하는 테이블이름인지 꼭 검사토록 하자.
		if ( addData.TypeName == _PNT("") || addData.UUID == Guid() )
		{
			m_s2cProxy.NotifyAddDataFailed(remote, g_ReliableSendForPN, tag, ErrorType_InvalidPacketFormat, _PNT("Not valid Data"), 0, _PNT(""), blocked);
			return true;
		}

		CPropNodePtr candidateData = loadData->m_data.GetNode(addData.UUID);
		if ( candidateData )
		{
			m_s2cProxy.NotifyAddDataFailed(remote, g_ReliableSendForPN, tag, ErrorType_AlreadyExists, _PNT("data guid Already Exist"), 0, _PNT(""), blocked);
			return true;
		}

		loadData->m_lastModifiedTime = GetPreciseCurrentTimeMs() - m_saveToDbmsInterval - OneSecond;
		//pendlist를 붙이도록 하자.
		DbmsWritePend2 newPend;
		newPend.m_dbClientID = remote;
		newPend.m_changePropNode.m_node = addData.CloneNoChildren();
		newPend.m_changePropNode.m_type = DWPNPT_AddPropNode;
		newPend.m_changePropNode.m_UUID = addData.UUID;
		newPend.m_DoneNotify = IDbWriteDoneNotifyPtr(new CDbWriteDoneAddNotify(this, tag, blocked));

		//작성한 리스트를 토대로 pendlist를 작성.
		loadData->m_dbmsWritePendList.AddTail(newPend);
		return true;
	}

	DEFRMI_DB2C2S_RequestUpdateData(CDbCacheServer2Impl)
	{
		rmiContext;

		CriticalSectionLock lock(m_cs, true);

		CDbRemoteClientPtr_S rc = GetAuthedRemoteClientByHostID(remote);
		if ( !rc )
		{
			return true;
		}

		//
		if ( CheckIsolateDataList(rootUUID) )
		{
			m_s2cProxy.NotifyUpdateDataFailed(remote, g_ReliableSendForPN, tag, ErrorType_PermissionDenied, _PNT("Data in the IsolateDataList"), 0, _PNT(""), blocked);
			return true;
		}

		CLoadedData2Ptr_S loadData = GetLoadedDataByRootGuid(rootUUID);
		if ( !loadData || loadData->GetLoaderClientHostID() != remote )
		{
			m_s2cProxy.NotifyUpdateDataFailed(remote, g_ReliableSendForPN, tag, ErrorType_InvalidPacketFormat, _PNT("No Exclusive Client"), 0, _PNT(""), blocked);
			return true;
		}

		CPropNode updateData(_PNT(""));
		updateData.FromByteArray(updateDataBlock);

		//!!!!주의 !!!존재하는 테이블이름인지 꼭 검사토록 하자.
		if ( updateData.TypeName == _PNT("") || updateData.UUID == Guid() )
		{
			m_s2cProxy.NotifyUpdateDataFailed(remote, g_ReliableSendForPN, tag, ErrorType_InvalidPacketFormat, _PNT("Not valid Data"), 0, _PNT(""), blocked);
			return true;
		}

		CPropNodePtr candidateData = loadData->m_data.GetNode(updateData.UUID);
		if ( !candidateData )
		{
			m_s2cProxy.NotifyUpdateDataFailed(remote, g_ReliableSendForPN, tag, ErrorType_InvalidPacketFormat, _PNT("updateData not Exist"), 0, _PNT(""), blocked);
			return true;
		}

		loadData->m_lastModifiedTime = GetPreciseCurrentTimeMs() - m_saveToDbmsInterval - OneSecond;
		//pendlist를 붙이도록 하자.
		DbmsWritePend2 newPend;
		newPend.m_dbClientID = remote;
		newPend.m_changePropNode.m_node = updateData.CloneNoChildren();
		newPend.m_changePropNode.m_type = DWPNPT_RequestUpdatePropNode;
		newPend.m_changePropNode.m_UUID = updateData.UUID;
		newPend.m_DoneNotify = IDbWriteDoneNotifyPtr(new CDbWriteDoneUpdateNotify(this, tag, blocked));

		//작성한 리스트를 토대로 pendlist를 작성.
		loadData->m_dbmsWritePendList.AddTail(newPend);
		return true;
	}

	DEFRMI_DB2C2S_RequestUpdateDataList(CDbCacheServer2Impl)
	{
		rmiContext;

		CriticalSectionLock lock(m_cs, true);

		// 클라 유효성 검사
		CDbRemoteClientPtr_S rc = GetAuthedRemoteClientByHostID(remote);
		if ( !rc )
		{
			return true;
		}

		// 격리 안된 것이어야 함
		if ( CheckIsolateDataList(rootUUID) )
		{
			m_s2cProxy.NotifyUpdateDataListFailed(remote, g_ReliableSendForPN, tag, ErrorType_PermissionDenied, _PNT("Data in the IsolateDataList"), 0, _PNT(""), blocked);
			return true;
		}

		// LD도 찾고 소유주도 확인
		CLoadedData2Ptr_S loadedData=GetLoadedDataByRootGuid(rootUUID);
		if ( !loadedData || loadedData->GetLoaderClientHostID() != remote )
		{
			m_s2cProxy.NotifyUpdateDataListFailed(remote, g_ReliableSendForPN, tag, ErrorType_InvalidPacketFormat, _PNT("No Exclusive Client"), 0, _PNT(""), blocked);
			return true;
		}

		DbmsWritePend2 newPend;
		newPend.m_dbClientID = remote;
		newPend.m_changePropNode.m_type = DWPNPT_RequestUpdatePropNodeList;
		newPend.m_changePropNode.m_node = loadedData->m_data.GetRootNode()->CloneNoChildren();
		newPend.m_transaction = transaction; // 트랜잭션 걸고 atomic하게 기록해야 하는 것인지?

		ExtractUpdateDataFromArray(remote, loadedData->m_data, changeBlock, newPend.m_changePropNodeArray);

		/* table name then node guid 순으로 사전적 정렬을 한다.

		MySQL, MS SQL은 UUID에 대해서 index 되는 순서가 서로 다르다.
		한편, MS SQL은 성능 향상을 위해, DB connection들이 잠금하는 영역이 지나치게
		깨알같이 많은 경우 잠금의 영역을 page 단위로 올려버리는 경향이 있다.
		이때 재수가 없으면 아래 페이지보다 위 페이지가 나중에 잠기게 되고 이것 때문에 데드락이 발생한다. 실검증된 경우임.
		이를 해결하려면 MS SQL의 경우 반드시 위로부터 아래 순으로 레코드가 잠기게 해주어야 데드락을 피한다.

		그나저나 디비 캐시가 메모리에서 atomic하게 다룬 후 DB에 플러시되는 방식인데, 사실상 이정도면 DB cache가 트랜잭션을
		건다는 점 자체가 논리적 오류 아닐까? (물론 디비에 플러시되는 도중에 디비캐시가 죽으면 atomic을 깨버리긴 하지만 희박한 확률임)
		-> 디비 캐시 서버 자체가 SPOF임. 유저수는 예측 불가능 메모리 폭주시 대책이 없음 희박한 확률이라고 보기힘듬.
		*/
		if ( transaction )
			QuickSort(newPend.m_changePropNodeArray.GetData(), CPendNodeCompare(m_dbmsType), (int)newPend.m_changePropNodeArray.GetCount());

		if ( newPend.m_changePropNodeArray.GetCount() <= 0 )
		{
			m_s2cProxy.NotifyUpdateDataListFailed(remote, g_ReliableSendForPN, tag, ErrorType_Unexpected, _PNT("No Update List"), 0, _PNT(""), blocked);
			return true;
		}

		loadedData->m_lastModifiedTime = GetPreciseCurrentTimeMs() - m_saveToDbmsInterval - OneSecond;

		newPend.m_DoneNotify = IDbWriteDoneNotifyPtr(new CDbWriteDoneUpdateListNotify(this, tag, blocked, transaction));

		//작성한 리스트를 토대로 pendlist를 작성.
		loadedData->m_dbmsWritePendList.AddTail(newPend);


		return true;
	}

	DEFRMI_DB2C2S_RequestRemoveData(CDbCacheServer2Impl)
	{
		rmiContext;

		CriticalSectionLock lock(m_cs, true);

		CDbRemoteClientPtr_S rc = GetAuthedRemoteClientByHostID(remote);
		if ( !rc )
		{
			return true;
		}

		//
		if ( CheckIsolateDataList(rootUUID) )
		{
			m_s2cProxy.NotifyRemoveDataFailed(remote, g_ReliableSendForPN, tag, ErrorType_PermissionDenied, _PNT("Data in the IsolateDataList"), 0, _PNT(""), blocked);
			return true;
		}

		CLoadedData2Ptr_S loadData = GetLoadedDataByRootGuid(rootUUID);
		if ( !loadData || loadData->GetLoaderClientHostID() != remote )
		{
			m_s2cProxy.NotifyRemoveDataFailed(remote, g_ReliableSendForPN, tag, ErrorType_InvalidPacketFormat, _PNT("No Exclusive client."), 0, _PNT(""), blocked);
			return true;
		}

		CPropNodePtr removedData = loadData->m_data.GetNode(removeUUID);

		if ( !removedData )
		{
			m_s2cProxy.NotifyRemoveDataFailed(remote, g_ReliableSendForPN, tag, ErrorType_InvalidPacketFormat, _PNT("No Data"), 0, _PNT(""), blocked);
			return true;
		}

		// rootUID와 removeUUID가 같다면 Unload 시켜야한다.
		// 어차피 loadeddata가 삭제될것이므로 독점권 이양을 하지 않는다.
		if ( rootUUID == removeUUID )
			UnloadLoadeeByRootUUID(remote, rootUUID);

		// DBMS에 즉시 기록
		loadData->m_lastModifiedTime = GetPreciseCurrentTimeMs() - m_saveToDbmsInterval - OneSecond;

		// 요청형이므로 아직 삭제를 수행하면 안된다.
		// 지워야할 노드와 노드의 하위노드들만 removelist에 추가하고 pendlist를 작성하자.
		ErrorType errtype = loadData->m_data._INTERNAL_AddRemovePropNodeList(removeUUID);
		if ( errtype != ErrorType_Ok )
		{
			m_s2cProxy.NotifyRemoveDataFailed(remote, g_ReliableSendForPN, tag, errtype, _PNT("AddRemovePropNodeList function is failed"), 0, _PNT(""), blocked);
			return true;
		}

		IDbWriteDoneNotifyPtr doneNotify = IDbWriteDoneNotifyPtr(new CDbWriteDoneRemoveNotify(this, tag, blocked));

		// 지울 노드들을 pendlist에 추가한다.
		loadData->AddWritePendListFromRemoveNodes(remote, removeUUID, doneNotify);

		return true;
	}


	// 요청 응답 하지 않습니다.
	// 에러는 어케 할지 생각해 봅시다.
	DEFRMI_DB2C2S_AddData(CDbCacheServer2Impl)
	{
		rmiContext;

		CriticalSectionLock lock(m_cs, true);

		CDbRemoteClientPtr_S rc = GetAuthedRemoteClientByHostID(remote);
		if ( !rc )
		{
			return true;
		}

		//
		if ( CheckIsolateDataList(rootUUID) )
		{
			m_s2cProxy.NotifyWarning(remote, g_ReliableSendForPN, ErrorType_PermissionDenied, ErrorType_PermissionDenied, _PNT("DB.AD"));
			return true;
		}

		CLoadedData2Ptr_S loadData = GetLoadedDataByRootGuid(rootUUID);
		if ( !loadData )
		{
			//modify by rekfkno1 - data가 없는경우 클라에게 알린다.
			m_s2cProxy.NotifyWarning(remote, g_ReliableSendForPN, ErrorType_LoadedDataNotFound, ErrorType_LoadedDataNotFound, _PNT("DB.AD"));
			return true;
		}

		if ( loadData->GetLoaderClientHostID() != remote )
		{
			//modify by rekfkno1 - 독점로딩한 클라가 아닐경우 클라에게 알린다.
			String str;
			str.Format(_PNT("%s - Not Exclusive Client"), _PNT("DB.AD"));
			m_s2cProxy.NotifyWarning(remote, g_ReliableSendForPN, ErrorType_PermissionDenied, ErrorType_InvalidPacketFormat, str);
			return true;
		}

		CPropNodePtr addData = CPropNodePtr(new CPropNode(_PNT("")));
		addData->FromByteArray(addDataBlock);

		//!!!!주의 !!!존재하는 테이블이름인지 꼭 검사토록 하자.
		if ( addData->TypeName == _PNT("") || addData->UUID == Guid() )
		{
			String str;
			str.Format(_PNT("%s - addDataBlock Invalid"), _PNT("DB.AD"));
			m_s2cProxy.NotifyWarning(remote, g_ReliableSendForPN, ErrorType_InvalidPacketFormat, ErrorType_InvalidPacketFormat, str);
			return true;
		}

		//데이터 중복.
		CPropNodePtr candidateData = loadData->GetData()->GetNode(addData->UUID);
		if ( candidateData )
		{
			//에러 enque하자.
			String str;
			str.Format(_PNT("%s - Already Exist Data"), _PNT("DB.AD"));
			m_s2cProxy.NotifyWarning(remote, g_ReliableSendForPN, ErrorType_AlreadyExists, ErrorType_InvalidPacketFormat, str);
			return true;
		}

		RefreshLastModifiedTimeMs(writeDbmsImmediately, loadData);

		//로컬에서 추가후 pend.
		CPropNodePtr ownerData = loadData->GetData()->GetNode(ownerUUID);
		loadData->GetData()->InsertChild(ownerData, addData);
		loadData->GetData()->_INTERNAL_ClearChangeNode();


		//pendlist를 붙이도록 하자.
		DbmsWritePend2 newPend;
		newPend.m_dbClientID = remote;
		newPend.m_changePropNode.m_node = addData->CloneNoChildren();
		newPend.m_changePropNode.m_type = DWPNPT_AddPropNode;
		newPend.m_changePropNode.m_UUID = addData->UUID;

		//작성한 리스트를 토대로 pendlist를 작성.
		loadData->m_dbmsWritePendList.AddTail(newPend);
		return true;
	}

	DEFRMI_DB2C2S_UpdateData(CDbCacheServer2Impl)
	{
		rmiContext;

		CriticalSectionLock lock(m_cs, true);

		CDbRemoteClientPtr_S rc = GetAuthedRemoteClientByHostID(remote);
		if ( !rc )
		{
			return true;
		}

		//
		if ( CheckIsolateDataList(rootUUID) )
		{
			m_s2cProxy.NotifyWarning(remote, g_ReliableSendForPN, ErrorType_PermissionDenied, ErrorType_PermissionDenied, _PNT("DB.UD"));
			return true;
		}

		CLoadedData2Ptr_S loadData = GetLoadedDataByRootGuid(rootUUID);
		if ( !loadData )
		{
			//modify by rekfkno1 - data가 없는경우 클라에게 알린다.
			m_s2cProxy.NotifyWarning(remote, g_ReliableSendForPN, ErrorType_LoadedDataNotFound, ErrorType_LoadedDataNotFound, _PNT("DB.UD"));
			return true;
		}

		if ( loadData->GetLoaderClientHostID() != remote )
		{
			//modify by rekfkno1 - 독점로딩한 클라가 아닐경우 클라에게 알린다.
			String str;
			str.Format(_PNT("%s - Not Exclusive Client"), _PNT("DB.UD"));
			m_s2cProxy.NotifyWarning(remote, g_ReliableSendForPN, ErrorType_PermissionDenied, ErrorType_InvalidPacketFormat, str);
			return true;
		}


		CPropNode updateData(_PNT(""));
		updateData.FromByteArray(updateDataBlock);

		//!!!!주의 !!!존재하는 테이블이름인지 꼭 검사토록 하자.
		if ( updateData.TypeName == _PNT("") || updateData.UUID == Guid() )
		{
			String str;
			str.Format(_PNT("%s - addDataBlock Invalid"), _PNT("DB.UD"));
			m_s2cProxy.NotifyWarning(remote, g_ReliableSendForPN, ErrorType_InvalidPacketFormat, ErrorType_InvalidPacketFormat, str);
			return true;
		}

		CPropNodePtr candidateData = loadData->m_data.GetNode(updateData.UUID);

		if ( !candidateData )
		{
			String str;
			str.Format(_PNT("%s - Not Found Data"), _PNT("DB.UD"));
			m_s2cProxy.NotifyWarning(remote, g_ReliableSendForPN, ErrorType_LoadedDataNotFound, ErrorType_InvalidPacketFormat, str);
			return true;
		}

		//로컬 변경.
		candidateData->m_map = updateData.m_map;
		candidateData->SoftWorkIssued();//hardwork와 겹치는걸 감지위해 꼭해주어야 한다.

		RefreshLastModifiedTimeMs(writeDbmsImmediately, loadData);

		//pendlist를 붙이도록 하자.
		DbmsWritePend2 newPend;
		newPend.m_dbClientID = remote;
		newPend.m_changePropNode.m_node = updateData.CloneNoChildren();
		newPend.m_changePropNode.m_type = DWPNPT_UnilateralUpdatePropNode;
		newPend.m_changePropNode.m_UUID = updateData.UUID;

		//작성한 리스트를 토대로 pendlist를 작성.
		loadData->m_dbmsWritePendList.AddTail(newPend);
		return true;
	}

	DEFRMI_DB2C2S_UpdateDataList(CDbCacheServer2Impl)
	{
		rmiContext;

		CriticalSectionLock lock(m_cs, true);

		CDbRemoteClientPtr_S rc = GetAuthedRemoteClientByHostID(remote);
		if ( !rc )
		{
			return true;
		}

		//
		if ( CheckIsolateDataList(rootUUID) )
		{
			m_s2cProxy.NotifyWarning(remote, g_ReliableSendForPN, ErrorType_PermissionDenied, ErrorType_PermissionDenied, _PNT("DB.UD"));
			return true;
		}

		CLoadedData2Ptr_S loadedData=GetLoadedDataByRootGuid(rootUUID);
		if ( !loadedData || loadedData->GetLoaderClientHostID() != remote )
		{
			m_s2cProxy.NotifyWarning(remote, g_ReliableSendForPN, ErrorType_LoadedDataNotFound, ErrorType_LoadedDataNotFound, _PNT("DB.UD"));
			return true;
		}

		DbmsWritePend2 newPend;
		newPend.m_dbClientID = remote;
		newPend.m_changePropNode.m_type = DWPNPT_UnilateralUpdatePropNodeList;
		newPend.m_changePropNode.m_node = loadedData->m_data.GetRootNode()->CloneNoChildren();
		newPend.m_transaction = transactional;

		ExtractUpdateDataFromArray(remote, loadedData->m_data, updateDataBlock, newPend.m_changePropNodeArray);

		/* table name then node guid 순으로 사전적 정렬을 한다.

		MySQL, MS SQL은 UUID에 대해서 index 되는 순서가 서로 다르다.
		한편, MS SQL은 성능 향상을 위해, DB connection들이 잠금하는 영역이 지나치게
		깨알같이 많은 경우 잠금의 영역을 page 단위로 올려버리는 경향이 있다.
		이때 재수가 없으면 아래 페이지보다 위 페이지가 나중에 잠기게 되고 이것 때문에 데드락이 발생한다. 실검증된 경우임.
		이를 해결하려면 MS SQL의 경우 반드시 위로부터 아래 순으로 레코드가 잠기게 해주어야 데드락을 피한다.

		그나저나 디비 캐시가 메모리에서 atomic하게 다룬 후 DB에 플러시되는 방식인데, 사실상 이정도면 DB cache가 트랜잭션을
		건다는 점 자체가 논리적 오류 아닐까? (물론 디비에 플러시되는 도중에 디비캐시가 죽으면 atomic을 깨버리긴 하지만 희박한 확률임) */
		if ( transactional )
			QuickSort(newPend.m_changePropNodeArray.GetData(), CPendNodeCompare(m_dbmsType), (int)newPend.m_changePropNodeArray.GetCount());

		if ( newPend.m_changePropNodeArray.GetCount() <= 0 )
		{
			m_s2cProxy.NotifyWarning(remote, g_ReliableSendForPN, ErrorType_InvalidPacketFormat, ErrorType_InvalidPacketFormat, _PNT("DB.UD"));
			return true;
		}

		RefreshLastModifiedTimeMs(writeDbmsImmediately, loadedData);

		//이제 로컬에 업데이트 하자.
		UpdateLocalDataList(remote, loadedData, 0, newPend.m_changePropNodeArray, false);

		loadedData->m_data._INTERNAL_ClearChangeNode();

		//작성한 리스트를 토대로 pendlist를 작성.
		loadedData->m_dbmsWritePendList.AddTail(newPend);
		return true;
	}


	DEFRMI_DB2C2S_MoveData(CDbCacheServer2Impl)
	{
		rmiContext;

		CriticalSectionLock lock(m_cs, true);

		//remote 유효성 검사 
		CDbRemoteClientPtr_S rc = GetAuthedRemoteClientByHostID(remote);
		if ( rc == NULL )
		{
			return true;
		}

		//
		if ( CheckIsolateDataList(rootUUID) )
		{
			m_s2cProxy.NotifyWarning(remote, g_ReliableSendForPN, ErrorType_PermissionDenied, ErrorType_PermissionDenied, _PNT("DB.MD"));
			return true;
		}

		//Note 유효성 검사
		CLoadedData2Ptr_S srcloadData = GetLoadedDataByRootGuid(rootUUID);
		if ( srcloadData == NULL )
		{
			m_s2cProxy.NotifyWarning(remote, g_ReliableSendForPN, ErrorType_LoadedDataNotFound, ErrorType_LoadedDataNotFound, _PNT("DB.MD"));
			return true;
		}

		CPropNodePtr srcNode = srcloadData->m_data.GetNode(nodeUUID);
		if ( srcNode == NULL )
		{
			m_s2cProxy.NotifyWarning(remote, g_ReliableSendForPN, ErrorType_LoadedDataNotFound, ErrorType_LoadedDataNotFound, _PNT("DB.MD"));
			return true;
		}

		CLoadedData2Ptr_S destLoadData = GetLoadedDataByRootGuid(destRootUUID);

		// 없으면 디비에서 로딩
		if ( destLoadData == NULL )
		{
			lock.Unlock();

			CLoadedData2Ptr_S loadDataCandidate;
			if ( !LoadDataFromDbms(rootTableName, destRootUUID, loadDataCandidate) )
			{
				m_s2cProxy.NotifyWarning(remote, g_ReliableSendForPN, ErrorType_LoadedDataNotFound, ErrorType_LoadedDataNotFound, _PNT("DB.MD"));
				return true;
			}

			lock.Lock();

			// 잠금 후 그새 다른 스레드에서 로딩 마쳤으면 스킵.
			// 아니면 그걸 LD list에 등록.
			destLoadData = GetLoadedDataByRootGuid(destRootUUID);

			if ( destLoadData == NULL )
			{
				// 새 객체 연결
				destLoadData = loadDataCandidate;
				if (destLoadData->m_data.RootUUID == Guid())
					throw Exception("Exception from MoveData!");

				m_loadedDataList.Add(destLoadData->m_data.RootUUID, destLoadData);
				destLoadData->MarkUnload();
			}
		}

		// node를 받을 LD
		CPropNodePtr destNode = destLoadData->m_data.GetNode(destNodeUUID);
		if ( destNode == NULL )
		{
			m_s2cProxy.NotifyWarning(remote, g_ReliableSendForPN, ErrorType_LoadedDataNotFound, ErrorType_LoadedDataNotFound, _PNT("DB.MD"));
			return true;
		}

		// pended work list에 추가할 새 항목. src node 용.
		DbmsWritePend2 newPend;
		newPend.m_dbClientID = remote;
		newPend.m_changePropNode.m_node = srcNode->CloneNoChildren();
		newPend.m_changePropNode.m_type = DWPNPT_MovePropNode;
		newPend.m_changePropNode.m_UUID = srcNode->UUID;

		// 로컬 메모리 안에서 노드 이동
		if ( srcloadData->m_data.MovePropNode(*destLoadData->GetData(), srcNode, destNode) != ErrorType_Ok )
			return true;

		// newPend에 들어갈 값들을 위한 임시 변수
		// Destination Node와 하위의 RootUUID를 변경할 노드를 저장한다.
		DbmsWritePropNodeListPend Info;

		Info.m_node = destNode->CloneNoChildren();

		newPend.m_changePropNodeArray.Add(Info);

		CFastList2<CPropNodePtr, int> tempQueue;
		CPropNode *changeRoot = srcNode->m_child;

		if ( changeRoot != NULL )
			tempQueue.AddTail(destLoadData->m_data.GetNode(changeRoot->UUID));

		CPropNodePtr node;

		// destLoadData의 모든 항목을 change prop node array에 넣는다.
		while ( tempQueue.GetCount() > 0 )
		{
			node = tempQueue.RemoveHead();

			if ( node->m_sibling )
			{
				tempQueue.AddTail(destLoadData->m_data.GetNode(node->m_sibling->UUID));
			}

			if ( node->m_child )
			{
				tempQueue.AddTail(destLoadData->m_data.GetNode(node->m_child->UUID));
			}

			Info.m_node = node->CloneNoChildren();

			newPend.m_changePropNodeArray.Add(Info);
		}

		// Self일 때만 Notify를 보내지 않는다.
		// Self가 아니면 Local to Local도 Notify를 받아야 되기 때문에 보내야 한다.
		// Notify는 MoveNode가 완료된 시점에서 보내야 된다. 
		// 또한 완료된 시점의 데이터 정보를 보내줘야 한다.
		if ( rootUUID != destNodeUUID )
		{
			// m_changePropNodeArray 최소한 한개의 노드가 들어간다.
			int moveNodeCount = (int)newPend.m_changePropNodeArray.GetCount();

			CMessage msg;

			msg.UseInternalBuffer();

			msg << moveNodeCount;
			msg << *srcNode;

			// 첫번째 노드는 DestNode의 정보다
			// 두번째 인덱스 부터 넣는다.
			for ( int i = 1; i < moveNodeCount; i++ )
			{
				msg << *newPend.m_changePropNodeArray[i].m_node;
			}

			ByteArray block;

			msg.CopyTo(block);

			m_s2cProxy.NotifySomeoneAddData(destLoadData->GetLoaderClientHostID(), g_ReliableSendForPN, destNode->RootUUID, block, ByteArray(), destNode->UUID);
		}

		// Destination LoadedData를 Refresh를 해주어야 한다.
		RefreshLastModifiedTimeMs(writeDbmsImmediately, destLoadData);

		//change된것을 clear한다.
		srcloadData->m_data._INTERNAL_ClearChangeNode();

		//작성한 리스트를 토대로 pendlist를 작성.
		// 이미 소유권이 Destination으로 넘어 갔기 때문에 Destination에서 PendList를 처리해야한다.
		destLoadData->m_dbmsWritePendList.AddTail(newPend);

		return true;
	}

	DEFRMI_DB2C2S_RemoveData(CDbCacheServer2Impl)
	{
		rmiContext;

		CriticalSectionLock lock(m_cs, true);

		CDbRemoteClientPtr_S rc = GetAuthedRemoteClientByHostID(remote);
		if ( !rc )
		{
			return true;
		}

		//
		if ( CheckIsolateDataList(rootUUID) )
		{
			m_s2cProxy.NotifyWarning(remote, g_ReliableSendForPN, ErrorType_PermissionDenied, ErrorType_PermissionDenied, _PNT("DB.RD"));
			return true;

		}
		CLoadedData2Ptr_S loadData = GetLoadedDataByRootGuid(rootUUID);
		if ( !loadData )
		{
			//modify by rekfkno1 - data가 없는경우 클라에게 알린다.
			m_s2cProxy.NotifyWarning(remote, g_ReliableSendForPN, ErrorType_LoadedDataNotFound, ErrorType_LoadedDataNotFound, _PNT("DB.RD"));
			return true;
		}

		if ( loadData->GetLoaderClientHostID() != remote )
		{
			//modify by rekfkno1 - 독점로딩한 클라가 아닐경우 클라에게 알린다.
			String str;
			str.Format(_PNT("%s - Not Exclusive Client"), _PNT("DB.RD"));
			m_s2cProxy.NotifyWarning(remote, g_ReliableSendForPN, ErrorType_PermissionDenied, ErrorType_InvalidPacketFormat, str);
			return true;
		}

		CPropNodePtr removeData = loadData->m_data.GetNode(removeUUID);
		if ( !removeData )
		{
			String str;
			str.Format(_PNT("%s - Not Found Data"), _PNT("DB.RD"));
			m_s2cProxy.NotifyWarning(remote, g_ReliableSendForPN, ErrorType_LoadedDataNotFound, ErrorType_InvalidPacketFormat, str);
			return true;
		}

		// 지우기전에 removelist에 추가해야함.
		ErrorType errtype = loadData->m_data._INTERNAL_AddRemovePropNodeList(removeUUID);
		if ( errtype != ErrorType_Ok )
		{
			String str;
			str.Format(_PNT("%s - AddRemovePropNodeList function is failed."), _PNT("DB.RD"));
			m_s2cProxy.NotifyWarning(remote, g_ReliableSendForPN, errtype, ErrorType_InvalidPacketFormat, str);
			return true;
		}

		// root와 remove가 같을경우 Unload해야한다.
		if ( rootUUID == removeUUID )
			UnloadLoadeeByRootUUID(remote, rootUUID);

		RefreshLastModifiedTimeMs(writeDbmsImmediately, loadData);

		// loadData안의 모든 RemoveNode를 WritePendList에 추가한다.
		loadData->AddWritePendListFromRemoveNodes(remote);

		// pendlist에 추가했으니 이제 실제 로컬에서 삭제한다.
		loadData->m_data.RemoveNode(removeData);

		//change된것을 clear한다.
		loadData->m_data._INTERNAL_ClearChangeNode();

		return true;
	}

	DEFRMI_DB2C2S_RequestNonExclusiveSnapshotDataByFieldNameAndValue(CDbCacheServer2Impl)
	{
		rmiContext;

		String queryString;
		queryString.Format(_PNT("%s=%s"), fieldName, VariantToString(cmpValue));
		CLoadRequestPtr request(new CLoadRequest(false, remote, tag, ByteArray(), GetPreciseCurrentTimeMs()));

		CriticalSectionLock mainLock(m_cs, true);
		LoadDataCore(mainLock, remote, rootTableName, queryString, request);
		return true;
	}


	DEFRMI_DB2C2S_RequestNonExclusiveSnapshotDataByGuid(CDbCacheServer2Impl)
	{
		rmiContext;

		CLoadRequestPtr request(new CLoadRequest(false, remote, tag, ByteArray(), GetPreciseCurrentTimeMs()));
		CriticalSectionLock mainLock(m_cs, true);

		// 유효하지 않은 Guid인지 검사.
		if ( rootUUID == Guid() )
		{
			CFailInfo_S info;
			info.m_reason = ErrorType_UserRequested;
			info.m_comment = _PNT("RootUUID is not valid");
			request->m_failList.AddTail(info);
			NotifyLoadDataCompleteOnlyIfUnloadWaiterIsNone(request);
			return true;
		}

		if ( !m_allowNonExclusiveAccess )
		{
			// 비독점 접근을 허용하지 않도록 설정되어있다면
			// 비독점 요청은 실패로 처리한다. 
			CFailInfo_S failInfo;
			failInfo.m_reason = ErrorType_PermissionDenied;
			failInfo.m_comment = _PNT("Non-exclusive data access is denied.");

			request->m_failList.AddTail(failInfo);
			NotifyLoadDataCompleteOnlyIfUnloadWaiterIsNone(request);
			return true;
		}

		// 메모리에 로드되어있는지 확인.
		LoadedDataList loadedDataList;
		CLoadedData2Ptr_S loadedData = CheckLoadedDataAndProcessRequest(rootUUID, request, loadedDataList);
		if ( loadedData == NULL )
		{
			// 로드되어있지 않다면 DB로부터 로드한다.
			LoadDataCore(mainLock, remote, rootTableName, GetUuidQueryString(rootUUID), request);
			return true;
		}

		// 메모리에 로드는 되어있는 경우 즉시 응답해준다. 
		LoadDataCore_Finish(mainLock, request, NULL);
		return true;
	}

	DEFRMI_DB2C2S_RequestNonExclusiveSnapshotDataByQuery(CDbCacheServer2Impl)
	{
		rmiContext;

		CLoadRequestPtr request(new CLoadRequest(false, remote, tag, ByteArray(), GetPreciseCurrentTimeMs()));
		CriticalSectionLock mainLock(m_cs, true);
		LoadDataCore(mainLock, remote, rootTableName, queryString, request);
		return true;
	}

	DEFRMI_DB2C2S_RequestNonExclusiveAddData(CDbCacheServer2Impl)
	{
		rmiContext;

		CriticalSectionLock lock(m_cs, true);

		CDbRemoteClientPtr_S rc = GetAuthedRemoteClientByHostID(remote);
		if ( !rc )
		{
			return true;
		}

		//
		if ( CheckIsolateDataList(rootUUID) )
		{
			m_s2cProxy.NotifyNonExclusiveAddDataFailed(remote, g_ReliableSendForPN, tag, ErrorType_PermissionDenied, _PNT("Data in the IsolateDataList"), 0, _PNT(""));
			return true;
		}

		if ( !IsValidRootTable(rootTableName) )
		{
			m_s2cProxy.NotifyNonExclusiveAddDataFailed(remote, g_ReliableSendForPN, tag, ErrorType_PermissionDenied, _PNT("RootTableName is Invalid"), 0, _PNT(""));
			return true;
		}

		CLoadedData2Ptr_S loadData = GetLoadedDataByRootGuid(rootUUID);
		if ( loadData )
		{
			CPropNodePtr ownerData = loadData->GetData()->GetNode(ownerUUID);

			if ( ownerData )
			{

				CPropNode addData(_PNT(""));
				addData.FromByteArray(addDataBlock);

				//!!!!주의 !!!존재하는 테이블이름인지 꼭 검사토록 하자.
				if ( addData.TypeName == _PNT("") || addData.UUID == Guid() )
				{
					m_s2cProxy.NotifyNonExclusiveAddDataFailed(remote, g_ReliableSendForPN, tag, ErrorType_InvalidPacketFormat, _PNT("Not valid Data"), 0, _PNT(""));
					return true;
				}

				CPropNodePtr candidateData = loadData->m_data.GetNode(addData.UUID);
				if ( candidateData )
				{
					m_s2cProxy.NotifyNonExclusiveAddDataFailed(remote, g_ReliableSendForPN, tag, ErrorType_AlreadyExists, _PNT("data guid Already Exist"), 0, _PNT(""));
					return true;
				}


				loadData->m_lastModifiedTime=GetPreciseCurrentTimeMs() - m_saveToDbmsInterval - OneSecond;

				DbmsWritePend2 newPend;
				newPend.m_dbClientID = remote;
				newPend.m_changePropNode.m_node = addData.CloneNoChildren();
				newPend.m_changePropNode.m_type = DWPNPT_AddPropNode;
				newPend.m_changePropNode.m_UUID = addData.UUID;
				newPend.m_DoneNotify = IDbWriteDoneNotifyPtr(new CDbWriteDoneNonExclusiveAddNotify(this, tag, messageToLoader));

				//작성한 리스트를 토대로 pendlist를 작성.
				loadData->m_dbmsWritePendList.AddTail(newPend);

			}
			else
				m_s2cProxy.NotifyNonExclusiveAddDataFailed(remote, g_ReliableSendForPN, tag, ErrorType_Unexpected, _PNT("owner Data Not exist"), 0, _PNT(""));
		}
		else
		{
			//로딩한게 없다면 로딩.
			lock.Unlock();

			CLoadedData2Ptr_S loadDataCandidate;
			if ( !LoadDataFromDbms(rootTableName, rootUUID, loadDataCandidate) )     // 로딩 중 오류가 나면 false다. 읽을 데이터가 없다고 해서 에러가 나지는 않는다.
			{
				m_s2cProxy.NotifyNonExclusiveAddDataFailed(remote, g_ReliableSendForPN, tag, ErrorType_Unexpected, _PNT("load from dbms Error"), 0, _PNT(""));
				return true;
			}

			lock.Lock();

			// lock이후 unlock됐었으므로 다시한번 있는지 검사.
			loadData=GetLoadedDataByRootGuid(rootUUID);
			if ( loadData )
			{
				CPropNodePtr ownerData = loadData->GetData()->GetNode(ownerUUID);

				if ( ownerData )
				{
					CPropNode addData(_PNT(""));
					addData.FromByteArray(addDataBlock);

					//!!!!주의 !!!존재하는 테이블이름인지 꼭 검사토록 하자.
					if ( addData.TypeName == _PNT("") || addData.UUID == Guid() )
					{
						m_s2cProxy.NotifyNonExclusiveAddDataFailed(remote, g_ReliableSendForPN, tag, ErrorType_InvalidPacketFormat, _PNT("Not valid Data"), 0, _PNT(""));
						return true;
					}

					CPropNodePtr candidateData = loadData->m_data.GetNode(addData.UUID);
					if ( candidateData )
					{
						m_s2cProxy.NotifyNonExclusiveAddDataFailed(remote, g_ReliableSendForPN, tag, ErrorType_AlreadyExists, _PNT("data guid Already Exist"), 0, _PNT(""));
						return true;
					}


					loadData->m_lastModifiedTime=GetPreciseCurrentTimeMs() - m_saveToDbmsInterval - OneSecond;

					DbmsWritePend2 newPend;
					newPend.m_dbClientID = remote;
					newPend.m_changePropNode.m_node = addData.CloneNoChildren();
					newPend.m_changePropNode.m_type = DWPNPT_AddPropNode;
					newPend.m_changePropNode.m_UUID = addData.UUID;
					newPend.m_DoneNotify = IDbWriteDoneNotifyPtr(new CDbWriteDoneNonExclusiveAddNotify(this, tag, messageToLoader));

					//작성한 리스트를 토대로 pendlist를 작성.
					loadData->m_dbmsWritePendList.AddTail(newPend);

				}
				else
					m_s2cProxy.NotifyNonExclusiveAddDataFailed(remote, g_ReliableSendForPN, tag, ErrorType_Unexpected, _PNT("owner Data Not exist"), 0, _PNT(""));

			}
			else
			{
				//새 객체 연결.
				loadData = loadDataCandidate;
				if (loadData->m_data.RootUUID == Guid())
					throw Exception("Exception from RequestNonExclusiveAddData!");

				m_loadedDataList.Add(loadData->m_data.RootUUID, loadData);
				loadData->MarkUnload();

				//이제 다시 pend를 하도록 하자.
				CPropNodePtr ownerData = loadData->GetData()->GetNode(ownerUUID);

				if ( ownerData )
				{

					CPropNode addData(_PNT(""));
					addData.FromByteArray(addDataBlock);

					//!!!!주의 !!!존재하는 테이블이름인지 꼭 검사토록 하자.
					if ( addData.TypeName == _PNT("") || addData.UUID == Guid() )
					{
						m_s2cProxy.NotifyNonExclusiveAddDataFailed(remote, g_ReliableSendForPN, tag, ErrorType_InvalidPacketFormat, _PNT("Not valid Data"), 0, _PNT(""));
						return true;
					}

					CPropNodePtr candidateData = loadData->m_data.GetNode(addData.UUID);
					if ( candidateData )
					{
						m_s2cProxy.NotifyNonExclusiveAddDataFailed(remote, g_ReliableSendForPN, tag, ErrorType_AlreadyExists, _PNT("data guid Already Exist"), 0, _PNT(""));
						return true;
					}


					loadData->m_lastModifiedTime=GetPreciseCurrentTimeMs() - m_saveToDbmsInterval - OneSecond;

					DbmsWritePend2 newPend;
					newPend.m_dbClientID = remote;
					newPend.m_changePropNode.m_node = addData.CloneNoChildren();
					newPend.m_changePropNode.m_type = DWPNPT_AddPropNode;
					newPend.m_changePropNode.m_UUID = addData.UUID;
					newPend.m_DoneNotify = IDbWriteDoneNotifyPtr(new CDbWriteDoneNonExclusiveAddNotify(this, tag, messageToLoader));

					//작성한 리스트를 토대로 pendlist를 작성.
					loadData->m_dbmsWritePendList.AddTail(newPend);

				}
				else
					m_s2cProxy.NotifyNonExclusiveAddDataFailed(remote, g_ReliableSendForPN, tag, ErrorType_Unexpected, _PNT("owner Data Not exist"), 0, _PNT(""));
			}

		}



		return true;
	}

	DEFRMI_DB2C2S_RequestNonExclusiveRemoveData(CDbCacheServer2Impl)
	{
		rmiContext;

		CriticalSectionLock lock(m_cs, true);

		CDbRemoteClientPtr_S rc = GetAuthedRemoteClientByHostID(remote);
		if ( !rc )
		{
			return true;
		}

		//
		if ( CheckIsolateDataList(rootUUID) )
		{
			m_s2cProxy.NotifyNonExclusiveRemoveDataFailed(remote, g_ReliableSendForPN, tag, ErrorType_PermissionDenied, _PNT("Data in the IsolateDataList"), 0, _PNT(""));
			return true;
		}

		if ( !IsValidRootTable(rootTableName) )
		{
			m_s2cProxy.NotifyNonExclusiveRemoveDataFailed(remote, g_ReliableSendForPN, tag, ErrorType_PermissionDenied, _PNT("RootTableName is Invalid"), 0, _PNT(""));
			return true;
		}

		CLoadedData2Ptr_S loadData = GetLoadedDataByRootGuid(rootUUID);
		if ( loadData )
		{
			CPropNodePtr removeData = loadData->m_data.GetNode(removeUUID);
			if ( removeData )
			{
				// 만약 RootUUID와 removeUUID가 같다면 Unload해야한다.
				// 어차피 모든 loadeddata가 삭제될것이므로 독점권이양을 하지말자.
				if ( rootUUID == removeUUID )
					UnloadLoadeeByRootUUID(remote, rootUUID);

				// remove노드와 연관된 하위노드들을 removePropNodelist에 추가
				ErrorType errtype = loadData->m_data._INTERNAL_AddRemovePropNodeList(removeUUID);
				if ( errtype != ErrorType_Ok )
				{
					m_s2cProxy.NotifyNonExclusiveRemoveDataFailed(remote, g_ReliableSendForPN, tag, errtype, _PNT("AddRemovePropNodeList function is failed"), 0, _PNT(""));
					return true;
				}

				loadData->m_lastModifiedTime=GetPreciseCurrentTimeMs() - m_saveToDbmsInterval - OneSecond;

				// removeNodelist에 내용들을 pendlist들에 추가.
				IDbWriteDoneNotifyPtr doneNotiry = IDbWriteDoneNotifyPtr(new CDbWriteDoneNonExclusiveRemoveNotify(this, tag, messageToLoader));
				loadData->AddWritePendListFromRemoveNodes(remote, removeUUID, doneNotiry);

			}
			else
				m_s2cProxy.NotifyNonExclusiveRemoveDataFailed(remote, g_ReliableSendForPN, tag, ErrorType_Unexpected, _PNT("Data Not exist"), 0, _PNT(""));
		}
		else
		{
			//로딩한게 없다면 로딩.
			lock.Unlock();

			CLoadedData2Ptr_S loadDataCandidate;
			if ( !LoadDataFromDbms(rootTableName, rootUUID, loadDataCandidate) )     // 로딩 중 오류가 나면 false다. 읽을 데이터가 없다고 해서 에러가 나지는 않는다.
			{
				m_s2cProxy.NotifyNonExclusiveRemoveDataFailed(remote, g_ReliableSendForPN, tag, ErrorType_Unexpected, _PNT("load from dbms Error"), 0, _PNT(""));
				return true;
			}

			lock.Lock();

			// lock이후 unlock됐었으므로 다시한번 있는지 검사.
			loadData=GetLoadedDataByRootGuid(rootUUID);
			if ( loadData )
			{
				CPropNodePtr removeData = loadData->GetData()->GetNode(removeUUID);

				if ( removeData )
				{
					if ( rootUUID == removeUUID )
						UnloadLoadeeByRootUUID(remote, rootUUID);

					// remove노드와 연관된 하위노드들을 removePropNodelist에 추가
					ErrorType errtype = loadData->m_data._INTERNAL_AddRemovePropNodeList(removeUUID);
					if ( errtype != ErrorType_Ok )
					{
						m_s2cProxy.NotifyNonExclusiveRemoveDataFailed(remote, g_ReliableSendForPN, tag, errtype, _PNT("AddRemovePropNodeList function is failed"), 0, _PNT(""));
						return true;
					}

					loadData->m_lastModifiedTime=GetPreciseCurrentTimeMs() - m_saveToDbmsInterval - OneSecond;

					// removeNodelist에 내용들을 pendlist들에 추가.
					IDbWriteDoneNotifyPtr doneNotiry = IDbWriteDoneNotifyPtr(new CDbWriteDoneNonExclusiveRemoveNotify(this, tag, messageToLoader));
					loadData->AddWritePendListFromRemoveNodes(remote, removeUUID, doneNotiry);
				}
				else
					m_s2cProxy.NotifyNonExclusiveRemoveDataFailed(remote, g_ReliableSendForPN, tag, ErrorType_Unexpected, _PNT("Data Not exist"), 0, _PNT(""));

			}
			else
			{
				//새 객체 연결.
				loadData = loadDataCandidate;
				if (loadData->m_data.RootUUID == Guid())
					throw Exception("Exception from RequestNonExclusiveRemoveData!");

				m_loadedDataList.Add(loadData->m_data.RootUUID, loadData);
				loadData->MarkUnload();

				//이제 다시 pend를 하도록 하자.
				CPropNodePtr removeData = loadData->GetData()->GetNode(removeUUID);

				if ( removeData )
				{
					if ( rootUUID == removeUUID )
						UnloadLoadeeByRootUUID(remote, rootUUID);

					// remove노드와 연관된 하위노드들을 removePropNodelist에 추가
					ErrorType errtype = loadData->m_data._INTERNAL_AddRemovePropNodeList(removeUUID);
					if ( errtype != ErrorType_Ok )
					{
						m_s2cProxy.NotifyNonExclusiveRemoveDataFailed(remote, g_ReliableSendForPN, tag, errtype, _PNT("AddRemovePropNodeList function is failed"), 0, _PNT(""));
						return true;
					}

					loadData->m_lastModifiedTime=GetPreciseCurrentTimeMs() - m_saveToDbmsInterval - OneSecond;

					// removeNodelist에 내용들을 pendlist들에 추가.
					IDbWriteDoneNotifyPtr doneNotiry = IDbWriteDoneNotifyPtr(new CDbWriteDoneNonExclusiveRemoveNotify(this, tag, messageToLoader));
					loadData->AddWritePendListFromRemoveNodes(remote, removeUUID, doneNotiry);
				}
				else
					m_s2cProxy.NotifyNonExclusiveRemoveDataFailed(remote, g_ReliableSendForPN, tag, ErrorType_Unexpected, _PNT("Data Not exist"), 0, _PNT(""));
			}

		}

		return true;

	}

	DEFRMI_DB2C2S_RequestNonExclusiveSetValueIf(CDbCacheServer2Impl)
	{
		rmiContext;

		CriticalSectionLock lock(m_cs, true);

		CDbRemoteClientPtr_S rc = GetAuthedRemoteClientByHostID(remote);
		if ( !rc )
		{
			return true;
		}

		//
		if ( CheckIsolateDataList(rootUUID) )
		{
			m_s2cProxy.NotifyNonExclusiveSetValueFailed(remote, g_ReliableSendForPN, tag, ErrorType_PermissionDenied, _PNT("Data in the IsolateDataList"), 0, _PNT(""));
			return true;
		}

		if ( !IsValidRootTable(rootTableName) )
		{
			m_s2cProxy.NotifyNonExclusiveSetValueFailed(remote, g_ReliableSendForPN, tag, ErrorType_PermissionDenied, _PNT("RootTableName is Invalid"), 0, _PNT(""));
			return true;
		}

		CLoadedData2Ptr_S loadData = GetLoadedDataByRootGuid(rootUUID);
		if ( loadData )
		{
			CPropNodePtr updateData = loadData->m_data.GetNode(nodeUUID);
			if ( updateData )
			{
				loadData->m_lastModifiedTime=GetPreciseCurrentTimeMs() - m_saveToDbmsInterval - OneSecond;

				DbmsWritePend2 newPend;
				newPend.m_dbClientID = remote;
				newPend.m_changePropNode.m_node = updateData->CloneNoChildren();
				newPend.m_changePropNode.m_type = DWPNPT_SetValueIf;
				newPend.m_DoneNotify = IDbWriteDoneNotifyPtr(new CDbWriteDoneNonExclusiveSetValueNotify(this, tag, propertyName, newValue, compareType, compareValue, messageToLoader));

				//작성한 리스트를 토대로 pendlist를 작성.
				loadData->m_dbmsWritePendList.AddTail(newPend);

			}
			else
				m_s2cProxy.NotifyNonExclusiveSetValueFailed(remote, g_ReliableSendForPN, tag, ErrorType_Unexpected, _PNT("Data Not exist"), 0, _PNT(""));
		}
		else
		{
			//로딩한게 없다면 로딩.
			lock.Unlock();

			CLoadedData2Ptr_S loadDataCandidate;
			if ( !LoadDataFromDbms(rootTableName, rootUUID, loadDataCandidate) )     // 로딩 중 오류가 나면 false다. 읽을 데이터가 없다고 해서 에러가 나지는 않는다.
			{
				m_s2cProxy.NotifyNonExclusiveSetValueFailed(remote, g_ReliableSendForPN, tag, ErrorType_Unexpected, _PNT("Data Not exist"), 0, _PNT(""));
				return true;
			}

			lock.Lock();

			// lock이후 unlock됐었으므로 다시한번 있는지 검사.
			loadData=GetLoadedDataByRootGuid(rootUUID);
			if ( loadData )
			{
				CPropNodePtr updateData = loadData->m_data.GetNode(nodeUUID);
				if ( updateData )
				{
					loadData->m_lastModifiedTime=GetPreciseCurrentTimeMs() - m_saveToDbmsInterval - OneSecond;

					DbmsWritePend2 newPend;
					newPend.m_dbClientID = remote;
					newPend.m_changePropNode.m_node = updateData->CloneNoChildren();
					newPend.m_changePropNode.m_type = DWPNPT_SetValueIf;
					newPend.m_DoneNotify = IDbWriteDoneNotifyPtr(new CDbWriteDoneNonExclusiveSetValueNotify(this, tag, propertyName, newValue, compareType, compareValue, messageToLoader));

					//작성한 리스트를 토대로 pendlist를 작성.
					loadData->m_dbmsWritePendList.AddTail(newPend);

				}
				else
					m_s2cProxy.NotifyNonExclusiveSetValueFailed(remote, g_ReliableSendForPN, tag, ErrorType_Unexpected, _PNT("Data Not exist"), 0, _PNT(""));

			}
			else
			{
				//새 객체 연결.
				loadData = loadDataCandidate;
				if ( loadData->m_data.RootUUID == Guid() )
					throw Exception("Exception from RequestNonExclusiveSetValueIf!");

				m_loadedDataList.Add(loadData->m_data.RootUUID, loadData);
				loadData->MarkUnload();

				//이제 다시 pend를 하도록 하자.
				CPropNodePtr updateData = loadData->m_data.GetNode(nodeUUID);
				if ( updateData )
				{
					loadData->m_lastModifiedTime=GetPreciseCurrentTimeMs() - m_saveToDbmsInterval - OneSecond;

					DbmsWritePend2 newPend;
					newPend.m_dbClientID = remote;
					newPend.m_changePropNode.m_node = updateData->CloneNoChildren();
					newPend.m_changePropNode.m_type = DWPNPT_SetValueIf;
					newPend.m_DoneNotify = IDbWriteDoneNotifyPtr(new CDbWriteDoneNonExclusiveSetValueNotify(this, tag, propertyName, newValue, compareType, compareValue, messageToLoader));

					//작성한 리스트를 토대로 pendlist를 작성.
					loadData->m_dbmsWritePendList.AddTail(newPend);

				}
				else
					m_s2cProxy.NotifyNonExclusiveSetValueFailed(remote, g_ReliableSendForPN, tag, ErrorType_Unexpected, _PNT("Data Not exist"), 0, _PNT(""));
			}

		}

		return true;
	}

	DEFRMI_DB2C2S_RequestNonExclusiveModifyValue(CDbCacheServer2Impl)
	{
		rmiContext;

		CriticalSectionLock lock(m_cs, true);

		CDbRemoteClientPtr_S rc = GetAuthedRemoteClientByHostID(remote);
		if ( !rc )
		{
			return true;
		}

		//
		if ( CheckIsolateDataList(rootUUID) )
		{
			m_s2cProxy.NotifyNonExclusiveModifyValueFailed(remote, g_ReliableSendForPN, tag, ErrorType_PermissionDenied, _PNT("Data in the IsolateDataList"), 0, _PNT(""));
			return true;

		}
		if ( !IsValidRootTable(rootTableName) )
		{
			m_s2cProxy.NotifyNonExclusiveModifyValueFailed(remote, g_ReliableSendForPN, tag, ErrorType_PermissionDenied, _PNT("RootTableName is Invalid"), 0, _PNT(""));
			return true;
		}

		CLoadedData2Ptr_S loadData = GetLoadedDataByRootGuid(rootUUID);
		if ( loadData )
		{
			CPropNodePtr updateData = loadData->m_data.GetNode(nodeUUID);
			if ( updateData )
			{
				loadData->m_lastModifiedTime=GetPreciseCurrentTimeMs() - m_saveToDbmsInterval - OneSecond;

				DbmsWritePend2 newPend;
				newPend.m_dbClientID = remote;
				newPend.m_changePropNode.m_node = updateData->CloneNoChildren();
				newPend.m_changePropNode.m_type = DWPNPT_ModifyValue;
				newPend.m_DoneNotify = IDbWriteDoneNotifyPtr(new CDbWriteDoneNonExclusiveModifyValueNotify(this, tag, propertyName, operType, value, messageToLoader));

				//작성한 리스트를 토대로 pendlist를 작성.
				loadData->m_dbmsWritePendList.AddTail(newPend);

			}
			else
				m_s2cProxy.NotifyNonExclusiveModifyValueFailed(remote, g_ReliableSendForPN, tag, ErrorType_Unexpected, _PNT("Data Not exist"), 0, _PNT(""));
		}
		else
		{
			//로딩한게 없다면 로딩.
			lock.Unlock();

			CLoadedData2Ptr_S loadDataCandidate;
			if ( !LoadDataFromDbms(rootTableName, rootUUID, loadDataCandidate) )     // 로딩 중 오류가 나면 false다. 읽을 데이터가 없다고 해서 에러가 나지는 않는다.
			{
				m_s2cProxy.NotifyNonExclusiveModifyValueFailed(remote, g_ReliableSendForPN, tag, ErrorType_Unexpected, _PNT("load from dbms Error"), 0, _PNT(""));
				return true;
			}

			lock.Lock();

			// lock이후 unlock됐었으므로 다시한번 있는지 검사.
			loadData=GetLoadedDataByRootGuid(rootUUID);
			if ( loadData )
			{
				CPropNodePtr updateData = loadData->m_data.GetNode(nodeUUID);
				if ( updateData )
				{
					loadData->m_lastModifiedTime=GetPreciseCurrentTimeMs() - m_saveToDbmsInterval - OneSecond;

					DbmsWritePend2 newPend;
					newPend.m_dbClientID = remote;
					newPend.m_changePropNode.m_node = updateData->CloneNoChildren();
					newPend.m_changePropNode.m_type = DWPNPT_ModifyValue;
					newPend.m_DoneNotify = IDbWriteDoneNotifyPtr(new CDbWriteDoneNonExclusiveModifyValueNotify(this, tag, propertyName, operType, value, messageToLoader));

					//작성한 리스트를 토대로 pendlist를 작성.
					loadData->m_dbmsWritePendList.AddTail(newPend);

				}
				else
					m_s2cProxy.NotifyNonExclusiveModifyValueFailed(remote, g_ReliableSendForPN, tag, ErrorType_Unexpected, _PNT("Data Not exist"), 0, _PNT(""));

			}
			else
			{
				//새 객체 연결.
				loadData = loadDataCandidate;
				if ( loadData->m_data.RootUUID == Guid() )
					throw Exception("Exception from RequestNonExclusiveModifyValue!");

				m_loadedDataList.Add(loadData->m_data.RootUUID, loadData);
				loadData->MarkUnload();

				//이제 다시 pend를 하도록 하자.
				CPropNodePtr updateData = loadData->m_data.GetNode(nodeUUID);
				if ( updateData )
				{
					loadData->m_lastModifiedTime=GetPreciseCurrentTimeMs() - m_saveToDbmsInterval - OneSecond;

					DbmsWritePend2 newPend;
					newPend.m_dbClientID = remote;
					newPend.m_changePropNode.m_node = updateData->CloneNoChildren();
					newPend.m_changePropNode.m_type = DWPNPT_ModifyValue;
					newPend.m_DoneNotify = IDbWriteDoneNotifyPtr(new CDbWriteDoneNonExclusiveModifyValueNotify(this, tag, propertyName, operType, value, messageToLoader));

					//작성한 리스트를 토대로 pendlist를 작성.
					loadData->m_dbmsWritePendList.AddTail(newPend);
				}
				else
					m_s2cProxy.NotifyNonExclusiveModifyValueFailed(remote, g_ReliableSendForPN, tag, ErrorType_Unexpected, _PNT("Data Not exist"), 0, _PNT(""));
			}

		}

		return true;
	}

	// 클라로부터 받은 propNode 변경 내역을 deserialize해서 DB write pend list도 생성해 출력한다.
	// loadData는, 새 노드 추가인지 기존 노드 갱신인지를 알기 위해 참조된다.
	void CDbCacheServer2Impl::ExtractUpdateDataFromArray(
		HostID remote,
		CLoadedData2 &loadData,
		const ByteArray& inArray,
		DbmsWritePropNodeListPendArray& outArray)
	{
		if ( inArray.GetCount() > 0 )
		{
			CMessage msg;
			msg.UseExternalBuffer((uint8_t*)inArray.GetData(), (int)inArray.GetCount());
			msg.SetLength((int)inArray.GetCount());

			int dirtyCount = 0;
			msg >> dirtyCount;

			DbmsWritePropNodeListPend newData;
			CPropNode newNode(_PNT(""));
			for ( int i=0; i < dirtyCount; ++i )
			{
				msg >> newNode;
				newData.m_node = newNode.CloneNoChildren();

				//add 인지 update인지 판단한다.
				CPropNodePtr node = loadData.GetNode(newData.m_node->UUID);

				if ( node == NULL )
				{
					//add
					newData.m_type = DWPNPT_AddPropNode;
				}
				else
				{
					//update
					newData.m_type = DWPNPT_UpdatePropNode;
				}

				outArray.Add(newData);
			}

			int removeCount = 0;
			msg >> removeCount;

			DbmsWritePropNodeListPend removeData;
			CPropNode removeNode(_PNT(""));
			removeData.m_type = DWPNPT_RemovePropNode;

			for ( int i=0; i < removeCount; ++i )
			{
				msg >> removeNode;
				removeData.m_node = removeNode.CloneNoChildren();

				CPropNodePtr node = loadData.GetNode(removeData.m_node->UUID);

				if ( node == NULL )
				{
					m_s2cProxy.NotifyWarning(remote, g_ReliableSendForPN, ErrorType_Unexpected, ErrorType_LoadedDataNotFound, _PNT("removeData Not Found in ExtractUpdateDataFromArray"));
					//없는것을 제거 하려는 경우...
					//일단은 continue;
					continue;
				}

				outArray.Add(removeData);

				//하위는 클라에서 가져오기때문에 넣지 않아도 무방
			}

			// 끝까지 못 읽었으면 프로토콜 경고.
			if ( msg.GetReadOffset() != msg.GetLength() )
			{
				m_s2cProxy.NotifyWarning(remote, g_ReliableSendForPN, ErrorType_Unexpected, ErrorType_InvalidPacketFormat, _PNT("MessageLength is wrong in ExtractUpdateDataFromArray"));
			}
		}
	}

	// 디비캐시 서버의 로컬 메모리에 propNode들의 변화를 적용한다.
	void CDbCacheServer2Impl::UpdateLocalDataList(HostID remote, CLoadedData2Ptr_S loadedData, int64_t tag, DbmsWritePropNodeListPendArray& modifyDataList, bool hardwork)
	{
		int nCount = (int)modifyDataList.GetCount();

		// modifyDataList의 각 node data는 uuid 순으로 정렬되어 왔다.
		// parent to child 순으로 들어가 있는게 아니다
		// 따라서 modifyDataList의 각 항목을 로컬 메모리에 반영하는 중에 아직 parent가 없는 
		// 경우가 있는데,
		// 이러한 경우 deserialize를 유보한다.

		CFastList2<DbmsWritePropNodeListPend, int> tempList;

		for ( int i = 0; i < nCount; i++ )
		{
			tempList.AddTail(modifyDataList[i]);
		}

		while ( tempList.GetCount() > 0 )
		{
			DbmsWritePropNodeListPend modifyNode = tempList.RemoveHead();
			switch ( modifyNode.m_type )
			{
				case DWPNPT_AddPropNode:
				{
					CPropNodePtr addData = CPropNodePtr(new CPropNode(modifyNode.m_node->m_INTERNAL_TypeName));
					*addData = *modifyNode.m_node;

					CPropNodePtr candidateData = loadedData->m_data.GetNode(modifyNode.m_node->UUID);

					if ( candidateData )
					{
						//list이므로 일단은 계속진행.
						m_s2cProxy.NotifyDbmsAccessError(remote, g_ReliableSendForPN, tag, ByteArray(), _PNT("AddList Final Already Exist"));
						break;
					}

					CPropNodePtr ownerData = loadedData->m_data.GetNode(modifyNode.m_node->OwnerUUID);

					// Transaction을 걸 경우 GUID 기준으로 정렬되기 때문에 추가된 노드가 순서상 owner가 없을 경우가 있다.
					if ( ownerData == NULL )
					{
						tempList.AddTail(modifyNode);
						nCount--;
						break;
					}

					loadedData->m_data.InsertChild(ownerData, addData);
				}
					break;
				case DWPNPT_RemovePropNode:
				{
					CPropNodePtr removeData = loadedData->m_data.GetNode(modifyNode.m_node->UUID);

					if ( !removeData )
					{
						if ( !loadedData->m_data.GetRemoveNode(modifyNode.m_node->UUID) )
						{
							//이런 상황이면 대략 난감이다...
							m_s2cProxy.NotifyDbmsAccessError(remote, g_ReliableSendForPN, tag, ByteArray(), _PNT("RemoveList Final but LocalMemory Not Found"));
						}
						break;
					}

					loadedData->m_data.RemoveNode(removeData);
				}
					break;
				case DWPNPT_UpdatePropNode:
				{
					CPropNodePtr updateData = loadedData->m_data.GetNode(modifyNode.m_node->UUID);
					if ( !updateData )
					{
						//이런 상황이면 대략 난감이다...
						m_s2cProxy.NotifyDbmsAccessError(remote, g_ReliableSendForPN, tag, ByteArray(), _PNT("UpdateList Final Not Exist"));
						break;
					}

					if ( hardwork )
					{
						//hard(req-response)가 진행되는 동안에 soft(unilateral)가 진행되었다면....accesserror
						if ( updateData->IsSoftWorkIssued() )
						{
							m_s2cProxy.NotifyDbmsAccessError(remote, g_ReliableSendForPN, tag, ByteArray(), _PNT("UpdateList Final but Dirty LocalMemory"));
							break;
						}
					}
					else
					{
						//accesserror감지를 위해.
						updateData->SoftWorkIssued();
					}

					// 캐스팅을 제거하지 말 것. (포인터 정보는 유지하고 값만 바꾼다)
					*updateData = (CProperty)*modifyNode.m_node;
					updateData->m_dirtyFlag = true;
				}
					break;
				default:
					m_s2cProxy.NotifyWarning(remote, g_ReliableSendForPN, ErrorType_Unexpected, ErrorType_InvalidPacketFormat, _PNT("Notify Data is Wrong"));
					break;
			}

			if ( nCount <= 0 )
			{
				break;
			}
		}
	}

	Proud::String CDbCacheServer2Impl::VariantToString(const CVariant& var)
	{
		String strRet;
		try
		{
			switch ( var.m_val.vt )
			{
				case VT_I1:
				case VT_I2:
				case VT_I4:
				case VT_I8:
				case VT_INT:
				{
					strRet.Format(_PNT("%d"), (int64_t)var);
				}
					break;
				case VT_R4:
				{
					strRet.Format(_PNT("%f"), (float)var);
				}
					break;
				case VT_R8:
				case VT_DATE:
				{
					strRet.Format(_PNT("%lf"), (double)var);
				}
					break;
				case VT_UI1:
				case VT_UI2:
				case VT_UI4:
				case VT_UI8:
				case VT_UINT:
				{
					strRet.Format(_PNT("%u"), (uint64_t)var);
				}
					break;
				case VT_BOOL:
				{
					strRet.Format(_PNT("%d"), ((BOOL)var) ? _PNT("1") : _PNT("0"));
				}
					break;
				case VT_BSTR:
				case VT_LPWSTR:
				case VT_LPSTR:
				{
					String strVar = var;
					strRet.Format(_PNT("'%s'"), strVar);
				}
					break;
				case VT_DECIMAL:
				{
					strRet.Format(_PNT("%d"), (int)var);
				}
					break;
				default:
					break;
			}
		}
		catch ( _com_error& )
		{
			return String();
		}
		return strRet;
	}

	bool CDbCacheServer2Impl::GetAllRemoteClientAddrPort(CFastArray<AddrPort> &ret)
	{
		int remoteCount = m_netServer->GetClientCount();
		if ( remoteCount <= 0 ) return false;

		POOLED_LOCAL_VAR(HostIDArray, hostIdArr);
		
		hostIdArr.SetCount(remoteCount);
		m_netServer->GetClientHostIDs(hostIdArr.GetData(), hostIdArr.GetCount());

		for ( int i = 0; i < hostIdArr.GetCount(); ++i )
		{
			CNetClientInfo clientInfo;
			m_netServer->GetClientInfo(hostIdArr[i], clientInfo);
			ret.Add(clientInfo.m_TcpAddrFromServer);
		}
		return true;
	}

	// LD의 마지막 갱신 시간을 설정한다. 
	// writeDbmsImmediately=true이면 heartbeat에서 즉시 DB write를 하게 한다.
	void CDbCacheServer2Impl::RefreshLastModifiedTimeMs(const bool & writeDbmsImmediately, CLoadedData2Ptr_S loadedData)
	{
		// PIDL로 넘어오는 값이라서 const를 쓰고 있습니다.
		if ( writeDbmsImmediately )
		{
			loadedData->m_lastModifiedTime=GetPreciseCurrentTimeMs() - m_saveToDbmsInterval - OneSecond;
		}
		else
		{
			if ( loadedData->m_lastModifiedTime == 0 )
				loadedData->m_lastModifiedTime=GetPreciseCurrentTimeMs();
			else
				loadedData->m_lastModifiedTime = min(loadedData->m_lastModifiedTime, GetPreciseCurrentTimeMs());
		}
	}

	// pendList에 있는 각 prop node에 대해서, root UUID를 DestRootNode가 가리키는 GUID 값으로 변경한다.
	void CDbCacheServer2Impl::ChangeDBRootUUID(CAdoConnection& conn, DbmsWritePropNodeListPendArray *pendList, const Guid& DestRootUUID)
	{
		if ( pendList == NULL )
			return;

		long affected = 0;

		intptr_t pendListCount = pendList->GetCount();

		for ( intptr_t i = 0; i < pendListCount; i++ )
		{
			if ( MsSql == m_dbmsType )
			{
				affected = conn.Execute(String::NewFormat(
					_PNT("update %s set RootUUID = '%s' where UUID = '%s' "),
					(*pendList)[i].m_node->m_INTERNAL_TypeName,
					DestRootUUID.ToString(),
					(*pendList)[i].m_node->UUID.ToString()));
			}
			else
			{
				affected = conn.Execute(String::NewFormat(
					_PNT("update %s set RootUUID = '%s' where UUID = '%s' "),
					(*pendList)[i].m_node->m_INTERNAL_TypeName,
					DestRootUUID.ToBracketString(),
					(*pendList)[i].m_node->UUID.ToBracketString()));
			}

			if ( affected != 1 )
				throw Exception("Update-UUID Not Found");

			// 일단 문자열 제한이 걸리는 문제와 하나의 구문이 실패할 경우 내부적으로 몇개는 성공하나
			// 실패한 것 때문에 실패로 처리될 가능성이 있음. 
			// 그것이 위에처럼 update 구문을 하나씩 실행하는 이유.
		}
	}

	/* 이슈 3331.
	닉네임을 받는 필드에 ????? 입력시 무한 루프 도는 현상. utf8mb4 관련.
	#utf8mb4
	*/
	void CDbCacheServer2Impl::SetAdditionalEncodingSettings(CAdoConnection& conn)
	{
		if (m_useUtf8mb4Encoding)
		{
			conn.Execute(_PNT("SET character_set_results = utf8mb4;"));
			conn.Execute(_PNT("SET character_set_client = utf8mb4;"));
			conn.Execute(_PNT("SET character_set_connection = utf8mb4;"));
		}
	}

}
