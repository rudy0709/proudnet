#include "stdafx.h"
#include "CriticalSectImpl.h"
#include "HeartbeatWork.h"
#include "../include/sysutil.h"

namespace Proud 
{
#ifndef __GNUC__
	CHeartbeatWorkThread::CHeartbeatWorkThread(void)
	{
		m_keepWork = true;

		int cpuCount = GetNoofProcessors();

		// 스레드 준비 및 시작
		for(int i=0;i<cpuCount;i++)
		{
			m_threadList.Add(ThreadPtr(new Thread(StaticWorkerProc,this)));
		}

		for(int i=0;i<cpuCount;i++)
		{
			m_threadList[i]->Start();
		}
	}

	CHeartbeatWorkThread::~CHeartbeatWorkThread(void)
	{
		CriticalSectionLock lock(m_cs,true);
		// 모든 work list가 다 청소된 채로 파괴자가 호출되어야 한다.
		if(m_workList.GetCount() > 0)
			ShowUserMisuseError(_PNT("CHeartbeatWorkThread must be cleaned up only after every work items are unregistered!"));

		// 스레드 종료
		m_keepWork = false;
		for(int i=0;i<(int)m_threadList.GetCount();i++)
			m_threadList[i]->Join();
	}

	void CHeartbeatWorkThread::Register( IHeartbeatWork* obj, uint32_t intervalMilisec )
	{
		// 워커 스레드에서 실행하는게 아니어야 함!
		Proud::AssertIsNotLockedByCurrentThread(m_cs);
		CriticalSectionLock lock(m_cs,true);

		if(obj->m_owner)
			ThrowInvalidArgumentException();

		// 새 항목을 맨 끝에 추가
		WorkData *wk = new WorkData;
		wk->m_obj = obj;
		wk->m_interval = intervalMilisec;
		
		obj->m_owner = this;

		m_workList.PushBack(wk);
	}

	void CHeartbeatWorkThread::Unregister( IHeartbeatWork* obj )
	{
		// 워커 스레드에서 실행하는게 아니어야 함!
		Proud::AssertIsNotLockedByCurrentThread(m_cs);
		CriticalSectionLock lock(m_cs,true);

		if(!obj->m_owner)
			return;

		if(obj->m_owner != this && obj->m_owner)
			ThrowInvalidArgumentException();

		// 항목을 찾아서 제거
		for(WorkData* wd = m_workList.GetFirst(); wd; wd = wd->GetNext())
		{
			if(wd->m_obj == obj)
			{
				obj->m_owner = NULL;
				m_workList.Erase(wd);
				//IHeartbeatWork* work = wd->m_obj; 2009.12.15 참조되지 않은 지역변수 주석

				lock.Unlock(); // 먼저 이렇게 해야 한다.

				// 더 이상 이 객체에 의해 사용중이 아닐 때까지 대기하기.
				// CHeartbeatWorkThread가 singleton이기에 이런 로직이 가능.
				while(wd->m_working)
				{
					Sleep(10);
				}

				delete wd;
				return;
			}
		}
	}

	void CHeartbeatWorkThread::StaticWorkerProc( void* ctx )
	{
		CHeartbeatWorkThread* c = (CHeartbeatWorkThread*)ctx;
		c->WorkerProc();
	}

	void CHeartbeatWorkThread::WorkerProc()
	{
		while(m_keepWork)
		{
			// 맨 앞의 항목을 꺼내서 시간이 되었으면 처리하도록 한다. 그리고 맨 뒤로 옮기기.
			CriticalSectionLock lock(m_cs,true);

			uint32_t currTime = CFakeWin32::GetTickCount();
			WorkData* wd = m_workList.GetFirst();
			if(wd && currTime - wd->m_lastWorkTime > wd->m_interval)
			{
				wd->m_lastWorkTime = currTime;
				//IHeartbeatWork* work = wd->m_obj;  2009.12.15 참조되지 않은 지역변수 주석

				m_workList.Erase(wd);
				m_workList.PushBack(wd);

				lock.Unlock();
				wd->m_working = true;
				wd->m_obj->Heartbeat();
				wd->m_working = false;
			}

			if(lock.IsLocked())
				lock.Unlock();

			Sleep(10);
		}
	}

	IHeartbeatWork::IHeartbeatWork()
	{
		m_owner = NULL;
	}

	CHeartbeatWorkThread::WorkData::WorkData()
	{
		m_lastWorkTime = 0;
		m_working = false;
	}
#endif
}