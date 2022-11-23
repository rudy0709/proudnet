// StdioFileEx.cpp: implementation of the CStdioFileEx2 class.
//
// Version 1.1 23 August 2003.	Incorporated fixes from Dennis Jeryd.
// Version 1.3 19 February 2005. Incorporated fixes from Howard J Oh and some of my own.
// Version 1.4 26 February 2005. Fixed stupid screw-up in code from 1.3.
// Version 1.5 18 November 2005. - Incorporated fixes from Andy Goodwin.
//											- Allows code page to be specified for reading/writing
//											- Properly calculates multibyte buffer size instead of
//												assuming lstrlen(s).
//											- Should handle UTF8 properly.
//
// Copyright David Pritchard 2003-2005. davidpritchard@ctv.es
//
// You can use this class freely, but please keep my ego happy 
// by leaving this comment in place.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "StdioFileEx2.h"
#include "Util.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

/*static*/ const UINT CStdioFileEx2::modeWriteUnicode = 0x20000; // Add this flag to write in Unicode

CStdioFileEx2::CStdioFileEx2(): CStdioFile()
{
	bReadOnly = false;
	m_nFileCodePage = -1;
	m_encodeType = eUNICODE;
}

CStdioFileEx2::CStdioFileEx2(LPCTSTR lpszFileName,UINT nOpenFlags)
	:CStdioFile(lpszFileName, ProcessFlags(lpszFileName, nOpenFlags))
{
	bReadOnly = false;
	m_nFileCodePage = -1;
	m_encodeType = eUNICODE;
}

void CStdioFileEx2::SetCodePage(IN const UINT nCodePage)
{
	m_nFileCodePage = (int)nCodePage; 
}

BOOL CStdioFileEx2::Open(LPCTSTR lpszFileName,UINT nOpenFlags,CFileException* pError /*=NULL*/)
{
	setlocale(LC_ALL, "Korean");
	//_wsetlocale(LC_ALL, _T("korean"));
	//setlocale(LC_ALL, "ko_KR.UTF-8");
	
	// 읽기 전용 파일은 파일 오픈조자 못하는 경우가 있다
	// 이부분은 파일을 연 후에는 사용할 수 없다.
#ifdef _UNICODE
	if( CUtil::IsFileReadonly((LPCSTR)CW2A(lpszFileName)) )
	{
		CUtil::FileReadonlyOff((LPCSTR)CW2A(lpszFileName));
		bReadOnly = true;
	}

#else
if( CUtil::IsFileReadonly(lpszFileName) )
{
	CUtil::FileReadonlyOff(lpszFileName);
	bReadOnly = true;
}
#endif
	
	// Process any Unicode stuff
	ProcessFlags(lpszFileName, nOpenFlags);
	if( CStdioFile::Open(lpszFileName, nOpenFlags, pError) )
	{
		m_FileName = lpszFileName;
		return true;
		
	}else
	{

#ifdef _UNICODE
		if( bReadOnly)
		{
			CUtil::FileReadonlyOn((LPCSTR)CW2A(lpszFileName));
			bReadOnly = true;
		}

#else
if( bReadOnly )
{
	CUtil::FileReadonlyOn(lpszFileName);
	bReadOnly = true;
}
#endif
	}
	
	return false;

	
}

void CStdioFileEx2::Close()
{
	CStdioFile::Close();

	// 파일을 읽기 전용옵션을 다시 킨다.
	// 파일포인터를 닫은 상태에서만 가능하다.
	if(bReadOnly == true)
	{
#ifdef _UNICODE
		CUtil::FileReadonlyOff((LPCSTR)CW2A(m_FileName));
#else
		CUtil::FileReadonlyOff((m_FileName));
#endif
	}
}

BOOL CStdioFileEx2::ReadString(CString& rString)
{
	const int	nMAX_LINE_CHARS = 4096;
	BOOL			bReadData = FALSE;
	LPTSTR		lpsz;
	int			nLen = 0;
	wchar_t*		pszUnicodeString = NULL;
	char	*		pszMultiByteString= NULL;
	int			nChars = 0;

	try
	{
		// If at position 0, discard byte-order mark before reading
		if (!m_pStream || (GetPosition() == 0))
		{
			wchar_t	cDummy;
	//		Read(&cDummy, sizeof(_TCHAR));
			Read(&cDummy, sizeof(wchar_t));
			
			if(cDummy == nUNICODE_BOM)
			{
				m_encodeType = eUNICODE;
			}else if(cDummy == 0xBBEF)
			{
				BYTE B_Dumy;
				Read(&B_Dumy, sizeof(BYTE));
				if(B_Dumy == 0xBF)
					m_encodeType = eUTF_8;
				else	// 틀리면 ANSI 코드 처리
				{
					//ANSI코드는 BOM이 없기 때문에 처음으로 돌아가야 함
					SeekToBegin();
					m_encodeType = eANSI;
				}
					
			}else		// ANSI 코드 처리
			{
				//ANSI코드는 BOM이 없기 때문에 처음으로 돌아가야 함
				m_encodeType = eANSI;
				SeekToBegin();
			}
		}

// If compiled for Unicode
#ifdef _UNICODE
		if (m_encodeType == eUNICODE)
		{
			// Do standard stuff - Unicode to Unicode. Seems to work OK.
			bReadData = CStdioFile::ReadString(rString);
		}
		else //if (m_encodeType == eANSI)	// 읽어 들일 때에는 ANSI와 UTF-8이 호환이 잘 된다. 하지만 http파일 같은 경우 깨지는 경우도 있다고 하니 주의하자.
		{
			pszUnicodeString	= new wchar_t[nMAX_LINE_CHARS]; 
			pszMultiByteString= new char[nMAX_LINE_CHARS]; 

			// Initialise to something safe
			memset(pszUnicodeString, 0, sizeof(wchar_t) * nMAX_LINE_CHARS);
			memset(pszMultiByteString, 0, sizeof(char) * nMAX_LINE_CHARS);
			
			// Read the string
			bReadData = (NULL != fgets(pszMultiByteString, nMAX_LINE_CHARS, m_pStream));

			if (bReadData)
			{
				// Convert multibyte to Unicode, using the specified code page
				nChars = GetUnicodeStringFromMultiByteString(pszMultiByteString, pszUnicodeString, nMAX_LINE_CHARS, m_nFileCodePage);

				if (nChars > 0)
				{
					rString = (CString)pszUnicodeString;
				}
			}
		}
	
#else

		if (!m_encodeType == eUNICODE)
		{
			// Do standard stuff -- read ANSI in ANSI
			bReadData = CStdioFile::ReadString(rString);

			// Get the current code page
			UINT nLocaleCodePage = GetCurrentLocaleCodePage();

			// If we got it OK...
			if (nLocaleCodePage > 0)
			{
				// if file code page does not match the system code page, we need to do a double conversion!
				if (nLocaleCodePage != (UINT)m_nFileCodePage)
				{
					int nStringBufferChars = rString.GetLength() + 1;

					pszUnicodeString	= new wchar_t[nStringBufferChars]; 

					// Initialise to something safe
					memset(pszUnicodeString, 0, sizeof(wchar_t) * nStringBufferChars);
					
					// Convert to Unicode using the file code page
					nChars = GetUnicodeStringFromMultiByteString(rString, pszUnicodeString, nStringBufferChars, m_nFileCodePage);

					// Convert back to multibyte using the system code page
					// (This doesn't really confer huge advantages except to avoid "mangling" of non-convertible special
					// characters. So, if a file in the E.European code page is displayed on a system using the 
					// western European code page, special accented characters which the system cannot display will be
					// replaced by the default character (a hash or something), rather than being incorrectly mapped to
					// other, western European accented characters).
					if (nChars > 0)
					{
						// Calculate how much we need for the MB buffer (it might be larger)
						nStringBufferChars = GetRequiredMultiByteLengthForUnicodeString(pszUnicodeString,nLocaleCodePage);
						pszMultiByteString= new char[nStringBufferChars];  

						nChars = GetMultiByteStringFromUnicodeString(pszUnicodeString, pszMultiByteString, nStringBufferChars, nLocaleCodePage);
						rString = (CString)pszMultiByteString;
					}
				}
			}
		}
		else
		{
			pszUnicodeString	= new wchar_t[nMAX_LINE_CHARS]; 

			// Initialise to something safe
			memset(pszUnicodeString, 0, sizeof(wchar_t) * nMAX_LINE_CHARS);
			
			// Read as Unicode, convert to ANSI

			// Bug fix by Dennis Jeryd 06/07/2003: initialise bReadData
			bReadData = (NULL != fgetws(pszUnicodeString, nMAX_LINE_CHARS, m_pStream));

			if (bReadData)
			{
				// Calculate how much we need for the multibyte string
				int nRequiredMBBuffer = GetRequiredMultiByteLengthForUnicodeString(pszUnicodeString,m_nFileCodePage);
				pszMultiByteString= new char[nRequiredMBBuffer];  

				nChars = GetMultiByteStringFromUnicodeString(pszUnicodeString, pszMultiByteString, nRequiredMBBuffer, m_nFileCodePage);

				if (nChars > 0)
				{
					rString = (CString)pszMultiByteString;
				}
			}

		}
#endif

		// Then remove end-of-line character if in Unicode text mode
		if (bReadData)
		{
			// Copied from FileTxt.cpp but adapted to Unicode and then adapted for end-of-line being just '\r'. 
			nLen = rString.GetLength();
			if (nLen > 1 && rString.Mid(nLen-2) == sNEWLINE)
			{
				rString.GetBufferSetLength(nLen-2);
			}
			else
			{
				lpsz = rString.GetBuffer(0);
				if (nLen != 0 && (lpsz[nLen-1] == _T('\r') || lpsz[nLen-1] == _T('\n')))
				{
					rString.GetBufferSetLength(nLen-1);
				}
			}
		}
	}
	// Ensure we always delete in case of exception
	catch(...)
	{
		if (pszUnicodeString)	delete [] pszUnicodeString;

		if (pszMultiByteString) delete [] pszMultiByteString;

		throw;
	}

	if (pszUnicodeString)		delete [] pszUnicodeString;

	if (pszMultiByteString)		delete [] pszMultiByteString;

	return bReadData;
}



// --------------------------------------------------------------------------------------------
//
//	CStdioFileEx2::WriteString()
//
// --------------------------------------------------------------------------------------------
// Returns:    void
// Parameters: LPCTSTR lpsz
//
// Purpose:		Writes string to file either in Unicode or multibyte, depending on whether the caller specified the
//					CStdioFileEx2::modeWriteUnicode flag. Override of base class function.
// Notes:		If writing in Unicode we need to:
//						a) Write the Byte-order-mark at the beginning of the file
//						b) Write all strings in byte-mode
//					-	If we were compiled in Unicode, we need to convert Unicode to multibyte if 
//						we want to write in multibyte
//					-	If we were compiled in multi-byte, we need to convert multibyte to Unicode if 
//						we want to write in Unicode.
// Exceptions:	None.
//
void CStdioFileEx2::WriteString(LPCTSTR lpsz)
{
	wchar_t*	pszUnicodeString	= NULL; 
	char	*	pszMultiByteString= NULL;
	
	try
	{
		// If writing Unicode and at the start of the file, need to write byte mark
		//if (m_nFlags & CStdioFileEx2::modeWriteUnicode)
		{
			// If at position 0, write byte-order mark before writing anything else
			if (!m_pStream || GetPosition() == 0)
			{
				//wchar_t cBOM = (wchar_t)nUNICODE_BOM;
				if(m_encodeType == eANSI)
				{
					// ANSI는 BOM이 없다.!!!!
				}
				else if(m_encodeType == eUNICODE)
				{
					wchar_t cBom = nUNICODE_BOM;
					CFile::Write((void*)&cBom, sizeof(wchar_t));
				}
				else if(m_encodeType == eUTF_8)
				{
					BYTE bBOM;
					bBOM = 0xEF;
					CFile::Write((void*)&bBOM, sizeof(BYTE));

					bBOM = 0xBB;
					CFile::Write((void*)&bBOM, sizeof(BYTE));

					bBOM = 0xBF;
					CFile::Write((void*)&bBOM, sizeof(BYTE));
				}
			}
		}

// If compiled in Unicode...
#ifdef _UNICODE

		// If writing Unicode, no conversion needed
		if (m_encodeType==eUNICODE)
		{
			// Write in byte mode
			CFile::Write(lpsz, lstrlen(lpsz) * sizeof(wchar_t));
		}
		// Else if we don't want to write Unicode, need to convert
		else if (m_encodeType==eANSI)		// 유니코드와 아스키 코드는 완벽하게 호환하나, 오로지 영문과 숫자에 한에서이다.
		{
			int		nChars = lstrlen(lpsz) + 1;				// Why plus 1? Because yes
//			int		nBufferSize = nChars * sizeof(char);	// leave space for multi-byte chars
			int		nCharsWritten = 0;
			int		nBufferSize = 0;

			pszUnicodeString	= new wchar_t[nChars]; 

			// Copy string to Unicode buffer
			lstrcpy(pszUnicodeString, lpsz);

			// Work out how much space we need for the multibyte conversion
			nBufferSize = GetRequiredMultiByteLengthForUnicodeString(pszUnicodeString, m_nFileCodePage);
			pszMultiByteString= new char[nBufferSize];  

			// Get multibyte string
			nCharsWritten = GetMultiByteStringFromUnicodeString(pszUnicodeString, pszMultiByteString, nBufferSize, m_nFileCodePage);

			if (nCharsWritten > 0)
			{
				// Do byte-mode write using actual chars written (fix by Howard J Oh)
	//			CFile::Write((const void*)pszMultiByteString, lstrlen(lpsz));
				CFile::Write((const void*)pszMultiByteString,
					nCharsWritten*sizeof(char));
			}
		}
		
		else if (m_encodeType==eUTF_8)		// 유니코드와 아스키 코드는 완벽하게 호환한다.
		{
			CString unicode = lpsz;
			int iSize = unicode.GetLength()*sizeof(wchar_t);
			CStringA utf8;

			
			utf8 = (LPCSTR)CW2A(unicode,CP_UTF8);
			
			CFile::Write((const void*)utf8,utf8.GetLength());
		}

	// Else if *not* compiled in Unicode
#else
		// If writing Unicode, need to convert
		if (m_encodeType==eUNICODE)
		{
			int		nChars = lstrlen(lpsz) + 1;	 // Why plus 1? Because yes
			int		nBufferSize = nChars * sizeof(wchar_t);
			int		nCharsWritten = 0;

			pszUnicodeString	= new wchar_t[nChars];
			pszMultiByteString= new char[nChars]; 

			// Copy string to multibyte buffer
			lstrcpy(pszMultiByteString, lpsz);

			nCharsWritten = GetUnicodeStringFromMultiByteString(pszMultiByteString, pszUnicodeString, nChars, m_nFileCodePage);

			if (nCharsWritten > 0)
			{
				// Do byte-mode write using actual chars written (fix by Howard J Oh)
	//			CFile::Write(pszUnicodeString, lstrlen(lpsz) * sizeof(wchar_t));
				CFile::Write(pszUnicodeString, nCharsWritten*sizeof(wchar_t));
			}
			else
			{
				ASSERT(false);
			}

		}
		// Else if we don't want to write Unicode, no conversion needed, unless the code page differs
		else if (m_encodeType==eANSI)
		{
	//		// Do standard stuff
	//		CStdioFile::WriteString(lpsz);

			// Get the current code page
			UINT nLocaleCodePage = GetCurrentLocaleCodePage();

			// If we got it OK, and if file code page does not match the system code page, we need to do a double conversion!
			if (nLocaleCodePage > 0 && nLocaleCodePage != (UINT)m_nFileCodePage)
			{
				int	nChars = lstrlen(lpsz) + 1;	 // Why plus 1? Because yes

				pszUnicodeString	= new wchar_t[nChars]; 

				// Initialise to something safe
				memset(pszUnicodeString, 0, sizeof(wchar_t) * nChars);
				
				// Convert to Unicode using the locale code page (the code page we are using in memory)
				nChars = GetUnicodeStringFromMultiByteString((LPCSTR)(const char*)lpsz, pszUnicodeString, nChars, nLocaleCodePage);

				// Convert back to multibyte using the file code page
				// (Note that you can't reliably read a non-Unicode file written in code page A on a system using a code page B,
				// modify the file and write it back using code page A, unless you disable all this double-conversion code.
				// In effect, you have to choose between a mangled character display and mangled file writing).
				if (nChars > 0)
				{
					// Calculate how much we need for the MB buffer (it might be larger)
					nChars = GetRequiredMultiByteLengthForUnicodeString(pszUnicodeString, m_nFileCodePage);

					pszMultiByteString= new char[nChars];  
					memset(pszMultiByteString, 0, sizeof(char) * nChars);

					nChars = GetMultiByteStringFromUnicodeString(pszUnicodeString, pszMultiByteString, nChars, m_nFileCodePage);

					// Do byte-mode write. This avoids annoying "interpretation" of \n's as \r\n
					CFile::Write((const void*)pszMultiByteString, nChars * sizeof(char));
				}
			}
			else
			{
				// Do byte-mode write. This avoids annoying "interpretation" of \n's as \r\n
				CFile::Write((const void*)lpsz, lstrlen(lpsz)*sizeof(char));
			}
		}else if (m_encodeType==eUTF_8)
		{
			CStringW Wstr = (LPCWSTR)CA2W(lpsz);
			string Astr = (LPCSTR)CW2A(Wstr,CP_UTF8);
			
			CFile::Write((const void*)Astr.c_str(), Astr.size());
		}

#endif
	}
	// Ensure we always clean up
	catch(...)
	{
		if (pszUnicodeString)	delete [] pszUnicodeString;
		if (pszMultiByteString)	delete [] pszMultiByteString;
		throw;
	}

	if (pszUnicodeString)	delete [] pszUnicodeString;
	if (pszMultiByteString)	delete [] pszMultiByteString;
}

UINT CStdioFileEx2::ProcessFlags(const CString& sFilePath, UINT& nOpenFlags)
{
	// If we have writeUnicode we must have write or writeRead as well
#ifdef _DEBUG
	if (nOpenFlags & CStdioFileEx2::modeWriteUnicode)
	{
		ASSERT(nOpenFlags & CFile::modeWrite || nOpenFlags & CFile::modeReadWrite);
	}
#endif

	m_encodeType = FileEncodetype(sFilePath);
	
	// If reading in text mode and not creating... ; fixed by Dennis Jeryd 6/8/03
	if (nOpenFlags & CFile::typeText && !(nOpenFlags & CFile::modeCreate) && !(nOpenFlags & CFile::modeWrite ))
	{
		// If it's Unicode, switch to binary mode
		if (m_encodeType != eANSI)
		{
			nOpenFlags ^= CFile::typeText;
			nOpenFlags |= CFile::typeBinary;
		}
	}

	m_nFlags = nOpenFlags;

	return nOpenFlags;
}

// --------------------------------------------------------------------------------------------
//
//	CStdioFileEx2::IsFileUnicode()
//
// --------------------------------------------------------------------------------------------
// Returns:    bool
// Parameters: const CString& sFilePath
//
// Purpose:		Determines whether a file is Unicode by reading the first character and detecting
//					whether it's the Unicode byte marker.
// Notes:		None.
// Exceptions:	None.
//
/*static*/ encodeType CStdioFileEx2::FileEncodetype(const CString& sFilePath)
{
	CFile				file;
	CFileException	exFile;
	encodeType EncodeType;

	// Open file in binary mode and read first character
	if (file.Open(sFilePath, CFile::typeBinary | CFile::modeRead, &exFile))
	{
		// If byte is Unicode byte-order marker, let's say it's Unicode
// 		if (file.Read(&cFirstChar, sizeof(wchar_t)) > 0 && cFirstChar == (wchar_t)nUNICODE_BOM)
// 		{
// 			bIsUnicode = true;
// 		}
 		wchar_t	cDummy;
 		//		Read(&cDummy, sizeof(_TCHAR));
 		file.Read(&cDummy, sizeof(wchar_t));
				
		if(cDummy == nUNICODE_BOM)
		{
			EncodeType = eUNICODE;
		}else if(cDummy == 0xBBEF)
		{
			BYTE B_Dumy;
			file.Read(&B_Dumy, sizeof(BYTE));
			if(B_Dumy == 0xBF)
				EncodeType = eUTF_8;
			else	// 틀리면 ANSI 코드 처리
			{
				EncodeType = eANSI;
			}

		}else		// ANSI 코드 처리
		{
			EncodeType = eANSI;
		}
		
		file.Close();
		return EncodeType;
	}
	else
	{
		// Handle error here if you like
	}

	// 파일이 열리지 않았다면 utf-8로 처리함 
	//우리 회사에서 utf8 을 가장 많이 사용하기 때문
	return eANSI;
}

unsigned long CStdioFileEx2::GetCharCount()
{
	int				nCharSize;
	unsigned long	nByteCount, nCharCount = 0;

	if (m_pStream)
	{
		// Get size of chars in file
		nCharSize = (m_encodeType == eUNICODE) ? sizeof(wchar_t): sizeof(char);

		// If Unicode, remove byte order mark from count
		nByteCount = (unsigned long)GetLength();
		
		if (m_encodeType == eUNICODE)
		{
			nByteCount = nByteCount - sizeof(wchar_t);
		}

		// Calc chars
		nCharCount = (nByteCount / nCharSize);
	}

	return nCharCount;
}

// Get the current user큦 code page
UINT CStdioFileEx2::GetCurrentLocaleCodePage()
{
	_TCHAR	szLocalCodePage[10];
	UINT		nLocaleCodePage = 0;
	int		nLocaleChars = ::GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IDEFAULTANSICODEPAGE, szLocalCodePage, 10);

	// If OK
	if (nLocaleChars > 0)
	{
		nLocaleCodePage = (UINT)_ttoi(szLocalCodePage);
		ASSERT(nLocaleCodePage > 0);
	}
	else
	{
		ASSERT(false);
	}

	// O means either: no ANSI code page (Unicode-only locale?) or failed to get locale
	// In the case of Unicode-only locales, what do multibyte apps do? Answers on a postcard.
	return nLocaleCodePage;
}

// --------------------------------------------------------------------------------------------
//
//	CStdioFileEx2::GetUnicodeStringFromMultiByteString()
//
// --------------------------------------------------------------------------------------------
// Returns:    int - num. of chars written (0 means error)
// Parameters: char *		szMultiByteString		(IN)		Multi-byte input string
//					wchar_t*		szUnicodeString		(OUT)		Unicode outputstring
//					int			nUnicodeBufferSize	(IN)		Size of Unicode output buffer in chars(IN)
//					UINT			nCodePage				(IN)		Code page used to perform conversion
//																			Default = -1 (Get local code page).
//
// Purpose:		Gets a Unicode string from a MultiByte string.
// Notes:		None.
// Exceptions:	None.
//
int CStdioFileEx2::GetUnicodeStringFromMultiByteString(IN LPCSTR szMultiByteString, OUT wchar_t* szUnicodeString, IN int nUnicodeBufferSize, IN int nCodePage)
{
	bool		bOK = true;
	int		nCharsWritten = 0;
		
	if (szUnicodeString && szMultiByteString)
	{
		// If no code page specified, take default for system
		if (nCodePage == -1)
		{
			nCodePage = GetACP();
		}

		try 
		{
			// Zero out buffer first. NB: nUnicodeBufferSize is NUMBER OF CHARS, NOT BYTES!
			memset((void*)szUnicodeString, '\0', sizeof(wchar_t) *
				nUnicodeBufferSize);

			// When converting to UTF8, don't set any flags (see Q175392).
			nCharsWritten = MultiByteToWideChar((UINT)nCodePage,
				(nCodePage==CP_UTF8 ? 0:MB_PRECOMPOSED), // Flags
				szMultiByteString,-1,szUnicodeString,nUnicodeBufferSize);
		}
		catch(...)
		{
			TRACE(_T("Controlled exception in MultiByteToWideChar!\n"));
		}
	}

	// Now fix nCharsWritten
	if (nCharsWritten > 0)
	{
		nCharsWritten--;
	}
	
	ASSERT(nCharsWritten > 0);
	return nCharsWritten;
}

// --------------------------------------------------------------------------------------------
//
//	CStdioFileEx2::GetMultiByteStringFromUnicodeString()
//
// --------------------------------------------------------------------------------------------
// Returns:    int - number of characters written. 0 means error
// Parameters: wchar_t *	szUnicodeString			(IN)	Unicode input string
//					char*			szMultiByteString			(OUT)	Multibyte output string
//					int			nMultiByteBufferSize		(IN)	Multibyte buffer size (chars)
//					UINT			nCodePage					(IN)	Code page used to perform conversion
//																			Default = -1 (Get local code page).
//
// Purpose:		Gets a MultiByte string from a Unicode string
// Notes:		Added fix by Andy Goodwin: make buffer into int.
// Exceptions:	None.
//
int CStdioFileEx2::GetMultiByteStringFromUnicodeString(wchar_t * szUnicodeString, char* szMultiByteString, 
																			int nMultiByteBufferSize, int nCodePage)
{
	BOOL		bUsedDefChar	= FALSE;
	int		nCharsWritten = 0;

	// Fix by Andy Goodwin: don't do anything if buffer is 0
	if ( nMultiByteBufferSize > 0 )
	{
		if (szUnicodeString && szMultiByteString) 
		{
			// Zero out buffer first
			memset((void*)szMultiByteString, '\0', nMultiByteBufferSize);
			
			// If no code page specified, take default for system
			if (nCodePage == -1)
			{
				nCodePage = GetACP();
			}

			try 
			{
				// If writing to UTF8, flags, default char and boolean flag must be NULL
				nCharsWritten = WideCharToMultiByte((UINT)nCodePage, 
					(nCodePage==CP_UTF8 ? 0 : WC_COMPOSITECHECK | WC_SEPCHARS), // Flags
					szUnicodeString,-1, 
					szMultiByteString, 
					nMultiByteBufferSize, 
					(nCodePage==CP_UTF8 ? NULL:sDEFAULT_UNICODE_FILLER_CHAR),	// Filler char
					(nCodePage==CP_UTF8 ? NULL: &bUsedDefChar));						// Did we use filler char?

				// If no chars were written and the buffer is not 0, error!
				if (nCharsWritten == 0 && nMultiByteBufferSize > 0)
				{
					TRACE1("Error in WideCharToMultiByte: %d\n", ::GetLastError());
				}
			}
			catch(...) 
			{
				TRACE(_T("Controlled exception in WideCharToMultiByte!\n"));
			}
		} 
	}

	// Now fix nCharsWritten 
	if (nCharsWritten > 0)
	{
		nCharsWritten--;
	}
	
	return nCharsWritten;
}

//---------------------------------------------------------------------------------------------------
//
//	CStdioFileEx2::GetRequiredMultiByteLengthForUnicodeString()
//
//---------------------------------------------------------------------------------------------------
// Returns:    int
// Parameters: wchar_t * szUnicodeString,int nCodePage=-1
//
// Purpose:		Obtains the multi-byte buffer size needed to accommodate a converted Unicode string.
//	Notes:		We can't assume that the buffer length is simply equal to the number of characters
//					because that wouldn't accommodate multibyte characters!
//
/*static*/ int CStdioFileEx2::GetRequiredMultiByteLengthForUnicodeString(wchar_t * szUnicodeString,int nCodePage /*=-1*/)
{
	int nCharsNeeded = 0;

	try 
	{
		// If no code page specified, take default for system
		if (nCodePage == -1)
		{
			nCodePage = GetACP();
		}

		// If writing to UTF8, flags, default char and boolean flag must be NULL
		nCharsNeeded = WideCharToMultiByte((UINT)nCodePage, 
			(nCodePage==CP_UTF8 ? 0 : WC_COMPOSITECHECK | WC_SEPCHARS), // Flags
			szUnicodeString,-1, 
			NULL, 
			0,	// Calculate required buffer, please! 
			(nCodePage==CP_UTF8 ? NULL:sDEFAULT_UNICODE_FILLER_CHAR),	// Filler char
			NULL);
	}
	catch(...) 
	{
		TRACE(_T("Controlled exception in WideCharToMultiByte!\n"));
	}

	return nCharsNeeded;
}

/*
//ANSI to UTF8
string CStdioFileEx2::ansi_to_utf8(string& ansi) 
{ 
	WCHAR unicode[1500]; 
	char utf8[1500]; 

	memset(unicode, 0, sizeof(unicode)); 
	memset(utf8, 0, sizeof(utf8)); 

	::MultiByteToWideChar(CP_ACP, 0, ansi.c_str(), -1, unicode, sizeof(unicode)); 
	::WideCharToMultiByte(CP_UTF8, 0, unicode, -1, utf8, sizeof(utf8), NULL, NULL); 

	return string(utf8); 
}

//UTF8 to ANSI
string CStdioFileEx2::utf8_to_ansi(string &utf8) 
{ 
	WCHAR unicode[1500]; 
	char ansi[1500]; 

	memset(unicode, 0, sizeof(unicode)); 
	memset(ansi, 0, sizeof(ansi)); 

	::MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, unicode, sizeof(unicode)); 
	::WideCharToMultiByte(CP_ACP, 0, unicode, -1, ansi, sizeof(ansi), NULL, NULL); 

	return string(ansi); 
}
*/