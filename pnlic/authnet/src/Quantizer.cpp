/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include "../include/Quantizer.h"

namespace Proud
{
	CQuantizer::CQuantizer( double min, double max, uint32_t granulation )
	{
		m_min=min;
		m_max=max;
		m_granulation=granulation;
	}

	CQuantizer::~CQuantizer(void)
	{
	}

	uint32_t CQuantizer::Quantize( double value )
	{
		value = PNMIN(m_max,value);
		value = PNMAX(m_min,value);
		double range = m_max - m_min;
		double offset = value - m_min;
		double offset2 = offset / range * (double)m_granulation;
		return (uint32_t)offset2;
	}

	double CQuantizer::Dequantize( uint32_t value )
	{
		double offset = (double)value;	
		double range = m_max - m_min;
		double offset2 = offset / (double)m_granulation * range;
		return offset2 + m_min;
	}

	// 스트림->값
	void CCompactScalarValue::MakeBlock( int64_t src )
	{
		m_filledBlockLength = 0;

		bool bNegative;	 // src가 음수인가?
		if (src < 0)
		{
			bNegative = true;
			src = ~src; // 예: -64 => 63
		}
		else
		{
			bNegative = false;
		}

		while (1)
		{
			char oneByte = 0;
			oneByte = src & 0x7f;  // 하위 7비트
			src >>= 7;	// 절대값에서 기록한 비트만큼 제거

			if (src != 0)  // 값을 저장하기 위해 더 많은 비트가 사용되고 있으면
			{
				WriteByte(oneByte | 0x80);  //  다음 내용이 있음을 의미하는 8th bit를 set 후 1바이트 기록. 그리고 다음 턴으로.
			}
			else // 이제 기록할 값이 6 or bit만 있는 경우
			{
				// 기록할 값이 7bit이 경우 1vvv vvvv 0s00 0000를, 6bit인 경우 0svv vvvv를 기록한다.
				if (oneByte & 0x40)
				{
					WriteByte((oneByte | 0x80)); // 7개 비트를 기록한다. 
					oneByte = 0;		// 이제 더 이상 기록할 bit가 없으므로 다음 byte에서는 0개의 zero bit가 들어간다.
				}

				if (bNegative)
					oneByte |= 0x40; // 6th bit는 sign bit이다.

				WriteByte(oneByte); // 기록하고 종료
				break;
			}

		}
	}

	// 값->스트림
	bool CCompactScalarValue::ExtractValue(uint8_t* src, int srcLength)
	{
		m_extracteeLength = 0;
		m_src = src;
		m_srcLength = srcLength;

		char oneByte = 0;
		int64_t fillee = 0; // 여기에 읽혀진 byte들이 채워짐
		int leftShiftOffset = 0;
		
		while (1)
		{
			if (m_extracteeLength == 10)
				return false; // int64에서 가능한 최대 크기는 10다. 이를 넘어갔다는 얘기는, 데이터가 뭔가 깨진 것이다.

			if (!ReadByte(oneByte))
				return false;  // 읽기 실패 처리

			if ((oneByte & 0x80) == 0) // 8th bit가 꺼져있으면 뒤에 더 읽을 것이 없다.
			{
				fillee |= (int64_t(oneByte & 0x3f) << leftShiftOffset); // 6개 하위 비트를 읽어서 다 채움

				if (oneByte & 0x40) // sign bit
					m_extractedValue = ~fillee;
				else
					m_extractedValue = fillee;

				return true;
			}
			else
			{
				// 7개 bit를 읽어 처리하고 나머지를 더 처리하자.
				fillee |= (int64_t(oneByte & 0x7f) << leftShiftOffset);
				leftShiftOffset += 7;
			}
		}
	}
}