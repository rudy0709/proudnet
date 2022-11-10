#include "StdAfx.h"
#include "FileRW.h"
#include "WaterMarkMgr.h"
#include <fstream>
#include <io.h>
#include "StdioFileEx2.h"
#define MaxBuffersize 10000

CString CFileReadWriteHelper::m_LogFileName = _T("WaterMark_Finded_Log.txt");

BinaryWaterMark::BinaryWaterMark(BYTE* startBytes,UINT startSize, BYTE* endBytes, UINT endSize )
{
	assert(startBytes);
	assert(endBytes);
	
	m_pStartBinary= new BYTE[startSize+1];
	memcpy(m_pStartBinary, startBytes,startSize);
	m_startSize=startSize;
	m_pStartBinary[startSize]='\0';
	
	
	m_pEndBinary= new BYTE[endSize+1];
	memcpy(m_pEndBinary, endBytes,endSize);
	m_endSize=endSize;
	m_pEndBinary[endSize]='\0';
};

BinaryWaterMark::~BinaryWaterMark()
{
	if(m_pStartBinary)
		delete[] m_pStartBinary;
	if(m_pEndBinary)
		delete[] m_pEndBinary;
		
	
	for(vecDetectedBinaryWaterMark::iterator iter = m_vecWaterMark.begin(); iter != m_vecWaterMark.end();++iter)
	{
		DetectedBinaryWaterMark *p = (*iter);
		delete p;
	}
}

CFileReadWriteHelper::CFileReadWriteHelper(void)
{
	emptyFileWrite(m_LogFileName, _T("/**************** Start ****************/"));
}

CFileReadWriteHelper::~CFileReadWriteHelper(void)
{
}

void CFileReadWriteHelper::WriteLogFile(CString comment)
{
	CStdioFileEx2 sourceFile;
	if(! GetStdioFileValue(sourceFile , m_LogFileName,CFile::modeReadWrite|CFile::modeNoTruncate|CFile::typeText) )
	{
		emptyFileWrite(m_LogFileName, comment);
		return;
	}
			
	sourceFile.SeekToEnd();
	sourceFile.WriteString(comment);

	sourceFile.Close();

}

void CFileReadWriteHelper::WriteLogFile_NewLine(CString comment)
{
	CString newLine = _T("\r\n")+comment;
	WriteLogFile(newLine);
}

bool CFileReadWriteHelper::emptyFileWrite( CString FileName, CString comment )
{
	CStdioFileEx2 sourceFile;
	if(! GetStdioFileValue(sourceFile, FileName, CFile::modeWrite|CFile::modeCreate|CFile::typeBinary))
		return false;
	
	sourceFile.SeekToBegin();
	sourceFile.WriteString(comment);

	sourceFile.Close();

 	return true;
}


bool CFileReadWriteHelper::WriteWaterMark( CString FileName, CString waterMark )
{
	CStdioFileEx2 sourceFile;
	if(! GetStdioFileValue(sourceFile , FileName,CFile::modeReadWrite|CFile::modeNoTruncate|CFile::typeBinary) )
		return false;
	UINT size = (UINT)sourceFile.GetLength();
	if(size == 0)  
		return false;

	CByteArray Buffer;
	Buffer.SetSize(size+1);
	BYTE *DumpBuffer=NULL;
	
	try
	{
		// 버퍼에서 Byte-Oder Mark를 빼준다
		sourceFile.SeekToBegin();
		sourceFile.Read(Buffer.GetData(), size);
			
		// Byte-oder Mark 는 빼주어야 한다.
 		if(sourceFile.GetFileEncodeType() == eANSI)				//ANSI는 Byte-oder Mark 없다
 		{
			DumpBuffer = Buffer.GetData();
 		}else if(sourceFile.GetFileEncodeType() == eUNICODE)	// BOM이 2BYTE이다
 		{
 			DumpBuffer = Buffer.GetData()+sizeof(wchar_t);
 			size = size - sizeof(wchar_t);
 			
 		}else if(sourceFile.GetFileEncodeType() == eUTF_8)		// BOM이 3BYTE이다
 		{
 			DumpBuffer = Buffer.GetData()+3;
 			size = sizeof(BYTE)*size - sizeof(BYTE)*3;
	 		
 		}else
		{
			return false;
		}

		sourceFile.Close();

		//----------------------------------------------------------------------------------------------------------
		// 스트링으로 처리할 때는TextType으로 열어주어야 한다
		if(! GetStdioFileValue(sourceFile , FileName,CFile::modeReadWrite|CFile::modeNoTruncate|CFile::typeText) )
			return false;
		sourceFile.SeekToBegin();
		// 워터 마크 삽입
 		sourceFile.WriteString(waterMark);
//  #ifdef TestMode
// 	sourceFile.WriteString(_T("\r\n\\요런 한글테스트 한번 안하면 꼭 뒤탈난다.\r\n"));
// #endif
 		
 		sourceFile.WriteString(_T("\r\n"));



// //  		sourceFile.WriteString(L"\r\n\\한글이 잘 지원되는지를 테스트 합니다.\r\n");
// //  		sourceFile.WriteString(L"\r\n\\한번 더 잘 지원되는지를 테스트 합니다.\r\n");

		ULONGLONG position = sourceFile.GetPosition();
		sourceFile.Close();
		
		//----------------------------------------------------------------------------------------------------------
		// Bynary로 파일을 다시 열어야 문제가 생기지 않는다
		if(! GetStdioFileValue(sourceFile , FileName,CFile::modeReadWrite|CFile::modeNoTruncate|CFile::typeBinary) )
			return false;
		// 본래의 내용을 다시 붙여준다.
		sourceFile.Seek(position,CFile::begin);
		sourceFile.Write(DumpBuffer,size);
	
		sourceFile.Close();
		
			
	}catch(...)
	{
		CString ErrorMsg;
		int ErrorNumber = GetLastError();
		ErrorMsg.Format(_T("GetStdioFileValue Error, Error Number is %d"), ErrorNumber);
		LOGWRITER(ErrorMsg);    // 에러 처리
	}
	

	CString printStr;
	printStr.Format(_T("%s 파일 워터마크 입력 완료"), FileName);
	LOGWRITER(printStr);
	
	return true;
}


CString CFileReadWriteHelper::FindStringInFile( CString fileName, CString WarterMarkHeader )
{
	CStdioFileEx2 sourceFile;
	if(! GetStdioFileValue(sourceFile , fileName,CFile::modeRead|CFile::modeNoTruncate|CFile::typeText) )
		return _T("");
	
	CString str;
	sourceFile.ReadString(str);
	if(str.Find(WarterMarkHeader) != -1)
	{
		CString printStr;
		printStr.Format(_T("%s 파일 워터마크 찾기 완료"), fileName);
		LOGWRITER(printStr);
		
		sourceFile.Close();
		return str;
	}
	return _T("");
}

void CFileReadWriteHelper::FindStringInForderTree( CStringA folderPath, CString WarterMarkHeader, vecFindedWaterMark* vec)
{	
	int size = folderPath.GetLength();

	if(folderPath[size-1] != '\\')
		folderPath+="\\";

	DectectFolderTree(folderPath, WarterMarkHeader, eRead, vec);
}


void CFileReadWriteHelper::WriteWaterMarkInFolderTree( CStringA folderPath, CString WarterMarkHeader)
{
	DectectFolderTree(folderPath, WarterMarkHeader, eWrite, NULL);
}


void CFileReadWriteHelper::DectectFolderTree( CStringA folderPath, CString strCommon, eRWMode RWMode, vecFindedWaterMark* vec=NULL)
{
	int folderPathSize = folderPath.GetLength();
	if(folderPath[folderPathSize-1] != '\\')
		folderPath+= "\\";
	CStringA FileType = "*.*";
	struct _finddata_t c_file;
	intptr_t hFile;
	CStringA findString = folderPath;
	findString += FileType;
	
	if( (hFile=_findfirst(findString, &c_file)) == -1L)
	{
		switch (errno) 
		{
		case ENOENT:
			//Logwriter(L":: 파일이 없음 ::\r\n"); 
			break;
		case EINVAL:
			LOGWRITER(_T("Invalid path name.\r\n"));  break;
		case ENOMEM:
			LOGWRITER(_T("Not enough memory or file name too long.\r\n")); break;
		default:
			LOGWRITER(_T("Unknown error.\r\n")); break;
		}
	}else
	{
		do 
		{
			if((c_file.attrib & _A_SUBDIR) == _A_SUBDIR)	// 폴더일 때 재귀
			{
				if(c_file.name[0] != '.' && c_file.name[0] != '..')
				{
					CStringA dir = folderPath;
					dir += c_file.name;
					dir += "\\";
					DectectFolderTree(dir, strCommon, RWMode, vec);
				}
			}else 
			{
				CString FileName = STRING_CONVERT_A_TO_T(c_file.name);
				FileName.MakeUpper();
				if((  FileName.Find(_T(".PIDL"))!=-1)  ||  (FileName.Find(_T(".H"))!=-1)  ||  (FileName.Find(_T(".CPP"))!=-1) ||  (FileName.Find(_T(".INL"))!=-1)  )
				{
					if(RWMode == eWrite)
						WriteDectectedFile(STRING_CONVERT_A_TO_T(folderPath), STRING_CONVERT_A_TO_T(c_file.name), strCommon);
					else
					{
						//WriteLogFile_NewLine(STRING_CONVERT_A_TO_T(folderPath));
						//WriteLogFile(STRING_CONVERT_A_TO_T(c_file.name));
						ReadDectectedFile( STRING_CONVERT_A_TO_T(folderPath), STRING_CONVERT_A_TO_T(c_file.name), strCommon, *vec);
					}
				}else if ((FileName.Find(_T(".EXE"))!=-1)  ||	(FileName.Find(_T(".LIP"))!=-1)  )
				{
					if(RWMode == eRead)
					{
						CStringA filePath = folderPath+c_file.name;
						m_pOwner->DetectBinaryWaterMark(filePath);
					}
				}
				
			}

		} while ( (_findnext(hFile, &c_file))==0 );
		_findclose(hFile);
	}
}

//------------------ Binary Dectecter ------------------

bool CFileReadWriteHelper::DetectBinaryWaterMarkInFile( CString FileName, BinaryWaterMark& BinaryMark)
{
	CStdioFileEx2 File;
	if( !GetStdioFileValue(File, FileName, CFile::modeRead|CFile::modeNoTruncate|CFile::typeBinary))
		return false;
	
	LONGLONG fullSize = File.GetLength();
	SimpleByteArray Buffer(10000000);
	bool result = false;

	if(Buffer.m_size>fullSize)
		Buffer.m_size = (int)fullSize;
		
	try
	{
		LONGLONG fullBufferPos=0;
		for(fullBufferPos; fullSize>fullBufferPos+Buffer.m_size;fullBufferPos+=Buffer.m_size)	// 현재 위치부터 버퍼 사이즈를 더한 값까지 검사한다.
		{	
			
			if(fullBufferPos != 0)
				// 워터마크가 블럭화된의 버퍼와 버퍼사이에 있을 수 있다. 넉넉히 내용에 대하여서는 BinaryWaterMarkLength정도 여유를 주자
				// 양쪽 워터마크 길이 + 내용 길이
				fullBufferPos-=(BinaryMark.m_startSize+BinaryMark.m_endSize+g_MaxBinaryWaterMarkSize+1);

			File.Seek(fullBufferPos,CFile::begin);
			File.Read(Buffer.m_pBytes, Buffer.m_size);

 			if(FindBinaryInBuffer(File, fullBufferPos, Buffer, BinaryMark))
 				result = true;
		}

		// 풀 버퍼에서 남은 부분을 체크
		File.Seek(fullBufferPos,CFile::begin);
		if(fullSize!=Buffer.m_size)
			Buffer.m_size = fullSize%Buffer.m_size;		// 사이즈를 파일 용량의 남은 사이즈로 마추어줌
		File.Read(Buffer.m_pBytes, Buffer.m_size);

		if(FindBinaryInBuffer(File, fullBufferPos, Buffer, BinaryMark))
			result = true;
 		
 		File.Close();
	}
	catch (...)
	{
		CString ErrorMsg;
		int ErrorNumber = GetLastError();
		ErrorMsg.Format(_T("GetStdioFileValue Error, Error Number is %d"), ErrorNumber);
		LOGWRITER(ErrorMsg);    // 에러 처리
	}
	
	
	return result;
}


bool CFileReadWriteHelper::FindBinaryInBuffer(CFile &File, LONGLONG fullBufferPos, SimpleByteArray &BinaryBuffer, BinaryWaterMark& BinaryMark)
{

	BYTE* pBinaryBuffer = BinaryBuffer.m_pBytes;
	const UINT BufferSize = BinaryBuffer.m_size;
	bool result = false;
	
	for(LONGLONG i=0; i < BufferSize-BinaryMark.m_startSize-BinaryMark.m_endSize ;++i)	// 현재의 버퍼블럭에서 모든 바이너리를 찾는다.
	{
		LONGLONG _StartPos =-1;
		LONGLONG _EndPos =-1;
		UINT j=0;
		// 시작 워터마크를 식별값을 찾는다.
		for(; i < BufferSize-BinaryMark.m_startSize;++i)	// 워터마크는 앞자리 14 뒷자리 14 으로 이루어져 있다.
		{
			if(pBinaryBuffer[i] == BinaryMark.m_pStartBinary[0])
			{
				for( j=0;j<BinaryMark.m_startSize;j++)	
				
					if(pBinaryBuffer[i+j] == BinaryMark.m_pStartBinary[j] )
					{
						if(j == BinaryMark.m_startSize-1) 
							_StartPos = i+BinaryMark.m_startSize;	// 마지막 위치를 잡아줌
					}else
					{
						break;
					}
			}
			if(_StartPos != -1)
				break;
		}
		// 끝 워터마크를 식별값을 찾는다.
		if(_StartPos!=-1)
		{
			UINT j=0;
			for(LONGLONG i=_StartPos; i<BufferSize-BinaryMark.m_endSize ;++i)	
			{
				if(pBinaryBuffer[i] == BinaryMark.m_pEndBinary[0])
					for(j=0;j<BinaryMark.m_endSize;j++)
					
						if( pBinaryBuffer[i+j] == BinaryMark.m_pEndBinary[j] )
						{
							if(j == BinaryMark.m_endSize-1)
								_EndPos = i;
						}else
						{
							break;
						}
				if(_EndPos != -1)
					break;
			}
			
		}
		
		// 양 사이드 를 찾았을 때 워터마크의 내용을 읽는다.
		if(_EndPos!=-1&&_StartPos!=-1)
		{
			// 사이즈 계산
			UINT _findSize=(UINT)(_EndPos-_StartPos);

			// 버퍼 읽기
 			DetectedBinaryWaterMark *pDectected = new DetectedBinaryWaterMark;
 			pDectected->m_StartPos = (UINT)_StartPos+(UINT)fullBufferPos;	//Read에 들어갈 수 있는 포지션값이 UINT값 임으로 더 큰 수 는 처리가 불가능하다
 			pDectected->m_EndPos =   (UINT)_EndPos+(UINT)fullBufferPos;
 			pDectected->m_WaterMark.SetSize(_findSize);

			File.SeekToBegin();
			File.Seek(pDectected->m_StartPos, CFile::begin);
			File.Read(pDectected->m_WaterMark.m_pBytes,_findSize);
			
			BinaryMark.m_vecWaterMark.push_back(pDectected);
			result = true;
		}
	}
	return result;
};


bool CFileReadWriteHelper::WriteSrcWaterMark( CString FilePath, CString sourceWaterMark )
{
	CStdioFileEx2 File;
	if( !GetStdioFileValue(File, FilePath, CFile::modeReadWrite|CFile::modeNoTruncate|CFile::typeText))
		return false;
		
	File.SeekToEnd();
	File.WriteString(sourceWaterMark);
	File.Close();
	
	CString Log;
	Log.Format(_T("%s 파일에 소스 워터마크 완료"), FilePath);
	LOGWRITER(Log);
	return true;
	

}


bool CFileReadWriteHelper::ChangeWaterMarkBinary( CString FilePath, CStringA WaterMark, vecDetectedBinaryWaterMark &Mark)
{
	CStdioFileEx2 File;
	int size=WaterMark.GetLength();
	if( !GetStdioFileValue(File, FilePath, CFile::modeReadWrite|CFile::modeNoTruncate|CFile::typeBinary))
		return false;
		
	EncryptBinaryWaterMark((BYTE*)WaterMark.GetBuffer(), size);

	for(vecDetectedBinaryWaterMark::iterator iter = Mark.begin();iter != Mark.end();++iter)
	{
		File.Seek((*iter)->m_StartPos,CFile::begin);
		
		// 스트링이 정해진 버퍼 크기조다 크면 가능한 크기 까지만 입력한다.
		if(    ((*iter)->m_EndPos-(*iter)->m_StartPos)  < (UINT)size    )
			size = ((*iter)->m_EndPos-(*iter)->m_StartPos);

		File.Write(WaterMark.GetBuffer(), size);
	}
	
	File.Close();
	return true;
}

void CFileReadWriteHelper::EncryptBinaryWaterMark(BYTE *pByte,int size)
{
	BYTE temp;
	for(int i=0;i<size;++i)
	{
		temp = pByte[i];
		pByte[i] = (temp>>6)&0x3;
		pByte[i] ^= ((temp>>4)&0x3)<<2;
		pByte[i] ^= ((temp>>2)&0x3)<<6;
		pByte[i] ^= ((temp)&0x3)<<4;
		pByte[i] ^= 0xD4;
	}
}

void CFileReadWriteHelper::DecryptBinaryWaterMark(BYTE *pByte,int size)
{
	BYTE temp;
	for(int i=0;i<size;++i)
	{
		pByte[i] ^= 0xD4;
		temp = pByte[i];
		pByte[i] = ((temp)&0x3)<<6;
		pByte[i] ^= ((temp>>2)&0x3)<<4;
		pByte[i] ^= ((temp>>4)&0x3);
		pByte[i] ^=  ((temp>>6)&0x3)<<2;
	}
}
//------------------ private ------------------


bool CFileReadWriteHelper::GetStdioFileValue( CStdioFileEx2 &sourceFile, CString fileName, UINT flag )
{
	CFileException ex;
	if(!sourceFile.Open(fileName,flag,&ex))
	{
		CString ErrorMsg;
		int ErrorNumber = GetLastError();
		ErrorMsg.Format(_T("GetStdioFileValue Error, Error Number is %d"), ErrorNumber);
		LOGWRITER(ErrorMsg);    // 에러 처리
		LOGWRITER(_T("파일을 열지 못하였습니다.\n"));
		return false;
	}
	return true;
}

void CFileReadWriteHelper::WriteDectectedFile( CString folderPath, CString fileName , CString waterMark )
{
	fileName.MakeUpper();

	if((  fileName.Find(_T(".PIDL"))!=-1)  |  (fileName.Find(_T(".H"))!=-1)  |  (fileName.Find(_T(".CPP"))!=-1)  )
	{
		int size = folderPath.GetLength();

		if(folderPath[size-1] != '\\')
			folderPath+="\\";

		WriteWaterMark(folderPath+fileName, waterMark);
	}
}

void CFileReadWriteHelper::ReadDectectedFile( CString folderPath, CString FileName , CString findedwaterMark, vecFindedWaterMark &vec )
{
	int size = folderPath.GetLength();

	if(folderPath[size-1] != '\\')
		folderPath+="\\";
	CString FilePath = folderPath;
	FilePath += FileName;
	CString strFinded = FindStringInFile( FilePath, findedwaterMark);

	if(strFinded.GetLength() == 0)
		return ;

	sFileInfo_t _fileInfo;
	_fileInfo.m_fileName = FileName;
	_fileInfo.m_filePath = folderPath;

	for(vecFindedWaterMark::iterator iter = vec.begin(); iter != vec.end() ; ++iter)
	{
		if((*iter).m_findedWaterMark.Compare(strFinded) == 0)	
		{	
			(*iter).m_vecFile.push_back(_fileInfo);
			return ;
		}
	}
	FindedWaterMarkInfo _SearchedInfo;
	_SearchedInfo.m_vecFile.push_back(_fileInfo);
	_SearchedInfo.m_findedWaterMark = strFinded;

	vec.push_back(_SearchedInfo);
}

void CFileReadWriteHelper::TestFunction()
{
	//Binary Encrypt Test
	BYTE byteArray[100]={0,};
	BYTE origineByteArray[100]={0,};
	memcpy(byteArray, origineByteArray,100);
	for(int i=0;i<100;++i)
	{
		byteArray[i]=i+1;
	}
	memcpy(origineByteArray, byteArray, 100);
	EncryptBinaryWaterMark(byteArray,100);
	DecryptBinaryWaterMark(byteArray,100);

	for(int i=0;i<100;++i)
	{
		if(byteArray[i]!=origineByteArray[i])
			LOGWRITER(_T("Bynary 암호화 실패"));
	}

}



/*
Byte-oder Mark

EF BB BF		UTF-8
FE FF			UTF-16/UCS-2 Big endian
FF FE			UTF-16/UCS-2 littel endian
00 00 FE FF		UTF-32/UCS-4 big endian
FF FE 00 00		UTF-32/UCS-4 littel endian

*/
