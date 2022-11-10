#include "StdAfx.h"
#include "WaterMarkMgr.h"

CWaterMarkMgr::CWaterMarkMgr(void)
{
	g_pLog = new CLogWriter;
	m_fileHelper.m_pOwner = this;
}

CWaterMarkMgr::~CWaterMarkMgr(void)
{
	delete g_pLog;
}


void CWaterMarkMgr::Init( CStringA customerInfoString )
{
	m_waterMarkString.init();
	m_waterMarkString.SetCustomerInfo(customerInfoString);
}

void CWaterMarkMgr::Init()
{
	m_waterMarkString.init();
}

bool CWaterMarkMgr::WriteWaterMark( CString fileName )
{
	CString MarkingStr = m_waterMarkString.GetWaterMarkString();
	return m_fileHelper.WriteWaterMark( fileName, MarkingStr );
}

CString CWaterMarkMgr::FindWaterMark( CString fileName )
{
	CString WaterStr = m_fileHelper.FindStringInFile( fileName, m_waterMarkString.GetWaterMarkHeader() );
	return WaterStr;
	
}

void CWaterMarkMgr::WriteWaterMrkInFoldertree( CString folderName )
{
	m_fileHelper.WriteWaterMarkInFolderTree(STRING_CONVERT_T_TO_A(folderName), m_waterMarkString.GetWaterMarkString());
}

bool CWaterMarkMgr::FindWaterMarkInFoldertree( CString folderName )
{
	vecFindedWaterMark vecInfo;
	// 폴더 트리를 뒤져서 스트링을 찾아낸다
	m_fileHelper.FindStringInForderTree(STRING_CONVERT_T_TO_A(folderName), m_waterMarkString.GetWaterMarkHeader(), &vecInfo);
	if(vecInfo.size() == 0)
	{
		LOGWRITER(_T("실패 : 문자열을 찾지 못했습니다. "));
		return false;
	}

	// 워터마크를 복호화 한다.
	for(vecFindedWaterMark::iterator iter = vecInfo.begin(); iter != vecInfo.end(); ++iter)
	{
		iter->m_findedWaterMark = m_waterMarkString.ConvertWatermarkToString(iter->m_findedWaterMark);
	}

	// 파일에 출력
	for(vecFindedWaterMark::iterator iter = vecInfo.begin(); iter != vecInfo.end(); ++iter)
	{
		int cout=0;

		m_fileHelper.WriteLogFile_NewLine(_T("======= 발견된 주석 워터마크 ======= "));
		m_fileHelper.WriteLogFile_NewLine(_T("워터마크 내용"));
		m_fileHelper.WriteLogFile_NewLine(_T("-------------------------"));
		m_fileHelper.WriteLogFile_NewLine(iter->m_findedWaterMark);
		m_fileHelper.WriteLogFile_NewLine(_T("-------------------------\r\n"));
		m_fileHelper.WriteLogFile_NewLine(_T("발견 위치 파일 목록"));
		
		for (vector<sFileInfo_t>::iterator iter2 = iter->m_vecFile.begin(); iter2 != iter->m_vecFile.end(); ++iter2)
		{
			m_fileHelper.WriteLogFile_NewLine(iter2->m_filePath);
			m_fileHelper.WriteLogFile(_T("\\   "));
			m_fileHelper.WriteLogFile(iter2->m_fileName);
			++cout;
		}
		CString EndStr;
		EndStr.Format(_T("%d개 파일 발견"), cout);
		m_fileHelper.WriteLogFile_NewLine(_T("\r\n")+EndStr);
		m_fileHelper.WriteLogFile_NewLine(_T("===============================================================\r\n\r\n"));
	}
	return true;
	
}

bool CWaterMarkMgr::DetectBinaryWaterMark( CStringA fileName )
{
	// 자기 자신도 검색해 버릴 때가 있다.
	if(    (fileName.Find("Watermark_Detecter.exe")!=-1)  ||  
		(fileName.Find("Watermark_Detecter.lib")!=-1)  ||  
		(fileName.Find("Watermark_Detecter.dll")!=-1)  ||  
		(fileName.Find("WatermarkDllTest.exe")!=-1))
		return true;

	bool bfind = false;
	BinaryWaterMark Mark(  (BYTE*)g_StartWaterMark,   sizeof(char)*14,   (BYTE*)g_EndtWaterMark, sizeof(char)*14  );
	
	if(  m_fileHelper.DetectBinaryWaterMarkInFile(STRING_CONVERT_A_TO_T(fileName), Mark)  )
	{
		for(vecDetectedBinaryWaterMark::iterator iter = Mark.m_vecWaterMark.begin();iter != Mark.m_vecWaterMark.end();++iter)
		{
			// 검색된 워터마크 들에 대한 로그를 찍는다.
			if(iter == Mark.m_vecWaterMark.begin())	// 첨에만 한번
			{
				bfind = true;
				m_fileHelper.WriteLogFile_NewLine(_T("\r\n----- 바이너리 워터마크 검색 시작 ----- \r\n"));
			}

			m_fileHelper.DecryptBinaryWaterMark((*iter)->m_WaterMark.m_pBytes, (*iter)->m_WaterMark.m_size);
			CString &strLog = (*iter)->ConvertedString();
			LOGWRITER(_T("\r\n")+strLog+_T("\r\n"));
			m_fileHelper.WriteLogFile_NewLine(strLog);
		}
		if(bfind == true)
		{
			m_fileHelper.WriteLogFile_NewLine(_T("\r\n"));
			m_fileHelper.WriteLogFile_NewLine(STRING_CONVERT_A_TO_T(fileName));
			m_fileHelper.WriteLogFile_NewLine(_T("----- 바이너리 워터마크 검색 완료 ----- \r\n"));
		}
	}else
	{
		LOGWRITER(_T("바이너리 워터마크를 찾지 못하였습니다."));
	}
	
	return true;
}


bool CWaterMarkMgr::WriteSrcWaterMark(CStringA filePath, CStringA fileName)
{
	fileName.MakeUpper();
// 	if(FileName.Find(".CPP")==-1)	//무조건 cpp파일에만 찍힘
// 		return false;
		
	
	CStringA strWaterMarkInfo_utf8;
	int pos =0;
	CStringA strFileName = fileName.Tokenize(".",pos);
	
	CStringA WaterMark = "\r\n#ifdef SRCCOPY\r\n";
	WaterMark += "unsigned char ";
	CStringA VariableName = "g_" + strFileName + "reference";
	WaterMark += VariableName;
	WaterMark += "[] = ";
	
	WaterMark += "{0x00, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xff, 0x00, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xff,";
	for(int i=0;i<g_MaxBinaryWaterMarkSize;++i)
	{
		WaterMark += "0x00, ";	
	}
	
	WaterMark += "0xaa, 0xbb, 0xcc, 0xdd, 0xff, 0x99, 0xaa, 0xbb, 0xcc, 0x99, 0xaa, 0xdd, 0xff, 0x00};\r\n";
	
	WaterMark += "void " + strFileName + "reference()";
	WaterMark += "\r\n{\r\n\t";
	WaterMark += VariableName + ";\r\n";
	WaterMark += "}\r\n";
	
	WaterMark += "#endif\r\n";
	
	filePath+= fileName;
	if(   !m_fileHelper.WriteSrcWaterMark(STRING_CONVERT_A_TO_T(filePath),STRING_CONVERT_A_TO_T(WaterMark))   )
	{
		LOGWRITER(_T("WriteSrcWaterMark 실패."));
	}

	return true;
}


bool CWaterMarkMgr::ChangeWaterMarkBinary(CStringA filePath, CStringA waterMarkInfo)
{
	CStringA WaterMark;
	
	CStringA strWaterMarkInfo_utf8;
	strWaterMarkInfo_utf8 = m_waterMarkString.ConvertANSIToUTF8(waterMarkInfo);

	BinaryWaterMark Mark(  (BYTE*)g_StartWaterMark,   sizeof(char)*14,   (BYTE*)g_EndtWaterMark, sizeof(char)*14  );

	if(  m_fileHelper.DetectBinaryWaterMarkInFile(STRING_CONVERT_A_TO_T(filePath), Mark)  )
	{
		m_fileHelper.ChangeWaterMarkBinary(STRING_CONVERT_A_TO_T(filePath), strWaterMarkInfo_utf8, Mark.m_vecWaterMark);
		LOGWRITER(_T("ChangeWaterMarkBinary 성공."));
		return true;
	}
	else
	{
		LOGWRITER(_T("ChangeWaterMarkBinary 워터마크 찍을 공간을 찾지 못하였음."));
	}
	return false;
}

/******************************  Test  **********************************/
bool CWaterMarkMgr::funTest()
{
	//Test1 : 스트링 관련 테스트들
 	if(!(m_waterMarkString.functionTest()))
 		return false;
// 	
		
	if( !WriteWaterMarkTest() )
		return false;
		
// 	if(!FindStrTestFromFolderTree())
// 		return false;
		
	return true;
}

bool CWaterMarkMgr::WriteWaterMarkTest()
{
	CopyTestFiles();
	
	CString file1 = _T("..\\..\\bin\\ANSI.cpp");
	CString file2 = _T("..\\..\\bin\\UNI.cpp");
	CString file3 = _T("..\\..\\bin\\UTF-8.cpp");

 	if(!m_fileHelper.WriteWaterMark(file1,m_waterMarkString.GetWaterMarkString() ))
 		return false;
	if(!m_fileHelper.WriteWaterMark(file2,m_waterMarkString.GetWaterMarkString() ))
		return false;
 	if(!m_fileHelper.WriteWaterMark(file3,m_waterMarkString.GetWaterMarkString() ))
 		return false;
		
	FindStringTestFromFolderTree();
	return true;
}

void CWaterMarkMgr::CopyTestFiles()
{
	CStringA commend = "copy ..\\TestSample\\";
	CStringA folder = "..\\..\\bin\\";
	CStringA file1 = "ANSI.cpp";
	CStringA file2 = "UNI.cpp";
	CStringA file3 = "UTF-8.cpp";

	CUtil::SystemCommand(commend, folder,file1);
	CUtil::SystemCommand(commend, folder,file2);
	CUtil::SystemCommand(commend, folder,file3);
}

bool CWaterMarkMgr::OneFileWriteWaterMarktest(CStringA fileName)
{
	{	// 파일 복사해 오기
		CStringA commend = "copy ..\\TestSample\\";
		CStringA folder = "..\\..\\bin\\";
		CUtil::SystemCommand(commend, folder,fileName);
	}

	// 마킹찍기
	{
		CString file = _T("..\\..\\bin\\");
		file += STRING_CONVERT_A_TO_T(fileName);

		if(m_fileHelper.WriteWaterMark(  file,m_waterMarkString.GetWaterMarkString() )    )
		{
			fileName += "파일 워터마크 찍기 완료";
			LOGWRITER(STRING_CONVERT_A_TO_T(fileName) );
		}else
 			return false;
	}

	return FindStringTestFromFolderTree();
}


bool CWaterMarkMgr::FindStringTest(CString testFileName)
{
	CString str;
	{
		str = m_fileHelper.FindStringInFile(testFileName, m_waterMarkString.GetWaterMarkHeader());
		if(str.GetLength() == 0)
			return false;

		str = m_waterMarkString.ConvertWatermarkToString(str);

		CString originCustomerInfo = STRING_CONVERT_A_TO_T(m_waterMarkString.m_originCustomerInfo);
		if(str.Compare(originCustomerInfo) != 0)
		{
			LOGWRITER(_T("글자가 맞지 않습니다.\r\n"));
			LOGWRITER(_T("최종 ")+str);
			LOGWRITER(_T("처음 ")+originCustomerInfo);
			return false;
		}

		LOGWRITER(_T("FindString 성공!!"));
		LOGWRITER(str);
	}
	return true;
}

bool CWaterMarkMgr::FindStringTestFromFolderTree()
{
	vecFindedWaterMark vecInfo;
	m_fileHelper.FindStringInForderTree("..\\..\\bin\\", m_waterMarkString.GetWaterMarkHeader(), &vecInfo);
	if(vecInfo.size() == 0)
	{
		LOGWRITER(_T("실패 : 문자열을 찾지 못했습니다. "));
		return false;
	}


	for(vecFindedWaterMark::iterator iter = vecInfo.begin(); iter != vecInfo.end(); ++iter)
	{
		int count=0;

		m_fileHelper.WriteLogFile_NewLine(iter->m_findedWaterMark);
		CString companyInfo = m_waterMarkString.ConvertWatermarkToString(iter->m_findedWaterMark);
		m_fileHelper.WriteLogFile_NewLine(companyInfo);
		m_fileHelper.WriteLogFile_NewLine(_T("\r\n"));
		m_fileHelper.WriteLogFile_NewLine(_T("발견 위치 파일 목록"));
		
		for (vector<sFileInfo_t>::iterator iter2 = iter->m_vecFile.begin(); iter2 != iter->m_vecFile.end(); ++iter2)
		{
			m_fileHelper.WriteLogFile_NewLine(iter2->m_filePath);
			m_fileHelper.WriteLogFile(_T("\\   "));
			m_fileHelper.WriteLogFile(iter2->m_fileName);
			++count;
		}
		CString endStr;
		endStr.Format(_T("%d개 파일 발견"), count);
		m_fileHelper.WriteLogFile_NewLine(endStr);
	}
	return true;
	
}
