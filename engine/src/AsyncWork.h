#pragma once

#include "../include/atomic.h"
#include "../include/BasicTypes.h"

namespace Proud
{
	class CAsyncWork;

	/*  비동기로 수행하는 작업이 모두 완료됐는지를 확인하는 역할을 한다.
	사용법: 메인 객체가 이것 인스턴스를 가진다.
	CAsyncWorker는 이것을 참조하게 해서 생성한다.
	메인 객체가 역할을 종료하기 전에 모든 비동기수행 작업이 끝났음을 확인하기 위해 지속적으로 IsFinished를 호출한다.
	*/
	class CAsyncWorksOwner
	{
		friend class CAsyncWork;
		// 기록이 대기중인 작업들
		volatile int m_asyncWorkCount;

	public:
		PROUD_API CAsyncWorksOwner();
		PROUD_API bool IsFinished();
	};

	// 사용자는 이것을 상속받아서 필요한 비동기 작업을 수행하기 위한 인자로서 쓰도록 하자.
	class CAsyncWork
	{
	public:
		CAsyncWorksOwner* m_owner;

		PROUD_API CAsyncWork(CAsyncWorksOwner* owner);
		PROUD_API virtual ~CAsyncWork();
	};
}
