#include "stdafx.h"
#include "../include/strpool.h"


namespace Proud
{

	CStringPool::CStringPool()
	{
		CriticalSectionLock lock(m_cs,true);
	}

	CStringPool::~CStringPool()
	{
		CriticalSectionLock lock(m_cs,true);
		m_map.Clear();
	}

	Proud::String CStringPool::DumpStatus()
	{
		String ret;

		CriticalSectionLock lock(m_cs,true);
		for(MapType::const_iterator i = m_map.begin();i!=m_map.end();i++)
		{
			String s = i->GetFirst();
			int c = i->GetSecond();

			String fmt;
			fmt.Format(_PNT("{%s:%d}"),s.GetString(),c);
			ret += fmt;
		}

		return ret;
	}

	CStringPool& CStringPool::GetUnsafeRef()
	{
		return CSingleton<CStringPool>::GetUnsafeRef();
	}

	const String CStringPool::GetString( const Proud::String &str )
	{
		CriticalSectionLock lock(m_cs,true);

		MapType::CPair* pNode = m_map.Lookup(str);
		if(!pNode)
		{
			pNode = m_map.SetAt(str, 1);
			return pNode->m_key;
		}
		else
		{
			return pNode->m_key;
		}
	}
}
