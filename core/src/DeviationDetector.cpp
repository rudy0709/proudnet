/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <new>
#include <math.h>

#include "DeviationDetector.h"
#include "../include/Exception.h"

namespace Proud
{

	CDeviationDetector::~CDeviationDetector(void)
	{
	}

	void CDeviationDetector::Push( double value )
	{
		m_pushCount++;

		double seriesAvgSoFar=m_seriesSum/m_seriesCount;
		m_seriesSum+=value;
		m_seriesCount++;
		
		if (m_seriesCount > m_seriesMaxLength)
			m_seriesSum-=seriesAvgSoFar;
	}

	CDeviationDetector* CDeviationDetector::New( double average, double threshold, int seriesLength )
	{
		if (seriesLength <= 0 || threshold <= 0)
			ThrowBadParameter();

		CDeviationDetector* ret = new CDeviationDetector;

		ret->m_seriesMaxLength = seriesLength;
		ret->m_average = average;
		ret->m_seriesSum=average;
		ret->m_seriesCount=1;
		ret->m_threshold = threshold;
		ret->m_pushCount = 0;

		return ret;
	}

	void CDeviationDetector::ThrowBadParameter()
	{
		throw Exception("Incorrect parameter!");
	}

	CDeviationDetector::CDeviationDetector()
	{
		m_seriesSum=0;
		m_seriesCount=0;
	}

	double CDeviationDetector::GetSeriesAverage()
	{
		return m_seriesSum/m_seriesCount;
	}

	bool CDeviationDetector::IsSeriesDeviated()
	{
		// 처음 한동안 오는 값 시리즈는 평균을 내도록 한다. 이때는 전체적 평균의 범위 이탈을 무시한다.
		if (m_pushCount <= m_seriesMaxLength*2)
			return false;

		// 전체적 평균이 의도한 범위를 크게 벗어나면 잡아내도록 한다
		double currentAvg = GetSeriesAverage();
		return (fabs(currentAvg - m_average) > m_threshold);
	}
}