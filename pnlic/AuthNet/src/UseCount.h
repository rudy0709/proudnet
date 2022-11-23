/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#pragma once

#include "../include/BasicTypes.h"
#include "../include/AddrPort.h"
#include "SendOpt.h"
//#include "SendFragRefs.h"
#include "FilterTag.h"
#include "IHostObject.h"

//#pragma pack(push,8)

namespace Proud
{
#if (_MSC_VER>=1400)
#pragma managed(push, off)
#endif


	// UDP socket, Remote 객체들 등은 NO lock(main) 상태에서 NO dispose 보장해야 한다. 이를 체크하기 위한 객체
	// use count가 1 이상이면 비파괴 보장을 한다. 
	class CUseCount
	{
	private:
		PROUDNET_VOLATILE_ALIGN int32_t m_inUseCount;
	public:
		PROUD_API CUseCount(void);
		virtual ~CUseCount(void);

		PROUD_API int GetUseCount();

		PROUD_API void IncreaseUseCount();
		PROUD_API void DecreaseUseCount();

		PROUD_API void AssertIsZeroUseCount();

#ifdef PN_LOCK_OWNER_SHOWN
		// CUseCount를 상속받은 객체가 현재 lock중인지 확인한다.
		// 일부분이라도 lock이면 true. 
		virtual bool IsLockedByCurrentThread() = 0;
#endif
	};

	class CUseCounter
	{
	private:
		CUseCount* m_useCount;
	public:
		CUseCounter(CUseCount& useCount);
		~CUseCounter();
	};

	/**  @} */
#if (_MSC_VER>=1400)
#pragma managed(pop)
#endif
}


//#pragma pack(pop)