
#pragma once

namespace Proud {

    enum LicenseType
    {
        LicenseType_None = 0x00000000 | 0x40000000,
        LicenseType_Flash = 0x00000001 | 0x40000000,
        LicenseType_UIPhone = 0x00000002 | 0x40000000,
        LicenseType_UAndroid = 0x00000004 | 0x40000000,
        LicenseType_UWeb = 0x00000008 | 0x40000000,
        LicenseType_NIPhone = 0x00000010 | 0x40000000,
        LicenseType_NAndroid = 0x0000020 | 0x40000000,
        LicenseType_PC = 0x0000040 | 0x40000000,
        LicenseType_Invalid = 0x0000080 | 0x40000000,

        LicenseType_MobileAll = LicenseType_None | (LicenseType_Flash | LicenseType_UIPhone | LicenseType_UAndroid | LicenseType_UWeb | LicenseType_NIPhone | LicenseType_NAndroid),
        //LicenseType_Full = LicenseType_MobileAll | LicenseType_PC,
        LicenseType_Personal = (0x0000100 | LicenseType_MobileAll | LicenseType_PC),
        LicenseType_Indie = (0x0000200 | LicenseType_MobileAll | LicenseType_PC),
        LicenseType_Pro = (0x0000400 | LicenseType_MobileAll | LicenseType_PC),

        // 주의 : dev004에서 다음과 같이 되어 있다.
        // LicenseType_ProudNet2 = (0x0008000 | LicenseType_MobileAll | LicenseType_PC)

        LicenseType_10CCU = (0x0010000 | LicenseType_MobileAll | LicenseType_PC),
        LicenseType_5CCU = (0x0020000 | LicenseType_MobileAll | LicenseType_PC),
    };
}

// ProudNet License Type
#define FLASHTYPE			_PNT("FLASH")
#define UIPHONETYPE			_PNT("UIPHONE")
#define UANDROIDTYPE		_PNT("UANDROID")
#define UWEBTYPE			_PNT("UWEB")
#define NIPHONETYPE			_PNT("NIPHONE")
#define NANDROIDTYPE		_PNT("NANDROID")

#define PNSRCLICENSE		_PNT("PNSRCLICENSE")
#define PNLIBLICENSE		_PNT("PNLIBLICENSE")
#define PNMOBILELICENSE		_PNT("PNMOBILELICENSE")

// 추가 라이선스 타입
#define PNPROLICENSE		_PNT("PNPROLICENSE")
#define PNINDIELICENSE		_PNT("PNINDIELICENSE")
#define PNPERSONALLICENSE	_PNT("PNPERSONALLICENSE")

// 아쉽게도 C/C++의 macro 이름은 숫자로 시작할 수 없다.
#define PN5CCU			_PNT("5CCU")
#define PN10CCU			_PNT("10CCU")
