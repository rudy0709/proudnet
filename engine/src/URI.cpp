#include "stdafx.h"

#include "URI.h"
#include "../include/PNString.h"
#include "../include/Ptr.h"

namespace Proud
{
#if defined(_WIN32)
	CUri::CUri()
	{
		InitFields();
		SetScheme(CUri::SCHEME_HTTP);
	}
	CUri::CUri(const CUri& url)
	{
		CopyFields(url);
	}
	CUri::~CUri()
	{
	}
	CUri& CUri::operator=(const CUri& url)
	{
		CopyFields(url);
		return (*this);
	}
	void CUri::InitFields()
	{
		m_portNumber = CUri::INVALID_PORT_NUMBER;
		m_scheme = CUri::SCHEME_UNKNOWN;

		m_schemeNameLength = 0;
		m_hostNameLength = 0;
		m_userNameLength = 0;
		m_urlPathLength = 0;
		m_passwordLength = 0;
		m_extraInfoLength = 0;

		m_schemeName[0] = '\0';
		m_hostName[0] = '\0';
		m_userName[0] = '\0';
		m_password[0] = '\0';
		m_urlPath[0] = '\0';
		m_extraInfo[0] = '\0';
	}
	const bool CUri::Parse(const TCHAR* url)
	{
		assert(url != NULL);

		TCHAR ch;
		bool bGotScheme = false;
		bool bGotUserName = false;
		bool bGotHostName = false;
		bool bGotPortNumber = false;
		String currentUrl;
		TCHAR* szCurrentUrl = currentUrl.GetBuffer(CUri::MAX_URL_LENGTH + 6);
		TCHAR* pszCurrentUrl = szCurrentUrl;
		size_t nUrlSize = 0;

		bool bInsideSquareBrackets = false;

		//parse lpszUrl using szCurrentUrl to store temporary data

		//this loop will get the following if it exists:
		//<protocol>://user:pass@server:port
		while ((ch = *url) != '\0')
		{
			if (nUrlSize >= CUri::MAX_URL_LENGTH + 5)
				goto error;

			if (ch == ':' && !bInsideSquareBrackets)
			{
				//3 cases:
				//(1) Just encountered a scheme
				//(2) Port number follows
				//(3) Form of username:password@

				// Check to see if we've just encountered a scheme
				*pszCurrentUrl = '\0';
				if (!bGotScheme)
				{
					if (!SetSchemeName(szCurrentUrl))
						goto error;

					//Set a flag to avoid checking for
					//schemes everytime we encounter a :
					bGotScheme = true;

					if (*(url + 1) == '/')
					{
						if (*(url + 2) == '/')
						{
							//the mailto scheme cannot have a '/' following the "mailto:" portion
							if (bGotScheme && m_scheme == CUri::SCHEME_MAILTO)
								goto error;

							//Skip these characters and continue
							url += 2;
						}
						else
						{
							//it is an absolute path
							//no domain name, port, username, or password is allowed in this case
							//break to loop that gets path
							url++;
							pszCurrentUrl = szCurrentUrl;
							nUrlSize = 0;
							break;
						}
					}

					//reset pszCurrentUrl
					pszCurrentUrl = szCurrentUrl;
					nUrlSize = 0;
					url++;

					//if the scheme is file, skip to getting the path information
					if (m_scheme == CUri::SCHEME_FILE)
						break;
					continue;
				}
				else if (!bGotUserName || !bGotPortNumber)
				{
					//It must be a username:password or a port number
					*pszCurrentUrl = '\0';

					pszCurrentUrl = szCurrentUrl;
					nUrlSize = 0;
					TCHAR tmpBuf[CUri::MAX_PASSWORD_LENGTH + 1];
					TCHAR* pTmpBuf = tmpBuf;
					int nCnt = 0;

					//get the user or portnumber (break on either '/', '@', or '\0'
					while (((ch = *(++url)) != '/') && (ch != '@') && (ch != '\0'))
					{
						if (nCnt >= CUri::MAX_PASSWORD_LENGTH)
							goto error;

						*pTmpBuf++ = ch;
						nCnt++;
					}
					*pTmpBuf = '\0';

					//if we broke on a '/' or a '\0', it must be a port number
					if (!bGotPortNumber && (ch == '/' || ch == '\0'))
					{
						//the host name must immediately preced the port number
						if (!CUri::SetHostName(szCurrentUrl))
							goto error;

						//get the port number
						m_portNumber = (uint16_t)_ttoi(tmpBuf);

						if (m_portNumber < 0)
							goto error;

						bGotPortNumber = bGotHostName = true;
					}
					else if (!bGotUserName && ch == '@')
					{
						//otherwise it must be a username:password
						if (!CUri::SetUserName(szCurrentUrl) || !CUri::SetPassword(tmpBuf))
							goto error;

						bGotUserName = true;
						url++;
					}
					else
					{
						goto error;
					}
				}
			}
			else if (ch == '@')
			{
				if (bGotUserName)
					goto error;

				//type is userinfo@
				*pszCurrentUrl = '\0';
				if (!SetUserName(szCurrentUrl))
					goto error;

				bInsideSquareBrackets = false;

				bGotUserName = true;
				url++;
				pszCurrentUrl = szCurrentUrl;
				nUrlSize = 0;
			}
			else if (ch == '/' || ch == '?' || (!*(url + 1)))
			{
				//we're at the end of this loop
				//set the domain name and break
				if (!*(url + 1) && ch != '/' && ch != '?')
				{
					if (nUrlSize >= CUri::MAX_URL_LENGTH + 4)
						goto error;

					*pszCurrentUrl++ = ch;
					nUrlSize++;
					url++;
				}
				*pszCurrentUrl = '\0';
				if (!bGotHostName)
				{
					if (!SetHostName(szCurrentUrl))
						goto error;
				}
				pszCurrentUrl = szCurrentUrl;
				nUrlSize = 0;
				break;
			}
			else
			{
				if (ch == '[' && bGotScheme && !bGotHostName)
					bInsideSquareBrackets = true;
				else if (ch == ']')
					bInsideSquareBrackets = false;

				*pszCurrentUrl++ = ch;
				url++;
				nUrlSize++;
			}
		}

		if (!bGotScheme)
			goto error;

		//Now build the path
		while ((ch = *url) != '\0')
		{
			if (nUrlSize >= CUri::MAX_URL_LENGTH + 5)
				goto error;

			//break on a '#' or a '?', which delimit "extra information"
			if (m_scheme != CUri::SCHEME_FILE && (ch == '#' || ch == '?'))
			{
				break;
			}
			*pszCurrentUrl++ = ch;
			nUrlSize++;
			url++;
		}
		*pszCurrentUrl = '\0';

		if (*szCurrentUrl != '\0' && !SetUrlPath(szCurrentUrl))
			goto error;

		pszCurrentUrl = szCurrentUrl;
		nUrlSize = 0;

		while ((ch = *url++) != '\0')
		{
			if (nUrlSize >= CUri::MAX_URL_LENGTH + 5)
				goto error;

			*pszCurrentUrl++ = ch;
			nUrlSize++;
		}

		*pszCurrentUrl = '\0';
		if (*szCurrentUrl != '\0' && !SetExtraInfo(szCurrentUrl))
			goto error;

		switch (m_scheme)
		{
		case CUri::SCHEME_FILE:
		case CUri::SCHEME_NEWS:
		case CUri::SCHEME_MAILTO:
			m_portNumber = CUri::INVALID_PORT_NUMBER;
			break;
		default:
			if (!bGotPortNumber)
				m_portNumber = (unsigned short)GetDefaultUrlPort(m_scheme);
		}

		return true;

	error:
		CUri::InitFields();
		return false;
	}
	void CUri::CopyFields(const Proud::CUri& url)
	{
		_tcscpy_s(m_schemeName, CUri::MAX_SCHEME_LENGTH + 1, url.m_schemeName);
		_tcscpy_s(m_hostName, CUri::MAX_HOST_NAME_LENGTH + 1, url.m_hostName);
		_tcscpy_s(m_userName, CUri::MAX_USER_NAME_LENGTH + 1, url.m_userName);
		_tcscpy_s(m_password, CUri::MAX_PASSWORD_LENGTH + 1, url.m_password);
		_tcscpy_s(m_urlPath, CUri::MAX_PATH_LENGTH + 1, url.m_urlPath);
		_tcscpy_s(m_extraInfo, CUri::MAX_PATH_LENGTH + 1, url.m_extraInfo);

		m_portNumber = url.m_portNumber;
		m_scheme = url.m_scheme;
		m_schemeNameLength = url.m_schemeNameLength;
		m_hostNameLength = url.m_hostNameLength;
		m_userNameLength = url.m_userNameLength;
		m_passwordLength = url.m_passwordLength;
		m_urlPathLength = url.m_urlPathLength;
		m_extraInfoLength = url.m_extraInfoLength;
	}

	const CUri::_schemeinfo* CUri::GetSchemes()
	{
		const static _schemeinfo s_schemes[] =
		{
			{ _PNT("ftp"), sizeof("ftp") - 1, CUri::DEFAULT_FTP_PORT },
			{ _PNT("gopher"), sizeof("gopher") - 1, CUri::DEFAULT_GOPHER_PORT },
			{ _PNT("http"), sizeof("http") - 1, CUri::DEFAULT_HTTP_PORT },
			{ _PNT("https"), sizeof("https") - 1, CUri::DEFAULT_HTTPS_PORT },
			{ _PNT("file"), sizeof("file") - 1, CUri::INVALID_PORT_NUMBER },
			{ _PNT("news"), sizeof("news") - 1, CUri::INVALID_PORT_NUMBER },
			{ _PNT("mailto"), sizeof("mailto") - 1, CUri::INVALID_PORT_NUMBER },
			{ _PNT("socks"), sizeof("socks") - 1, CUri::DEFAULT_SOCKS_PORT }
		};

		return s_schemes;
	}
	const bool CUri::CrackUrl(const TCHAR* const url, const uint32_t flags)
	{
		assert(url != NULL);
		assert((flags == 0) || (flags == CUri::ESCAPE) || (flags == CUri::DECODE) || (flags == (CUri::DECODE | CUri::ESCAPE)));

		CUri::InitFields();

		bool bRet = CUri::Parse(url);

		if ((flags & CUri::ESCAPE) || (flags & CUri::DECODE))
		{
			if (bRet && (m_userNameLength > 0))
			{
				bRet = UnescapeUrl(m_userName, m_userName, &m_userNameLength, CUri::MAX_USER_NAME_LENGTH + 1);
				if (bRet)
				{
					--m_userNameLength;
				}
			}

			if (bRet && (m_passwordLength > 0))
			{
				bRet = UnescapeUrl(m_password, m_password, &m_passwordLength, CUri::MAX_PASSWORD_LENGTH + 1);
				if (bRet)
				{
					--m_passwordLength;
				}
			}

			if (bRet && (m_hostNameLength > 0))
			{
				bRet = UnescapeUrl(m_hostName, m_hostName, &m_hostNameLength, CUri::MAX_HOST_NAME_LENGTH + 1);
				if (bRet)
				{
					--m_hostNameLength;
				}
			}

			if (bRet && (m_urlPathLength > 0))
			{
				bRet = UnescapeUrl(m_urlPath, m_urlPath, &m_urlPathLength, CUri::MAX_PATH_LENGTH + 1);
				if (bRet)
				{
					--m_urlPathLength;
				}
			}

			if (bRet && (m_extraInfoLength > 0))
			{
				bRet = UnescapeUrl(m_extraInfo, m_extraInfo, &m_extraInfoLength, CUri::MAX_PATH_LENGTH + 1);
				if (bRet)
				{
					--m_extraInfoLength;
				}
			}
		}
		return bRet;
	}
	const bool CUri::SetScheme(CUri::SCHEME scheme)
	{
		if ((scheme < 0) || (scheme >= CUri::MAX_SCHEME_SIZE))
		{
			// invalid scheme
			return false;
		}

		const CUri::_schemeinfo* schemes = GetSchemes();
		if (schemes == NULL)
			return false;

		m_scheme = (CUri::SCHEME)scheme;
		m_schemeNameLength = schemes[scheme].schemeLength;
		m_portNumber = (uint16_t)schemes[scheme].urlPort;

#if (_MSC_VER>=1400)
		_tcscpy_s(m_schemeName, CUri::MAX_SCHEME_LENGTH + 1, schemes[scheme].schemeName);
#else
		Tstrcpy(m_schemeName, schemes[scheme].schemeName);
#endif


		return true;
	}
	const bool CUri::SetSchemeName(const TCHAR* schemeName)
	{
		assert(schemeName != NULL);

		const _schemeinfo *pSchemes = GetSchemes();

		assert(pSchemes != NULL);

		int nScheme = -1;

		for (int i = 0; i < CUri::MAX_SCHEME_SIZE; i++)
		{
			if (pSchemes[i].schemeName && _tcsicmp(schemeName, pSchemes[i].schemeName) == 0)
			{
				nScheme = i;
				break;
			}
		}

		if (nScheme != -1)
		{
			m_scheme = (CUri::SCHEME)nScheme;
			m_schemeNameLength = pSchemes[nScheme].schemeLength;
			m_portNumber = (uint16_t)pSchemes[nScheme].urlPort;
		}
		else
		{
			// unknown scheme
			m_scheme = CUri::SCHEME_UNKNOWN;
			m_schemeNameLength = (uint32_t)_tcslen(schemeName);
			if (m_schemeNameLength > CUri::MAX_SCHEME_LENGTH)
			{
				// scheme name too long
				return false;
			}

			m_portNumber = CUri::INVALID_PORT_NUMBER;
		}

		::_tcsncpy_s(m_schemeName, CUri::MAX_SCHEME_LENGTH + 1, schemeName, m_schemeNameLength);
		m_schemeName[m_schemeNameLength] = '\0';

		return true;
	}
	const bool CUri::SetHostName(const TCHAR* hostName)
	{
		assert(hostName != NULL);

		uint32_t dwLen = (uint32_t)Tstrlen(hostName);
		if (dwLen > CUri::MAX_HOST_NAME_LENGTH)
			return false;

		::_tcsncpy_s(m_hostName, CUri::MAX_HOST_NAME_LENGTH + 1, hostName, dwLen);
		m_hostNameLength = dwLen;

		return true;
	}
	const bool CUri::SetUserName(const TCHAR* userName)
	{
		assert(userName != NULL);

		uint32_t dwLen = (uint32_t)Tstrlen(userName);
		if (dwLen > CUri::MAX_USER_NAME_LENGTH)
			return false;

		::_tcsncpy_s(m_userName, CUri::MAX_USER_NAME_LENGTH + 1, userName, dwLen);
		m_userNameLength = dwLen;

		return true;
	}
	const bool CUri::SetPassword(const TCHAR* password)
	{
		assert(password != NULL);

		if (*password && !*m_userName)
			return false;

		uint32_t dwLen = (uint32_t)Tstrlen(password);
		if (dwLen > CUri::MAX_PASSWORD_LENGTH)
			return false;

		::_tcsncpy_s(m_password, CUri::MAX_PASSWORD_LENGTH + 1, password, dwLen);
		m_passwordLength = dwLen;

		return true;
	}
	const bool CUri::SetUrlPath(const TCHAR* urlPath)
	{
		assert(urlPath != NULL);

		uint32_t dwLen = (uint32_t)Tstrlen(urlPath);
		if (dwLen > CUri::MAX_PATH_LENGTH)
			return false;

		::_tcsncpy_s(m_urlPath, CUri::MAX_PATH_LENGTH + 1, urlPath, dwLen);
		m_urlPathLength = dwLen;

		return true;
	}
	const bool CUri::SetExtraInfo(const TCHAR* extraInfo)
	{
		assert(extraInfo != NULL);

		uint32_t dwLen = (uint32_t)Tstrlen(extraInfo);
		if (dwLen > CUri::MAX_PATH_LENGTH)
			return false;

		::_tcsncpy_s(m_extraInfo, CUri::MAX_PATH_LENGTH + 1, extraInfo, dwLen);
		m_extraInfoLength = dwLen;

		return true;
	}
	const bool CUri::Canonicalize(uint32_t dwFlags)
	{
		::_tcslwr_s(m_schemeName, CUri::MAX_SCHEME_LENGTH + 1);
		TCHAR szTmp[CUri::MAX_URL_LENGTH];
		::_tcscpy_s(szTmp, CUri::MAX_URL_LENGTH, m_schemeName);

		bool bRet = Proud::EscapeUrl(szTmp, m_userName, &m_userNameLength, CUri::MAX_USER_NAME_LENGTH, dwFlags);
		if (bRet)
		{
			m_userNameLength--;
			::_tcscpy_s(szTmp, CUri::MAX_URL_LENGTH, m_password);
			bRet = Proud::EscapeUrl(szTmp, m_password, &m_passwordLength, CUri::MAX_PASSWORD_LENGTH, dwFlags);
		}
		if (bRet)
		{
			m_passwordLength--;
			::_tcscpy_s(szTmp, CUri::MAX_URL_LENGTH, m_hostName);
			bRet = Proud::EscapeUrl(szTmp, m_hostName, &m_hostNameLength, CUri::MAX_HOST_NAME_LENGTH, dwFlags);
		}
		if (bRet)
		{
			m_hostNameLength--;
			::_tcscpy_s(szTmp, CUri::MAX_URL_LENGTH, m_urlPath);
			bRet = Proud::EscapeUrl(szTmp, m_urlPath, &m_urlPathLength, CUri::MAX_PATH_LENGTH, dwFlags);
			if (bRet)
				m_urlPathLength--;
		}


		//in ATL_URL_BROWSER mode, the portion of the URI following the '?' or '#' is not encoded
		if (bRet && (dwFlags & CUri::BROWSER_MODE) == 0 && m_extraInfo != NULL && m_extraInfo[0] != 0)
		{
			::_tcscpy_s(szTmp, CUri::MAX_URL_LENGTH, m_extraInfo);
			bRet = Proud::EscapeUrl(szTmp + 1, m_extraInfo + 1, &m_extraInfoLength, CUri::MAX_PATH_LENGTH - 1, dwFlags);
		}

		return bRet;
	}
	const uint16_t GetDefaultUrlPort(CUri::SCHEME scheme)
	{
		switch (scheme)
		{
		case CUri::SCHEME_FTP:
			return CUri::DEFAULT_FTP_PORT;
		case CUri::SCHEME_GOPHER:
			return CUri::DEFAULT_GOPHER_PORT;
		case CUri::SCHEME_HTTP:
			return CUri::DEFAULT_HTTP_PORT;
		case CUri::SCHEME_HTTPS:
			return CUri::DEFAULT_HTTPS_PORT;
		case CUri::SCHEME_SOCKS:
			return CUri::DEFAULT_SOCKS_PORT;
		default:
			return CUri::INVALID_PORT_NUMBER;
		}
	}
	inline short HexValue(char chIn)
	{
		unsigned char ch = (unsigned char)chIn;
		if (ch >= '0' && ch <= '9')
			return (short)(ch - '0');
		if (ch >= 'A' && ch <= 'F')
			return (short)(ch - 'A' + 10);
		if (ch >= 'a' && ch <= 'f')
			return (short)(ch - 'a' + 10);
		return -1;
	}
	inline bool IsUnsafeUrlChar(char chIn)
	{
		unsigned char ch = (unsigned char)chIn;
		switch (ch)
		{
		case ';': case '\\': case '?': case '@': case '&':
		case '=': case '+': case '$': case ',': case ' ':
		case '<': case '>': case '#': case '%': case '\"':
		case '{': case '}': case '|':
		case '^': case '[': case ']': case '`':
			return true;
		default:
		{
			if (ch < 32 || ch > 126)
				return true;
			return false;
		}
		}
	}
	const bool UnescapeUrl(const wchar_t* stringIn, wchar_t* stringOut, uint32_t* pStringLength, uint32_t maxLength)
	{
		assert(stringIn != NULL);
		assert(stringOut != NULL);
		/// convert to UTF8
		bool bRet = false;

		int nSrcLen = ::wcslen(stringIn);
		int nCnt = ::WideCharToMultiByte(CP_UTF8, 0, stringIn, nSrcLen, NULL, 0, NULL, NULL);
		if (nCnt == 0)
			return false;

		nCnt++;
		RefCount<char> szIn;

		char szInBuf[CUri::MAX_URL_LENGTH];
		char *pszIn = szInBuf;

		if (nCnt <= 0)
			return false;

		// try to avoid allocation
		if (nCnt > CUri::MAX_URL_LENGTH)
		{
			szIn = RefCount<char>(new char[nCnt]);
			if (!szIn)
			{
				// out of memory
				return false;
			}
			pszIn = szIn;
		}

		nCnt = ::WideCharToMultiByte(CP_UTF8, 0, stringIn, nSrcLen, pszIn, nCnt, NULL, NULL);
		assert(nCnt != 0);

		pszIn[nCnt] = '\0';

		char szOutBuf[CUri::MAX_URL_LENGTH];
		char *pszOut = szOutBuf;
		RefCount<char> szTmp;

		// try to avoid allocation
		if (maxLength > CUri::MAX_URL_LENGTH)
		{
			szTmp = RefCount<char>(new char[maxLength]);
			if (!szTmp)
			{
				// out of memory
				return false;
			}
			pszOut = szTmp;
		}

		uint32_t stringLength = 0;
		bRet = UnescapeUrl(pszIn, pszOut, &stringLength, maxLength);
		if (bRet != false)
		{
			// it is now safe to convert using any codepage, since there
			// are no non-ASCII characters
			try
			{
				::wmemcpy_s(stringOut, maxLength, StringA2W(pszOut).GetString(), stringLength);
			}
			__pragma(warning(push)) __pragma(warning(disable: 4571)) catch (...) __pragma(warning(pop))
			{
				bRet = false;
			}
		}
		if (pStringLength)
		{
			*pStringLength = stringLength;
		}

		return bRet;
	}
	const bool EscapeUrl(const wchar_t* stringIn, wchar_t* stringOut, uint32_t* pStringLength, uint32_t maxLength, uint32_t flags)
	{
		assert(stringIn != NULL);
		assert(stringOut != NULL);
		// convert to UTF8
		bool bRet = false;

		int nSrcLen = ::wcslen(stringIn);
		if (nSrcLen == 0) // handle the case of an empty string
		{
			if (pStringLength != NULL)
			{
				*pStringLength = 1; //one for null
			}
			*stringOut = '\0';
			return true;
		}
		int nCnt = WideCharToMultiByte(CP_UTF8, 0, stringIn, nSrcLen, NULL, 0, NULL, NULL);
		if (nCnt != 0)
		{
			nCnt++;
			RefCount<char> szIn;

			char szInBuf[CUri::MAX_URL_LENGTH];
			char *pszIn = szInBuf;

			// try to avoid allocation
			if (nCnt <= 0)
			{
				return false;
			}

			if (nCnt > CUri::MAX_URL_LENGTH)
			{
				szIn = RefCount<char>(new char[nCnt]);
				if (!szIn)
				{
					// out of memory
					return false;
				}
				pszIn = szIn;
			}

			nCnt = ::WideCharToMultiByte(CP_UTF8, 0, stringIn, nSrcLen, pszIn, nCnt, NULL, NULL);
			assert(nCnt != 0);

			pszIn[nCnt] = '\0';

			char szOutBuf[CUri::MAX_URL_LENGTH];
			char *pszOut = szOutBuf;
			RefCount<char> szTmp;

			// try to avoid allocation
			if (maxLength > CUri::MAX_URL_LENGTH)
			{
				szTmp = RefCount<char>(new char[maxLength]);
				if (!szTmp)
				{
					// out of memory
					return false;
				}
				pszOut = szTmp;
			}

			uint32_t stringLength = 0;
			bRet = Proud::EscapeUrl(pszIn, pszOut, &stringLength, maxLength, flags);
			if (bRet != false)
			{
				// it is now safe to convert using any codepage, since there
				// are no non-ASCII characters
				try
				{
					::wmemcpy_s(stringOut, maxLength, StringA2W(pszOut).GetString(), stringLength);
				}
				__pragma(warning(push)) __pragma(warning(disable: 4571)) catch (...) __pragma(warning(pop))
				{
					bRet = false;
				}
			}
			if (pStringLength)
			{
				*pStringLength = stringLength;
			}
		}

		return bRet;
	}
	const bool EscapeUrl(const char* stringIn, char* stringOut, uint32_t* pStringLength, uint32_t maxLength, uint32_t flags)
	{
		assert(stringIn != NULL);
		assert(stringOut != NULL);
		assert(stringIn != stringOut);

		char ch;
		uint32_t dwLen = 0;
		bool bRet = true;
		bool bSchemeFile = false;
		uint32_t dwColonPos = 0;
		uint32_t dwFlagsInternal = flags;
		//The next 2 are for buffer security checks
		char* szOrigStringOut = stringOut;
		_pn_unused(szOrigStringOut);
		char* szStringOutEnd = (stringOut + maxLength);

		while ((ch = *stringIn++) != '\0')
		{
			//if we are at the maximum length, set bRet to false
			//this ensures no more data is written to szStringOut, but
			//the length of the string is still updated, so the user
			//knows how much space to allocate
			if (dwLen == maxLength)
			{
				bRet = false;
			}

			//Keep track of the first ':' position to match the weird way
			//InternetCanonicalizeUrl handles it
			if (ch == ':' && (dwFlagsInternal & CUri::CANONICALIZE) && !dwColonPos)
			{
				if (bRet)
				{
					*stringOut = '\0';
					LPSTR pszStrToLower = stringOut - dwLen;
					assert(pszStrToLower >= szOrigStringOut &&  pszStrToLower <= szStringOutEnd);
					::_strlwr_s(pszStrToLower, szStringOutEnd - pszStrToLower + 1);

					if (dwLen == 4 && !strncmp("file", (stringOut - 4), 4))
					{
						bSchemeFile = true;
					}
				}

				dwColonPos = dwLen + 1;
			}
			else if (ch == '%' && (dwFlagsInternal & CUri::DECODE))
			{
				//decode the escaped sequence
				if (*stringIn != '\0')
				{
					short nFirstDigit = Proud::HexValue(*stringIn++);

					if (nFirstDigit < 0)
					{
						bRet = false;
						break;
					}
					ch = static_cast<char>(16 * nFirstDigit);
					if (*stringIn != '\0')
					{
						short nSecondDigit = Proud::HexValue(*stringIn++);

						if (nSecondDigit < 0)
						{
							bRet = false;
							break;
						}
						ch = static_cast<char>(ch + nSecondDigit);
					}
					else
					{
						break;
					}
				}
				else
				{
					break;
				}
			}
			else if ((ch == '?' || ch == '#') && (dwFlagsInternal & CUri::BROWSER_MODE))
			{
				//ATL_URL_BROWSER mode does not encode after a '?' or a '#'
				dwFlagsInternal |= CUri::NO_ENCODE;
			}

			if ((dwFlagsInternal & CUri::CANONICALIZE) && (dwFlagsInternal & CUri::NO_ENCODE) == 0)
			{
				//canonicalize the '\' to '/'
				if (ch == '\\' && (dwColonPos || (dwFlagsInternal & CUri::COMBINE)) && bRet)
				{
					//if the scheme is not file or it is file and the '\' is in "file:\\"
					//NOTE: This is to match the way InternetCanonicalizeUrl handles this case
					if (!bSchemeFile || (dwLen < 7))
					{
						ch = '/';
					}
				}
				else if (ch == '.' && dwLen > 0 && (dwFlagsInternal & CUri::NO_META) == 0)
				{
					//if we are escaping meta sequences, attempt to do so
					if (Proud::EscapeUrlMetaHelper(&stringOut, stringOut - 1, dwLen, (char**)(&stringIn), &dwLen, dwFlagsInternal, dwColonPos))
						continue;
				}
			}

			//if we are encoding and it is an unsafe character
			if (Proud::IsUnsafeUrlChar(ch) && (dwFlagsInternal & CUri::NO_ENCODE) == 0)
			{
				//if we are only encoding spaces, and ch is not a space or
				//if we are not encoding meta sequences and it is a dot or
				//if we not encoding percents and it is a percent
				if (((dwFlagsInternal & CUri::ENCODE_SPACES_ONLY) && ch != ' ') ||
					((dwFlagsInternal & CUri::NO_META) && ch == '.') ||
					(((dwFlagsInternal & CUri::ENCODE_PERCENT) == 0) && ch == '%'))
				{
					//just output it without encoding
					if (bRet)
						*stringOut++ = ch;
				}
				else
				{
					//if there is not enough space for the escape sequence
					if (dwLen >= (maxLength - 3))
					{
						bRet = false;
					}
					if (bRet)
					{
						//output the percent, followed by the hex value of the character
						LPSTR pszTmp = stringOut;
						*pszTmp++ = '%';
						if ((unsigned char)ch < 16)
						{
							*pszTmp++ = '0';
						}
						::_ultoa_s((unsigned char)ch, pszTmp, szStringOutEnd - pszTmp, 16);
						stringOut += sizeof("%FF") - 1;
					}
					dwLen += sizeof("%FF") - 2;
				}
			}
			else //safe character
			{
				if (bRet)
					*stringOut++ = ch;
			}
			dwLen++;
		}

		if (bRet && dwLen < maxLength)
			*stringOut = '\0';

		if (pStringLength)
			*pStringLength = dwLen + 1;

		if (dwLen + 1 > maxLength)
			bRet = false;

		return bRet;
	}
	const bool EscapeUrlMetaHelper(char** ppszOutUrl, const char* szPrev, uint32_t outLength, char** ppszInUrl, uint32_t* length, uint32_t flags, uint32_t colonPos)
	{
		assert(ppszOutUrl != NULL);
		assert(szPrev != NULL);
		assert(ppszInUrl != NULL);
		assert(length != NULL);

		char* szOut = *ppszOutUrl;
		char* szIn = *ppszInUrl;
		uint32_t dwUrlLen = outLength;
		char chPrev = *szPrev;
		bool bRet = false;

		//if the previous character is a directory delimiter
		if (chPrev == '/' || chPrev == '\\')
		{
			char chNext = *szIn;

			//if the next character is a directory delimiter
			if (chNext == '/' || chNext == '\\')
			{
				//the meta sequence is of the form /./*
				szIn++;
				bRet = true;
			}
			else if (chNext == '.' && ((chNext = *(szIn + 1)) == '/' ||
				chNext == '\\' || chNext == '\0'))
			{
				//otherwise if the meta sequence is of the form "/../"
				//skip the preceding "/"
				szOut--;

				//skip the ".." of the meta sequence
				szIn += 2;
				uint32_t dwOutPos = dwUrlLen - 1;
				LPSTR szTmp = szOut;

				//while we are not at the beginning of the base url
				while (dwOutPos)
				{
					szTmp--;
					dwOutPos--;

					//if it is a directory delimiter
					if (*szTmp == '/' || *szTmp == '\\')
					{
						//if we are canonicalizing the url and NOT combining it
						//and if we have encountered the ':' or we are at a position before the ':'
						if ((flags & CUri::CANONICALIZE) && ((flags & CUri::COMBINE) == 0) &&
							(colonPos && (dwOutPos <= colonPos + 1)))
						{
							//NOTE: this is to match the way that InternetCanonicalizeUrl and
							//		InternetCombineUrl handle this case
							break;
						}

						//otherwise, set the current output string position to right after the '/'
						szOut = szTmp + 1;

						//update the length to match
						dwUrlLen = dwOutPos + 1;
						bRet = true;
						break;
					}
				}

				//if we could not properly escape the meta sequence
				if (dwUrlLen != dwOutPos + 1)
				{
					//restore everything to its original value
					szIn -= 2;
					szOut++;
				}
				else
				{
					bRet = true;
				}
			}
		}
		//update the strings
		*ppszOutUrl = szOut;
		*ppszInUrl = szIn;
		*length = dwUrlLen;
		return bRet;
	}
	const bool UnescapeUrl(const char* stringIn, char* stringOut, uint32_t* pStringLength, const uint32_t maxLength)
	{
		assert(stringIn != NULL);
		assert(stringOut != NULL);

		char ch;
		uint32_t dwLen = 0;

		while ((ch = *stringIn) != 0)
		{
			if (ch == '%')
			{
				if ((*(stringIn + 1) == '\0') || (*(stringIn + 2) == '\0'))
				{
					// '%' sequence incomplete, set the output buffer size to
					// what we've counted so far, but it would not reflect the real
					// size requirements
					if (pStringLength)
						*pStringLength = dwLen + 1;
					return false;
				}

				// Per RFC2396, two hex values follow the '%'.
				short nFirstDigit = Proud::HexValue(*(++stringIn));
				short nSecondDigit = Proud::HexValue(*(++stringIn));

				if (nFirstDigit < 0 || nSecondDigit < 0 || (nFirstDigit == 0 && nSecondDigit == 0))
				{
					// Unexpected sequence after '%', or NUL encountered. Set the output buffer size to
					// what we've counted so far, but it would not reflect the real size requirements.
					if (pStringLength)
						*pStringLength = dwLen + 1;
					return false;
				}

				// If the buffer is not large enough, use the loop to compute the size requirements
				if (dwLen < maxLength)
					*stringOut++ = static_cast<char>(((16 * nFirstDigit) + nSecondDigit));
			}
			else
			{
				// Non-escaped character
				// If the buffer is not large enough, use the loop to compute the size requirements
				if (dwLen < maxLength)
					*stringOut++ = ch;
			}

			dwLen++;
			stringIn++;
		}

		// Only own setting null termination when succeeded
		if (dwLen < maxLength)
			*stringOut = '\0';

		// Size requirements, plus one byte for null termination
		if (pStringLength)
			*pStringLength = dwLen + 1;

		if (dwLen + 1 > maxLength)
			return false;

		return true;
	}
#endif
}
