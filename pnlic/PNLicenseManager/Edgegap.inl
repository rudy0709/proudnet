#ifndef _WIN32

#include <assert.h>
#include <cstring>
#include <curl/curl.h>
#include <stdlib.h>


#endif

namespace Proud {
#ifndef _WIN32
    // Edgegap에서 실행중인가?
    bool IsProudNetLicensedEdgegapContainerCandidate() {
        // Edgegap은 Linux Container에서만 작동한다.
        // 환경변수가 있는지 체크
        const char* v = getenv("ARBITRIUM_PROUDNET_ENABLED");
        return v != nullptr;
    }

    struct memory {
        char* response{ nullptr };
        size_t size{ 0 };
    };

    static size_t OnRESTReceive(void* data, size_t size, size_t nmemb,
        void* userPtr) {
        size_t realsize = size * nmemb;
        struct memory* mem = (struct memory*)userPtr;

        char* ptr = (char*)realloc((void*)mem->response, mem->size + realsize + 1);
        if (ptr == nullptr)
            return 0; /* out of memory! */

        mem->response = ptr;
        memcpy((void*)(mem->response + mem->size), data, realsize);
        mem->size += realsize;
        mem->response[mem->size] = 0;

        return realsize;
    }

    // 이미 Edgegap Container에서 실행 가능한 curl을 실행해서, 응답을 받아온다.
    // https://docs.edgegap.com/docs/arbitrium/deployment-injected-variables#inject-context-variables
    // https://docs.edgegap.com/docs/example/deployment#get-deployment-context-from-a-running-deployment
    // 이 함수는 블러킹 함수다.
    void CheckEdgegapLicense() {
        // 이 함수는 실행 후 성공/실패 여부에 따라 PrepareAndExecuteAndDeleteApp의
        // 마지막 처리를 따라한다.

        // env var상 Edgegap 안에 있다고 했지만, 정작 curl이 실패하는 경우(가령
        // Edgegap 측 내부 문제), PNLicenseHidden이 실행되면 Edgegap 내 타사
        // Container에 악영향을 줄 수 있다. 따라서 Hidden을 실행하지 않고 그냥 실패
        // 처리한다. 어차피 서버 강제종료는 Hidden App이 하는데, Edgegap 환경에서는
        // Hidden App 자체를 안 띄운다. 그냥 Personal License처럼 작동시켜버린다.
        {
            const char* contextURL = getenv("ARBITRIUM_CONTEXT_URL");
            const char* token = getenv("ARBITRIUM_CONTEXT_TOKEN");

            if (contextURL == nullptr || token == nullptr) {
                goto failed;
            }

            CURLcode ret;
            char auth1[100000];
            sprintf(auth1, "authorization: %s", token);
            curl_slist* slist1 = curl_slist_append(slist1, auth1);

            CURL* hnd = curl_easy_init();
            curl_easy_setopt(hnd, CURLOPT_BUFFERSIZE, 102400L);
            curl_easy_setopt(hnd, CURLOPT_URL, contextURL);
            curl_easy_setopt(hnd, CURLOPT_NOPROGRESS, 1L);
            curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, slist1);
            curl_easy_setopt(hnd, CURLOPT_USERAGENT, "curl/7.58.0");
            curl_easy_setopt(hnd, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(hnd, CURLOPT_MAXREDIRS, 50L);
            curl_easy_setopt(hnd, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_NONE);
            curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "GET");
            curl_easy_setopt(hnd, CURLOPT_FTP_SKIP_PASV_IP, 1L);
            curl_easy_setopt(hnd, CURLOPT_TCP_KEEPALIVE, 1L);

            /* send all data to this function  */
            curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, OnRESTReceive);

            memory chunk;
            /* we pass our 'chunk' struct to the callback function */
            curl_easy_setopt(hnd, CURLOPT_WRITEDATA, (void*)&chunk);

            FILE* filep = fopen("/dev/null", "wb");
            assert(filep);
            curl_easy_setopt(hnd, CURLOPT_STDERR, filep);

            /* Here is a list of options the curl code used that cannot get generated
                as source easily. You may select to either not use them or implement
                them yourself.

            CURLOPT_WRITEDATA set to a objectpointer
            CURLOPT_INTERLEAVEDATA set to a objectpointer
            CURLOPT_WRITEFUNCTION set to a functionpointer
            CURLOPT_READDATA set to a objectpointer
            CURLOPT_READFUNCTION set to a functionpointer
            CURLOPT_SEEKDATA set to a objectpointer
            CURLOPT_SEEKFUNCTION set to a functionpointer
            CURLOPT_ERRORBUFFER set to a objectpointer
            CURLOPT_STDERR set to a objectpointer
            CURLOPT_HEADERFUNCTION set to a functionpointer
            CURLOPT_HEADERDATA set to a objectpointer

            */

            ret = curl_easy_perform(hnd);

            curl_easy_cleanup(hnd);
            hnd = NULL;
            curl_slist_free_all(slist1);
            slist1 = NULL;

            fclose(filep);

            if (ret != 0) {
                goto failed;
            }
            if (strstr(chunk.response, "public_ip") == nullptr) {
                goto failed;
            }

            {
                CriticalSectionLock lock(g_PNLicSingleton.m_critSec, true);
                // 성공
                g_PNLicSingleton.m_LT = LicenseType_Pro;

                // #HiddenAppExit Hidden 실행이 다 마친것처럼 행세한다.
                g_PNLicSingleton.m_HAS = HiddenAppExited;
            }

            return;
        }
    failed:

        {
            CriticalSectionLock lock(g_PNLicSingleton.m_critSec, true);
            g_PNLicSingleton.m_LT = LicenseType_Personal;

            // #HiddenAppExit Hidden 실행이 다 마친것처럼 행세한다.
            g_PNLicSingleton.m_HAS = HiddenAppExited;
        }

        cout << "Edgegap authorization failed. Running as a Personal License. Number of client connections and some features are limited." << endl;
    }
#endif
} // namespace Proud
