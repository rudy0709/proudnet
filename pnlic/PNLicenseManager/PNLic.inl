#pragma once

#include "PNLic.h"
#include "AppExecuter.inl"

#include "../PNLicenseManager/MemoryHopper.inl"
#include "../PNLicenseManager/HiddenAppExecuter.inl"
#include "../PNLicenseManager/Edgegap.inl"

#if defined(__linux__)
#include "PNLicenseHiddenImage.inl"
#else
#include "PNLicenseHiddenImage.inl"
#endif

namespace Proud
{
    void PNLic_OnSpawnComplete(bool success, HANDLE hSpawnedProcess);

    inline CPNLicSingleton::CPNLicSingleton()
    {
        m_HAS = BeforeHiddenAppExecution;
        m_DHAET = 0; // 처음에는 0이지만 heartbeat이 처음 호출될 때 미래로 세팅될 것이다.
        m_EPA = INVALID_HANDLE_VALUE;
        //m_readFD = 0;
        m_LT = 0;

        m_critSec.m_neverCallDtor = true; // 이 객체는 전역으로 다루어지므로 이렇게 해야 프로세스 종료시 엉뚱한 크래시 안남. QueueUserWorkItem에 의해 뒤늦게 억세스될 수 있으니.
    }

    inline CPNLicSingleton::~CPNLicSingleton()
    {
        CriticalSectionLock lock(m_critSec, true);

        for (auto& iThread : m_hiddenAppCleanTaskList)
        {
            if (true == iThread.joinable())
            {
                iThread.join();
            }
        }

        m_hiddenAppCleanTaskList.clear();
    }

    // Process On Server Heartbeat
    inline void CPNLicSingleton::POSH(int64_t currTimeMs)
    {
        CriticalSectionLock lock(m_critSec, true);

        switch (m_HAS)
        {
        case BeforeHiddenAppExecution:
            // 사용자는 동시다발적으로 서버 인스턴스를 생성할 수 있는데 이때 hidden app이 동시다발적으로 마구 생기면 시스템에 과부하를 준다.
            // 따라서 Hidden app을 무조건 즉시 실행하지 말고, 랜덤한 시간에 (10~60초) 이후에 실행되게 해야 한다.
            if (m_DHAET == 0)
            {

#if defined(_DEBUG) || defined(DEBUG)
                m_DHAET = currTimeMs + 10000 + (rand() % 30000); //빨리 테스트할 때는 편리하지만 디버그 빌드라도 배포 때는 켜지 말 것.
#else
                m_DHAET = currTimeMs + 20000 + (rand() % 60000);
#endif
            }

            // 때가 되면 hidden app을 실행
            if (m_DHAET < currTimeMs)
            {
                if (m_EPA == INVALID_HANDLE_VALUE) {
                    m_HAS = HiddenAppNotExecuted;

                    // 이 스레드가 아니라, 별도 스레드를 만들어서 거기서 한다. POSH는 게임서버 내부로직이라 블러킹 불허.
                    m_hiddenAppCleanTaskList.emplace_back(
                        []()
                        {
                            // 여기는 별도 스레드이다.
                            bool normalSituation = true;    // Edgegap을 구동하지 않는 통상적인 환경인가
#ifndef _WIN32
                            if (IsProudNetLicensedEdgegapContainerCandidate())
                            {
                                normalSituation = false;
                                CheckEdgegapLicense();
                            }
#endif
                            if (normalSituation)
                            {
                                // hidden app execute를 한다.
                                CHiddenAppExecuter hiddenAppExecuter(const_cast<BYTE*>(g_PNLicenseHiddenImage), sizeof(g_PNLicenseHiddenImage), PNLic_OnSpawnComplete, _PNT("4096"), 0);
                                hiddenAppExecuter.PrepareAndExecuteAndDeleteApp(); // CHiddenAppExecuter 의 파괴자에서 실행파일 삭제를 반복 재시도할 것이다.
                            }
                        }
                    );
                }
            }
            break;

        case HiddenAppNotExecuted:

            if (m_EPA != INVALID_HANDLE_VALUE) {
                m_HAS = HiddenAppRunning;
            }
            else
            {			// 때가 되면 hidden app을 실행
                if (m_DHAET > currTimeMs)
                {
                    m_HAS = HiddenAppExited;
                }
            }
            break;

        case HiddenAppRunning:
            // hidden app을 실행하였고 그 hidden app이 종료하면서 hidden app을 실행시켰던 스레드가 종료되었다면, 라이선스 타입을 얻어온다.
            if (m_EPA != INVALID_HANDLE_VALUE)
            {
                // 라이선스 타입 전용 싱글톤을 가져와 그 안의 라이선스 타입을 끄집어 낸다.
                CMemoryHopper& memoryHopper = CMemoryHopper::GetInstance();
                unsigned int* t_LT = memoryHopper.GetLicenseType();

                // 히든앱 생성 스레드가 라이선스 타입 전용 싱글톤에 저장한 라이선스 타입을 라이선스 싱글톤에 넣는다.
                m_LT = *t_LT;

                // 만약 얻어온 라이선스 타입이 유효하지 않거나 라이선스 타입이 아니라면 0으로 초기화한다.                
                if (m_LT == Proud::LicenseType_Invalid || m_LT == Proud::LicenseType_None)
                {
                    // (NOTE: #PNLic_no_key_then_personal 에서, 키 인증한 적 없으면 Personal로 처리해주고 있다. 따라서 여기에 올 일은 없다.)
                    m_LT = 0;
                }

                // #HiddenAppExit 싱글톤의 히든앱 상태 변수를 히든앱 종료로 바꾼다. 
                m_HAS = HiddenAppExited;

                // 인디와 퍼스널, 5CCU, 10CCU일 경우 콘솔에 출력되는 안내문구.
                if ((m_LT & LicenseType_Indie) == LicenseType_Indie)
                {
                    cout << "You are using ProudNet Indie License." << endl;
                }
                else
                {
                    bool isPersonal = ((m_LT & LicenseType_Personal) == LicenseType_Personal);
                    bool is5CCU = ((m_LT & LicenseType_5CCU) == LicenseType_5CCU);
                    bool is10CCU = ((m_LT & LicenseType_10CCU) == LicenseType_10CCU);

                    if (true == isPersonal || true == is5CCU || true == is10CCU)
                    {
                        cout << "You are using ProudNet Personal License. Number of client connections and some features are limited. Note: As of April 2022, any inactive ProudNet license will be treated as a Personal License." << endl;
                    }
                }
            }
            break;

        case HiddenAppExited:
            // 라이선스 인증을 통과했거나 미인증임에도 불구하고 해커가 뚫어서 여기 왔을 수도 있다.
            // 해커가 뚫었을 경우 2차 방어 장치(신규 클라이언트를 받지 말기)가 작동해야 한다.
            // 그리고 모바일 라이선스인 경우 PC 클라이언트는 최대 10개 IP만 허락해야 한다.
            break;

        }
    }

    CPNLicSingleton g_PNLicSingleton;

    // Process On Server Start
    inline void CPNLic::POSS()
    {
        return;
    }

    inline void PNLic_OnSpawnComplete(bool success, HANDLE hSpawnedProcess)
    {
        // 여기까지 왔을때, 안타깝게도 CPNLic의 소유주 즉 서버는 이미 파괴된 상태일 수도 있다. (사용자가 서버 켜자마자 끊을 수도 있으니)
        // 이러한 경우 여기서는 CPNLic를 접근해서는 안된다.

        CriticalSectionLock lock(g_PNLicSingleton.m_critSec, true);

        if (success)
        {
            // 실행된 hidden app의 process handle을 얻어온다.
            g_PNLicSingleton.m_EPA = hSpawnedProcess;
        }
        else
        {
            // 안티 바이러스 앱에 의해 프로세스 실행이 실패할 수 있다.
            // 이러한 경우 서버 실행 인증이 실패한 것으로 간주한다.
            g_PNLicSingleton.m_LT = 0; // 라이선스 인증 실패!
            g_PNLicSingleton.m_HAS = HiddenAppExited;
        }
    }

    // Process On Server Heartbeat
    inline void CPNLic::POSH(int64_t currTimeMs)
    {
        g_PNLicSingleton.POSH(currTimeMs);
        //printf("success to call posh!\n");
    }

    inline CPNLic::~CPNLic()
    {
        POSE();
    }

    // 서버에 접속 들어온 클라이언트가 라이선스에 부합되는지 검사한다.
    inline bool CPNLic::IsAllowedConcurrentUserNumbers(const int ccuNum)
    {
        CriticalSectionLock lock(g_PNLicSingleton.m_critSec, true);

        // 아직 라이선스 검사를 마치지 않았다면(hidden app 실행중) 클라 접속을 모두 허가한다.
        if (g_PNLicSingleton.m_HAS != HiddenAppExited)
            return true;

        //인증 실패했는데도 여전히 서버가 켜져 있다면 클라 접속을 모두 거부한다.
        if ((g_PNLicSingleton.m_LT & 0x40000000) == 0)
            return false;

        // personal license인 경우 IP만 20개만 허락한다.  (더미 클라이언트 테스트를 위해 같은 IP로부터 접속한 클라이언트는 중복 처리하지 않음)
        // pro license인 경우 무제한 허락.
        if ((g_PNLicSingleton.m_LT & LicenseType_Personal) == LicenseType_Personal)
        {
            if (ccuNum > ClientMaxCountForPersonalLicense)
                return false;
        }
        else if ((g_PNLicSingleton.m_LT & LicenseType_5CCU) == LicenseType_5CCU)
        {
            if (ccuNum > ClientMaxCountFor5CCULicense)
                return false;
        }
        else if ((g_PNLicSingleton.m_LT & LicenseType_10CCU) == LicenseType_10CCU)
        {
            if (ccuNum > ClientMaxCountFor10CCULicense)
                return false;
        }

        return true;
    }

    // 서버에 접속 들어온 클라이언트가 라이선스에 부합되는지 검사한다.
    inline bool CPNLic::IsLicensedPlatform(RuntimePlatform clientPlatform, const AddrPort& clientAddr, const int ccuNum)
    {
        CriticalSectionLock lock(m_critSec, true);

        // 아직 라이선스 검사를 마치지 않았다면(hidden app 실행중) 클라 접속을 모두 허가한다.
        if (g_PNLicSingleton.m_HAS != HiddenAppExited)
            return true;

        //인증 실패했는데도 여전히 서버가 켜져 있다면 클라 접속을 모두 거부한다.
        if ((g_PNLicSingleton.m_LT & 0x40000000) == 0)
            return false;

        switch (clientPlatform)
        {
        case RuntimePlatform_UOSXEditor:
        case RuntimePlatform_UWindowsEditor:
            //라이선스 종류 막론하고 UnityEditor에서의 접속은 무제한 허락한다.
            return true;
        case RuntimePlatform_C:
        case RuntimePlatform_UOSXPlayer:
        case RuntimePlatform_UWindowsPlayer:
        case RuntimePlatform_UOSXWebPlayer:
        case RuntimePlatform_UOSXDashboardPlayer:
        case RuntimePlatform_UWindowsWebPlayer:
        case RuntimePlatform_UWiiPlayer:
        case RuntimePlatform_UXBOX360:
        case RuntimePlatform_UPS3:
        case RuntimePlatform_ULinuxPlayer:
        case RuntimePlatform_Flash:
        case RuntimePlatform_UIPhonePlayer:
        case RuntimePlatform_UAndroid:
        case RuntimePlatform_NIPhone:
        case RuntimePlatform_NAndroid:
        case RuntimePlatform_UNaCl:

            // personal license인 경우 IP만 20개만 허락한다.  (더미 클라이언트 테스트를 위해 같은 IP로부터 접속한 클라이언트는 중복 처리하지 않음)
            // pro license인 경우 무제한 허락.
            if ((g_PNLicSingleton.m_LT & LicenseType_Personal) == LicenseType_Personal)
            {
                if (ccuNum > ClientMaxCountForPersonalLicense)
                    return false;
            }
            else if ((g_PNLicSingleton.m_LT & LicenseType_5CCU) == LicenseType_5CCU)
            {
                if (ccuNum > ClientMaxCountFor5CCULicense)
                    return false;
            }
            else if ((g_PNLicSingleton.m_LT & LicenseType_10CCU) == LicenseType_10CCU)
            {
                if (ccuNum > ClientMaxCountFor10CCULicense)
                    return false;
            }

            return true;

        case RuntimePlatform_Last:
            // 잘못된 값이면
            return false;
        default:
            // 나머지는 모바일 플랫폼이다. 이것들은 all platform이건 mobile platform이건 접속을 모두 허가한다.
            return true;
        }
    }

    // 특정 플랫폼의 클라이언트가 접속한 총 IP 갯수를 얻는다.
    inline int CPNLic::GetIPCountByPlatform(RuntimePlatform clientPlatform)
    {
        CriticalSectionLock lock(m_critSec, true);

        CFastMap2<uint32_t, int, int>* ipToCountMap;
        if (!m_PICMap.TryGetValue(clientPlatform, ipToCountMap))
        {
            return 0;
        }

        // key의 갯수, 즉 IP의 갯수를 구한다.
        return ipToCountMap->GetCount();
    }

    // 동시접속자가 300명 넘어가면 인증 서버에 서버 구동 기록을 보낸다.
    // 딱 한번만.
    void CPNLic::ReportConcurrentUserNumbersOverLogDatum(const int ccuNum)
    {
        CriticalSectionLock lock(g_PNLicSingleton.m_critSec, true);

        // indie, personal license, 5CCU, 10CCU인 경우 최초 동접자수 300이 넘는 경우에 한번만 인증서버에 서버정보 전송
        if ((g_PNLicSingleton.m_LT & LicenseType_Personal) == LicenseType_Personal ||
            (g_PNLicSingleton.m_LT & LicenseType_Indie) == LicenseType_Indie ||
            (g_PNLicSingleton.m_LT & LicenseType_5CCU) == LicenseType_5CCU ||
            (g_PNLicSingleton.m_LT & LicenseType_10CCU) == LicenseType_10CCU)
        {
            if (ccuNum >= SendLogThresholdForIndieOrPersonalLicense)
            {
                if (false == m_isReported) // 0 => 1 change
                {
                    m_isReported = true;


                    g_PNLicSingleton.m_hiddenAppCleanTaskList.emplace_back(
                        [ccuNum]()
                        {
                            CHiddenAppExecuter hiddenAppExecuter(const_cast<BYTE*>(g_PNLicenseHiddenImage), sizeof(g_PNLicenseHiddenImage), PNLic_OnSpawnComplete, _PNT("4096"), ccuNum);
                            hiddenAppExecuter.PrepareAndExecuteAndDeleteApp();
                        }
                    );
                }
            }
        }
    }

    bool CPNLic::POCJ(const int ccuNum)
    {
        CriticalSectionLock lock(m_critSec, true);

        if (!IsAllowedConcurrentUserNumbers(ccuNum))
            return false;

        ReportConcurrentUserNumbersOverLogDatum(ccuNum);
        //printf("success to call posh! ipcountbyPlatform(%d), ipcount(%d)\n", GetIPCountByPlatform(clientPlatform), GetIPCount());
        return true;
    }

    // Process Client Join
    bool CPNLic::POCJ(uint32_t clientPlatform0, const AddrPort& clientAddr, const int ccuNum)
    {
        RuntimePlatform clientPlatform = (RuntimePlatform)clientPlatform0;

        CriticalSectionLock lock(m_critSec, true);

        //인증 성공. 통계를 설정한다.
        if (RuntimePlatform_Last != clientPlatform)
            AddPlatformCount(clientPlatform, clientAddr);

        if (!IsLicensedPlatform(clientPlatform, clientAddr, ccuNum))
            return false;

        ReportConcurrentUserNumbersOverLogDatum(ccuNum);

        //printf("success to call posh! ipcountbyPlatform(%d), ipcount(%d)\n", GetIPCountByPlatform(clientPlatform), GetIPCount());
        return true;
    }

    // Process Client Leave
    void CPNLic::POCL(uint32_t clientPlatform0, const AddrPort& clientAddr)
    {
        RuntimePlatform clientPlatform = (RuntimePlatform)clientPlatform0;

        CriticalSectionLock lock(m_critSec, true);

        if (RuntimePlatform_Last != clientPlatform)
            RemovePlatformCount(clientPlatform, clientAddr);
    }

    // addr의 port는 무시된다!
    inline void CPNLic::AddPlatformCount(RuntimePlatform type, const AddrPort& addr)
    {
        CriticalSectionLock lock(m_critSec, true);

        // platform type에 대한 value를 얻거나 새로 추가.
        CFastMap2<uint32_t, int, int>* ipToCountMap;
        if (!m_PICMap.TryGetValue(type, ipToCountMap))
        {
            ipToCountMap = new CFastMap2 < uint32_t, int, int >;
            m_PICMap.Add(type, ipToCountMap);
        }
        // port값은 챙길 필요 없다. 공유기 IP만 챙긴다. 드물지만 서로 다른 내부 주소에 대해 서로 같은 (외부 주소:포트)가 배정되는 라우터를 위해서다.
        uint32_t ip = addr.m_addr.v4; // TODO: ipv6를 지원하게 되면 이것은 int64가 되어야.

        // 클라 접속-해제가 반복되는 부분이므로 속도 챙겨야.
        CFastMap2<uint32_t, int, int>::iterator iv = ipToCountMap->find(ip);
        if (iv == ipToCountMap->end())
        {
            // 새 항목 추가
            ipToCountMap->Add(ip, 1);
        }
        else
        {
            // 기존 항목의 +1
            iv.SetSecond(iv.GetSecond() + 1);
        }
    }

    // addr의 port는 무시된다!
    inline void CPNLic::RemovePlatformCount(RuntimePlatform type, const AddrPort& addr)
    {
        CriticalSectionLock lock(m_critSec, true);

        CFastMap2<uint32_t, int, int>* ipToCountMap;
        if (m_PICMap.TryGetValue(type, ipToCountMap))
        {
            // port값은 챙길 필요 없다. 공유기 IP만 챙긴다. 드물지만 서로 다른 내부 주소에 대해 서로 같은 외부 주소:포트가 배정되는 라우터를 위해서다.
            uint32_t ip = addr.m_addr.v4;

            CFastMap2<uint32_t, int, int>::iterator iv = ipToCountMap->find(ip);
            if (iv != ipToCountMap->end())
            {
                // 기존 항목의 -1
                iv.SetSecond(iv.GetSecond() - 1);

                // 0이 되었으면
                if (iv.GetSecond() <= 0)
                {
                    // 메모리를 아끼기 위해
                    ipToCountMap->erase(iv);
                }
            }
        }
    }

    bool CPNLic::m_isReported = false;
}

