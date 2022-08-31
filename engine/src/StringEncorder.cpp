#include "stdafx.h"
#include "../include/StringEncoder.h"
#include "../include/Exception.h"
#include "FreeList.h"
#include "PnIconv.h"

namespace Proud
{
	class CStringEncoderImpl
	{
	public:
		// 소스 인코딩 식별자. m_cdPool에 항목 추가때마다 참조된다.
		const char* m_srcCodpageStr;
		// 위와 마찬가지
		const char* m_destCodpageStr;

		// 아래를 보호
		CriticalSection m_cdPoolCritSec;

		// 여러 스레드에서 동시다발적으로 CStringEncoder 객체를 사용할 때, iconv_t 한개가 여러 스레드에서 동시에 억세스되면 안된다.
		// 따라서 이것을 두어서 동시에 필요로 하는 스레드에게 할당해주는 역할을 한다.
		CObjectPool<CPnIconv> m_converterPool;

		CStringEncoderImpl(const char* srcCodepage, const char* destCodepage)
		{
			m_srcCodpageStr = srcCodepage;
			m_destCodpageStr = destCodepage;
		};

		~CStringEncoderImpl(){};
	};

	/* KJSA : 2013.06.26
	*  Pimpl 패턴을 이용하여 캡슐화 한 객체를 생성자에서 생성한다.
	*/
	CStringEncoder::CStringEncoder(const char* srcCodepage, const char* destCodepage)
	{
		m_pimpl = new CStringEncoderImpl(srcCodepage, destCodepage);
	}

	/* KJSA : 2013.06.26
	*  Pimpl 패턴을 이용하여 캡슐화 한 객체를 소멸자에서 삭제한다.
	*/
	CStringEncoder::~CStringEncoder()
	{
		delete m_pimpl;
	}

	/* CObjectPool에서 CPnIconv 객체 하나를 리턴한다.
	Pool에 사용 가능한 객체가 없을 경우 객체를 생성하고 초기화 시킨다.
	초기화 중 에러가 발생할 경우 Exception을 throw한다.
	쓰고 나서는 ReleaseIconv로 반환되어야 한다.
	*/
	CPnIconv* CStringEncoder::GetIconv()
	{
		CriticalSectionLock lock(m_pimpl->m_cdPoolCritSec, true);

		CPnIconv * ret = m_pimpl->m_converterPool.NewOrRecycle();

		if(ret->m_cd == NULL)
		{
			if(!ret->InitializeIconv(m_pimpl->m_srcCodpageStr, m_pimpl->m_destCodpageStr))
				throw new Exception("iconv_open exception");
		}

		return ret;
	}

	/* KJSA : 2013.06.26
	*  사용한 CPnIconv 객체를 반납한다.
	*/
	void CStringEncoder::ReleaseIconv(CPnIconv *obj)
	{
		CriticalSectionLock lock(m_pimpl->m_cdPoolCritSec, true);

		m_pimpl->m_converterPool.Drop(obj);
	}

	/* KJSA : 2013.06.26
	*  CStringEncoder 객체를 생성한다.
	*/
	CStringEncoder* CStringEncoder::Create(const char* srcCodepage, const char* destCodepage)
	{
		return new CStringEncoder(srcCodepage, destCodepage);
	}
}
