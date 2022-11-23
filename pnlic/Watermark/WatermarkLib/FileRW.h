#pragma once


using namespace std;

class CStdioFileEx2;
class CWaterMarkMgr;

enum eCodeType
{
	eUnicode,
	eUtf8,
	eNon
};

struct sFileInfo_t	// 파일정보
{
	CString m_filePath;
	CString m_fileName;
};

struct FindedWaterMarkInfo	//찾아진 워터마크
{
	vector<sFileInfo_t> m_vecFile;
	CString m_findedWaterMark;
};
typedef vector<FindedWaterMarkInfo> vecFindedWaterMark;

struct BinaryWaterMark	//바이너리 워터마크용
{
	BYTE* m_pStartBinary;
	UINT m_startSize;

	vecDetectedBinaryWaterMark m_vecWaterMark;

	BYTE* m_pEndBinary;
	UINT m_endSize;
	
	BinaryWaterMark(BYTE* startBytes,UINT startSize, BYTE* endBytes, UINT endSize);
	~BinaryWaterMark();
};

struct SimpleByteArray;

// File에 워터마크를 찍거나 찾아서 읽어 들이는 일을 하는 class이다.
class CFileReadWriteHelper
{
	enum eRWMode{
		eWrite,
		eRead
	};

public:
	CFileReadWriteHelper(void);
	~CFileReadWriteHelper(void);

	CWaterMarkMgr *m_pOwner;
	static CString m_LogFileName;

	// 찾아진 파일 정보의 로그들을 파일로 저장하여 남김
	static void WriteLogFile(CString);
	static void WriteLogFile_NewLine(CString);

	static bool emptyFileWrite(CString fileName, CString comment);
	static bool WriteWaterMark(CString fileName, CString waterMark);

	/********************************************************************/
	// 회사 정보의 워터마크형 스트링을 넣어주면 같은 스트링을 찾는다
	static CString FindStringInFile(CString fileName, CString warterMarkHeader);

	// 해당 폴더 내부 모든 폴더들을 검색하여 해당 스트링이 있는 파일들을 찾아줌
	void FindStringInForderTree(CStringA folderPath, CString warterMarkHeader, vecFindedWaterMark* vec);

	// 해당 폴더 내부 모든 폴더들을 검색하여 특정 파일들에 워터마크를 찍음
	void WriteWaterMarkInFolderTree(CStringA folderPath, CString warterMarkHeader);

	// 소스 워터마크를 찍는다. 파일의 마지막에 찍음
	bool WriteSrcWaterMark(CString fileName, CString sourceWaterMark);

	// 라이브러리 파일을 읽어들여서 해당하는 바이트를 워터마크로 바꾼다.
	bool ChangeWaterMarkBinary(CString fileName, CStringA waterMark, vecDetectedBinaryWaterMark &mark);
	
	// 바이너리 워터마크를 찍거나 찾을 때 단순암호화를 한다.
public:
	static void EncryptBinaryWaterMark(BYTE *pByte,int size);
	static void DecryptBinaryWaterMark(BYTE *pByte,int size);


private:

	// 파일 열기 - 예외처리 포함 랩핑
	static bool GetStdioFileValue(CStdioFileEx2&,CString,UINT);

	// CDetectFolderParam으로 모든 폴더내의 해당 파일들 정보를 넣어준다.
	void DectectFolderTree( CStringA, CString, eRWMode, vecFindedWaterMark* );


	//binary WaterMark
public:
	// WarterMark Binary를 해당 파일에서 찾는다.
	bool DetectBinaryWaterMarkInFile( CString fileName, BinaryWaterMark& binaryMark);

private:
	bool FindBinaryInBuffer(CFile &file, LONGLONG fullBufferPos, SimpleByteArray &fileBinary, BinaryWaterMark& binaryMark);

	void WriteDectectedFile(CString folderPath, CString fileName , CString waterMark);
	void ReadDectectedFile(CString folderPath, CString fileName , CString findedwaterMark, vecFindedWaterMark &vec);

public:
	void TestFunction();
};

