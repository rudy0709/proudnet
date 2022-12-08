#pragma once

namespace Proud
{
    class CHiddenAppExecuter : public CAppExecuter
    {
#if defined(__linux__)
        // 히든앱과 히든앱 생성 스레드간에 연결될 파이프의 이름
        TSTRING m_fifoName;

        int m_serverFd = -1;
#else
        // 주의 윈도우의 네임드 파이프는 반드시 지켜야 하는 이름 포맷이 있다 !!!
        // 히든앱과 히든앱 생성 스레드간에 연결될 파이프의 이름
        TSTRING m_pipeName;

        // 파이프 동기화에 사용하는 이벤트의 이름
        TSTRING m_writeEventName;
        TSTRING m_readEventName;
#endif
    public:
        // 히든앱을 생성하는 스레드를 실행하는 함수
#if defined(_WIN32)
        virtual PROCESS_INFORMATION ExecuteApp() override;
#else
        virtual pid_t ExecuteApp() override;
        virtual void DoLeftJobAfterAppExit() override;
#endif

        // #TODO
        // 히든앱 파일의 경우, 아직 파일이 잔존할 수 있는 경우가 있다.
        // 라이선스 정보가 잘못된 상태라서 TerminateApp될 때 그러한데,
        // 그러할 때 파일이 잔존하는 이유는 TerminateApp 함수를 실행하는 게 히든앱인데 ~CAppExecuter를 실행시키는 게 POSH이기 때문이다.
        // POSH가 실행되기 전에 TerminateProcess되어져 버린다.
        // 추후 히든앱에 의해 Terminate되어지기 전에 이전 잔존 히든앱 파일들을 일괄 삭제해주는 다음과 같은 함수를 만들어넣자.
        // void DeleteAllHiddenAppFilesTask();

        CHiddenAppExecuter(BYTE* image, int imageLength, OnSpawnCompleteHandler onSpawnComplete, String imageName, const int ccuNum);
        ~CHiddenAppExecuter() {}; // 주의 : = default 이런 형태는 VS2012에서 사용할 수 없다.
    };

    CHiddenAppExecuter::CHiddenAppExecuter(BYTE* image, int imageLength, OnSpawnCompleteHandler onSpawnComplete, String imageName, const int ccuNum)
        :CAppExecuter(image, imageLength, onSpawnComplete, imageName, ccuNum)
    {
#if defined(__linux__)
        Tstringstream fifoNameStream;

        fifoNameStream << LINUX_PIPE_NAME;
        fifoNameStream << m_strParentPID.c_str();
        m_fifoName = fifoNameStream.str();
#else
        Tstringstream readEventNameStream;
        Tstringstream writeEventNameStream;
        Tstringstream pipeNameStream;

        pipeNameStream << WIN_PIPE_NAME1;
        pipeNameStream << WIN_PIPE_NAME2;
        pipeNameStream << m_strParentPID.c_str();
        m_pipeName = pipeNameStream.str();

        writeEventNameStream << EVENT_FOR_WRITE;
        writeEventNameStream << m_strParentPID.c_str();
        m_writeEventName = writeEventNameStream.str();

        readEventNameStream << EVENT_FOR_READ;
        readEventNameStream << m_strParentPID.c_str();
        m_readEventName = readEventNameStream.str();
#endif
    }

    // hidden app을 실행하는 스레드 프로시저
#if defined(_WIN32)
    // 윈도우
    PROCESS_INFORMATION CHiddenAppExecuter::ExecuteApp()
#else
    // 리눅스
    pid_t CHiddenAppExecuter::ExecuteApp()
#endif
    {
#if defined(_WIN32)
        // 파이프 동기화를 위한 이벤트 핸들들
        HANDLE writeEvent, readEvent;

        // 파이프 핸들
        HANDLE hPipe;

        STARTUPINFO si;
        PROCESS_INFORMATION pi;

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        // 주의 윈도우의 네임드 파이프는 반드시 지켜야 하는 이름 포맷이 있다 !!!
        // 히든앱과 히든앱 생성 스레드간에 연결될 파이프의 이름
        TSTRING pipeName;
        Tstringstream pipeNameStream;
        pipeNameStream << WIN_PIPE_NAME1;
        pipeNameStream << WIN_PIPE_NAME2;
        pipeNameStream << m_strParentPID.c_str();
        pipeName = pipeNameStream.str();

        // 읽기 전용 파이프를 생성한다. 바이트 스트림으로 비동기로 읽는다. 원격의 클라이언트가 접속하지 못하게 한다.
        hPipe = CreateNamedPipe(pipeName.c_str(), PIPE_ACCESS_INBOUND, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE
            | PIPE_NOWAIT | PIPE_REJECT_REMOTE_CLIENTS, PIPE_UNLIMITED_INSTANCES
            , sizeof(unsigned int), sizeof(unsigned int), 1000, NULL);

        if (hPipe == INVALID_HANDLE_VALUE)
        {
            NTTNTRACE("CreateNamePipe failed.");

            if (m_onSpawnComplete != NULL) {
                m_onSpawnComplete(false, INVALID_HANDLE_VALUE);
            }

            return pi;
        }

        TSTRING m_writeEventName;
        Tstringstream m_writeEventNameStream;
        m_writeEventNameStream << EVENT_FOR_WRITE;
        m_writeEventNameStream << m_strParentPID.c_str();
        m_writeEventName = m_writeEventNameStream.str();

        // 히든앱이 파이프를 통해서 뭔가 쓸 때까지 이벤트로 대기하고 있게 하기 위한 이벤트를 만든다.
        writeEvent = CreateEvent(NULL, true, false, m_writeEventName.c_str());
        if (writeEvent == NULL)
        {
            NTTNTRACE("CreateEvent for writeEvent failed.");
            CloseHandle(hPipe);

            if (m_onSpawnComplete != NULL) {
                m_onSpawnComplete(false, INVALID_HANDLE_VALUE);
            }

            return pi;
        }

        TCHAR* tmpStr = new TCHAR[m_strArgList.length() + 1];
        _tcscpy(tmpStr, m_strArgList.c_str());

        // temp 파일을 실행한다. 실행만 건 직후 바로 리턴한다.
        if (::CreateProcess(NULL, tmpStr, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi) == FALSE) {
            if (m_onSpawnComplete != NULL) {
                m_onSpawnComplete(false, INVALID_HANDLE_VALUE);
            }

            delete[] tmpStr;
            CloseHandle(hPipe);
            CloseHandle(writeEvent);
            return pi;
        }

        delete[] tmpStr;

        BOOL isSuccess = FALSE;

        if (!ConnectNamedPipe(hPipe, NULL))
        {
            // 리턴값이 0이면 이게 오류인지 더 체크해본다.
            DWORD tmpError = GetLastError();
            if (tmpError != ERROR_IO_PENDING && tmpError != ERROR_PIPE_CONNECTED
                && tmpError != ERROR_PIPE_LISTENING)
            {
                NTTNTRACE("ConnectNamedPipe failed.");
                CloseHandle(hPipe);
                CloseHandle(writeEvent);

                if (m_onSpawnComplete != NULL) {
                    m_onSpawnComplete(false, INVALID_HANDLE_VALUE);
                }

                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
                return pi;
            }
        }

        // 히든앱이 파이프를 통하여 라이선스 정보를 버퍼에 쓸 때까지 기다린다.
        DWORD waitResult = WaitForSingleObject(writeEvent, INFINITE);
        if (waitResult == WAIT_FAILED)
        {
            NTTNTRACE("WaitForSingleObject for writeEvent failed.");
            CloseHandle(hPipe);
            CloseHandle(writeEvent);

            if (m_onSpawnComplete != NULL) {
                m_onSpawnComplete(false, INVALID_HANDLE_VALUE);
            }

            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return pi;
        }

        DWORD bytesRead = 0;

        // 히든앱이 파이프를 통하여 기록해둔 라이선스 타입을 지역변수로 읽어들인다.
        unsigned int t_LT;

        // 연결된 파이프로부터 라이선스 정보를 읽는다.
        bool result = ReadFile(hPipe, (void*)&t_LT, sizeof(unsigned int), &bytesRead, NULL);
        if (!result && GetLastError() != ERROR_PIPE_LISTENING)
        {
            NTTNTRACE("ReadFile failed.");
            CloseHandle(hPipe);
            CloseHandle(writeEvent);

            if (m_onSpawnComplete != NULL) {
                m_onSpawnComplete(false, INVALID_HANDLE_VALUE);
            }

            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return pi;
        }

        // 이전에 지역변수에 저장한 라이선스 타입을 라이선스 타입 전용 싱글톤 안에 넣는다.
        CMemoryHopper& memoryHopper = CMemoryHopper::GetInstance();
        memoryHopper.SetLicenseType(t_LT);

        // 파이프 작업이 다 끝났음을 히든앱에게 알려줄 때 쓸 이벤트를 오픈한다.
        // 이미 히든앱이 write이벤트를 셋한 상태이므로 read이벤트가 생성되었음을 확신할 수 있다.
        readEvent = OpenEvent(EVENT_ALL_ACCESS | EVENT_MODIFY_STATE, false, m_readEventName.c_str());
        if (readEvent == NULL)
        {
            NTTNTRACE("OpenEvent for readEvent failed - in Hidden app");
            CloseHandle(hPipe);
            CloseHandle(writeEvent);

            if (m_onSpawnComplete != NULL) {
                m_onSpawnComplete(false, INVALID_HANDLE_VALUE);
            }

            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return pi;
        }

        // 파이프 반대편인 히든앱이 파이프를 통해 남긴 라이선스 정보를 다 읽었다고 알린다.
        SetEvent(readEvent);

        CloseHandle(hPipe);
        CloseHandle(writeEvent);
        CloseHandle(readEvent);

        return pi;
#else
        // 리눅스
        pid_t curPID = 0;

        // 히든앱과 히든앱 생성 스레드간에 연결될 파이프의 이름
        Tstringstream fifoNameStream;
        fifoNameStream << LINUX_PIPE_NAME;
        fifoNameStream << m_strParentPID.c_str();
        m_fifoName = fifoNameStream.str();

        // 리눅스는 파일에 대한 접근 권한을 4자리의 8진수로 표현을 합니다.
        // 네번재 자리는 sticky bit등의 특수권한을 지정하는 자리라서 보통 0(특수권한없음)을 지정합니다.
        // 세번째 자리부터 첫번째자리까지는 순서대로 소유자, 그룹 소유자, 기타 사용자의 접근 권한을 뜻합니다.
        // 해당 자리에 들어가는 숫자는 1(x, execute), 2(w, write), 4(r, read)이 세가지 숫자들 중 해당 자리에 속하는 이용자가
        // 가지게 될 권한에 해당하는 숫자들을 더한 값이 들어가게 됩니다.
        // 예를 들어 0666이면 특수권한은 없고 소유자 ~ 기타 사용자 모두 읽기&쓰기 권한만 있는 파일로 만들라는 뜻입니다.
        // 파이프 파일에 대한 접근 권한을 0666로 하여 fifo(네임드 파이프)를 생성한다.
        if (mkfifo(m_fifoName.c_str(), 0666) == -1)
        {
            // 무슨 이유에서 에러인지 출력해준다.
            NTTNTRACE("Can't not create a pipe.");

            if (m_onSpawnComplete != NULL) {
                m_onSpawnComplete(false, INVALID_HANDLE_VALUE);
            }

            return curPID;
        }

        // 읽기 전용이고 비동기로 네임드 파이프 한쪽을 연다.
        m_serverFd = open(m_fifoName.c_str(), O_RDONLY | O_NONBLOCK);
        if (m_serverFd == -1)
        {
            NTTNTRACE("open - serverFd");

            if (m_onSpawnComplete != NULL) {
                m_onSpawnComplete(false, INVALID_HANDLE_VALUE);
            }

            //remove(m_fifoName.c_str());
            unlink(m_fifoName.c_str());
            return curPID;
        }

        // 도플갱어를 복제한다.
        curPID = fork();
        switch (curPID)
        {
        case 0:
            // 자식 프로세스는 자신을 히든앱으로 탈바꿈한다.
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

            close(m_serverFd);

            //remove(m_fifoName.c_str());
            unlink(m_fifoName.c_str());

            break;
        default:
            break;
        }

        return curPID;
#endif
    }

#if !defined(_WIN32)
    // 리눅스 전용

    // 필독 :
    // 리눅스에서 named pipe인 FIFO는 파일형태로 존재하게 되는데,
    // 송수신 프로세스 양쪽에서 unlink해줬을 때 파일이 자동 삭제되게 된다.
    // 이러한 FIFO의 특성으로 인하여,
    // 리눅스에서는 한쪽 프로세스가 unlink하였어도 다른쪽에서도 unlink하기 전까지는 FIFO 파일의 내용을 읽을 수 있는 것이다.
    void CHiddenAppExecuter::DoLeftJobAfterAppExit()
    {
        // 히든앱이 파이프를 통하여 기록해둔 라이선스 타입을 지역변수로 읽어들인다.
        unsigned int t_LT;
        if (read(m_serverFd, (void*)&t_LT, sizeof(unsigned int)) != sizeof(unsigned int))
        {
            NTTNTRACE("read");

            if (m_onSpawnComplete != NULL) {
                m_onSpawnComplete(false, INVALID_HANDLE_VALUE);
            }

            close(m_serverFd);

            //remove(m_fifoName.c_str());
            unlink(m_fifoName.c_str());

            return;
        }

        // 이전에 지역변수에 저장한 라이선스 타입을 라이선스 타입 전용 싱글톤 안에 넣는다.
        CMemoryHopper& memoryHopper = CMemoryHopper::GetInstance();
        memoryHopper.SetLicenseType(t_LT);

        // 리눅스에서 파이프를 만들면 이는 파일로 생성되어 프로그램이 종료되어 남아있으므로 삭제해준다.
        close(m_serverFd);

        //remove(m_fifoName.c_str());
        unlink(m_fifoName.c_str());
    }
#endif
}
