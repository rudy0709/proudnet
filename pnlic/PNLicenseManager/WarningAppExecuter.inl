#pragma once

namespace Proud
{
    class CWarningAppExecuter : public CAppExecuter
    {
    public:
#if defined(_WIN32)
        // 윈도우
        virtual PROCESS_INFORMATION ExecuteApp() override;
#else
        // 리눅스
        virtual pid_t ExecuteApp() override;

        virtual void DoLeftJobAfterAppExit() override {};
#endif
        CWarningAppExecuter(BYTE* image, int imageLength, OnSpawnCompleteHandler onSpawnComplete, String imageName, const int ccuNum);
        ~CWarningAppExecuter() = default;
    };

    CWarningAppExecuter::CWarningAppExecuter(BYTE* image, int imageLength, OnSpawnCompleteHandler onSpawnComplete, String imageName, const int ccuNum)
        :CAppExecuter(image, imageLength, onSpawnComplete, imageName, ccuNum)
    {
    }

    // warning app을 실행하는 스레드 프로시저
#if defined(_WIN32)
    // 윈도우
    PROCESS_INFORMATION CWarningAppExecuter::ExecuteApp()
#else
    // 리눅스
    pid_t CWarningAppExecuter::ExecuteApp()
#endif
    {
#if defined(_WIN32)
        STARTUPINFO si;
        PROCESS_INFORMATION pi;

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        TCHAR* tmpStr = new TCHAR[m_strArgList.length() + 1];
        _tcscpy(tmpStr, m_strArgList.c_str());

        // 워닝앱 파일을 실행한다. 실행만 건 직후 바로 리턴한다.
        if (::CreateProcess(NULL, tmpStr, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi) == FALSE) {
            if (m_onSpawnComplete != NULL) {
                m_onSpawnComplete(false, INVALID_HANDLE_VALUE);
            }
            delete[] tmpStr;

            return pi;
        }

        delete[] tmpStr;

        // 핸들러 호출
        if (m_onSpawnComplete != NULL) {
            m_onSpawnComplete(true, pi.hProcess);
        }

        // 자식 프로세스 핸들 닫기는 CAppExecuter의 WaitForAppExit에서 한다.
        return pi;
#else
        pid_t curPID = fork();
        switch (curPID)
        {
        case 0:
            // 자식 프로세스는 자신을 워닝앱으로 탈바꿈한다.
            if (execlp(m_appFullPath.c_str(), m_appFullPath.c_str(), "tqxc2", m_strParentPID.c_str(), m_strCcuNum.c_str(), NULL) == -1) {
                if (m_onSpawnComplete != NULL) {
                    m_onSpawnComplete(false, INVALID_HANDLE_VALUE);
                }
            }
            break;
        case -1:
            // 뭔가 포크가 잘못되었다.
            if (m_onSpawnComplete != NULL) {
                m_onSpawnComplete(false, INVALID_HANDLE_VALUE);
            }
            break;
        default:
            break;
        }

        return curPID;
#endif
    }
}
