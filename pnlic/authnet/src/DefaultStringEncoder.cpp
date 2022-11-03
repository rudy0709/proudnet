#include "stdafx.h"
#include "DefaultStringEncoder.h"

namespace Proud
{
	CDefaultStringEncoder::CDefaultStringEncoder()
	{		
		const char* srcCodepage;
		const char* destCodepage;

#if (WCHAR_LENGTH == 2)
#if defined __GNUC__// MARMALADE ARM, etc.
		srcCodepage = "UTF-8";
		destCodepage = "UTF-16LE";
#else
		srcCodepage = "CP949";
		destCodepage = "UTF-16LE";
#endif
#elif (WCHAR_LENGTH == 4) // NDK and MAC
		srcCodepage = "UTF-8";
		destCodepage = "UTF-32LE";
#else
	#error You must include WCHAR_LENGTH definition!
#endif

		m_A2WStringEncoder = CStringEncoder::Create(srcCodepage, destCodepage);
		m_W2AStringEncoder = CStringEncoder::Create(destCodepage, srcCodepage);

#if (WCHAR_LENGTH == 4)
		m_UTF8toUTF16LEEncoder = CStringEncoder::Create("UTF-8", "UTF-16LE");
		m_UTF16LEtoUTF8Encoder = CStringEncoder::Create("UTF-16LE", "UTF-8");
		m_UTF32LEtoUTF16LEEncoder = CStringEncoder::Create("UTF-32LE", "UTF-16LE");
		m_UTF16LEtoUTF32LEEncoder = CStringEncoder::Create("UTF-16LE", "UTF-32LE");
#endif
	}

	CDefaultStringEncoder::~CDefaultStringEncoder()
	{
		delete m_A2WStringEncoder;
		delete m_W2AStringEncoder;
#if (WCHAR_LENGTH == 4)
		delete m_UTF8toUTF16LEEncoder;
		delete m_UTF16LEtoUTF8Encoder;
		delete m_UTF32LEtoUTF16LEEncoder;
		delete m_UTF16LEtoUTF32LEEncoder;
#endif
	}
}