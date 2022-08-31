#pragma once

#include <map>
#include "../include/HlaSpace_S.h"
#include "../include/Enums.h"
#include "../include/FastSet.h"

using namespace std;

namespace Proud
{
	class IHlaViewer_S;
	class CHlaSessionHostImpl_S;

	class CHlaSpaceInternal_S
	{
	public:
		// 이 space에 종속된 entity들
		CFastSet<CHlaEntity_S*> m_entities;

		// 정렬되어 있어야 old space to new space 이동시 disappear,appear,n/a를 선별이 빠름
		map<HostID, IHlaViewer_S*> m_viewers;

		~CHlaSpaceInternal_S()
		{
			assert(m_entities.IsEmpty());
			assert(m_viewers.empty());
		}
	};
}
