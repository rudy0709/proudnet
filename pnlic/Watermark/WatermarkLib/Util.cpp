#include "StdAfx.h"
#include <io.h>

CUtil::CUtil(void)
{
}

CUtil::~CUtil(void)
{
}

void CUtil::FileReadonlyOn( CStringA fileName )
{
	CStringA strfileOpenlock = "attrib +r ";
	strfileOpenlock += fileName;
	system(strfileOpenlock);
}

void CUtil::FileReadonlyOff( CStringA fileName )
{
	CStringA strfileOpenlock = "attrib -r ";
	strfileOpenlock += fileName;
	system(strfileOpenlock);
}


void CUtil::SystemCommand(CStringA commend,CStringA copiedfolder,CStringA fileName)
{
	commend += fileName;
	commend += " ";
	commend += copiedfolder;
	commend += fileName;
	system(commend);
}

int CUtil::IsFileReadonly(CStringA filename) 
{
	_finddatai64_t c_file;
	intptr_t hFile;
	int result = 0;

	if ( (hFile = _findfirsti64(filename, &c_file)) == -1L )
		result = -1;
	else
		if (c_file.attrib & _A_RDONLY) 
			result = 1;


	_findclose(hFile);

	return result;
}