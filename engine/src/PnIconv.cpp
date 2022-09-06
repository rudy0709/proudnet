#include "stdafx.h"
#include "PnIconv.h"

#ifdef PROUDNET_HAS_ICONV
// iconv를 제공 안하는 OS들은 이것을 쓴다.
#include "../OpenSources/libiconv-v1.14/include/iconv.h"
#else
#include <iconv.h>
#endif

#include "../include/StringEncoder.h"
#include "../include/Exception.h"

namespace Proud
{
	CPnIconv::CPnIconv()
	{
		m_cd = NULL;
	}

	CPnIconv::~CPnIconv()
	{
		if(m_cd != NULL)
			iconv_close((iconv_t)m_cd);
	}

	/* KJSA : 2013.06.26
	*  iconv_t를 사용하기 위해서는 iconv_open 통해 파라미터를 두개를 받아서 초기화를 해주어야 하는데
	*  CObjectPool 내부에서는 디폴트 생성자 이외 호출할 수 없다.
	*  따라서 초기화 함수를 통해서 따로 초기화 해주어야 한다.
	*/
	bool CPnIconv::InitializeIconv(const char* src, const char* dest)
	{
		/* seungchul.lee : 2014.05.28
		 * ProudNet에서는 Win32 Platform에서는 Codepage가 기본적으로 CP949로 설정되어 있다.
		 * StringW2A 혹은 StringA2W 함수에서 StringEncoder를 사용하지 않고, CP949에서 지원되지 않는 문자(예. 중국어)를 입력할 경우 예외가 발생한다.
		 * 이렇게 예외를 던지는 것보단 해당 문자가 변환에 실패하는 것이 낫다고 판단하(사장님)에 아래와 같은 Suffix를 추가해주었다.
		 * 참고: http://man7.org/linux/man-pages/man3/iconv_open.3.html */
		string suffixes = dest;
		suffixes.append("//TRANSLIT//IGNORE");
		m_cd = iconv_open(suffixes.c_str(), src);

		if (m_cd == NULL)
			return false;
		else
			return true;
	}

	void iconv_string_convert(CStringEncoder* encoder, const char* input, size_t* inbytesleft, char* out, size_t* outbytesleft)
	{
		CPnIconv* iconv_obj = encoder->GetIconv();
		// 내장된 iconv는 입력 인자가 const인데 linux sdk는 그렇지 않다. (사실 전자가 맞음)
		// 아무튼 맞춰 주어야 -_-;
#ifdef PROUDNET_HAS_ICONV
		if (iconv((iconv_t)(iconv_obj->m_cd), &input, inbytesleft, &out, outbytesleft) == -1)
#else
		if (iconv((iconv_t)(iconv_obj->m_cd), (char**)&input, inbytesleft, &out, outbytesleft) == -1)
#endif
		{
			encoder->ReleaseIconv(iconv_obj);
			throw Exception("iconv convert error");

		}

		encoder->ReleaseIconv(iconv_obj);
	}
}
