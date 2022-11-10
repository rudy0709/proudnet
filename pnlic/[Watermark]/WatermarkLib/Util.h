#pragma once

using namespace std;

class CUtil
{
public:
	CUtil(void);
	~CUtil(void);
	
	// 파일을 읽기 전용옵션을 다시 키고 끈다.
	static void FileReadonlyOn(CStringA fileName);
	static void FileReadonlyOff(CStringA fileName);
	// 이 파일이 읽기 전용 옵션이 켜져 있는지 확인한다.
	static int IsFileReadonly(CStringA filename);
	
	
	static void SystemCommand(CStringA commend,CStringA copiedfolder,CStringA fileName);
};
