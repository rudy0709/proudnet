#include "stdafx.h"

#if defined(__linux__)
#else

#include "PNFileSecureAttribute.h"

CPNFileSecureAttribute::CPNFileSecureAttribute()
{
	m_pAcl = PACL(NULL);
	m_uiAclSize = sizeof(ACL);
	m_pExOwnerSid = PSID(NULL);
	m_pExGroupSid = PSID(NULL);
}

CPNFileSecureAttribute::~CPNFileSecureAttribute()
{
	if (m_pAcl != NULL) { delete m_pAcl; }
	if (m_pExOwnerSid != NULL) { delete m_pExOwnerSid; }
	if (m_pExGroupSid != NULL) { delete m_pExGroupSid; }
}

// Security Descriptor 및 Security Attributes 를 초기화 합니다. 
bool CPNFileSecureAttribute::Init()
{
	memset(&m_securityAttributes, 0, sizeof(SECURITY_ATTRIBUTES));
	m_securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
	InitializeSecurityDescriptor(&m_securityDescriptor, SECURITY_DESCRIPTOR_REVISION);
	return true;
}

// ACL 객체를 생성후 모든 엔트리(사용자 또는 그룹) 접근 권한을 설정합니다.   
bool CPNFileSecureAttribute::ApplyACL()
{
	m_pAcl = (PACL) new BYTE[m_uiAclSize];
	InitializeAcl(m_pAcl, m_uiAclSize, ACL_REVISION_DS);
	ApplyAceList(ALLOW);
	ApplyAceList(DENY);
	return true;
}

// Access Control Entry 타입(Allow, Deny)에 따라 앞서 설정한 엔트리(사용자 또는 그룹) 접근 권한을 ACL 객체에 일괄 설정합니다.   
bool CPNFileSecureAttribute::ApplyAceList(ACE_TYPE type)
{
	list<std::shared_ptr<CAclEntry>>::const_iterator pos;
	std::shared_ptr<CAclEntry> aclEntry;

	for (pos = m_AceList.begin(); pos != m_AceList.end();) {
		aclEntry = *pos++;

		if (aclEntry->m_type == type)
		{
			switch (aclEntry->m_type) 
			{
			case ALLOW:
				AddAccessAllowedAce(m_pAcl, ACL_REVISION_DS, aclEntry->m_uiRights, aclEntry->m_pSid);
				break;

			case DENY:
				AddAccessDeniedAce(m_pAcl, ACL_REVISION_DS, aclEntry->m_uiRights, aclEntry->m_pSid);
				break;
			}
		}
	}

	return true;
}

// 해당 Secure ID를 가진 사용자 또는 그룹에 대한 접근과 권한을 허용합니다. 
bool CPNFileSecureAttribute::Allow(PSID pSid, UINT uiRights)
{
	if (pSid == PSID(NULL) || !IsValidSid(pSid)) {
		return false;
	}
	m_uiAclSize += sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pSid) - sizeof(DWORD);
	std::shared_ptr<CAclEntry> entry(new CAclEntry(ALLOW, pSid, uiRights));
	m_AceList.push_back(entry);
	return true;
}

// 해당 Secure ID를 가진 사용자 또는 그룹에 대한 접근과 권한을 거부합니다. 
bool CPNFileSecureAttribute::Deny(PSID pSid, UINT uiRights)
{
	if (pSid == PSID(NULL) || !IsValidSid(pSid)) {
		return false;
	}
	m_uiAclSize += sizeof(ACCESS_DENIED_ACE) + GetLengthSid(pSid) - sizeof(DWORD);
	std::shared_ptr<CAclEntry> entry(new CAclEntry(DENY, pSid, uiRights));
	m_AceList.push_back(entry);
	return true;
}

// 사용자 또는 그룹에 대한 Secure ID를 얻어 옵니다.
PSID CPNFileSecureAttribute::GetSID(string strUserName, string strComputerName)
{
	string strDomainName;
	DWORD dwSIDSize = 0;
	DWORD dwDomainNameSize = 0;
	SID_NAME_USE snuType = SidTypeUnknown;

	PSID pSid = PSID(NULL);
	LookupAccountNameA(&strComputerName[0], &strUserName[0], NULL, &dwSIDSize, NULL, &dwDomainNameSize, &snuType);

	if (dwSIDSize) {
		//	Success, now allocate our buffers
		pSid = (PSID) new BYTE[dwSIDSize];
		if (!LookupAccountNameA(NULL, &strUserName[0], pSid, &dwSIDSize, &strDomainName[0], &dwDomainNameSize, &snuType)) {
			return PSID(NULL);
		}
	}

	return pSid;
}

// 파일 사용자 또는 그룹에 대한 Access 제어 정보를 갱신합니다. 
void CPNFileSecureAttribute::SetDACL()
{
	SetSecurityDescriptorDacl(&m_securityDescriptor, TRUE, m_pAcl, FALSE);
	m_securityAttributes.lpSecurityDescriptor = &m_securityDescriptor;
}

// 시스템 Access 제어 정보를 갱신합니다. 
void CPNFileSecureAttribute::SetSACL()
{
	SetSecurityDescriptorSacl(&m_securityDescriptor, TRUE, m_pAcl, FALSE);
	m_securityAttributes.lpSecurityDescriptor = &m_securityDescriptor;
}

// 파일 소유자에 대한 정보를 갱신합니다. 
bool CPNFileSecureAttribute::SetOwner(string strOwner, string strComputerName)
{
	DWORD dwLen = UNLEN;
	PSID  pSid = PSID(NULL);
	char stOwner[MAX_OWERNAME_SIZE] = { 0 };

	if (strOwner.length() == 0) {
		if (GetUserNameA(stOwner, &dwLen) == false) {
			return false;
		}

		strOwner = stOwner;
	}

	if ((pSid = GetSID(strOwner, strComputerName)) == PSID(NULL)) {
		return false;
	}

	if (m_pExOwnerSid != NULL) { delete m_pExOwnerSid; };
	m_pExOwnerSid = pSid;
	SetSecurityDescriptorOwner(&m_securityDescriptor, pSid, FALSE);
	return true;
}

// 파일 사용자 그룹에 대한 정보를 갱신합니다. 
bool CPNFileSecureAttribute::SetGroup(string strGroup, string strComputerName)
{
	PSID pSid = PSID(NULL);

	if ((pSid = GetSID(strGroup, strComputerName)) == PSID(NULL)) {
		return false;
	}

	if (m_pExGroupSid != NULL) { delete m_pExGroupSid; };
	m_pExGroupSid = pSid;
	SetSecurityDescriptorGroup(&m_securityDescriptor, pSid, FALSE);
	return true;
}

#endif