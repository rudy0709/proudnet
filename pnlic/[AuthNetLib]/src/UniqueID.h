#pragma once 

#include "../include/BasicTypes.h"

namespace Proud
{
	// 릴레이나 라우트될 메시지의 최종 대상을 향해 송신을 할 때를 빼고 이 값이 쓰인다.
	enum UniqueID_RelayerSeparator
	{
		UniqueID_RelayerSeparator_None = 0,
		UniqueID_RelayerSeparator_Relay1 = 1,
		UniqueID_RelayerSeparator_RoutedMulticast1,
	};

	// 사용자가 입력한 unique ID와 내부적으로 식별자로 사용되는 internal 값의 조합.
	struct UniqueID
	{
		// 사용자가 입력한 unique ID.
		// 이 값만이 유의미하게 사용된다.
		int64_t m_value;

		/* 서버도 포함된 P2P Group G={C1,C2,S}가 있다. C1는 릴레이 상태다.
		클라가 G에게 unique ID를 넣고 메시지를 보낸다.
		이때 클라는 서버에게 C2S 메시지[1]와 함께 MessageType_...Relay1... 메시지[2]가 보내진다. To-server TCP or UDP를 통해.
		이때 [1]의 unique ID와 [2]의 것은 서로 구별되어야 한다. 하지만 사용자는 이미 동일한 unique ID를 넣었다.
		이러한 경우 보내는 쪽에서는 구별해서 보내줄 필요가 있다.

		따라서 사용자는 unique ID를 넣더라도 내부적으로 그것에 대한 추가적인 식별 값을 넣어주어야 한다.
		이는 그러한 목적으로 사용된다. 
		
		UniqueID_RelayerSeparator_XXX 중 하나가 쓰인다. */
		char m_relayerSeparator;

		inline UniqueID() :m_value(0), m_relayerSeparator(UniqueID_RelayerSeparator_None) {}

		inline bool operator==(const UniqueID& rhs) const
		{
			return m_value == rhs.m_value && m_relayerSeparator == rhs.m_relayerSeparator;
		}

		inline bool operator<(const UniqueID& rhs) const
		{
			// lexicographical compare
			if (m_value < rhs.m_value)
				return true;
			if (m_value > rhs.m_value)
				return false;

			if (m_relayerSeparator < rhs.m_relayerSeparator)
				return true;
			if (m_relayerSeparator > rhs.m_relayerSeparator)
				return false;

			return 0;

		}
	};



	class UniqueIDTraits
	{
	public:
		typedef const UniqueID& INARGTYPE;
		typedef UniqueID& OUTARGTYPE;

		inline static uint32_t Hash(const UniqueID& element)
		{
			return CPNElementTraits<int64_t>::Hash(element.m_value) ^ CPNElementTraits<char>::Hash(element.m_relayerSeparator << 24);
		}

		inline static bool CompareElements(const UniqueID& element1, const UniqueID& element2)
		{
			return (element1 == element2);
		}

		inline static int CompareElementsOrdered(const UniqueID& element1, const UniqueID& element2)
		{
			if (element1 < element2)
			{
				return(-1);
			}
			else if (element1 == element2)
			{
				return(0);
			}
			else
			{
				//assert(element1 > element2); 쓸데없이 operator> 함수 요구하니까 막음
				return(1);
			}
		}
	};


}