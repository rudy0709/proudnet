#pragma once

#include "../include/Enums.h"

namespace Proud
{
	// P2P 직빵 연결이 된 클라이언트 리스트
	// LAN 클섭에서는: P2P 직빵 연결(TCP)이 된 클라이언트 리스트
	struct RCPair
	{
		HostID m_first, m_second;
	};

	class RCPairTraits
	{
	public:
		typedef const RCPair& INARGTYPE;
		typedef RCPair& OUTARGTYPE;

		inline static uint32_t Hash( const RCPair& element )
		{
			assert(sizeof(HostID) == 4);

			return (int(element.m_first)<<16) ^ int(element.m_second);
			//return CPNElementTraits<ULONGLONG>::Hash( (ULONGLONG(element.m_first)<<32) + (ULONG)element.m_second );
		}

		inline static bool CompareElements( INARGTYPE element1, INARGTYPE element2 ) throw()
		{
			return element1.m_first == element2.m_first &&
				element1.m_second == element2.m_second;
		}

		inline static int CompareElementsOrdered( INARGTYPE l, INARGTYPE r ) throw()
		{
			if (l.m_first < r.m_first)
				return -1;
			if (l.m_first > r.m_first)
				return 1;
			if (l.m_second < r.m_second)
				return -1;
			else if (l.m_second == r.m_second)
				return 0;
			else
				return 1;
		}
	};
}
