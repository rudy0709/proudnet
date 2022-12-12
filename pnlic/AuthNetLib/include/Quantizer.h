/*
ProudNet v1.7


이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의 : 저작물에 관한 위의 명시를 제거하지 마십시오.


This program is soley copyrighted by Nettention.
Any use, correction, and distribution of this program are subject to the terms and conditions of the License Agreement.
Any violated use of this program is prohibited and will be cause of immediate termination according to the License Agreement.

** WARNING : PLEASE DO NOT REMOVE THE LEGAL NOTICE ABOVE.


此程序的版权归Nettention公司所有。
与此程序的修改、使用、发布相关的事项要遵守此程序的所有权者的协议。
不遵守协议时要原则性的禁止擅自使用。
擅自使用的责任明示在与此程序所有权者的合同书里。

** 注意：不要移除关于制作物的上述明示。


このプログラムの著作権はNettentionにあります。
このプログラムの修正、使用、配布に関する事項は本プログラムの所有権者との契約に従い、
契約を遵守しない場合、原則的に無断使用を禁じます。
無断使用による責任は本プログラムの所有権者との契約書に明示されています。

** 注意：著作物に関する上記の明示を除去しないでください。

*/

#pragma once

#include "ProcHeap.h"
#include "Exception.h"

namespace Proud
{
#if (_MSC_VER>=1400)
#pragma managed(push, off)
#endif

	/** \addtogroup util_group
	*  @{
	*/

	/** 
	\~korean
	\ref quantizer  기능을 담당하는 클래스입니다.

	\~english
	Class manages \ref quantizer function

	\~chinese
	担任\ref quantizer%功能的类 。

	\~japanese
	\~
	*/
	class CQuantizer
	{
		double m_min,m_max;
		uint32_t m_granulation;
	public:
		/** 
		\~korean
		생성자
		\param min 양자화되는 값은 이 값 이하로는 들어가지 않습니다.
		\param max 양자화되는 값은 이 값 이상으로는 들어가지 않습니다.
		\param granulation 양자화되는 값은 정수 타입입니다. 본 값은 양자화되는 값이 0부터 얼마까지의 정수형으로 변환되느냐를 지칭합니다.
		값이 클수록 양자화된 값의 정밀도가 높지만 양자화된 데이터가 필요로 하는 비트수가 증가하게 됩니다. 

		\~english
		Constructor
		\param min the quantized value will not be entered and no less than this value.
		\param max the quantized value will not be entered and no more than this value.
		\param granulation the quantized value has integer type. This value points that the quantized value is to be converted from 0 to which integer.
		Larger value provides more pricise quantized value but the number of bits required by quantized data.

		\~chinese
		\param min 输入量子化的值不能低于此值。
		\param ma 输入量子化的值不能高于此值。
		\param granulation 量子化的值是正数类。此值是指称量子化的值转换为从0到多少的正数类。
		值越大，量子化的值精度越大，但是量子化的数据需要的bit值会增加。

		\~japanese
		\~
		*/
		CQuantizer( double min, double max, uint32_t granulation );
		~CQuantizer(void);

		/** 
		\~korean
		양자화를 합니다. 

		\~english
		Quantizing

		\~chinese
		进行量子化。

		\~japanese
		\~
		*/
		uint32_t Quantize(double value);

		/** 
		\~korean
		양자화된 값을 복원합니다. 

		\~english
		Recover quantized value

		\~chinese
		恢复量子化的值。

		\~japanese
		\~
		*/
		double Dequantize(uint32_t value);

#ifdef _WIN32
#pragma push_macro("new")
#undef new
		// 이 클래스는 ProudNet DLL 경우를 위해 커스텀 할당자를 쓰되 fast heap을 쓰지 않는다.
		DECLARE_NEW_AND_DELETE
#pragma pop_macro("new")
#endif
	};

	// srcclr에서 쓰므로 노출시킨 API이지만 사용자가 직접 쓰는건 비추.
	class CCompactScalarValue
	{
		// 1바이트 처리.
		// 내부적으로 사용된다.
		inline void WriteByte( char a )
		{
			char* ptr = (char*)(m_filledBlock + m_filledBlockLength);
			// http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.faqs/ka15414.html 에 의하면,
			// GCC는 char* 타입에 대해서 1byte align을 가정하고 작동하게 되어있다.
			// 즉, 컴파일러가 알아서 4byte align에 맞게 조작하게 작동된다. 
			// 따라서 과거 코드(marmalade에서는 memcpy사용)이 불필요하다. (memcpy(ptr, &a, sizeof(a));)
			*ptr = a; 
			m_filledBlockLength += sizeof(char);
		}

		// 1바이트 처리.
		// 내부적으로 사용된다.
		inline bool ReadByte( char &a )
		{
			if(m_extracteeLength + (int)sizeof(a) > m_srcLength)
				return false;

			// http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.faqs/ka15414.html 에 의하면,
			// GCC는 char* 타입에 대해서 1byte align을 가정하고 작동하게 되어있다.
			// 즉, 컴파일러가 알아서 4byte align에 맞게 조작하게 작동된다. 
			// 따라서 과거 코드(marmalade에서는 memcpy사용)이 불필요하다. (memcpy(ptr, &a, sizeof(a));)
			a = *(char*)(m_src + m_extracteeLength);

			m_extracteeLength += sizeof(a);

			return true;
		}

		uint8_t* m_src;
		int m_srcLength;
	public:
		// MakeBlock에서 사용됨
		uint8_t m_filledBlock[100];
		int m_filledBlockLength;
		
		// ExtractValue에서 사용됨
		int64_t m_extractedValue;
		int m_extracteeLength; // read offset

		// 읽기/쓰기 주요 함수
		void MakeBlock(int64_t src);
		bool ExtractValue( uint8_t* src, int srcLength );
	};

	/**  @} */
#if (_MSC_VER>=1400)
#pragma managed(pop)
#endif
}
