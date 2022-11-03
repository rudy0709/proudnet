/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <new>
#include "../include/FakeCLRRandom.h"

namespace Proud
{


	double Random::NextDouble()
	{
		uint32_t d1 = (uint32_t)m_rand.GetInt();
		double d2 = (double)d1 / (double)UINT_MAX;
		return PNMAX(d2, 0);
	}

	int Random::Next( int maxVal )
	{
		double d1 = NextDouble();
		double d2 = d1 * (double)(maxVal + 1);
		int d3 = (int)d2;
		d3 = PNMIN(d3, maxVal);
		return d3;
	}

	// GUID generator와 달리 unique를 보장하지는 않지만 매우 빠른 속도로 guid를 난수 생성한다.
	Proud::Guid Random::NextGuid()
	{
		Guid ret;
		double* ret2 = (double*) & ret;
		for (int i = 0;i < 2;i++)
		{
			ret2[i] = NextDouble();
		}

		return ret;
	}
}