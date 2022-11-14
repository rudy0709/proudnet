#include "stdafx.h"
#include "PNLicenseHidden.h"
#include "../PNLicenseManager/AppExecuter.inl"
#include "../PNLicenseManager/WarningAppExecuter.inl"

#include "../PNLicenseManager/KernelObjectNames.h"

// 아래 inl파일을 만들어내려면, BuildHelper나 pnlic-generate-test-image.bat 를 사용하세요.
#if defined(__linux__)
#include "../PNLicenseManager/linux/PNLicenseWarningImage.inl"
#include <sys/types.h>
#include <signal.h>
#else
#include "../PNLicenseManager/PNLicenseWarningImage.inl"
#endif

#if defined(__linux__)
#define PN_HANDLE HANDLE
#else
#define PN_HANDLE DWORD
#endif

bool TerminateApp(PN_HANDLE parentPid, int regState = 0, int extendedInfo = 0)
{
    VIRTUALIZER_TIGER_RED_START
        //StringA strResult;
        //strResult.Format("%d", licenseType);

        // 경고앱 실행
        CWarningAppExecuter warningAppExecuter(const_cast<BYTE*>(g_PNLicenseWarningImage), sizeof(g_PNLicenseWarningImage), NULL, _PNT("2048"), 0);
    warningAppExecuter.PrepareAndExecuteAndDeleteApp();

    //cout << "failed to recognize licensekey. (REG: " << regState << ", EXT: " << extendedInfo << ", errorcode: " << PNErrorCreateThread << ")" << endl;

#if defined(__linux__)
    // Server process를 죽이자.
    //if(fdWrite > 2) {
    //	strResult.Format("%d", licenseType);
    //	write(fdWrite, strResult.GetString(), strResult.GetLength());
    //}

    kill(parentPid, 9);
#else
    // Server process를 죽이자.
    HANDLE handle = ::OpenProcess(PROCESS_TERMINATE, FALSE, parentPid);
    if (handle == NULL)
    {
        //cout << "failed to recognize licensekey. (REG: " << regState << ", EXT: " << extendedInfo << ", errorcode: " << PNErrorCreateThread << ")" << endl;
        return false;
    }

    ::TerminateProcess(handle, 0);
#endif

    //cout << "failed to recognize licensekey. (REG: " << regState << ", EXT: " << extendedInfo << ", errorcode: " << PNErrorGetLicense << ")" << endl;
    VIRTUALIZER_TIGER_RED_END
        return true;
}

int PNRegGetStatusProxy(int* pExtendedInfo, CPNErrorInfo& errorInfo, char* argv[])
{
    int ret;
#if defined(_WIN32)
    __try
    {
#endif
        // Code Virtualizer 개발사인 Oreans 측의 권고 사항대로 try ~ except ... 구문을 감싸고 있던 VIRTUALIZER_* 매크로의 위치를 스코프 안으로 쪼개서 옮김.
        // https://oreans.com/help/cv/hm_faq_can-code-virtualizer-macros-pr.htm
        VIRTUALIZER_FISH_RED_START
            ret = PNRegGetStatus(pExtendedInfo, errorInfo);
        VIRTUALIZER_FISH_RED_END
#if defined(_WIN32)
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        VIRTUALIZER_FISH_RED_START
            // 여기가 호출된다는 것은 이미 비 정상적인 상황이므로, 서버 프로세스를 강제로 죽이도록 합니다
            HANDLE handle = ::OpenProcess(PROCESS_TERMINATE, FALSE, atoi(argv[2]));
        if (handle == NULL)
        {
            return false;
        }
        ::TerminateProcess(handle, 0);
        VIRTUALIZER_FISH_RED_END
    }
#endif
    return ret;
}

PNLicErrorCode PNRegGetLicenseInfoProxy(String& strName, String& strCompanyName, String& strLicenseType, CPNErrorInfo& errorInfo, char* argv[])
{
    VIRTUALIZER_FISH_RED_START
        PNLicErrorCode ret;
#if defined(_WIN32)
    __try
    {
#endif
        ret = PNRegGetLicenseInfoAndUpdateLastAccessTime(strName, strCompanyName, strLicenseType, errorInfo);

#if defined(_WIN32)
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        TerminateApp(atoi(argv[2]));
    }
#endif
    // VIRTUALIZER_FISH_RED_END가 return 뒤에 있으면 컴파일러가 최적화하면서 없애버릴 수도 있다. 위치에 신경쓰자!
    VIRTUALIZER_FISH_RED_END
        return ret;
}

#if defined(__linux__)
char** _argv = NULL;
void handler(int sig)
{
    kill(atoi(_argv[2]), SIGKILL);
}
#endif

int main(int argc, char* argv[])
{
    VIRTUALIZER_FISH_RED_START

        unsigned int licenseType = Proud::LicenseType_Invalid; //3221225781;

#if defined(__linux__)
    _argv = argv;
    signal(SIGINT, handler);
    signal(SIGSEGV, handler);
    signal(SIGTERM, handler);
    signal(SIGABRT, handler);
    signal(SIGILL, handler);
    signal(SIGBUS, handler);

    // 서버 쪽의 스레드와 연결할 파이프의 이름
    TSTRING fifoName;
    Tstringstream fifoNameStream;
    fifoNameStream << LINUX_PIPE_NAME;
    fifoNameStream << argv[2];
    fifoName = fifoNameStream.str();
#endif

    int extendedInfo = 0;
    int regState = 0;

    if (argc != 4) {
        //cout << "failed to recognize licensekey. (reason: invalid arguments. errorcode: " << PNErrorInvalidArgCount << ")" << endl;
        return licenseType;
    }

    if (strcmp(argv[1], "tqxc2") != 0) {
        //cout << "failed to recognize licensekey [" << argv[1] << "]" << endl;
        //cout << "failed to recognize licensekey. (reason: invalid arguments. errorcode: " << PNErrorInvalidMagicKey << ")" << endl;
        return licenseType;
    }

    //cout << "success to recognize licensekey [" << argv[0] << "]" << endl;

    CPNLicenseInfo licenseInfo;
    CPNErrorInfo errorInfo;

    // < PNRegGetLicenseInfoProxy 함수에서 PNLicErrorCode::OK가 반환되지 않는 모든 경우 >
    //
    //	1. CommonAppData 경로 ( All Users\Application Data) 경로를 WinSDK 함수로 못 얻어온 경우 ( 이런 경우는 없다고 봐도 될 것이다. ) ( comment - LRFFSCAWF )
    //	2. pnrglic 파일이 없어서 빈파일을 새로 만든 경우 ( comment - LRFFSE )
    //	3. pnrglic 파일이 있지만 여는 데 실패한 경우 ( comment - LRFFSOF )
    //	4. 열린 pnrglic 파일로부터 바이너리를 읽는데 실패한 경우 ( comment - LRFFSRB)
    //		4-1. std::fstream.is_open이 false 인 경우
    //		4-2. std::istream.read 직후 fstream.fail()이 true인 경우
    //	5. 직렬화 형식에 맞지 않는 경우 ( comment - LRFLLIFRF, LRFFSCF )
    //	6. Base64 디코딩 결과 validity가 올바르지 않은 경우 ( comment - LRFLLIFRF, LRFFSCF )
    //
    PNLicErrorCode getInfoResult = PNRegGetLicenseInfoProxy(licenseInfo.m_projectName, licenseInfo.m_companyName, licenseInfo.m_licenseType, errorInfo, argv);

    // #PNLic_no_key_then_personal 라이선스키가 유효던 무효던 어떤 타입이던 상관없이 Personal License로 강제로 간주해버린다.
    bool forceLicenseTypeAsPersonal = false;
    /* 아래는 과거에 쓰였던 게임서버 강제 종료 코드. 기록 취지로 남김.
    #if defined(__linux__)
            unlink(fifoName.c_str());
    #endif
            TerminateApp(atoi(argv[2]), regState, extendedInfo);

    */
    // 라이선스키가 비정상(만료/위조/무효/미설치)이면 Personal로 간주해버린다. 2022년 04월 라이선스 정책부터 그렇게 한다.
    if (PNLicErrorCode::RegistryFileNotExists == getInfoResult)
    {
        forceLicenseTypeAsPersonal = true;
    }
    else if (PNLicErrorCode::OK == getInfoResult)
    {
        regState = PNRegGetStatusProxy(&extendedInfo, errorInfo, argv);
        if (PNIsRegistered != regState)
        {
            forceLicenseTypeAsPersonal = true;
        }
    }
    else // OK도 아니고, RegistryFileNotExists도 아닌 경우
    {
        forceLicenseTypeAsPersonal = true;
    }

    int ccuNum = atoi(argv[3]);
    //cout << "Get CCU Number: " << ccuNum << endl;

    // 일정 동접을 넘으면, 서버 사용 기록을 넷텐션 측에 남긴다.
    if (ccuNum >= SendLogThresholdForIndieOrPersonalLicense)
    {
        int limitCount = 0;
        CPNLicInformer informer;
        informer.SetLicenseInfo(licenseInfo);
        informer.SetCcuNumber(ccuNum);
        informer.SetProcessNameByPID(atoi(argv[2]));
        informer.Connect(g_serverIp, g_port);
        while (informer.DidSendRMIToAuthServer() == false)
        {
            if (limitCount++ > 10000) {
                break;
            }
            informer.FrameMove();
            Proud::Sleep(1);
        }
        informer.Disconnect();
    }

    if (forceLicenseTypeAsPersonal == false)
    {
        int pos = 0;
        String strTmp;
        String strLicenseType;
        String strCustom(licenseInfo.m_licenseType);
        String curOption;

        curOption = strCustom.Tokenize(_PNT(","), pos);

        // 라이선스 종류 문구로부터 라이선스 종류 비트조합 값을 얻는다.
        if (curOption.Compare(PNSRCLICENSE) == 0 ||
            curOption.Compare(PNLIBLICENSE) == 0 ||
            curOption.Compare(PNPROLICENSE) == 0)
        {
            licenseType = Proud::LicenseType_Pro;
        }
        else if (curOption.Compare(PNINDIELICENSE) == 0)
        {
            licenseType = Proud::LicenseType_Indie;
        }
        else if (curOption.Compare(PNPERSONALLICENSE) == 0)
        {
            licenseType = Proud::LicenseType_Personal;
        }
        else if (curOption.Compare(PN5CCU) == 0)
        {
            licenseType = Proud::LicenseType_5CCU;
        }
        else if (curOption.Compare(PN10CCU) == 0)
        {
            licenseType = Proud::LicenseType_10CCU;
        }
        else if (curOption.Compare(PNMOBILELICENSE) == 0)
        {
#if defined(__linux__)
            // PNMOBILELICENSE는 Linux 버전 출시 전 라이선스였다. 즉 미지원으로 처리한다.
            licenseType = Proud::LicenseType_None;
#endif
            while ((curOption = strCustom.Tokenize(_PNT(","), pos)) && curOption.GetLength() > 0)
            {
                if (curOption.Compare(FLASHTYPE) == 0)
                {
                    licenseType |= Proud::LicenseType_Flash;
                }
                else if (curOption.Compare(UIPHONETYPE) == 0)
                {
                    licenseType |= Proud::LicenseType_UIPhone;
                }
                else if (curOption.Compare(UANDROIDTYPE) == 0)
                {
                    licenseType |= Proud::LicenseType_UAndroid;
                }
                else if (curOption.Compare(UWEBTYPE) == 0)
                {
                    licenseType |= Proud::LicenseType_UWeb;
                }
                else if (curOption.Compare(NIPHONETYPE) == 0)
                {
                    licenseType |= Proud::LicenseType_NIPhone;
                }
                else if (curOption.Compare(NANDROIDTYPE) == 0)
                {
                    licenseType |= Proud::LicenseType_NAndroid;
                }
            }
        }
        else if (Proud::LicenseType_Personal != licenseType)
        {
            licenseType = Proud::LicenseType_None;
        }
    }
    else
    {
        licenseType = Proud::LicenseType_Personal;
    }

    //#if defined(__linux__)
    //	if (fdWrite > 2) {
    //		strResult.Format("%d", licenseType);
    //	}
    //	write(fdWrite, strResult.GetString(), strResult.GetLength());
    //#endif

    //cout << "success to recognize licensekey!!!! (" << licenseType << ")" << endl;
    VIRTUALIZER_FISH_RED_END
#if defined(__linux__)

        // 쓰기 전용으로 네임드 파이프를 연다.
        int serverFd = open(fifoName.c_str(), O_WRONLY);
    if (serverFd == -1)
    {
        // 뭔가 네임드 파이프 오픈이 잘못되었다.
        NTTNTRACE("Fifo open failed - in Hidden app");
        return -1;
    }

    // 오픈한 네임드 파이프를 통해서 인증받은 라이선스 타입을 커널 버퍼에 쓴다.
    if (write(serverFd, (void*)&licenseType, sizeof(unsigned int)) != sizeof(unsigned int))
    {
        NTTNTRACE("Fifo write failed - in Hidden app");
        close(serverFd);
        return -1;
    }
    close(serverFd);
#else
        // 연결할 파이프의 이름
        // 주의 윈도우의 네임드 파이프는 반드시 지켜야 하는 이름 포맷이 있다 !!!
        TSTRING pipeName;
    Tstringstream pipeNameStream;
    pipeNameStream << WIN_PIPE_NAME1;
    pipeNameStream << WIN_PIPE_NAME2;
    pipeNameStream << argv[2];
    pipeName = pipeNameStream.str();

    // 라이선스 정보를 파이프를 통해 넘겨주었다는 것을 알려주기 위한 이벤트의 이름
    TSTRING writeEventName;
    Tstringstream writeEventNameStream;
    writeEventNameStream << EVENT_FOR_WRITE;
    writeEventNameStream << argv[2];
    writeEventName = writeEventNameStream.str();

    // 버퍼의 내용을 파이프 반대편에서 읽어갔다는 것을 통지받기 위한 이벤트의 이름
    TSTRING readEventName;
    Tstringstream readEventNameStream;
    readEventNameStream << EVENT_FOR_READ;
    readEventNameStream << argv[2];
    readEventName = readEventNameStream.str();

    HANDLE hPipe, writeEvent, readEvent;

    // 라이선스 정보를 파이프를 통해 넘겨주었다는 것을 알려주기 위한 이벤트
    // 히든앱을 생성하기 전에 만드는 이벤트이므로 생성되어 있다는 것을 확신할 수 있다.
    writeEvent = OpenEvent(EVENT_ALL_ACCESS | EVENT_MODIFY_STATE, false, writeEventName.c_str());
    if (writeEvent == NULL)
    {
        NTTNTRACE("OpenEvent for writeEvent failed - in Hidden app");
        return -1;
    }

    // 파이프를 열되 쓰기 전용으로 연다.
    hPipe = CreateFile(pipeName.c_str(), GENERIC_WRITE,
        0, NULL, OPEN_EXISTING, 0, NULL);

    if (hPipe == INVALID_HANDLE_VALUE)
    {
        NTTNTRACE("CreateFile failed. - hidden app");
        CloseHandle(writeEvent);
        return -1;
    }

    DWORD bytesWritten = 0;

    // 연결된 파이프로 라이선스 정보를 쓴다. 자세한건 위키에서 "연결된 파이프로 라이선스 정보를 쓴다"로 검색하면 나오는 페이지 참고.
    if (!WriteFile(hPipe, (void*)&licenseType, sizeof(unsigned int), &bytesWritten, NULL))
    {
        NTTNTRACE("WriteFile failed. - hidden app");
        CloseHandle(writeEvent);
        CloseHandle(hPipe);
        return -1;
    }

    // 버퍼의 내용을 파이프 반대편에서 읽어갔다는 것을 통지받기 위한 이벤트를 만든다.
    readEvent = CreateEvent(NULL, true, false, readEventName.c_str());

    if (readEvent == NULL)
    {
        NTTNTRACE("CreateEvent for readEvent failed. - hidden app");
        CloseHandle(writeEvent);
        CloseHandle(hPipe);
        return -1;
    }

    // 라이선스 정보를 파이프를 통해서 버퍼에 썼다고 알려줘 읽어가게 만든다.
    SetEvent(writeEvent);

    // 파이프 반대편에서 읽어가길 기다린다.
    DWORD waitResult = WaitForSingleObject(readEvent, INFINITE);
    if (waitResult == WAIT_FAILED)
    {
        NTTNTRACE("WaitForSingleObject for readEvent failed. - hidden app");
        CloseHandle(writeEvent);
        CloseHandle(hPipe);
        CloseHandle(readEvent);
        return -1;
    }

    CloseHandle(hPipe);
    CloseHandle(writeEvent);
    CloseHandle(readEvent);
#endif
    return 0;
}

