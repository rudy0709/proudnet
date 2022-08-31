#pragma once

#include "../include/Singleton.h"
#include "../include/FastArray.h"
#include "../include/IRMIProxy.h"
#include "../include/IRmiStub.h"
#include "FastList2.h"
#include "../include/PNSemaphore.h"

namespace Proud
{
	class CDbCacheJobQueue
	{
		friend class CDbCacheServer2Impl;

	public:
		enum JobType
		{
			JobType_None = 0,
			JobType_SaveToDbmsContext,
			JobType_ExclusiveLoadDataByFieldNameAndValue,
			JobType_ExclusiveLoadDataByGuid,
			JobType_ExclusiveLoadDataByQuery,
			JobType_ExclusiveLoadData_New,
			JobType_Max
		};

		struct JobData
		{
			JobType	m_jobType;
			void * m_jobData;
		};
		typedef CFastList2<JobData *, int> JobList;

	public:
		CDbCacheJobQueue():m_jobSemaphore(0,INT32_MAX) { }
		~CDbCacheJobQueue() { }


		template <typename T>
		void PushJob(JobType JobType, T JobData)
		{
			CDbCacheJobQueue::JobData * pstJobData = new CDbCacheJobQueue::JobData;
			pstJobData->m_jobType = JobType;
			pstJobData->m_jobData = (void *)JobData;

			{
				CriticalSectionLock lock(m_csJobQueue, true);
				m_listJob.AddTail(pstJobData);
			}

			m_jobSemaphore.Release();
		}
		CDbCacheJobQueue::JobData * PopJob()
		{
			JobData * pJobData = NULL;
			{
				CriticalSectionLock lock(m_csJobQueue, true);
				if (m_listJob.GetCount() <= 0)
					return NULL;

				pJobData = m_listJob.RemoveHead();
			}
			return pJobData;
		}
		bool IsEmpty() const
		{
			CriticalSectionLock lock(m_csJobQueue, true);
			return m_listJob.GetCount() > 0 ? false : true;
		}
		Semaphore * GetSemaphore() { return &m_jobSemaphore; }

	protected:
		mutable CriticalSection		m_csJobQueue;
		JobList				m_listJob;
		Semaphore			m_jobSemaphore;
	};

	class CDbCacheServer2Impl;
	class CDBCacheWorker
	{
	private:
		CDbCacheServer2Impl* m_owner;

	public:
		CDBCacheWorker(int ThreadCount, CDbCacheServer2Impl *owner) { m_bRun = false; m_ThreadCount = ThreadCount; m_owner = owner; }
		~CDBCacheWorker() { Stop(); }

		void Start();
		void Stop();

	protected:
		static void WorkProc(void * ctx);;

		void WorkThread();

	protected:
		volatile bool	m_bRun;

		int			m_ThreadCount;
		CFastArray<ThreadPtr> m_threadList;
	};
}
