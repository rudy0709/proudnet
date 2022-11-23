#pragma once
#include "WaterMarkStrMgr.h"
#include "FileRW.h"

using namespace std;

class CWaterMarkMgr
{
public:

	CWaterMarkString m_waterMarkString;
	CFileReadWriteHelper m_fileHelper;
	
	CWaterMarkMgr(void);
	~CWaterMarkMgr(void);
	
	// 워터마크를 찍어주기 용 프로젝트 일 때 고객 정보에 대한 스트링을 넣어줌.
	void Init(CStringA customerInfoString );
	
	// 워터마크 딕텍터일 때에만 이 함수를 사용한다.
	void Init();
	
	// 한 파일에 대하여 워터 마크를 씀
	bool WriteWaterMark(CString fileName);
	// 한 파일에 대하여 워터 마크를 찾음
	CString FindWaterMark(CString fileName);
	
	/**************************************** Init후 밑의 함수만 사용하면 충분하다. *****************************************/
	//!! 폴더 주소 주의!! 엄한 파일들까지 모조리 워터마크를 찍어버릴 수 있다.
	
	// 폴더 내에 모든 cpp,h,PIDL 파일에 워터마크를 찍음
	void WriteWaterMrkInFoldertree(CString folderName);
	// 폴더 내에 모든 cpp,h,PIDL 파일에 워터마크를 찾음
	bool FindWaterMarkInFoldertree(CString folderName);
	
	// 바이너리 워터마크를 찾는다.
	bool DetectBinaryWaterMark(CStringA fileName);
	
	// 소스 워터 마크를 찍는다.
	bool WriteSrcWaterMark(CStringA filePath,CStringA fileName);
	
	// 라이브러리를 읽어들여서 해당 메모리에 워터마크를 씌움
	bool ChangeWaterMarkBinary(CStringA filePath, CStringA waterMarkInfo);
	
	//테스트
	bool funTest();
	void CopyTestFiles();
	bool OneFileWriteWaterMarktest(CStringA fileName);
	
	
private:
	bool FindStringTest(CString testFileName);
	bool FindStringTestFromFolderTree();
	bool WriteWaterMarkTest();
};

