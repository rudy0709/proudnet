/*
ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.

ProudNet

This program is soley copyrighted by Nettention.
Any use, correction, and distribution of this program are subject to the terms and conditions of the License Agreement.
Any violated use of this program is prohibited and will be cause of immediate termination according to the License Agreement.

** WARNING : PLEASE DO NOT REMOVE THE LEGAL NOTICE ABOVE.

*/
#pragma once

#include "../include/FastArray.h"

namespace Proud
{
	/* 입력값 시리즈를 계속 받으면서, 입력값 시리즈들이 지나치게 표준 범위를 벗어나서 들어오고 있으면
	감지하는 역할을 한다.

	예컨대 스피드핵 감지에서 쓰인다.

	주요 특징
	- 처음 한동안 오는 값 시리즈는 평균을 내도록 한다. 이때는 전체적 평균의 범위 이탈을 무시한다.
	- 전체적 평균에 비해 확확 튀는 값은 무시하도록 한다.....가 아님.
	- 전체적 평균이 의도한 범위를 크게 벗어나면 잡아내도록 한다. IsSeriesDeviated로.
	*/
	class CDeviationDetector
	{
		// 총 누적된 값들의 총합
		double m_seriesSum;
		// 총 누적된 값을의 갯수
		double m_seriesCount;

		int m_seriesMaxLength;
		// 기대하고 있던 평균
		double m_average;

		// 넘어서지 말아야 할 편차
		double m_threshold;

		// 값이 누적된 총 횟수
		int m_pushCount;

		CDeviationDetector();
		static void ThrowBadParameter();
	public:
		~CDeviationDetector(void);

		/** 객체 생성 함수
		- 표준 범위의 중심값 average와 표준 범위를 중심으로 하는 양측 범위 threshold를
		지정한다. */
		static CDeviationDetector* New(double average, double threshold, int seriesLength);

		/** 값 1개를 입력한다. */
		void Push(double value);

		void Reset(double average, double threshold, int seriesLength);

		/** 입력되고 있는 값들이 평균의 범위를 이미 이탈했는가? */
		bool IsSeriesDeviated();

		double GetSeriesAverage();
	};
}
