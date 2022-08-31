#pragma once

/*
* This class is cloned from ATL::CUrl, for resolving compatibility problem with Unreal Engine 4.
*/

#include "../include/pnstdint.h"
#include "../include/pntchar.h"

#if defined (WIN32)
#include <tchar.h>
#endif //defined (WIN32)

namespace Proud
{
#if defined(_WIN32)
	class CUri // Unique Resource Identifier
	{
	public:
		enum
		{ // The same values with defined on atlutil.h/CUrl
			INVALID_PORT_NUMBER = 0,
			DEFAULT_FTP_PORT = 21,			// default for FTP servers
			DEFAULT_GOPHER_PORT = 70,		//	"     "  gopher "
			DEFAULT_HTTP_PORT = 80,			//	"     "  HTTP   "
			DEFAULT_HTTPS_PORT = 443,		//	"     "  HTTPS  "
			DEFAULT_SOCKS_PORT = 1080,		// default for SOCKS firewall servers.

			MAX_HOST_NAME_LENGTH = 256,
			MAX_USER_NAME_LENGTH = 128,
			MAX_PASSWORD_LENGTH = 128,
			MAX_PORT_NUMBER_LENGTH = 5,			// ATL_URL_PORT is unsigned short
			MAX_PORT_NUMBER_VALUE = 65535,		// maximum unsigned short value
			MAX_PATH_LENGTH = 2048,
			MAX_SCHEME_LENGTH = 32,				// longest protocol name length
			MAX_SCHEME_SIZE = 8,
			MAX_URL_LENGTH = (MAX_SCHEME_LENGTH + sizeof("://") + MAX_PATH_LENGTH),
		};
		enum SCHEME
		{
			SCHEME_UNKNOWN = -1,
			SCHEME_FTP = 0,
			SCHEME_GOPHER = 1,
			SCHEME_HTTP = 2,
			SCHEME_HTTPS = 3,
			SCHEME_FILE = 4,
			SCHEME_NEWS = 5,
			SCHEME_MAILTO = 6,
			SCHEME_SOCKS = 7,
		};
		enum
		{ // atlutils.h/ //flags.
			ESCAPE = 1,   // (un)escape URI characters
			NO_ENCODE = 2,   // Don't convert unsafe characters to escape sequence
			DECODE = 4,   // Convert %XX escape sequences to characters
			NO_META = 8,   // Don't convert .. etc. meta path sequences
			ENCODE_SPACES_ONLY = 16,  // Encode spaces only
			BROWSER_MODE = 32,  // Special encode/decode rules for browser
			ENCODE_PERCENT = 64,  // Encode percent (by default, not encoded)
			CANONICALIZE = 128, // Internal: used by Canonicalize for AtlEscapeUrl: Cannot be set via SetFlags
			COMBINE = 256, // Internal: Cannot be set via SetFlags
		};

		struct _schemeinfo
		{
			const PNTCHAR* schemeName;
			uint32_t schemeLength;
			uint16_t urlPort;
		};

	public:
		inline uint16_t GetPortNumber() const
		{
			return m_portNumber;
		}
		inline const PNTCHAR* const GetHostName() const
		{
			return m_hostName;
		}
		inline const PNTCHAR* const GetUrlPath() const
		{
			return m_urlPath;
		}

		const bool CrackUrl(const PNTCHAR* const url, const uint32_t dwFlags = 0);
		const bool SetScheme(CUri::SCHEME scheme);
		const bool SetSchemeName(const PNTCHAR* schemeName);
		const bool SetHostName(const PNTCHAR* hostName);
		const bool SetUserName(const PNTCHAR* userName);
		const bool SetPassword(const PNTCHAR* password);
		const bool SetUrlPath(const PNTCHAR* urlPath);
		const bool SetExtraInfo(const PNTCHAR* extraInfo);
		const bool Canonicalize(uint32_t dwFlags = 0);

	public:
		void InitFields();
		void CopyFields(const Proud::CUri& url);
		const bool Parse(const PNTCHAR* url);
		const CUri::_schemeinfo* GetSchemes();

	public:
		CUri();
		CUri(const CUri& url);
		~CUri();

		CUri& operator=(const CUri& url);

	private:
		PNTCHAR m_schemeName[CUri::MAX_SCHEME_LENGTH + 1]; //scheme names cannot contain escape/unsafe characters
		PNTCHAR m_hostName[CUri::MAX_HOST_NAME_LENGTH + 1]; //host names cannot contain escape/unsafe characters
		PNTCHAR m_userName[CUri::MAX_USER_NAME_LENGTH + 1];
		PNTCHAR m_password[CUri::MAX_PASSWORD_LENGTH + 1];
		PNTCHAR m_urlPath[CUri::MAX_PATH_LENGTH + 1];
		PNTCHAR m_extraInfo[CUri::MAX_PATH_LENGTH + 1];

		uint16_t m_portNumber;
		CUri::SCHEME m_scheme;

		uint32_t m_schemeNameLength;
		uint32_t m_hostNameLength;
		uint32_t m_userNameLength;
		uint32_t m_passwordLength;
		uint32_t m_urlPathLength;
		uint32_t m_extraInfoLength;
	};
	const uint16_t GetDefaultUrlPort(CUri::SCHEME scheme);
	const bool UnescapeUrl(const wchar_t* stringIn, wchar_t* stringOut, uint32_t* pStringLength, uint32_t maxLength);
	const bool UnescapeUrl(const char* stringIn, char* stringOut, uint32_t* pStringLength, uint32_t maxLength);
	const bool EscapeUrl(const wchar_t* stringIn, wchar_t* stringOut, uint32_t* pStringLength, uint32_t maxLength, uint32_t flags = 0);
	const bool EscapeUrl(const char* stringIn, char* stringOut, uint32_t* pStringLength, uint32_t maxLength, uint32_t flags = 0);
	const bool EscapeUrlMetaHelper(char** ppszOutUrl, const char* szPrev, uint32_t outLength, char** ppszInUrl, uint32_t* length, uint32_t flags = 0, uint32_t colonPos = CUri::MAX_URL_LENGTH);

#endif
}
