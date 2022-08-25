#pragma once

//////////////////////////////////////////////////////////////////////////
// 이 파일은 SWIG에 의해 분석되어 .cs 등의 래퍼 모듈로 변환되는 데 사용된다.

#include "NativeType.h"

extern bool CopyNativeDataToManageByteArray(void* dst, void* src, int length);
extern bool CopyNativeByteArrayToManageByteArray(void* dst, Proud::ByteArray* src);

extern bool ManageByteArrayToCopyNativeByteArray(Proud::ByteArray* dst, uint8_t* src, int length);

// Minidump
extern Proud::MiniDumpAction StartUpDump(Proud::String dumpFileName, int miniDumpType);

extern void ManualFullDump();

extern void ManualMiniDump();

extern void WriteDumpFromHere(Proud::String fileName, bool fullDump);

// LogWriter
extern void* LogWriter_Create(Proud::String logFileName, int newFileForLineLimit, bool putFileTimeString);
extern void LogWriter_Destory(void* obj);
extern void LogWriter_SetFileName(void* obj, Proud::String logFileName);
extern void LogWriter_WriteLine(void* obj, int logLevel, int logCategory, int logHostID, Proud::String logMessage, Proud::String logFunction, int logLine);
extern void LogWriter_WriteLine(void* obj, Proud::String logMessage);
extern void LogWriter_SetIgnorePendedWriteOnExit(void* obj, bool flag);
