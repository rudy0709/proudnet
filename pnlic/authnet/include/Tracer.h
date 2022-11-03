﻿/*
ProudNet v1.x.x


이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의 : 저작물에 관한 위의 명시를 제거하지 마십시오.


This program is soley copyrighted by Nettention.
Any use, correction, and distribution of this program are subject to the terms and conditions of the License Agreement.
Any violated use of this program is prohibited and will be cause of immediate termination according to the License Agreement.

** WARNING : PLEASE DO NOT REMOVE THE LEGAL NOTICE ABOVE.


此程序的版权归Nettention公司所有。
与此程序的修改、使用、发布相关的事项要遵守此程序的所有权者的协议。
不遵守协议时要原则性的禁止擅自使用。
擅自使用的责任明示在与此程序所有权者的合同书里。

** 注意：不要移除关于制作物的上述明示。


このプログラムの著作権はNettentionにあります。
このプログラムの修正、使用、配布に関する事項は本プログラムの所有権者との契約に従い、
契約を遵守しない場合、原則的に無断使用を禁じます。
無断使用による責任は本プログラムの所有権者との契約書に明示されています。

** 注意：著作物に関する上記の明示を除去しないでください。

*/

#pragma once
#include "PNString.h"
#include "Enums.h"

//#pragma pack(push,8)

namespace Proud
{
#if (_MSC_VER>=1400)
#pragma managed(push, off)
#endif

	/** \addtogroup util_group
	*  @{
	*/

	/**
	\~korean
	Tracer.h 파일을 Include해야 합니다.
	파일에 로그를 기록하는 모듈입니다. ( \ref logwriter_file  참고)

	일반적 용도
	- CLogWriter.New 를 써서 이 객체를 생성합니다. 생성시 저장할 로그 파일 이름과 새로운 파일을 만들 라인수를 지정합니다.
	- WriteLine, WriteLine를 써서 로그를 기록합니다. 저장된 로그는 비동기로 파일에 저장됩니다.

	\~english
	 Module that records log to a file (Refer \ref logwriter_file)

	General usage
	- Use CLogWriter.New to create this object. Designates log file name when created.
	- Use WriteLine, WriteLine to record log. Saved log is saved in file as asynchronous.

	\~chinese
	往文件记录log的模块。（参考 \ref logwriter_file）

	一般用途
	- 使用 CLogWriter.New%生成此对象。生成的时候指定要保存的log文件名和要创建新文件的行数。
	- 使用WriteLine, WriteLine记录log。保存的log以非同步形式保存到文件。

	\~japanese
	\~
	*/
	class CLogWriter
	{
	public:
		enum eLogType
		{
			eLogTypeNewFile = 0,
			eLogTypeDefault,
			eLogTypeUserDefine
		};

		/**
		\~korean
		\brief 새 CLogWriter 객체를 생성합니다.
		\param logFileName 로그 파일 이름
		\param newFileForLineLimit 로그 줄 수가 여기서 지정한 값 이상으로 커지면 새 로그 파일에 기록하기 시작합니다. 0이면 무제한입니다.
		\param putFileTimeString true를 지정하면 파일명에 현재 시간정보(연월일시분초)를 붙여 파일명 중복을 방지합니다.

		\~english TODO:translate needed.

		\~chinese
		\brief 生成新的 CLogWriter%对象。
		\param logFileName log文件名。
		\param newFileForLineLimit log行数比在这里指定值更多的话，开始记录到新文件里。

		\~japanese
		\~
		*/
		PROUD_API static CLogWriter* New(const String& logFileName, unsigned int newFileForLineLimit = 0, bool putFileTimeString = true);

	protected:
		CLogWriter(); // CLogWriter::New()를 대신 사용하세요!

	public:
		virtual ~CLogWriter();

		/**
		\~korean
		로그 파일을 다른 이름으로 바꿉니다. 기록 중이던 파일은 닫히고 다른 이름으로 바뀐 파일로 새로 열립니다.
		- 이 함수는 비동기로 실행됩니다.  즉, 즉시 리턴됩니다.
		- 만약 로그 파일을 다른 이름으로 바꾸는 것이 실패하면 이 함수는 오류를 리턴하지 않습니다. 그 대신 로그를 기록하는 함수를
		사용할 때 Proud.Exception 예외가 발생할 것입니다.
		\param logFileName 새 로그 파일 이름

		\~english
		Replaces log file name to other name. The file that was being recorded is closed and opened with a new file name.
		- This function is asynchronous so returned immediately.
		- If renaming fails then error will not be returned. Instead, when using the function that records the log will cause 'Proud.Exception' exception.
		\param logFileName new log file name

		\~chinese
		把log文件修改为其他名字。记录中的文件将会关闭，之后打开修改为其他名字的文件。
		- 此函数以非同步实行。即会立即返回。
		- 如果把log文件修改成其他名字失败的话，此函数不会返回错误。但是在使用记录log的函数时会发生 Proud.Exception%例外。
		\param logFileName 新log文件名。

		\~japanese
		\~
		*/
		virtual void SetFileName(const String& logFileName) = 0;

		/**
		\~korean
		로그 한줄을 ProudNet 양식에 맞춰 파일에 기록합니다.
		- 이 함수는 비동기로 실행됩니다. 즉, 즉시 리턴됩니다.
		 -이 함수는 아래 예제처럼 최종 출력 됩니다.
		  logWriteLineTime: logLevel / logCategory(logHostID): logMessage {logFunction(logLine)}
		\param logLevel 로그 내용의 심각도
		\param logCategory 어떤 종류의 로그인지?
		\param logHostID 이 로그를 전송한 호스트
		\param logMessage 로그 메시지
		\param logFunction 로그를 호출한 함수 이름
		\param logLine 로그를 호출한 라인

		\~english
		\~chinese
		\~japanese
		\~
		*/
		virtual void WriteLine(int logLevel, LogCategory logCategory, HostID logHostID, const String& logMessage, const String& logFunction = _PNT(""), int logLine = 0) = 0;

		/**
		\~korean
		로그 한줄을 파일에 기록합니다.
		- 이 함수는 비동기로 실행됩니다. 즉, 즉시 리턴됩니다.
		- 단, void WriteLine(TraceID TID, LPCWSTR text) 와 달리 아무런 헤더를 포함하지 않습니다.
		\param logMessage 로그를 찍을 문자열.
		- String 형태 또는 WriteLine(String::NewFormat(L"Test Log %d", 10)) 형태로 사용 가능합니다.

		\~english
		Writes a line of log to the file
		- This function is asynchronous so returned immediately.
		- But not like void WriteLine(TraceID TID, LPCWSTR text), does not have any header.
		\param logMessage text string to record log.
		- You are able to use string type or WriteLine(String::NewFormat(L"Test Log %d", 10)) type

		\~chinese
		把一行log记录到文件。
		- 此函数以非同步进行。即，会立即返回。
		- 但是，与void WriteLine(TraceID TID, LPCWSTR text)不同并不会包括任何header。
		\param logMessage 记录log的字符串。
		- You are able to use string type or WriteLine(String::NewFormat(L"Test Log %d", 10)) type

		\~japanese
		\~
		*/
		virtual void WriteLine(const String& logMessage) = 0;

		/**
		\~korean
		- CLogWriter 객체를 제거(종료) 할 때 미처 출력하지 못한 로그들을 마저 출력할지 또는 무시할지 여부를 결정합니다.
		- 기본값은 false입니다.
		- true로 설정되면 출력하지 못한 로그가 있어도 무시하고 즉시 종료합니다.
		- false로 설정된 경우 출력하지 못한 로그들을 모두 출력 할 때 까지 종료가 지연됩니다.

		\param flag 이 옵션을 사용할지 여부입니다.

		\~english
		TODO:translate needed.

		\~chinese
		TODO:translate needed.

		\~japanese
		TODO:translate needed.
		\~
		*/
		virtual void SetIgnorePendedWriteOnExit(bool flag) = 0;
	};

	/**  @} */

	String GetFileTimeString();
#if (_MSC_VER>=1400)
#pragma managed(pop)
#endif
}

//#pragma pack(pop)
