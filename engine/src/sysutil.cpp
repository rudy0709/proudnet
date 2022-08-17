/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <fstream>
#include <iostream>
#include <new>
#include <stdlib.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include "../include/Exception.h"
#include "../include/NetConfig.h"
#include "../include/sysutil.h"
#include "GlobalTimerThread.h"
#include "PnIconv.h"

#if !defined(_WIN32)
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#if !defined(__ANDROID__) && !defined(__ORBIS__) // glob는 안드로이드와 PS4에서 못 쓴다.
#include <glob.h>
#endif
#else
#include <Shlwapi.h>
#include <tchar.h>
#include <tlhelp32.h>
#include <Lm.h>
#pragma comment(lib, "netapi32.lib")
#endif

#if defined(__linux__)
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <signal.h>
#endif

#ifdef __MACH__
#include <sys/sysctl.h>
#endif

#if defined(__MACH__)
#include <unistd.h>
#endif

#if defined(__ANDROID__)
#include <android/log.h>
#endif

#define INFO_BUFFER_SIZE 1024

// chrono::microseconds는 VS2012부터 지원한다.
#if (_MSC_VER>=1700) || (__cplusplus > 199711L)
#include <chrono>
#include <thread>
#endif


#include "IncludeFileSystem.h"
#include "../include/PNPath.h"

#ifdef _MSC_VER 
#pragma warning(disable:4996)
#endif


namespace Proud
{
	bool CreateDirectory(const PNTCHAR* lpPathName);

	// USE_UnsafeFastMemcpy 매크로 선언쪽에 넣을 설명을 굳이 여기로 옮겼음. 대외 공개하고 싶지 않으므로.
	// USE_UnsafeFastMemcpy를 켠 이유:
	// code optimize 켜고 Concurrency Visualizer로 측정 결과, 이걸 켜는 것이 더 CPU를 적게 먹는다.
	//

#if defined(_WIN32)

#ifdef _MSC_VER
#pragma warning(disable:4702)
#endif


	// NetWkstaGetInfo 함수가 겁나 느려서 값 캐싱을 해야함
	CriticalSection g_winVersionVariableCS;
	DWORD g_winMajorCache_MUST_LOCK_CS = 0xFF;
	DWORD g_winMinorCache_MUST_LOCK_CS = 0xFF;
	DWORD g_winServicePackCache_MUST_LOCK_CS = 0xFF;
	bool  g_isRetrievedForGetWindowsVersionAPI_MUST_LOCK_CS = false;

	bool GetWindowsVersion(DWORD& major, DWORD& minor, DWORD& servicePack)
	{
		CriticalSectionLock lock(g_winVersionVariableCS, true);

		// 이미 값이 캐싱 되어 있으면 캐싱 된 값을 가져간다.
		if (g_isRetrievedForGetWindowsVersionAPI_MUST_LOCK_CS)
		{
			if (g_winMajorCache_MUST_LOCK_CS != 0xFF
				&& g_winMinorCache_MUST_LOCK_CS != 0xFF
				&& g_winServicePackCache_MUST_LOCK_CS != 0xFF)
			{
				major = g_winMajorCache_MUST_LOCK_CS;
				minor = g_winMinorCache_MUST_LOCK_CS;
				servicePack = g_winServicePackCache_MUST_LOCK_CS;

				return true;
			}
			else
			{
				major = 0;
				minor = 0;
				servicePack = 0;

				return false;
			}
		}

		g_isRetrievedForGetWindowsVersionAPI_MUST_LOCK_CS = true;

		// Windows 의 major, minor 버전을 먼저 가져 온다.
		{
			LPWKSTA_INFO_100 pBuf = NULL;
			if (NERR_Success != NetWkstaGetInfo(NULL, 100, (LPBYTE*)&pBuf))
				return false;

			g_winMajorCache_MUST_LOCK_CS = pBuf->wki100_ver_major;
			g_winMinorCache_MUST_LOCK_CS = pBuf->wki100_ver_minor;

			NetApiBufferFree(pBuf);
		}

		// Windows 의 ServicePack 버전을 가져온다.
		{
			const int servicePackMax = 10;
			OSVERSIONINFOEX osvi;
			DWORDLONG conditionMask = 0;

			// ikpil.choi : 2016-11-03, memory safe function
			//ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
			memset_s(&osvi, sizeof(osvi), 0, sizeof(osvi));

			osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
			VER_SET_CONDITION(conditionMask, VER_SERVICEPACKMAJOR, VER_EQUAL);

			for (int i = 0; i < servicePackMax; ++i)
			{
				osvi.wServicePackMajor = static_cast<WORD>(i);
				if (VerifyVersionInfo(&osvi, VER_SERVICEPACKMAJOR, conditionMask) == false)
				{
					return false;
				}
				else
				{
					g_winServicePackCache_MUST_LOCK_CS = i;
					break;
				}
			}
		}

		// output param 에 캐싱 된 값을 복사
		major = g_winMajorCache_MUST_LOCK_CS;
		minor = g_winMinorCache_MUST_LOCK_CS;
		servicePack = g_winServicePackCache_MUST_LOCK_CS;

		return true;
	}
#endif

	/* Microsecond Sleep.
	NOTE: Sleep(1)을 넣으나 USleep(200)을 넣으나 C++11 sleep_for를 넣으나
	usleep-test.sln에서는 5ms를 기다려 버린다.
	어떻게 해결해야 할까? 골치네.
	*/
	void USleep(long sleepTimeUs)
	{
		assert(0);// 이 함수 일단 사용 금지. 윈도 전용 함수이지만 정작 Windows Server에서 최소 50ms를 기다린다. Windows 8에서는 5ms.

#ifdef _WIN32
	// chrono::microseconds는 VS2012부터 지원한다.
	#if (_MSC_VER>=1700) || (__cplusplus > 199711L)
		// C++11을 지원하면 이것을 쓰자. 왜냐하면 이 함수는 C++이 계속 발전하면서 더 나아질테니까.
		std::this_thread::sleep_for(std::chrono::microseconds(sleepTimeUs));
	#else
		// VS2010 이하에서
		LARGE_INTEGER ft;

		ft.QuadPart = -(10 * sleepTimeUs); // Convert to 100 nanosecond interval, negative value indicates relative time

		// Win32 API를 써서 us 단위의 sleep을 한다.
		// 이를 위해 object pool에서 timer handle을 먼저 가져온다.
		MicroSecondTimer* timer = CGlobalTimerThread::GetSharedPtr()->GetMicrosecondTimer();
		HANDLE hTimer = timer->m_handle;

		assert(hTimer);

		// 마이크로초 이내에 깰 수 있게 설정한다.
		// 이를 처리할 핸들을 object pool에서 가져온다.
		// 안타깝게도, 마이크로초 단위로 깬 후부터는, periodic하게 깨우고는 싶지만, 그게 하필 '밀리초'다.
		SetWaitableTimer(hTimer, &ft, 0, NULL, NULL, 0);

		// 기다림
		WaitForSingleObject(hTimer, INFINITE);

		// 가져와서 처리 다 끝났으니, 이제 도루 반환하도록 하자.
		CGlobalTimerThread::GetSharedPtr()->ReleaseMicrosecondTimer(timer);
	#endif
#else
		// C++11을 지원하지 않지만 unix 플랫폼이면
		// 이렇게 이미 제공되는 us 단위 wait 함수를 쓰자.
		::usleep(sleepTimeUs);
#endif
	}

#if !defined(__ANDROID__) && !defined(__ORBIS__) // glob는 안드로이드와 PS4에서 못 쓴다.
	void FindFiles(const String& directory, const String& searchFilter, CFastArray<String> &output)
	{
		output.Clear();

		String fileNameWithWildcard = Path::Combine(directory, searchFilter);
#ifdef _WIN32
		WIN32_FIND_DATA wfd = { 0 };

		HANDLE finder = FindFirstFile(fileNameWithWildcard.c_str(), &wfd);
		if (finder == INVALID_HANDLE_VALUE) {
			return;
		}

		BOOL found = TRUE;
		while (found)
		{
			if (Tstrcmp((const PNTCHAR*)wfd.cFileName, _PNT(".")) != 0
				&& Tstrcmp((const PNTCHAR*)wfd.cFileName, _PNT("..")) != 0
				&& wfd.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY)
			{
				// 경로 이름과 파일 이름을 재조합하자.
				output.Add(Path::Combine(directory, wfd.cFileName));
			}

			found = FindNextFile(finder, &wfd);
		}
		FindClose(finder);
#else 
 		int ret;
 		int num;
 
 		glob_t globbuf;
 		globbuf.gl_pathc = 0;
 		globbuf.gl_pathv = NULL;
 		globbuf.gl_offs = 0;
 		ret = glob(fileNameWithWildcard.c_str(), GLOB_DOOFFS | GLOB_MARK, NULL, &globbuf);
 
 		if (ret != 0 && ret != GLOB_NOMATCH)
 			return;
 
 		if (ret == GLOB_NOMATCH)
 			num = 0;
 		else
 			num = globbuf.gl_pathc;
 
 		for (int i = 0; i < num; i++)
 		{
 			string fileName;
 			string tempListArray = (char*)((globbuf.gl_pathv)[i]);
 
 			size_t pos = tempListArray.find_last_of('/');
 			if (pos != string::npos)
 			{
 				fileName = tempListArray.substr(pos + 1);
 				output.push_back(Path::Combine(directory, fileName));
 			}
 			else
 			{
 				output.push_back(Path::Combine(directory, tempListArray));
 			}
 		}
 
 		//frees the dynamically allocated storage for glob
 		globfree(&globbuf);
#endif
	}
#endif
	
	uint32_t GetNoofProcessors()
	{
#ifdef _WIN32
		SYSTEM_INFO info;
		GetSystemInfo(&info);

		return info.dwNumberOfProcessors;
#elif defined(__ORBIS__)
		return 1; // TODO: 나중에 실제로 갯수를 정확히 얻어야.
#else
		uint32_t numCPU = (uint32_t)sysconf(_SC_NPROCESSORS_ONLN);

		return numCPU;

#endif
	}

	// ProudNet 사용자가 실수한 경우에만 보여주는 에러다.
	void ShowUserMisuseError(const PNTCHAR* text)
	{
		CFakeWin32::OutputDebugStringT(text);

		if (CNetConfig::UserMisuseErrorReaction == ErrorReaction_MessageBox)
		{
#ifdef _WIN32
			// 대화 상자를 표시한다.
			int userInput = ::MessageBox(NULL, text, _PNT("ProudNet"), MB_ABORTRETRYIGNORE | MB_ICONHAND);

			if (userInput == IDIGNORE)
			{
				// nothing
			}
			else if (userInput == IDABORT)
			{
				// 2016.3.18 ExitProcess 함수의 인자는 UINT 타입이므로 -1에서 1로 수정 (0이 아닌 값은 보통 실패를 의미하므로)
				::ExitProcess(1);
			}
			else if (userInput == IDRETRY)
			{
				DebugBreak();
			}
#endif // _WIN32
		}
		else if (CNetConfig::UserMisuseErrorReaction == ErrorReaction_DebugBreak)
		{
#ifdef _WIN32
			DebugBreak();
#endif
		}
		else if (CNetConfig::UserMisuseErrorReaction == ErrorReaction_DebugOutput)
		{
			// 이미 위에서 찍었으니 여기선 할 거 없음
		}
		else
		{

			// 크래시를 유발한다.
			int * b = NULL;
			*b = 100;
		}
	}

	String GetProcessName()
	{
#if defined(__ORBIS__)
		return _PNT("");
#elif !defined(_WIN32)
		char module_file_name[1024];
		memset(module_file_name, 0, sizeof(module_file_name));
		getcwd(module_file_name, sizeof(module_file_name));
		return StringA2T(module_file_name);
#else
		PNTCHAR module_file_name[LongMaxPath];
		GetModuleFileName(NULL, module_file_name, LongMaxPath);
		return module_file_name;
#endif
	}

	uint64_t GetCurrentThreadID()
	{
#if defined(_WIN32)
		return ::GetCurrentThreadId();
#else
		assert(sizeof(pthread_t) <= sizeof(uint64_t)); // 이걸 가정하고 만들어진 함수다.
		return (uint64_t)pthread_self();
#endif
	}

	// tab을 4개의 space로 바꾼다.
	// yaml load등에서 쓴다.
	StringA UntabifyText(const StringA& text)
	{
		StringA ret = text;
		ret.Replace("\t", "    ");
		return ret;
	}

#if !defined(__ORBIS__)
	// false를 리턴하면 실패다. GetLastError로 체크하세요.
	bool CreateTempDirectory(const PNTCHAR* prefix, String& output)
	{
		PNTCHAR tempDir2[20000];
#ifdef _WIN32
        PNTCHAR tempDir[20000];
		GetTempPath(20000, tempDir);
		GetTempFileName(tempDir, prefix, 0, tempDir2);
#else
		string t = Path::Combine("/tmp/", prefix);
		strcpy(tempDir2, t.c_str());
		strcat(tempDir2, "XXXXXX");
		mkstemp(tempDir2);
#endif
		Tstrcat(tempDir2, _PNT("0"));
		if (!Proud::CreateDirectory(tempDir2))
		{
			return false;
		}
		output = tempDir2;
		return true;
	}

	bool FileExists(const PNTCHAR* fileName)
	{
#ifdef _WIN32
		DWORD r = GetFileAttributes(fileName);
		return (r != INVALID_FILE_ATTRIBUTES);
#else
		return (access(fileName, F_OK) != -1);
#endif

		// 리눅스 static lib에서 boost를 의존하는 경우 문제가 된다.
		// 리눅스에서 c++17에서나 가능한 std::filesystem을 쓸 수 있게 되면 그때 여기 자체구현을 대체하도록 하자.
// #ifdef USE_STD_FILESYSTEM
// 		return fs::exists(fs::path(StringT2A(fileName).GetString()));
// #else
// 		throw Exception("Not supported. Use newer compiler.");
// #endif
	}
#endif // #if !defined(__ORBIS__)

	void TraceA(const char * fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		StringA text;
		text.FormatV(fmt, args);
		va_end(args);

		CFakeWin32::OutputDebugStringA(text.GetString());
	}

#ifdef _WIN32
	size_t GetOccupiedPhysicalMemory()
	{
		MEMORYSTATUS mmstate;
		GlobalMemoryStatus(&mmstate);
		return mmstate.dwTotalPhys - mmstate.dwAvailPhys;
	}

	size_t GetTotalPhysicalMemory()
	{
		MEMORYSTATUS mmstate;
		GlobalMemoryStatus(&mmstate);
		return mmstate.dwTotalPhys;
	}

	String GetComputerName()
	{
		PNTCHAR s[1000];
		DWORD s2 = 1000;
		if (::GetComputerName(s, &s2))
			return s;
		return _PNT("");
	}

#endif

#if !defined(__ORBIS__)
	// 현재 실행중인 프로그램의 폴더 위치를 얻는다.
	String GetCurrentProgramDirectory()
	{
#ifdef _WIN32
		PNTCHAR module_file_name[LongMaxPath];
		GetModuleFileName(NULL, module_file_name, LongMaxPath);

		PNTCHAR path[LongMaxPath];
		PNTCHAR drive[LongMaxPath];
		PNTCHAR dir[LongMaxPath];
		PNTCHAR fname[LongMaxPath];
		PNTCHAR ext[LongMaxPath];
#if (_MSC_VER>=1400)
		_tsplitpath_s(module_file_name, drive, LongMaxPath, dir, LongMaxPath, fname, LongMaxPath, ext, LongMaxPath);
		_tmakepath_s(path, LongMaxPath, drive, dir, _PNT(""), _PNT(""));

		//_tsplitpath_s( module_file_name,
		//	drive, sizeof(drive),
		//	dir,sizeof(dir),
		//	fname,sizeof(fname),
		//	ext ,sizeof(ext ));
		//_tmakepath_s( path, _COUNTOF(path),drive, dir, _PNT(""), _PNT("") );
#else
		_tsplitpath(module_file_name, drive, dir, fname, ext);
		_tmakepath(path, drive, dir, _PNT(""), _PNT(""));
#endif
		return path;
#else 
		char exePath[10000];
		readlink("/proc/self/exe", exePath, 9999);
		String ret = Path::GetDirectoryName(exePath);
		return ret;
#endif
	}

	int SetCurrentDirectory(const PNTCHAR* directoryName)
	{
#ifdef _WIN32
		return ::SetCurrentDirectory(directoryName);
#else 
		if (chdir(directoryName) == 0)
			return 0;
		else
			return errno;
#endif
	}

	void SetCurrentDirectoryWhereProgramExists()
	{
		String path = GetCurrentProgramDirectory();
		Proud::SetCurrentDirectory(path.GetString());
	}
#endif // #if !defined(__ORBIS__)

	bool GetFileLastWriteTime(const PNTCHAR* fileName, std::time_t& output)
	{
#ifdef _WIN32
		struct _stat64 s;
		int ret = _wstat64(fileName, &s);
#else
		struct stat s;
		int ret = stat(fileName, &s);
#endif
		if (ret == 0) // 성공
		{
			output = s.st_mtime;
			return true;
		}
		else 
			return false;
	}

#ifdef _WIN32
	bool EnableLowFragmentationHeap()
	{
		static volatile LONG did = 0;

		if (InterlockedCompareExchange(&did, 1, 0) == 0)
		{
			uint32_t  HeapFragValue = 2;

			if (CKernel32Api::Instance().HeapSetInformation)
			{
				CKernel32Api::Instance().HeapSetInformation(GetProcessHeap(),
					HeapCompatibilityInformation,
					&HeapFragValue,
					sizeof(HeapFragValue));
			}
		}
		
		return true;
	}

	bool IsServiceMode()
	{
		// 서비스 모드로 실행중인지 확인하기
		char szBuf[INFO_BUFFER_SIZE];
		DWORD dwBufSize = INFO_BUFFER_SIZE;
		if (::GetUserNameA(szBuf, &dwBufSize) == FALSE)
			return false;

		if (strcmp(szBuf, "SYSTEM") == 0 ||
			strcmp(szBuf, "NETWORK SERVICE") == 0 ||
			strcmp(szBuf, "LOCAL SERVICE") == 0)
		{
			return true;
		}

		return false;
	}


	bool IsGqcsExSupported()
	{
		return CKernel32Api::Instance().GetQueuedCompletionStatusEx != NULL;
	}

	 uint32_t CountSetBits(uintptr_t bitMask)
	{
		uint32_t LSHIFT = sizeof(uintptr_t) * 8 - 1;
		uint32_t bitSetCount = 0;
		uintptr_t bitTest = (uintptr_t)1 << LSHIFT;
		uint32_t i;

		for (i = 0; i <= LSHIFT; ++i)
		{
			bitSetCount += ((bitMask & bitTest) ? 1 : 0);
			bitTest /= 2;
		}

		return bitSetCount;
	}

	bool IsHyperThreading()
	{
		if (CKernel32Api::Instance().GetLogicalProcessInformation == NULL)
			return false;

		static volatile DWORD processorCoreCount = 0;
		static volatile DWORD logicalProcessorCount = 0;

		if (processorCoreCount != 0 && logicalProcessorCount != 0)
			return (processorCoreCount != logicalProcessorCount);

		processorCoreCount = 0;
		logicalProcessorCount = 0;

		PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = NULL;
		PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = NULL;
		DWORD returnLength = 0;
		DWORD byteOffset = 0;

		DWORD rc = CKernel32Api::Instance().GetLogicalProcessInformation(buffer, &returnLength);

		if (!rc)
		{
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				if (buffer)
					free(buffer);

				buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(returnLength);
				if (buffer == NULL)
					throw std::bad_alloc();

				if (NULL == buffer)
				{
					return false;
				}

				rc = CKernel32Api::Instance().GetLogicalProcessInformation(buffer, &returnLength);

				if (FALSE == rc)
				{
					free(buffer);
					return false;
				}
			}
			else
			{
				//실패라면 hyper-threading 없다고 가정.
				return false;
			}
		}

		ptr = buffer;
		while (byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnLength)
		{
			switch (ptr->Relationship)
			{
				//주석 되어 있는것들은 필요하면 사용하도록 하자.
				//case RelationNumaNode:
				//	// Non-NUMA systems report a single record of this type.
				//	numaNodeCount++;
				//	break;

			case RelationProcessorCore:
				processorCoreCount++;

				// A hyperthreaded core supplies more than one logical processor.
				logicalProcessorCount += CountSetBits(ptr->ProcessorMask);
				break;

				//case RelationCache:
				//	// Cache data is in ptr->Cache, one CACHE_DESCRIPTOR structure for each cache.
				//	Cache = &ptr->Cache;
				//	if (Cache->Level == 1)
				//	{
				//		processorL1CacheCount++;
				//	}
				//	else if (Cache->Level == 2)
				//	{
				//		processorL2CacheCount++;
				//	}
				//	else if (Cache->Level == 3)
				//	{
				//		processorL3CacheCount++;
				//	}
				//	break;

				//case RelationProcessorPackage:
				//	// Logical processors share a physical package.
				//	processorPackageCount++;
				//	break;

			default:
				break;
			}
			byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
			ptr++;
		}
		free(buffer);

		return (processorCoreCount != logicalProcessorCount);
	}


	String GetFullPath(const PNTCHAR* fileName)
	{
		//	CSingleLock lock(&m_mutex,1);
		String path;
		if (::PathIsRelative(fileName) == TRUE)
		{
			path.Append(GetCurrentDirectory());

			PNTCHAR buffer[MAX_PATH];
			::PathCombine(buffer, GetCurrentDirectory().GetString(), fileName);
			return String(buffer);
		}

		return String(fileName);
	}

#endif //_WIN32

#if !defined(__ORBIS__)
	String GetCurrentDirectory()
	{
#ifdef _WIN32
		PNTCHAR path[LongMaxPath];
		::GetCurrentDirectory(LongMaxPath, path);
		return path;
#else 
		char path[LongMaxPath];
		if (getcwd(path, sizeof(path)) == NULL)
			return "";
		else 
			return path;
#endif
	}
#endif //#if !defined(__ORBIS__)

	int64_t GetCurrentProcessId()
	{
#ifdef _WIN32
		return ::GetCurrentProcessId();
#else
		return ::getpid();
#endif
	}

	bool CreateDirectory(const PNTCHAR* lpPathName)
	{
#ifdef _WIN32
		String pathname2 = GetFullPath(lpPathName);
		return ::CreateDirectory(pathname2.GetString(), 0);
#else
		return ::mkdir(lpPathName, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0;
#endif
	}

#ifdef _WIN32
	bool CreateFullDirectory(const PNTCHAR* lpPathName, String& outCreatedDirectory)
	{
		String refinedPath;
		refinedPath = GetFullPath(lpPathName);

		outCreatedDirectory = refinedPath;

		int curPos = 0;

		String incPath;
		String resToken = refinedPath.Tokenize(_PNT("\\"), curPos);
		while (resToken != _PNT(""))
		{
			incPath += resToken;
			incPath += _PNT("\\");

			if (!CreateDirectory(incPath.GetString()) && GetLastError() != ERROR_FILE_EXISTS && GetLastError() != ERROR_ALREADY_EXISTS)
				return false;

			resToken = refinedPath.Tokenize(_PNT("\\"), curPos);
		};

		return true;
	}

	CKernel32Api::CKernel32Api()
	{
		m_dllHandle = LoadLibraryA("kernel32.dll");
		if (!m_dllHandle)
			throw Exception("Cannot find kernel32.dll handle!");

		HeapSetInformation = (HeapSetInformationProc)GetProcAddress(m_dllHandle, "HeapSetInformation");
		GetQueuedCompletionStatusEx = (GetQueuedCompletionStatusExProc)GetProcAddress(m_dllHandle, "GetQueuedCompletionStatusEx");

		InitializeCriticalSectionEx = (InitializeCriticalSectionExProc)GetProcAddress(m_dllHandle, "InitializeCriticalSectionEx");
		GetLogicalProcessInformation = (GetLogicalProcessorInformation)GetProcAddress(m_dllHandle, "GetLogicalProcessorInformation");
	}

	//modify by rekfkno1 - 이렇게 멤버변수로 두는것은 생성자 호출이 늦는경우가 있더라.
	//그래서 static 함수내의 static 로컬변수로 싱글턴을 하는것이 좋을 듯하다.
	//CKernel32Api CKernel32Api::g_instance;
	CKernel32Api& CKernel32Api::Instance()
	{
		static CKernel32Api instance;
		// 여러스레드에서 동시다발적으로 ctor가 실행돼도 문제는 없음. 파괴자 루틴도 없겠다, 따라서 그냥 이렇게 고고고.
		return instance;
	}


	CLocale::CLocale()
	{
		m_languageID = 0;

		char szNation[7];
		if (0 != GetLocaleInfoA(LOCALE_SYSTEM_DEFAULT, LOCALE_ICOUNTRY, szNation, 7))
		{
			m_languageID = atoi(szNation);
		}

		/*		switch( m_languageID )
		{
		case 82:        // KOR(대한민국)
		case 1:        // USA(미국)
		case 7:        // RUS(러시아)
		case 81:        // 일본
		case 86:        // 중국
		break;
		}*/

	}

	CLocale& CLocale::GetUnsafeRef()
	{
		return __super::GetUnsafeRef();
	}

	CSystemInfo::CSystemInfo()
	{
		memset(&m_info, 0, sizeof(m_info));
		::GetSystemInfo(&m_info);
	}

	CSystemInfo& CSystemInfo::GetUnsafeRef()
	{
		return __super::GetUnsafeRef();
	}

	int GetTotalThreadCount()
	{
		// http://weseetips.com/2008/05/02/how-to-iterate-all-running-process-in-your-system/ 에서 발췌
		HANDLE hProcessSnapShot = CreateToolhelp32Snapshot(
			TH32CS_SNAPALL,
			0);

		// Initialize the process entry structure.
		PROCESSENTRY32 ProcessEntry = { 0 };
		ProcessEntry.dwSize = sizeof(ProcessEntry);

		// Get the first process info.
		BOOL Return = FALSE;
		Return = Process32First(hProcessSnapShot,
			&ProcessEntry);

		// Getting process info failed.
		if (!Return)
		{
			return -1;
		}

		uint32_t pid = (DWORD)GetCurrentProcessId();
		do
		{
			/*// print the process details.
			cout<<"Process EXE File:" << ProcessEntry.szExeFile
			<< endl;
			cout<<"Process ID:" << ProcessEntry.th32ProcessID
			<< endl;
			cout<<"Process References:" << ProcessEntry.cntUsage
			<< endl;
			cout<<"Process Thread Count:" << ProcessEntry.cntThreads
			<< endl;*/


			// check the PROCESSENTRY32 for other members.

			if (ProcessEntry.th32ProcessID == pid)
				return ProcessEntry.cntThreads;
		} while (Process32Next(hProcessSnapShot, &ProcessEntry));

		// Close the handle
		CloseHandle(hProcessSnapShot);

		return -1;
	}

	DWORD GetThreadStatus(HANDLE const handle)
	{
		// https://msdn.microsoft.com/ko-kr/library/windows/desktop/ms683189(v=vs.85).aspx
		DWORD dwExitCode = 0;
		::GetExitCodeThread(handle, &dwExitCode);
		return dwExitCode;
	}

#endif // _WIN32


	unsigned int CFakeWin32::GetTickCount()
	{
#if !defined(_WIN32)
		timeval tick;
		gettimeofday(&tick, 0);
		return (unsigned int)(tick.tv_sec * 1000 + tick.tv_usec / 1000);
#else
		return ::GetTickCount();
#endif
	}

	bool CFakeWin32::IsDebuggerPresent()
	{
#ifdef _WIN32
		return ::IsDebuggerPresent() ? true : false;
#elif defined __linux__
		return false;
#elif defined __ORBIS__
		return false;
#else // __MACH__
		int mib[4];
		struct kinfo_proc info;
		size_t size;

		info.kp_proc.p_flag = 0;
		mib[0] = CTL_KERN;
		mib[1] = KERN_PROC;
		mib[2] = KERN_PROC_PID;
		mib[3] = getpid();

		size = sizeof(info);
		sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0);

		return ((info.kp_proc.p_flag & P_TRACED) != 0);
#endif
	}


	// non-win32에서는 이 함수가 없어서, 콘솔에 디버깅 문자열을 출력한다.
	void CFakeWin32::OutputDebugStringA(const char* text)
	{
		if (CNetConfig::AllowOutputDebugString)
		{
#ifdef _WIN32
			// 디버그 창에
			::OutputDebugStringA(text);
#elif defined(__ANDROID__)
			// 안드로이드는 이걸
			__android_log_print(ANDROID_LOG_INFO, "PROUD_LOG", "%s", text);
#else
			// ps4 등.
			std::cout << text;
#endif
		}
	}

	void CFakeWin32::OutputDebugStringW(const wchar_t *text)
	{
		if (CNetConfig::AllowOutputDebugString)
		{
#ifdef _WIN32
			::OutputDebugStringW(text);
#elif defined(__ANDROID__)
			OutputDebugStringA(StringW2A(text).GetString());
#else
			std::wcout << text;
#endif
		}
	}

#ifdef _WIN32
	String GetMessageTextFromWin32Error(uint32_t errorCode)
	{
		PNTCHAR* lpMsgBuf = NULL;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			errorCode,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,
			0, NULL);
		String ret = lpMsgBuf;
		LocalFree(lpMsgBuf);
		return ret;
	}
#endif

#ifndef _WIN32
	// 유효한 handle인가?
	bool fd_is_valid(int fd)
	{
		return fcntl(fd, F_GETFL) != -1 || errno != EBADF;
	}
#endif


}





