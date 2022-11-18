#include "stdafx.h"
#include "PNFileSystem.h"

#if defined(__linux__)
#include "../[AuthNetLib]/src/Base64.cpp"
#endif

typedef unsigned char byte;
const char *fileOpenType[] = { "rb", "w", "rb+" };

bool CPNFileSystem::IsOpen()
{
	return m_file.is_open();
}

// 파일이 존재하는지 검사한다.
bool CPNFileSystem::Exists(String filePath, CPNErrorInfo& errorInfo)
{
#if defined(_WIN32)
	if (PathFileExists(filePath.GetString()) == false) {
		errorInfo.AppendErrorCode(GetLastError(), "");
		return false;
	}
#else
	if (access(filePath.GetString(), F_OK) != 0) {
		errorInfo.AppendErrorCode(errno, "");
		return false;
	}
#endif
	return true;
}

bool CPNFileSystem::OpenFile(String filePath, OpenType mode, CPNErrorInfo& errorInfo)
{
	std::ios_base::openmode accessMode;
	switch (mode)
	{
	case WRITEONLY:
		accessMode = std::fstream::out;
		break;
	case READWRITE:
		accessMode = std::fstream::in | std::fstream::out;
		break;
	default:
		accessMode = std::fstream::in;
		break;
	}

	m_file.open(filePath.GetString(), accessMode | std::fstream::binary);
	if (m_file.fail() == true) {
		errorInfo.AppendErrorCode(errno, "");
		return false;
	}

	m_filePath = filePath;
	return true;
}

bool CPNFileSystem::CreateAllUserWritableRegistryFile(String filePath, OpenType mode, CPNErrorInfo& errorInfo)
{
#if defined(_WIN32)
	/*
	Windows Service 프로세스는 Users 그룹에 없습니다. 즉 파일 액세스가 실패할 수 있습니다.
	따라서, Users, LOCAL SERVICE, NETWORK SERVICE, LOCAL SYSTEM을 모두 추가했습니다.

	https://msdn.microsoft.com/en-us/library/windows/desktop/ms686005(v=vs.85).aspx
	관련될지 모르겠지만 참고 링크 https://goo.gl/qW0g5L
	*/
	CPNFileSecureAttribute fileSecureAttrib;
	fileSecureAttrib.Init();
	fileSecureAttrib.Allow(fileSecureAttrib.GetSID("Users"));
	fileSecureAttrib.Allow(fileSecureAttrib.GetSID("SYSTEM"));
	fileSecureAttrib.Allow(fileSecureAttrib.GetSID("LocalSystem"));
	fileSecureAttrib.Allow(fileSecureAttrib.GetSID("LocalService"));
	fileSecureAttrib.Allow(fileSecureAttrib.GetSID("NetworkService"));
	fileSecureAttrib.ApplyACL();
	fileSecureAttrib.SetGroup("Users");
	fileSecureAttrib.SetGroup("SYSTEM");
	fileSecureAttrib.SetGroup("LocalSystem");
	fileSecureAttrib.SetGroup("LocalService");
	fileSecureAttrib.SetGroup("NetworkService");
	fileSecureAttrib.SetDACL();

	DWORD accessMode;
	switch (mode)
	{
		//case READONLY:
		//	accessMode = GENERIC_READ;
		//	break;
	case WRITEONLY:
		accessMode = GENERIC_WRITE;
		break;
	case READWRITE:
		accessMode = GENERIC_READ | GENERIC_WRITE;
		break;
	default:
		accessMode = GENERIC_READ;
		break;
	}

	HANDLE hFile = ::CreateFile(filePath.GetString(), accessMode, 0, &fileSecureAttrib, CREATE_NEW, FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_HIDDEN, 0);
	if (hFile == INVALID_HANDLE_VALUE) {
		errorInfo.AppendErrorCode(GetLastError(), "");
		return false;
	}
	else
	{
		CloseHandle(hFile);
	}
#else

	std::ios_base::openmode accessMode;
	switch (mode)
	{
		//case READONLY:
		//	accessMode = std::fstream::in | std::fstream::binary;
		//	break;
	case WRITEONLY:
		accessMode = std::fstream::out;
		break;

	case READWRITE:
		accessMode = std::fstream::in | std::fstream::out;
		break;

	default:
		accessMode = std::fstream::in;
		break;
	}

	fstream file;
	file.open(filePath.GetString(), accessMode | std::fstream::binary);
	if (file.fail() == true) {
		errorInfo.AppendErrorCode(errno, "");
		return false;
	}
	else
	{
		file.close();
	}
	ChangeFileAttrib(filePath.GetString(), 0666);
#endif
}

// 레지스트리 숨은 파일 즉 모든 유저가 읽기쓰기 가능한 숨은 파일을 생성하고(있으면 생략) 그것을 연다.
bool CPNFileSystem::CreateAllUserWritableRegistryFileAndOpen(String filePath, OpenType mode, CPNErrorInfo& errorInfo)
{
	if (false == CreateAllUserWritableRegistryFile(filePath, mode, errorInfo))
	{
		return false;
	}

	if (OpenFile(filePath, mode, errorInfo) == false) {
		errorInfo.AppendErrorCode(errno, "");
		return false;
	}

	return true;
}

bool CPNFileSystem::ChangeFileAttrib(String filePath, uint32_t mode)
{
#if defined(__linux__)
	if (chmod(filePath.GetString(), (mode_t)mode) == -1)
	{
		return false;
	}
#endif
	return true;
}

bool CPNFileSystem::GetFileState(CPNFileInfo &fileInfo, CPNErrorInfo& errorInfo)
{
#if defined(WIN32)
	if (_wstat64i32(m_filePath.GetString(), &fileInfo) != 0) {
#else
	if (stat(m_filePath.GetString(), &fileInfo) != 0) {
#endif
		errorInfo.AppendErrorCode(errno, "");
		return false;
	}

	return true;
}

bool CPNFileSystem::ReadLine(String &strFileData, CPNErrorInfo& errorInfo)
{
	string strTempData;

	if (m_file.is_open() == false) {
		return false;
	}

	getline(m_file, strTempData, '\0');
	strFileData = StringA2T(strTempData.c_str());

	return true;
}

// 파일에서 바이너리를 읽는다.
// byteFileData에 미리 지정해 놓은 크기와 byteSize 중 작은 값을 선택 후 그만큼 읽어들인다.
bool CPNFileSystem::ReadBinary(ByteArray &byteFileData, uint64_t byteSize, CPNErrorInfo& errorInfo)
{
	if (m_file.is_open() == false) {
		return false;
	}

	m_file.read((char*)byteFileData.GetData(), PNMIN(byteFileData.GetCount(), byteSize));
	if (m_file.fail() == true) {
		errorInfo.AppendErrorCode(errno, "");
		return false;
	}

	return true;
}

bool CPNFileSystem::WriteLine(String strFileData, CPNErrorInfo& errorInfo)
{
	string strData = StringT2A(strFileData).GetString();

	if (m_file.is_open() == false) {
		return false;
	}

	m_file.write(strData.data(), strData.size());
	if (m_file.fail() == true)	{
		errorInfo.AppendErrorCode(errno, "");
		return false;
	}

	return true;
}

bool CPNFileSystem::WriteBinary(ByteArray &byteFileData, uint64_t byteSize, CPNErrorInfo& errorInfo)
{
	if (m_file.is_open() == false) {
		return false;
	}

	m_file.write((char*)byteFileData.GetData(), (byteFileData.GetCount() > byteSize) ? byteSize : byteFileData.GetCount());
	if (m_file.fail() == true)	{
		errorInfo.AppendErrorCode(errno, "");
		return false;
	}

	return true;
}

bool CPNFileSystem::WriteMessage(CMessage &msgFileData, CPNErrorInfo& errorInfo)
{
	if (m_file.is_open() == false) {
		return false;
	}

	m_file.write((char*)msgFileData.GetData(), msgFileData.GetLength());
	if (m_file.fail() == true)	{
		errorInfo.AppendErrorCode(errno, "");
		return false;
	}

	return true;
}
bool CPNFileSystem::CloseFile(CPNErrorInfo& errorInfo)
{
	m_file.close();
	if (m_file.fail() == true)	{
		return false;
	}

	return true;
}
