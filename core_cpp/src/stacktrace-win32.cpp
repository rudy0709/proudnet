// 본 소스 파일의 저작권은 http://stackoverflow.com/questions/5693192/win32-backtrace-from-c-code 에서 이 소스를 답변에 첨부한 사람에게 있습니다.

#include "stdafx.h"

#ifdef _WIN32

#include <winbase.h>
#include <Dbghelp.h>

#include "stacktrace-win32.h"
#include "..\include\sysutil.h"

namespace Proud 
{
	// SymInitialize를 최초 1회 하는 역할을 한다.
	class SymbolInit:public CSingleton<SymbolInit>
	{
		bool m_ok;
		DWORD m_win32Error;
		HANDLE m_hProcess;

	public:
		bool IsOK() { return m_ok; }
		DWORD GetWin32Error() { return m_win32Error;  }
		HANDLE GetProcess() { return m_hProcess; }

		SymbolInit()
		{
			m_hProcess = GetCurrentProcess();

			BOOL r = SymInitialize(m_hProcess, NULL, TRUE);
			if (!r)
			{
				// exe가 있는 경로에서도 다시 시도를 한다.
				r = SymInitialize(m_hProcess, StringT2A(GetCurrentProgramDirectory()).GetString(), TRUE);
			}

			if (!r)
			{
				m_win32Error = GetLastError();
				m_ok = false;
			}
			else
			{
				m_ok = true;
			}
		}
	};

	void GetStackTrace_Win32(CFastArray<String>& output)
	{
		output.Clear();

		void* stack[100];
		unsigned short frames;

		if (!SymbolInit::GetUnsafeRef().IsOK())
		{
			Tstringstream ss;
			ss << _PNT("Symbol init failed. Win32 error=") << SymbolInit::GetUnsafeRef().GetWin32Error();
			output.Add(ss.str().c_str());
			return;
		}

		frames = CaptureStackBackTrace(0, 62, stack, NULL);
		SYMBOL_INFO  * symbol;
		symbol = (SYMBOL_INFO *)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
		symbol->MaxNameLen = 255;
		symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

		for (int i = 0; i < frames; i++)
		{
			SymFromAddr(SymbolInit::GetUnsafeRef().GetProcess(), (DWORD64)(stack[i]), 0, symbol);

			Tstringstream ss;
			ss << frames - i - 1 << ": " << symbol->Name << " - " << symbol->Address;
			output.Add(ss.str().c_str());
		}
		free(symbol);
	}
}

#endif // _WIN32
