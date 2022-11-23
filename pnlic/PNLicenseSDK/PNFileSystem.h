#pragma once

#include "stdafx.h"

#include <fstream>
#include <string>
#include <locale>
#include <cerrno>
#include <sys/stat.h>
#include "PNErrorInfo.h"
#include "PNTime.h"

#if defined(__linux__)
#include <stdio.h>	
#include <unistd.h>
#include "PNSecure.h"
#else
#include <codecvt>
#include <shlwapi.h>
#include "WinTime.h"
#include "PNFileSecureAttribute.h"
#endif

#include "../AuthNet/src/Base64.h"
#include "../AuthNet/include/Message.h"
#include "../AuthNet/include/Exception.h"

extern const char *fileOpenType[];

using namespace std;
using namespace Proud;

// platform-dependent file info.
#if defined(WIN32)
typedef struct _stat64i32 CPNFileInfo;
#else
typedef struct stat CPNFileInfo;
#endif

class CPNFileSystem
{
public:
	CPNFileSystem() {}
	~CPNFileSystem() {
		if (m_file.is_open() == true) { m_file.close(); }
	}
	
	enum OpenType { READONLY = 0, WRITEONLY = 1, READWRITE = 2 };
	enum PositionType { F_SEEK_SET = 0, F_SEEK_CUR = 1, F_SEEK_END = 2 };

	bool IsOpen();
	bool Exists(Proud::String filePath, CPNErrorInfo& errorInfo);
	bool OpenFile(Proud::String filePath, OpenType mode, CPNErrorInfo& errorInfo);
	bool CreateAllUserWritableRegistryFileAndOpen(Proud::String filePath, OpenType mode, CPNErrorInfo& errorInfo);
	bool CreateAllUserWritableRegistryFile(Proud::String filePath, OpenType mode, CPNErrorInfo& errorInfo);
	bool GetFileState(CPNFileInfo &fileInfo, CPNErrorInfo& errorInfo);

	bool ReadLine(Proud::String &strFileData, CPNErrorInfo& errorInfo);
	bool ReadBinary(ByteArray &byteFileData, uint64_t byteSize, CPNErrorInfo& errorInfo);

	bool WriteLine(Proud::String strFileData, CPNErrorInfo& errorInfo);
	bool WriteBinary(ByteArray &byteFileData, uint64_t byteSize, CPNErrorInfo& errorInfo);
	bool WriteMessage(CMessage &messageFileData, CPNErrorInfo& errorInfo);

	bool CloseFile(CPNErrorInfo& errorInfo);
	bool ChangeFileAttrib(Proud::String filePath, uint32_t mode);
	
private:
	Proud::String m_filePath;
	fstream m_file;
};
