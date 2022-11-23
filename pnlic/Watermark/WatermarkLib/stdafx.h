// stdafx.h : 자주 사용하지만 자주 변경되지는 않는
// 표준 시스템 포함 파일 및 프로젝트 관련 포함 파일이
// 들어 있는 포함 파일입니다.
//

#pragma once

#include "targetver.h"


#define WIN32_LEAN_AND_MEAN             // 거의 사용되지 않는 내용은 Windows 헤더에서 제외합니다.
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // 일부 CString 생성자는 명시적으로 선언됩니다.

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // 거의 사용되지 않는 내용은 Windows 헤더에서 제외합니다.
#endif

#include <afx.h>
#include <afxwin.h>         // MFC 핵심 및 표준 구성 요소입니다.



// TODO: 프로그램에 필요한 추가 헤더는 여기에서 참조합니다.
#include "vars.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // 일부 CString 생성자는 명시적으로 선언됩니다.



#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace std;






const unsigned char g_StartWaterMark[] = {0x00, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xff, 0x00, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xff,};
const unsigned char g_EndtWaterMark[] =  {0xaa, 0xbb, 0xcc, 0xdd, 0xff, 0x99, 0xaa, 0xbb, 0xcc, 0x99, 0xaa, 0xdd, 0xff, 0x00 };

const int g_MaxBinaryWaterMarkSize = 300;