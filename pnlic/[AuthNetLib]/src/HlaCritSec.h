#pragma once

namespace Proud
{
	class IHlaDelegate_Common;

 	/* HLA session 객체는 자체적인 crit sec을 가지지 않고 사용자가 제공하는 것을 사용하도록 만들어져 있다. 
 	entity iteration 등을 접근하는 API를 crit sec을 내부에서 직접 다룰 경우 성능 패널티 때문.
 	따라서 delegate callback을 통해 crit sec을 접근하게 만들어져 있다.
 	HLA 엔진 개발자는, exception 이 발생해도 safe unroll을 위해 이 클래스를 사용하라. 
 	
 	CCriticalSectionLock 클래스와 사용법 동일.
 	*/
 	class CHlaCritSecLock
 	{
		IHlaDelegate_Common* m_lockee;
		bool m_isLocked;
 	public:
 		CHlaCritSecLock(IHlaDelegate_Common* lockee, bool initLock = false);
 		~CHlaCritSecLock();
 
 		void Lock();
 		void Unlock();
 	};
}