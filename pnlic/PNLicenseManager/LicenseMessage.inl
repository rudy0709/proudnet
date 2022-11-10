#include "LicenseMessage.h"

const PNTCHAR* TypeToString_Kor(LicenseMessageType type)
{
	switch (type)
	{
	case LMType_RegSuccess:
		return _PNT("라이선스 키가 성공적으로 시스템에 등록되었습니다.");

	case LMType_UnIssuedKey:
	case LMType_StoppedProject:
	case LMType_DisabledMachine:
	case LMType_DisabledKey:
	case LMType_ExpiredDateKey:
		return _PNT("사용할 수 없는 라이선스 키입니다.\nhttp://ndn.nettention.com/ 에 문의해 주십시오.");
	case LMType_Mismatch:
		return _PNT("라이선스 키와 설치 프로그램의 라이선스 타입이 다릅니다.\nhttp://ndn.nettention.com/ 에 문의해 주십시오.");
	case LMType_LicenseInstallError:
		return _PNT("라이선스 키를 설치하는 데 실패했습니다.\nhttp://ndn.nettention.com/ 에 문의해 주십시오.");
	case LMType_AuthServerConnectionFailed:
		return _PNT("인증서버에 접속할 수 없습니다.\n인터넷 상태를 확인하신 후 문제가 없으시면\nhttp://ndn.nettention.com/ 에 문의해주십시오");
	case LMType_Unexpected:
		return _PNT("예상치 못한 오류로 인증이 실패하였습니다.\nhttp://ndn.nettention.com/ 에 문의해주십시오.");
	case LMType_Incorrect:
		return _PNT("라이선스 키가 잘못 입력되었습니다.");
	case LMType_NotRegist:
		return _PNT("인증된 PC가 아닙니다.\n프라우드넷 사용을 위한 인증이 필요합니다.\n이 메시지가 계속 나온다면,\nhttp://ndn.nettention.com 에 문의하여 주십시오");
	case LMType_NowAuthorizing:
		return _PNT("인증 요청 중입니다. 종료시 인증이 완료되지 않습니다.\n종료 하시겠습니까?");
	}

	return _PNT("<none>");
}

const PNTCHAR* TypeToString_Eng(LicenseMessageType type)
{
	switch (type)
	{
	case LMType_RegSuccess:
		return _PNT(" The normal license key has been registered in the system.");


	case LMType_UnIssuedKey:
	case LMType_StoppedProject:
	case LMType_DisabledMachine:
	case LMType_DisabledKey:
	case LMType_ExpiredDateKey:
		return _PNT("This is an unavailable license key.\nPlease make inquiries at http://ndn.nettention.com/.");

	case LMType_Mismatch:
		return _PNT("Installed program license type and license key are incorrect or different. \nPlease make inquiries at http://ndn.nettention.com/.");
	case LMType_LicenseInstallError:
		return _PNT("Failed to install the license key in the system.\nPlease make inquiries at http://ndn.nettention.com/.");
	case LMType_AuthServerConnectionFailed:
		return _PNT("Unable to connect to the authentication server.\nIf there is no problem with internet/network access,\nPlease make inquiries at http://ndn.nettention.com/");
	case LMType_Unexpected:
		return _PNT("Authentication has failed due to the unexpected error.\nPlease make inquiries at http://ndn.nettention.com/.");
	case LMType_Incorrect:
		return _PNT("Entered license key is unavailable.");
	case LMType_NotRegist:
		return _PNT("This PC is not an authenticated PC.\nAuthentication is required in order to use the ProudNet .\nIf this message appears constantly,\nplease make an inquiry at http://ndn.nettention.com.");
	case LMType_NowAuthorizing:
		return _PNT("The authentication is in progress.\nAuthentication cannot be completed if you exit.\nConfirm to exit?");
	}

	return _PNT("<none>");
}

const PNTCHAR* GetLicenseMessageString(short langid, LicenseMessageType type)
{
	if (langid == 0x12) //LANG_KOREAN
		return TypeToString_Kor(type);

	return TypeToString_Eng(type);
}
