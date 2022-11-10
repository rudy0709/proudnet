#pragma once
#include <iostream>
#include <assert.h>
#include <vector>

#include <string>
#include "Util.h"

// 다음 ifdef 블록은 DLL에서 내보내기하는 작업을 쉽게 해 주는 매크로를 만드는 
// 표준 방식입니다. 이 DLL에 들어 있는 파일은 모두 명령줄에 정의된 _EXPORTS 기호로
// 컴파일되며, 동일한 DLL을 사용하는 다른 프로젝트에서는 이 기호를 정의할 수 없습니다.
// 이렇게 하면 소스 파일에 이 파일이 들어 있는 다른 모든 프로젝트에서는 
// COMMONDLL_API 함수를 DLL에서 가져오는 것으로 보고, 이 DLL은
// 이 DLL은 해당 매크로로 정의된 기호가 내보내지는 것으로 봅니다.
#ifdef _UNICODE
#define STRING_CONVERT_T_TO_A(cStringw) (LPCSTR)CW2A(cStringw)
#define STRING_CONVERT_A_TO_T(cStringa) (LPCTSTR)CA2W(cStringa)
#else
#define STRING_CONVERT_T_TO_A(cStringw) (cStringw)
#define STRING_CONVERT_A_TO_T(cStringa) (cStringa)
#endif


struct SimpleByteArray
{
	BYTE* m_pBytes;
	int m_size;

	void SetBuffer(BYTE* pdummy, int size)		{ if(m_pBytes==NULL)SetSize(size);assert(m_size==size); memcpy(m_pBytes, pdummy, size);	m_size=size; }
	void SetSize(int size)						{ assert(m_pBytes==NULL); m_pBytes=new BYTE[size+1]; m_pBytes[size]='\0'; m_size=size;}

	SimpleByteArray(int size):m_pBytes(0)					{ SetSize(size); }
	SimpleByteArray(BYTE* pdummy, int size):m_pBytes(0)		{ SetSize(size); SetBuffer(pdummy,size);}
	SimpleByteArray():m_pBytes(0)							{}
	~SimpleByteArray()										{ if(m_pBytes!=NULL)delete[] m_pBytes; }
};


struct DetectedBinaryWaterMark
{
	SimpleByteArray m_WaterMark;
	UINT m_StartPos;
	UINT m_EndPos;

	void SetDetectedBinary(BYTE* pdummy, int size,UINT StartPos,UINT EndPos)		{m_WaterMark.SetBuffer(pdummy,size); m_StartPos=StartPos; m_EndPos=EndPos;}


	CString ConvertedString()
	{
		CStringW cStringw = (LPCWSTR)CA2W((LPCSTR)(m_WaterMark.m_pBytes), CP_UTF8);

		CString cString;
#ifdef _UNICODE
		cString = cStringw;
#else 
		cString = (LPCSTR)CW2A(cStringw);
#endif 
		return cString;
	}

};
typedef vector<DetectedBinaryWaterMark*> vecDetectedBinaryWaterMark;




//#define TestMode
#ifdef TestMode
#define LOGWRITER(cStringw) CLogWriter::EventLog(cStringw)
#else
#define LOGWRITER(cStringw) __noop(cStringw)
#endif


// 주의!! 이 키값이 바뀌면 워터마크는 무의미 해짐. 앞으로도 절대 바뀌어선 안됨.
extern char g_EncryptKey[];
using namespace std;

class CLogWriter
{
public:
	static void EventLog(CString);

private:
	virtual void WriteLog(CString );
};

extern CLogWriter *g_pLog;


