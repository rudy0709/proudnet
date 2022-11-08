#pragma once

#if defined(__linux__)
#else
#include <windows.h>
#include <lmcons.h>
#include <assert.h>
#include <list>
#include <memory>
#include <string>

using namespace std;

const int MAX_OWERNAME_SIZE = 256;

class CPNFileSecureAttribute
{
	enum ACE_TYPE
	{
		ALLOW, 
		DENY
	};

	class CAclEntry
	{	
	public:
		CAclEntry(ACE_TYPE type, PSID pSid, UINT uiRights) : m_type(type), m_pSid(pSid), m_uiRights(uiRights) {};
		~CAclEntry() {};

	public:
		ACE_TYPE m_type;
		PSID m_pSid;
		UINT m_uiRights;
	};

public:
	CPNFileSecureAttribute();
	~CPNFileSecureAttribute();

	bool Init();
	bool ApplyACL();

	bool Allow(PSID pSid, UINT uiRights = STANDARD_RIGHTS_ALL | GENERIC_ALL);
	bool Deny(PSID pSid, UINT uiRights = STANDARD_RIGHTS_ALL | GENERIC_ALL);

	PSID GetSID(string strUserName = "", string strComputerName = "");
	void SetDACL();
	void SetSACL();
	bool SetOwner(string strOwner = "", string strComputerName = "");
	bool SetGroup(string strGroup, string strComputerName = "");

public:
	LPSECURITY_ATTRIBUTES operator&()	{ return &m_securityAttributes; }

private:
	bool ApplyAceList(ACE_TYPE type);

private:
	SECURITY_ATTRIBUTES m_securityAttributes; // 접근 권한 객체
	SECURITY_DESCRIPTOR m_securityDescriptor; // 보안 기술자
	list<std::shared_ptr<CAclEntry>> m_AceList; // Access Control Entry 리스트
	PSID			m_pExOwnerSid; // 파일 소유자 SID
	PSID			m_pExGroupSid; // 파일 그룹 SID
	PACL			m_pAcl;
	PSID			m_pSid;
	UINT			m_uiAclSize;
};

#endif