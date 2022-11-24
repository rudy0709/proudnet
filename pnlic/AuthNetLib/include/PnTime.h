/*
ProudNet v1


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
#if !defined(_WIN32)
    #include "BasicTypes.h"
#endif

//#pragma pack(push,8)

#ifdef _WIN32
// 윈도 버전이라도 #include <oledb.h>를 include할 수 없는 환경 때문에 전방 선언만 한다.
struct tagDBTIMESTAMP;
typedef tagDBTIMESTAMP DBTIMESTAMP;
#endif // _WIN32

namespace Proud
{
#if defined(_WIN32)

	/** \addtogroup util_group
	*  @{
	*/

	/**
	\~korean
	두 날짜시간의 시간차

	\~english
	Time difference between two different dates

	\~chinese
	两个日期时间的时间差。

	\~japanese
	\~
	*/
	class PROUD_API CPnTimeSpan
	{
		// Constructors
	public:
		/**
		\~korean
		생성자

		\~english
		Constructor

		\~chinese
		生成者。

		\~japanese
		\~
		*/
		CPnTimeSpan() throw();

		/**
		\~korean
		생성자

		\~english
		Constructor

		\~chinese
		生成者。

		\~japanese
		\~
		*/
		CPnTimeSpan(double dblSpanSrc) throw();
		/**
		\~korean
		생성자

		\~english
		Constructor

		\~chinese
		生成者。

		\~japanese
		\~
		*/
		CPnTimeSpan(int32_t lDays, int nHours, int nMins, int nSecs, int nMillisecs) throw();

		// Attributes
		enum DateTimeSpanStatus
		{
			valid = 0,
			invalid = 1,    // Invalid span (out of range, etc.)
			null = 2,       // Literally has no value
		};

		double m_span;
		DateTimeSpanStatus m_status;

		void SetStatus(DateTimeSpanStatus status) throw();
		DateTimeSpanStatus GetStatus() const throw();

		/**
		\~korean
		일,시,분,초,밀리초로 총계 환산한다.

		\~english
		Converts into day,hour,minute,second,millisecond format.

		\~chinese
		用日、时、分、秒、毫秒 总计换算。

		\~japanese
		\~
		*/
		double GetTotalDays() const throw();    // span in days (about -3.65e6 to 3.65e6)

		/**
		\~korean
		일,시,분,초,밀리초로 총계 환산한다.

		\~english
		Converts into day,hour,minute,second,millisecond format.

		\~chinese
		用日、时、分、秒、毫秒 总计换算。

		\~japanese
		\~		*/
		double GetTotalHours() const throw();   // span in hours (about -8.77e7 to 8.77e6)

		/**
		\~korean
		일,시,분,초,밀리초로 총계 환산한다.

		\~english
		Converts into day,hour,minute,second,millisecond format.

		\~chinese
		用日、时、分、秒、毫秒 总计换算。

		\~japanese
		\~
		*/
		double GetTotalMinutes() const throw(); // span in minutes (about -5.26e9 to 5.26e9)

		/**
		\~korean
		일,시,분,초,밀리초로 총계 환산한다.

		\~english
		Converts into day,hour,minute,second,millisecond format.

		\~chinese
		用日、时、分、秒、毫秒 总计换算。

		\~japanese
		\~
		*/
		double GetTotalSeconds() const throw(); // span in seconds (about -3.16e11 to 3.16e11)

		/**
		\~korean
		일,시,분,초,밀리초로 총계 환산한다.

		\~english
		Converts into day,hour,minute,second,millisecond format.

		\~chinese
		用日、时、分、秒、毫秒 总计换算。

		\~japanese
		\~
		*/
		double GetTotalMilliseconds() const throw();	// span in milliseconds

		/**
		\~korean
		일,시,분,초를 얻는다.

		\~english
		Gets day,hour,minute,second.

		\~chinese
		获得日、时、分、秒。

		\~japanese
		\~
		*/
		int32_t GetDays() const throw();       // component days in span

		/**
		\~korean
		일,시,분,초를 얻는다.

		\~english
		Gets day,hour,minute,second.

		\~chinese
		获得日、时、分、秒。

		\~japanese
		\~
		*/
		int32_t GetHours() const throw();      // component hours in span (-23 to 23)

		/**
		\~korean
		일,시,분,초를 얻는다.

		\~english
		Gets day,hour,minute,second.

		\~chinese
		获得日、时、分、秒。

		\~japanese
		\~
		*/
		int32_t GetMinutes() const throw();    // component minutes in span (-59 to 59)

		/**
		\~korean
		일,시,분,초를 얻는다.

		\~english
		Gets day,hour,minute,second.

		\~chinese
		获得日、时、分、秒。

		\~japanese
		\~
		*/
		int32_t GetSeconds() const throw();    // component seconds in span (-59 to 59)

		int GetMilliseconds() const throw();

		/**
		\~korean
		복사 대입 연산자

		\~english
		Copy assignment operator

		\~chinese
		复制赋值运算符。

		\~japanese
		\~
		*/
		CPnTimeSpan& operator=(double dblSpanSrc) throw();

		/**
		\~korean
		비교 연산자

		\~english
		Comparison operator

		\~chinese
		比较运算符。

		\~japanese
		\~
		*/
		bool operator==(const CPnTimeSpan& dateSpan) const throw();
		/**
		\~korean
		비교 연산자

		\~english
		Comparison operator

		\~chinese
		比较运算符。

		\~japanese
		\~
		*/
		bool operator!=(const CPnTimeSpan& dateSpan) const throw();
		/**
		\~korean
		비교 연산자

		\~english
		Comparison operator

		\~chinese
		比较运算符。

		\~japanese
		\~
		*/
		bool operator<(const CPnTimeSpan& dateSpan) const throw();

		/**
		\~korean
		비교 연산자

		\~english
		Comparison operator

		\~chinese
		比较运算符。

		\~japanese
		\~
		*/
		bool operator>(const CPnTimeSpan& dateSpan) const throw();

		/**
		\~korean
		비교 연산자

		\~english
		Comparison operator

		\~chinese
		比较运算符。

		\~japanese
		\~
		*/
		bool operator<=(const CPnTimeSpan& dateSpan) const throw();

		/**
		\~korean
		비교 연산자

		\~english
		Comparison operator

		\~chinese
		比较运算符。

		\~japanese
		\~
		*/
		bool operator>=(const CPnTimeSpan& dateSpan) const throw();

		/**
		\~korean
		날짜 사칙연산자

		\~english
		Four-fundamental operator for date

		\~chinese
		日期四则运算符。

		\~japanese
		\~
		*/
		CPnTimeSpan operator+(const CPnTimeSpan& dateSpan) const throw();

		/**
		\~korean
		날짜 사칙연산자

		\~english
		Four-fundamental operator for date

		\~chinese
		日期四则运算符。

		\~japanese
		\~
		*/
		CPnTimeSpan operator-(const CPnTimeSpan& dateSpan) const throw();

		/**
		\~korean
		날짜 사칙연산자

		\~english
		Four-fundamental operator for date

		\~chinese
		日期四则运算符。

		\~japanese
		\~
		*/
		CPnTimeSpan& operator+=(const CPnTimeSpan dateSpan) throw();

		/**
		\~korean
		날짜 사칙연산자

		\~english
		Four-fundamental operator for date

		\~chinese
		日期四则运算符。

		\~japanese
		\~
		*/
		CPnTimeSpan& operator-=(const CPnTimeSpan dateSpan) throw();

		/**
		\~korean
		날짜 사칙연산자

		\~english
		Four-fundamental operator for date

		\~chinese
		日期四则运算符。

		\~japanese
		\~
		*/
		CPnTimeSpan operator-() const throw();

//		operator double() const throw();

		/**
		\~korean
		날짜시간 설정

		\~english
		Setting date and time

		\~chinese
		设置日期时间。

		\~japanese
		\~
		*/
		void SetDateTimeSpan(int32_t lDays, int nHours, int nMins, int nSecs, int nMillisecs) throw();

		/**
		\~korean
		날짜시간을 문자열로 만든다.

		\~english
		Converts date and time into text string

		\~chinese
		把日期时间转换成字符串。

		\~japanese
		\~
		*/
		String Format(const TCHAR *pFormat) const;

		/**
		\~korean
		날짜시간을 문자열로 만든다.

		\~english
		Converts date and time into text string

		\~chinese
		把日期时间转换成字符串。

		\~japanese
		\~
		*/
		String Format(uint32_t nID) const;

		// Implementation
		void CheckRange();

	private:
		static const double OLE_DATETIME_HALFSECOND;
	};

	/**
	\~korean
	날짜시간 데이터 타입

	\~english
	Date and time data type

	\~chinese
	日期时间数据类型。

	\~japanese
	\~
	*/
	class PROUD_API CPnTime  // CTime이나 CDateTime, CPnTime은 ATL 등과 이름이 혼동될 수 있으므로
	{
		// Constructors
	public:
		static CPnTime GetCurrentTime() throw();

		/**
		\~korean
		생성자

		\~english
		Constructor

		\~chinese
		生成者。

		\~japanese
		\~
		*/
		CPnTime() throw();

		/**
		\~korean
		생성자

		\~english
		Constructor

		\~chinese
		生成者。

		\~japanese
		\~
		*/
		CPnTime(const VARIANT& varSrc) throw();
		//CPnTime(DATE dtSrc) throw();

		/*
		\~korean
		생성자

		\~english
		Constructor

		\~chinese
		生成者。

		\~japanese
		\~
		*/
		// 		CPnTime(__time32_t timeSrc) throw();

		/**
		\~korean
		생성자

		\~english
		Constructor

		\~chinese
		生成者。

		\~japanese
		\~
		*/
		//CPnTime(__time64_t timeSrc) throw();

		/**
		\~korean
		생성자

		\~english
		Constructor

		\~chinese
		生成者。

		\~japanese
		\~
		*/
		CPnTime(const SYSTEMTIME& systimeSrc) throw();

		/**
		\~korea
		생성자

		\~english
		Constructor

		\~chinese
		生成者。

		\~japanese
		\~
		*/
		CPnTime(const FILETIME& filetimeSrc) throw();

		/**
		\~korean
		생성자

		\~english
		Constructor

		\~chinese
		生成者。

		\~japanese
		\~
		*/
		CPnTime(int nYear, int nMonth, int nDay,
			int nHour, int nMin, int nSec, int nMillisec) throw();

		/**
		\~korean
		생성자

		\~english
		Constructor

		\~chinese
		生成者。

		\~japanese
		\~
		*/
		CPnTime(uint16_t wDosDate, uint16_t wDosTime) throw();

		/**
		\~korean
		DATE 타입으로부터 이 객체를 생성

		\~english
		Creates this object from date type

		\~chinese
		从DATE类型生成此对象。

		\~japanese
		\~
		*/
		static CPnTime FromDATE(DATE dtSrc);

#ifdef _WIN32
		CPnTime( const DBTIMESTAMP& dbts) throw();
		bool GetAsDBTIMESTAMP( DBTIMESTAMP& dbts ) const throw();
#endif // _WIN32

		/**
		\~korean
		날짜시간의 유효 상태

		\~english
		Valid state of date and time

		\~chinese
		日期时间的有效状态。

		\~japanese
		\~
		*/
		enum DateTimeStatus
		{
			/**
			\~korean
			에러

			\~english
			Error

			\~chinese
			错误。

			\~japanese
			\~
			*/
			error = -1,

			/**
			\~korean
			유효

			\~english
			Valid

			\~chinese
			有效。

			\~japanese
			\~
			*/
			valid = 0,

			/**
			\~korean
			잘못된 날짜 (범위 초과 등)

			\~english
			Invalid date (out of range, etc.)

			\~chinese
			错误的日期（超出范围等）

			\~japanese

			\~

			*/
			invalid = 1,

			/**
			\~korean
			말 그대로 값이 없음.

			\~english
			Literally has no value

			\~chinese
			不存在值

			\~japanese

			\~

			*/
			null = 2
		};

		DATE m_dt;
		DateTimeStatus m_status;

		void SetStatus(DateTimeStatus status) throw();
		DateTimeStatus GetStatus() const throw();

		bool GetAsSystemTime(SYSTEMTIME& sysTime) const throw();
		bool GetAsUDATE( UDATE& udate ) const throw();

		/**
		\~korean
		연도를 얻는다.

		\~english
		Gets year

		\~chinese
		获得年度。

		\~japanese
		\~
		*/
		int GetYear() const throw();

		/**
		\~korean
		월 값을 얻는다.

		\~english
		Gets month value

		\~chinese
		获得月值。

		\~japanese
		\~
		*/
		int GetMonth() const throw();

		/**
		\~korean
		일 값을 얻는다.

		\~english
		Gets day value

		\~chinese
		获得日值。

		\~japanese
		\~
		*/
		int GetDay() const throw();

		/**
		\~korean
		시 값을 얻는다.

		\~english
		Gets hour value

		\~chinese
		获得时值。

		\~japanese
		\~
		*/
		int GetHour() const throw();

		/**
		\~korean
		분 값을 얻는다.

		\~english
		Gets minute value

		\~chinese
		获得分值。

		\~japanese
		\~
		*/
		int GetMinute() const throw();

		/**
		\~korean
		초 값을 얻는다.

		\~english
		Gets second value

		\~chinese
		获得秒值。

		\~japanese
		\~
		*/
		int GetSecond() const throw();

		/**
		\~korean
		 밀리초 값을 얻는다.

		\~english
		Gets millisecond value

		\~chinese
		获得毫秒值。

		\~japanese
		\~
		*/
		int GetMillisecond() const throw();

		/**
		\~korean
		요일 값을 얻는다. (1: 일요일, 2: 월요일...)

		\~english
		Gets day of the week value (1: Sunday, 2: Monday...)

		\~chinese
		获得星期值。（1：星期日、2：星期一。。。）

		\~japanese
		\~
		*/
		int GetDayOfWeek() const throw();

		/**
		\~korean
		1년중 몇번째 날인지를 얻는다. (1: 1월 1일)

		\~english
		Gets which day in a year (1: 1st January)

		\~chinese
		获得是一年中的哪一天。（1：一月一日）

		\~japanese
		\~
		*/
		int GetDayOfYear() const throw();

		/**
		\~korean
		복사 대입 연산자

		\~english
		Copy assignment operator

		\~chinese
		复制赋值运算符。

		\~japanese
		\~
		*/
		CPnTime& operator=(const VARIANT& varSrc) throw();
		//CPnTime& operator=(DATE dtSrc) throw();

//
		/*
		\~korean
		복사 대입 연산자

		\~english
		Copy assignment operator

		\~chinese
		复制赋值运算符。
		*/
// 		CPnTime& operator=(const __time32_t& timeSrc) throw();

		/**
		\~korean
		복사 대입 연산자

		\~english
		Copy assignment operator

		\~chinese
		复制赋值运算符。

		\~japanese
		\~
		*/
//		CPnTime& operator=(const __time64_t& timeSrc) throw();

		/**
		\~korean
		복사 대입 연산자

		\~english
		Copy assignment operator

		\~chinese
		复制赋值运算符。

		\~japanese
		\~
		*/
		CPnTime& operator=(const SYSTEMTIME& systimeSrc) throw();

		/**
		\~korean
		복사 대입 연산자

		\~english
		Copy assignment operator

		\~chinese
		复制赋值运算符。

		\~japanese
		\~
		*/
		CPnTime& operator=(const FILETIME& filetimeSrc) throw();

		/**
		\~korean
		복사 대입 연산자

		\~english
		Copy assignment operator

		\~chinese
		复制赋值运算符。

		\~japanese
		\~
		*/
		CPnTime& operator=(const UDATE& udate) throw();

		/**
		\~korean
		비교 연산자

		\~english
		Comparison operator

		\~chinese
		比较运算符。

		\~japanese
		\~
		*/
		bool operator==(const CPnTime& date) const throw();

		/**
		\~korean
		비교 연산자

		\~english
		Comparison operator

		\~chinese
		比较运算符。

		\~japanese
		\~
		*/
		bool operator!=(const CPnTime& date) const throw();

		/**
		\~korean
		비교 연산자

		\~english
		Comparison operator

		\~chinese
		比较运算符。

		\~japanese
		\~
		*/
		bool operator<(const CPnTime& date) const throw();

		/**
		\~korean
		비교 연산자

		\~english
		Comparison operator

		\~chinese
		比较运算符。

		\~japanese
		\~
		*/
		bool operator>(const CPnTime& date) const throw();

		/**
		\~korean
		비교 연산자

		\~english
		Comparison operator

		\~chinese
		比较运算符。

		\~japanese
		\~
		*/
		bool operator<=(const CPnTime& date) const throw();

		/**
		\~korean
		비교 연산자

		\~english
		Comparison operator

		\~chinese
		比较运算符。

		\~japanese
		\~
		*/
		bool operator>=(const CPnTime& date) const throw();

		/**
		\~korean
		날짜 사칙연산자

		\~english
		Four-fundamental operator for date

		\~chinese
		日期四则运算符。

		\~japanese
		\~
		*/
		CPnTime operator+(CPnTimeSpan dateSpan) const throw();

		/**
		\~korean
		날짜 사칙연산자

		\~english
		Four-fundamental operator for date

		\~chinese
		日期四则运算符。

		\~japanese
		\~
		*/
		CPnTime operator-(CPnTimeSpan dateSpan) const throw();

		/**
		\~korean
		날짜 사칙연산자

		\~english
		Four-fundamental operator for date

		\~chinese
		日期四则运算符。

		\~japanese
		\~
		*/
		CPnTime& operator+=(CPnTimeSpan dateSpan) throw();

		/**
		\~korean
		날짜 사칙연산자

		\~english
		Four-fundamental operator for date

		\~chinese
		日期四则运算符。

		\~japanese
		\~
		*/
		CPnTime& operator-=(CPnTimeSpan dateSpan) throw();

		/**
		\~korean
		날짜 사칙연산자

		\~english
		Four-fundamental operator for date

		\~chinese
		日期四则运算符。

		\~japanese
		\~
		*/
		CPnTimeSpan operator-(const CPnTime& date) const throw();

		//operator DATE() const throw();

		/**
		\~korean
		날짜시간 설정

		\~english
		Setting date and time

		\~chinese
		设置日期时间。

		\~japanese
		\~
		*/
		int SetDateTime(int nYear, int nMonth, int nDay,
			int nHour, int nMin, int nSec, int nMillisec) throw();

		/**
		\~korean
		날짜시간 설정

		\~english
		Setting date and time

		\~chinese
		设置日期时间。

		\~japanese
		\~
		*/
		int SetDate(int nYear, int nMonth, int nDay) throw();

		/**
		\~korean
		날짜시간 설정

		\~english
		Setting date and time

		\~chinese
		设置日期时间。

		\~japanese
		\~
		*/
		int SetTime(int nHour, int nMin, int nSec, int nMillisec) throw();

		/**
		\~korean
		날짜시간을 문자열로부터 얻는다.

		\~english
		Gets date and time from text string

		\~chinese
		从字符串获得日期时间。

		\~japanese
		\~
		*/
		bool ParseDateTime(const TCHAR* lpszDate, uint32_t dwFlags = 0,
			LCID lcid = LANG_USER_DEFAULT) throw();

		/**
		\~korean
		날짜시간을 문자열로 만든다.

		\~english
		Converts date and time into text string

		\~chinese
		把日期时间转换成字符串。

		\~japanese
		\~
		*/
		String Format(uint32_t dwFlags = 0, LCID lcid = LANG_USER_DEFAULT) const;

		/**
		\~korean
		날짜시간을 문자열로 만든다.

		\~english
		Converts date and time into text string

		\~chinese
		把日期时间转换成字符串。

		\~japanese
		\~
		*/
		String Format(const TCHAR* lpszFormat) const;

		/**
		\~korean
		날짜시간을 문자열로 만든다.

		\~english
		Converts date and time into text string

		\~chinese
		把日期时间转换成字符串。

		\~japanese
		\~
		*/
		String Format(uint32_t nFormatID) const;


	protected:
		static double DoubleFromDate( DATE date ) throw();
		static DATE DateFromDouble( double f ) throw();

		void CheckRange();
		BOOL ConvertSystemTimeToVariantTime(const SYSTEMTIME& systimeSrc);

	};

	/**  @} */
#endif
}

//#pragma pack(pop)
