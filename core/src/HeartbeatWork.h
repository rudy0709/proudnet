#pragma once

#include "../include/Singleton.h"
#include "../include/ListNode.h"
#include "../include/FastArray.h"
#include "../include/FakeClrBase.h"

namespace Proud 
{
#ifndef __GNUC__
	class CHeartbeatWorkThread;

	class IHeartbeatWork
	{
		friend CHeartbeatWorkThread;
		CHeartbeatWorkThread* m_owner;
	public:
		IHeartbeatWork();
		virtual ~IHeartbeatWork() {}
		virtual void Heartbeat() = 0;
	};

	/* 스레드 풀에서 일정 시간마다 유저 함수가 호출되게 하는 기능 제공.

	사용법: 
	이 객체는 싱글톤.
	Register()로 등록.
	Unregister()로 등록 해제. 
	*/
	class CHeartbeatWorkThread:public CSingleton<CHeartbeatWorkThread>
	{
		CriticalSection m_cs;
		class WorkData:public CListNode<WorkData>
		{
		public:
			IHeartbeatWork* m_obj;
			uint32_t m_lastWorkTime;
			uint32_t m_interval;
			volatile bool m_working;
			
			WorkData();
		};		
		// 맨 앞에 있는 항목은 처리되면 맨 뒤로 옮겨진다.
		class WorkDataList:public CListNode<WorkData>::CListOwner
		{
		public:
		};
		WorkDataList m_workList; // per CPU core
		CFastArray<ThreadPtr> m_threadList;

		static void StaticWorkerProc(void* ctx);
		void WorkerProc();

		volatile bool m_keepWork;

	public:
		CHeartbeatWorkThread(void);
		~CHeartbeatWorkThread(void);

		void Register(IHeartbeatWork* obj, uint32_t intervalMilisec);
		void Unregister(IHeartbeatWork* obj);
	};
#endif
}