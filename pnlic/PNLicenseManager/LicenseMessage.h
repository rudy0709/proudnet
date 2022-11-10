#pragma once

#include "PNTypes.h"

enum LicenseMessageType
{
	LMType_RegSuccess,                 //인증 성공
	LMType_UnIssuedKey,                //발급된 키가 아님
	LMType_StoppedProject,             //프로젝트가 중지됨
	LMType_DisabledMachine,            //해당 머신에 라이선스키를 등록할 수 없습니다.
	LMType_DisabledKey,                //사용할 수 없는 키입니다.
	LMType_ExpiredDateKey,             //만료된 키입니다.
	LMType_Mismatch,                  //라이선스 키와 설치 파일의 플랫폼이 다릅니다.
	LMType_LicenseInstallError,        //머신에 키 등록 실패.
	LMType_AuthServerConnectionFailed, //인증서버에 접속할 수 없음.
	LMType_Unexpected,                 //비정상적인 발생 
	LMType_Incorrect,                  //잘못 입력
	LMType_NotRegist,                   //등록되지 않은 PC
	LMType_NowAuthorizing,              //인증 중...
	LMType_Nothing
};

const PNTCHAR* TypeToString_Kor(LicenseMessageType type);
const PNTCHAR* TypeToString_Eng(LicenseMessageType type);
const PNTCHAR* GetLicenseMessageString(short langid, LicenseMessageType type);
