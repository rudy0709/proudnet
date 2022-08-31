#include "stdafx.h"
#include "DBCacheJob.h"
#include "DbCacheServer2Impl.h"
#include "marshaler-private.h"
#include "pidl/DB_proxy.h"
#include "pidl/DB_stub.h"
namespace Proud
{


	void CDBCacheWorker::Start()
	{
		for ( int i = 0; i < m_ThreadCount; i++ )
			m_threadList.Add(ThreadPtr(new Thread(WorkProc, this)));
		for ( int i = 0; i < m_ThreadCount; i++ )
			m_threadList[i]->Start();
	}

	void CDBCacheWorker::Stop()
	{
		m_bRun = false;
		if ( m_threadList.GetCount() > 0 )
		{
			for ( int i = 0; i < m_ThreadCount; i++ )
				m_threadList[i]->Join();
			m_threadList.Clear();
		}
	}

	void CDBCacheWorker::WorkProc(void * ctx)
	{
		CDBCacheWorker * pWorker = (CDBCacheWorker *)ctx;
		pWorker->WorkThread();
	}

	void CDBCacheWorker::WorkThread()
	{
		Semaphore* pEvent = m_owner->m_dbJobQueue->GetSemaphore();

		m_bRun = true;
		while ( m_bRun == true )
		{
			CDbCacheJobQueue::JobData * pJob = NULL;
			if ( (pJob = m_owner->m_dbJobQueue->PopJob()) == NULL )
			{
				pEvent->WaitOne(100);
				continue;
			}

			switch ( pJob->m_jobType )
			{
			case CDbCacheJobQueue::JobType_SaveToDbmsContext:
				CDbCacheServer2Impl::StaticSaveToDbms(pJob->m_jobData);
				break;
			case CDbCacheJobQueue::JobType_ExclusiveLoadDataByFieldNameAndValue:
				RequestExclusiveLoadDataByFieldNameAndValue_Core_Static(pJob->m_jobData);
				break;
			case CDbCacheJobQueue::JobType_ExclusiveLoadDataByGuid:
				RequestExclusiveLoadDataByGuid_Core_Static(pJob->m_jobData);
				break;
			case CDbCacheJobQueue::JobType_ExclusiveLoadDataByQuery:
				RequestExclusiveLoadDataByQuery_Core_Static(pJob->m_jobData);
				break;
			case CDbCacheJobQueue::JobType_ExclusiveLoadData_New:
				RequestExclusiveLoadNewData_Core_Static(pJob->m_jobData);
				break;
			default:
				//!< TODO:: Error ......
				break;
			}

			delete pJob;
		}
	}
}
