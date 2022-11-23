/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <new>
#include <iostream>
#include "../include/sysutil.h"
#include "../include/Exception.h"
#include "../include/NetConfig.h"
//#include "ReportError.h"
#include "PnIconv.h"

#if !defined(_WIN32)
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#else
#include <Shlwapi.h>
#include <tchar.h>
#include <tlhelp32.h>
#endif

#ifdef __MACH__
#include <sys/sysctl.h>
#endif

#if defined(__MARMALADE__) || defined(__MACH__)
#include <unistd.h>	
#endif

#if defined(__ANDROID__)
#include <android/log.h>
#endif

#define _COUNTOF(array) (sizeof(array)/sizeof(array[0]))
#define INFO_BUFFER_SIZE 1024

namespace Proud
{
	uint32_t GetNoofProcessors()
	{
#ifdef _WIN32
		SYSTEM_INFO info;
		GetSystemInfo(&info);

		return info.dwNumberOfProcessors;
#elif defined(__MARMALADE__) || defined(__ORBIS__)
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
				::ExitProcess(-1);
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




	void TraceA(const char * fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		StringA text;
		text.FormatV(fmt, args);
		va_end(args);

		CFakeWin32::OutputDebugStringA(text);
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

	void SetCurrentDirectoryWhereProgramExists()
	{
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
		SetCurrentDirectory(path);
	}

#ifdef _WIN32
	bool EnableLowFragmentationHeap()
	{
		static PROUDNET_VOLATILE_ALIGN LONG did = 0;

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
#endif

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

	PROUD_API uint32_t CountSetBits(uintptr_t bitMask)
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

		static PROUDNET_VOLATILE_ALIGN DWORD processorCoreCount = 0;
		static PROUDNET_VOLATILE_ALIGN DWORD logicalProcessorCount = 0;

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
				if (buffer = NULL)
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

	String GetCurrentDirectory()
	{
		PNTCHAR path[LongMaxPath];
		::GetCurrentDirectory(LongMaxPath, path);
		return path;
	}

	BOOL CreateDirectory(const PNTCHAR* lpPathName)
	{
		String pathname2 = GetFullPath(lpPathName);
		return ::CreateDirectory(pathname2, 0);
	}

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

			if (!CreateDirectory(incPath) && GetLastError() != ERROR_FILE_EXISTS && GetLastError() != ERROR_ALREADY_EXISTS)
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

	CLocale& CLocale::Instance()
	{
		return __super::Instance();
	}

	CSystemInfo::CSystemInfo()
	{
		memset(&m_info, 0, sizeof(m_info));
		::GetSystemInfo(&m_info);
	}

	CSystemInfo& CSystemInfo::Instance()
	{
		return __super::Instance();
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

		uint32_t pid = GetCurrentProcessId();
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
#elif defined __MARMALADE__
		return false;
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
			::OutputDebugStringA(text);
#elif defined(__ANDROID__)
			__android_log_print(ANDROID_LOG_INFO, "PROUD_LOG", "%s", text);
#else
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
			OutputDebugStringA(StringW2A(text));
#else
			std::wcout << text;
#endif
		}
	}

#ifdef _WIN32
	String GetMessageTextFromWin32Error(DWORD errorCode)
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






