#pragma once

#include <thread>
#include <cassert>
#include "RunLicenseApp.h"
#include "../PNLicenseSDK/PNSecure.cpp"
#include "../PNLicenseManager/MemoryHopper.inl"

// 프라우드넷 퍼스널 버전 서버가 최대한 접속 허용할 수 있는 클라이언트의 갯수
// 10은 너무 적다. 클라를 몇만 붙이려면 이정도 수는 되어야 한다.
// 2022.03.23 회의후 20인제한에서 5인제한으로 변경
const int ClientMaxCountForPersonalLicense = 5;

// 캡클라우드를 위한 CCU 5 & 10 제한 라이선스
const int ClientMaxCountFor5CCULicense = 5;
const int ClientMaxCountFor10CCULicense = 10;

// 퍼스널과 인디, 5CCU, 10CCU일 경우, 이 숫자만큼의 클라가 접속하면, 넷텐션 인증 서버로 서버 구동 기록을 보낸다.
const int SendLogThresholdForIndieOrPersonalLicense = 300;

#include "KernelObjectNames.h"

#if defined(__linux__)
const char PATH_LICENSEMGR_FILE[] = "L3RtcC9QTk1vbi50bXAuAA==";
#else
const char PATH_LICENSEMGR_FILE[] = "YzovL3dpbmRvd3MvL3RlbXAvL1BOTW9uLnRtcC4A";
#endif

using namespace Proud;

#pragma warning(disable:4996)

namespace Proud
{
    // handle wrapper. PN 소스와 격리시켜야 해커로부터 방어하니까 별도 클래스다.
    class CPnHandle
    {
    public:
        HANDLE m_handle;
        inline CPnHandle()
        {
            m_handle = INVALID_HANDLE_VALUE;
        }
        inline ~CPnHandle()
        {
            if (m_handle != INVALID_HANDLE_VALUE) {
#if defined(__linux__)
#else
                CloseHandle(m_handle);
#endif
            }
        }
    };

    // Proud::finally를 여기에서 쓰고 싶지만,
    // 이상하게도 잠금장치 내부에서는 사용이 안되어진다.
    // 아쉽지만 그래서 이렇게 RAII용으로 별도의 클래스를 만든다.
    // finally나 별도의 클래스를 사용하려고 하는 이유는
    // CAppExecuter의 소멸자에서 RAII되게 하면 안된다고 하셨기 때문이다.
    // CAppExecuter를 상속하는 클래스가 없는 경우라면 그렇게 해도 되지만,
    // 현재 CAppExecuter처럼 상속하는 클래스가 있다면 문제가 될 소지가 있다고 하셨다.
    class CPNLicFileCleaner final
    {
        const TSTRING& m_appFullPath;
    public:
        CPNLicFileCleaner(const TSTRING& appFullPath) : m_appFullPath(appFullPath) {}
        void DeleteAppFile();
        ~CPNLicFileCleaner()
        {
            if (false == m_appFullPath.empty())
            {
                DeleteAppFile();
            }
        }
    };

    void CPNLicFileCleaner::DeleteAppFile()
    {
        for (int k = 0; k < 10; ++k)
        {
            // 아쉽게도, DeleteFile과 remove는 서로 리턴값의 의미가 정반대이다.
            // 윈도우의 DeleteFile은 성공시 nonzero가 반환된다고 MSDN에 써있다.
            // 반면에 리눅스의 remove는 성공시 zero가 반환된다고 man page에 써있다.
#if defined(_WIN32)
            BOOL ret = DeleteFile(m_appFullPath.c_str());

            if (0 != ret)
            {
                // 파일이 없는 경우 ERROR_FILE_NOT_FOUND(2)가 반환되어진다.
                break;
            }

            // 실패시 GetLastError 값 확인은 err, hr을 watch에 기입해서 확인하자.
#else
            int ret = remove(m_appFullPath.c_str());

            if (0 == ret)
            {
                break;
            }
            else if (2 == errno)
            {
                // errno 2 값은 ENOENT이다.
                // ENOENT는 "No such file or directory" 의미이다.
                break;
            }

            // 실패시 errno 값 확인은 err, hr을 watch에 기입해서 확인하자.
#endif

            Proud::Sleep(500);
        }
    }

    class CAppExecuter
    {
    protected:
        Proud::String m_strTempPath;
        TSTRING m_strCcuNum;
        TSTRING m_strParentPID;
        TSTRING m_appFullPath;
        TSTRING m_appPathOnly;
        TSTRING m_strArgList;

        const BYTE* const m_image;
        int m_imageLength;
        OnSpawnCompleteHandler m_onSpawnComplete;
        String m_imageName;
        int m_ccuNum;

        bool PrepareAppExecute();

#if defined(_WIN32)
        // 윈도우
        virtual PROCESS_INFORMATION ExecuteApp() = 0;
        void WaitForAppExit(const PROCESS_INFORMATION&);
#else
        // 리눅스
        virtual pid_t ExecuteApp() = 0;
        void WaitForAppExit(const pid_t&);

        virtual void DoLeftJobAfterAppExit() = 0;
#endif
        
    public:
        CAppExecuter(BYTE* image, int imageLength, OnSpawnCompleteHandler onSpawnComplete, String imageName, const int ccuNum);
        virtual ~CAppExecuter() {};  // 주의 : = default 이런 형태는 VS2012에서 사용할 수 없다.

        // hidden app 실행파일 생성->실행->파일삭제
        void PrepareAndExecuteAndDeleteApp()
        {
            // 실행파일 생성에 실패하면 아무것도 하지 않는다. 할 수 없다.
            if (PrepareAppExecute() == false)
            {
                if (m_onSpawnComplete != nullptr)
                    m_onSpawnComplete(false, INVALID_HANDLE_VALUE);

                return;
            }

            // appFullPath가 결정되고 Cleaner를 만들어준다. 마지막에 결정된 파일이름을 찾아 지우도록.
            CPNLicFileCleaner cleaner(m_appFullPath);

#if defined(_WIN32)
            PROCESS_INFORMATION pi = ExecuteApp();
            WaitForAppExit(pi);
#else
            pid_t forkResult = ExecuteApp();
            if (0 != forkResult && -1 != forkResult)
            {
                WaitForAppExit(forkResult);

                // 주의 반드시 parent process에서만 실행되어져야 한다.
                DoLeftJobAfterAppExit();

                // 핸들러 호출
                if (m_onSpawnComplete != nullptr)
                {
                    m_onSpawnComplete(true, forkResult);
                }
            }
#endif
        }
    };

#if defined(_WIN32)
    // 윈도우
    void CAppExecuter::WaitForAppExit(const PROCESS_INFORMATION& pi)
#else
    // 리눅스
    void CAppExecuter::WaitForAppExit(const pid_t& curPID)
#endif
    {
#if defined(_WIN32)
        // 최대 3분간 워닝앱이 정상 종료되길 기다려 본다.
        // 라이선스 모듈은 3분까지도 걸리는 경우가 있을 수 있다.
        DWORD waitResult = WaitForSingleObject(pi.hProcess, 3 * 60 * 1000);

        // 타임아웃이라면 강제 종료를 시도해본다.
        if (WAIT_TIMEOUT == waitResult)
        {
            TerminateProcess(pi.hProcess, 0);
        }

        // 성공 핸들러 호출
        if (m_onSpawnComplete != NULL) {
            m_onSpawnComplete(true, pi.hProcess);
        }

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
#else
        // 부모 프로세스이면 자식 프로세스인 히든앱이 종료될때까지 기다린다.
        wait(NULL);
#endif
    }


    CAppExecuter::CAppExecuter(BYTE* image, int imageLength, OnSpawnCompleteHandler onSpawnComplete, String imageName, const int ccuNum)
        :m_image(image), m_imageLength(imageLength), m_onSpawnComplete(onSpawnComplete), m_imageName(imageName),m_ccuNum(ccuNum)
    {
        Tstringstream strParentPIDStream;
    #if defined(__linux__)
        strParentPIDStream << getpid();
    #else
        strParentPIDStream << ::GetCurrentProcessId();
    #endif
        Tstringstream strCcuNumStream;

        m_strParentPID = strParentPIDStream.str();
        strCcuNumStream << m_ccuNum;
        m_strCcuNum = strCcuNumStream.str();
    }

    // hidden app 준비
    bool CAppExecuter::PrepareAppExecute()
    {
        // temp 폴더 위치를 얻어온다.
    #ifdef _WIN32
        DWORD dwRetVal = 0;
        Proud::StrBuf pTempPathBuf(m_strTempPath, MAX_PATH_LENGTH);

        // Temp 폴더의 위치를 얻어옵니다. 일부 윈도우는 Temp 폴더의 위치가 다를 수 있습니다. ( 고객사 제보 )
        dwRetVal = GetTempPath(MAX_PATH_LENGTH, pTempPathBuf);

        // GetTempPath 호출에 대한 예외처리
        if (dwRetVal > MAX_PATH_LENGTH || dwRetVal == 0)
        {
            if (m_onSpawnComplete != NULL) {
                m_onSpawnComplete(false, INVALID_HANDLE_VALUE);
            }

            return false;
        }
    #else
        m_strTempPath = StringA2T("/tmp/");
    #endif

        m_appPathOnly = m_strTempPath.GetString();

        Tstringstream appFileNameOnlyStringStream;      // 실행파일 이름
        Tstringstream argListStringStream;      // args

        appFileNameOnlyStringStream << _PNT("PNMon.");
        appFileNameOnlyStringStream << m_imageName.GetString();
        appFileNameOnlyStringStream << _PNT(".");
        appFileNameOnlyStringStream << m_strParentPID.c_str();
        appFileNameOnlyStringStream << _PNT(".tmp");

        // 우선은 Temp/PNMon으로 히든앱 생성 경로를 지정합니다.
        m_appFullPath = m_appPathOnly + appFileNameOnlyStringStream.str();

        argListStringStream << _PNT(" tqxc2 ");
        argListStringStream << m_strParentPID.c_str();
        argListStringStream << _PNT(" ");
        argListStringStream << m_strCcuNum.c_str();
        argListStringStream << _PNT(" ");

        // 히든앱을 CreateProcess하기 위해 넘기는 args를 완성합니다.
        m_strArgList = m_appFullPath.c_str()  + argListStringStream.str();

        ofstream fs;
        fs.open(m_appFullPath.c_str(), std::fstream::out | std::fstream::binary | std::fstream::trunc);
        if (fs.fail() == true) 
        {
            // Endpoint Protection 등을 쓰는 업체의 경우, PNMon이라는 특정 이름을 가진 폴더를 검사에서 제외시킬 수 있습니다.
            // 이를 위해, 윈도우의 경우는 users/[username]/AppData/Local/Temp 하위에 PNMon 폴더를 만듭니다. 
            // 리눅스의 경우는 /tmp 하위에 PNMon 폴더를 만듭니다.
            // 필요하면 고객사에게 이 경로를 알려주어도 됩니다.
#ifdef _WIN32
            // 윈도우의 경우, GetTempPath가 반환하는 경로 문자열에서 각 디렉토리를 구분하는 구분자는 '\'입니다.
            m_appPathOnly += _PNT("\\PNMon\\");
#else
            // 리눅스의 경우, 경로 문자열에서 각 디렉토리를 구분하는 구분자는 '/'입니다.
            m_appPathOnly += _PNT("/PNMon/");
#endif

            // Temp/PNMon 경로로 히든앱 생성에 실패하면 경로를 바꿔서 다시 시도합니다.
            m_appFullPath = m_appPathOnly + appFileNameOnlyStringStream.str();
            // args도 Temp를 타겟으로 하여 다시 만듭니다.
            m_strArgList = m_appFullPath.c_str() + argListStringStream.str();
            fs.open(m_appFullPath.c_str(), std::fstream::out | std::fstream::binary | std::fstream::trunc);
            if (fs.fail() == true)
            {
                // 양 쪽 경로 모두 히든앱 생성에 실패한 경우입니다.
                if (m_onSpawnComplete != NULL)
                {
                    m_onSpawnComplete(false, INVALID_HANDLE_VALUE);
                }
                return false;
            }
        }

    #if defined(__linux__)
        // 실행 가능하게 파일속성 변경
        chmod(m_appFullPath.c_str(), 0777);
    #endif

        // 실행 파일 생성
        fs.write((char*)m_image, m_imageLength);
        if (fs.fail() == true)	{
            fs.close();
            if (m_onSpawnComplete != NULL) {
                m_onSpawnComplete(false, INVALID_HANDLE_VALUE);
            }
            return false;
        }

        fs.close();

        return true;
    }
}
