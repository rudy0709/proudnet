#include "NativeType.h"
#include "ProudNetServerPlugin.h"
#include "Tracer.h"

bool CopyNativeDataToManageByteArray(void* dst, void* src, int length)
{
	if ((dst == nullptr) || (src == nullptr) || (length <= 0))
	{
		return false;
	}

	bool ret = true;

	try
	{
		memcpy(dst, src, length);
	}
	catch (...)
	{
		ret = false;
	}

	return ret;
}

bool CopyNativeByteArrayToManageByteArray(void* dst, Proud::ByteArray* src)
{
	if ((dst == nullptr) || (src == nullptr) || (src->GetCount() <= 0))
	{
		return false;
	}

	bool ret = true;

	try
	{
		memcpy(dst, src->GetData(), src->GetCount());
	}
	catch (...)
	{
		ret = false;
	}
	return ret;
}

bool ManageByteArrayToCopyNativeByteArray(Proud::ByteArray* dst, uint8_t* src, int length)
{
	if ((dst == nullptr) || (src == nullptr) || (length <= 0))
	{
		return false;
	}

	bool ret = true;
	try
	{
		dst->SetCount(length);
		memcpy(dst->GetData(), src, length);
	}
	catch (...)
	{
		ret = false;
	}

	return ret;
}

//////////////////////////////////////////////////////////////////////////

Proud::MiniDumpAction StartUpDump(Proud::String dumpFileName, int miniDumpType)
{
	using namespace Proud;

#if defined(_WIN32)
	CMiniDumpParameter parameter;
	parameter.m_dumpFileName = dumpFileName;
	parameter.m_miniDumpType = (MINIDUMP_TYPE)miniDumpType;

	return CMiniDumper::GetSharedPtr()->Startup(parameter);;
#else
	return Proud::MiniDumpAction::MiniDumpAction_None;
#endif
}

void ManualFullDump()
{
	using namespace Proud;
#if defined(_WIN32)
	CMiniDumper::GetSharedPtr()->ManualFullDump();
#endif
}

void ManualMiniDump()
{
	using namespace Proud;
#if defined(_WIN32)
	CMiniDumper::GetSharedPtr()->ManualMiniDump();
#endif
}

void WriteDumpFromHere(Proud::String fileName, bool fullDump)
{
	using namespace Proud;
#if defined(_WIN32)
	CMiniDumper::GetSharedPtr()->WriteDumpFromHere(fileName.GetString(), fullDump);
#endif
}

//////////////////////////////////////////////////////////////////////////

void* LogWriter_Create(Proud::String logFileName, int newFileForLineLimit, bool putFileTimeString)
{
	using namespace Proud;
	return CLogWriter::New(logFileName, newFileForLineLimit, putFileTimeString);
}

void LogWriter_Destory(void* obj)
{
	using namespace Proud;
	if (obj!= NULL)
	{
		CLogWriter* logWriter = static_cast<CLogWriter*>(obj);
		delete logWriter;
	}
}

void LogWriter_SetFileName(void* obj, Proud::String logFileName)
{
	using namespace Proud;
	if (obj != NULL)
	{
		CLogWriter* logWriter = static_cast<CLogWriter*>(obj);
		logWriter->SetFileName(logFileName);
	}
}

void LogWriter_WriteLine(void* obj, int logLevel, int logCategory, int logHostID, Proud::String logMessage, Proud::String logFunction, int logLine)
{
	using namespace Proud;
	if (obj != NULL)
	{
		CLogWriter* logWriter = static_cast<CLogWriter*>(obj);
		logWriter->WriteLine(logLevel, (LogCategory)logCategory, (HostID)logHostID, logMessage, logFunction, logLine);
	}
}

void LogWriter_WriteLine(void* obj, Proud::String logMessage)
{
	using namespace Proud;
	if (obj != NULL)
	{
		CLogWriter* logWriter = static_cast<CLogWriter*>(obj);
		logWriter->WriteLine(logMessage);
	}
}

void LogWriter_SetIgnorePendedWriteOnExit(void* obj, bool flag)
{
	using namespace Proud;
	if (obj != NULL)
	{
		CLogWriter* logWriter = static_cast<CLogWriter*>(obj);
		logWriter->SetIgnorePendedWriteOnExit(flag);
	}
}