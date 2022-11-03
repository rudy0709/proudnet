#include "stdafx.h"
#include "../include/PnTime.h"
#include "../include/sysutil.h"

#if defined(_WIN32)
#include <time.h>
#include <oledb.h>
#include <tchar.h>
#include <math.h>
#endif

namespace Proud 
{
#if defined(_WIN32)
	const int maxTimeBufferSize = 128;
	const int maxDaysInSpan  =	3615897L;
	const PNTCHAR* szInvalidDateTime = _PNT("Invalid DateTime");
	const PNTCHAR* szInvalidDateTimeSpan = _PNT("Invalid DateTimeSpan");

	CPnTimeSpan::CPnTimeSpan() throw() : m_span(0), m_status(valid)
	{
	}

	CPnTimeSpan::CPnTimeSpan(double dblSpanSrc) throw() : m_span(dblSpanSrc), m_status(valid)
	{
		CheckRange();
	}

	CPnTimeSpan::CPnTimeSpan(int32_t lDays, int nHours, int nMins, int nSecs, int nMillisecs) throw()
	{
		SetDateTimeSpan(lDays, nHours, nMins, nSecs, nMillisecs);
	}

	void CPnTimeSpan::SetStatus(DateTimeSpanStatus status) throw()
	{
		m_status = status;
	}

	CPnTimeSpan::DateTimeSpanStatus CPnTimeSpan::GetStatus() const throw()
	{
		return m_status;
	}

	__declspec(selectany) const double
		CPnTimeSpan::OLE_DATETIME_HALFSECOND =
		1.0 / (2.0 * (60.0 * 60.0 * 24.0));

	double CPnTimeSpan::GetTotalDays() const throw()
	{
		assert(GetStatus() == valid);
		return (double)LONGLONG(m_span + (m_span < 0 ?
			-OLE_DATETIME_HALFSECOND : OLE_DATETIME_HALFSECOND));
	}

	double CPnTimeSpan::GetTotalHours() const throw()
	{
		assert(GetStatus() == valid);
		return (double)LONGLONG((m_span + (m_span < 0 ? 
			-OLE_DATETIME_HALFSECOND : OLE_DATETIME_HALFSECOND)) * 24);
	}

	double CPnTimeSpan::GetTotalMinutes() const throw()
	{
		assert(GetStatus() == valid);
		return (double)LONGLONG((m_span + (m_span < 0 ?
			-OLE_DATETIME_HALFSECOND : OLE_DATETIME_HALFSECOND)) * (24 * 60));
	}

	double CPnTimeSpan::GetTotalSeconds() const throw()
	{
		assert(GetStatus() == valid);
		return (double)LONGLONG((m_span + (m_span < 0 ?
			-OLE_DATETIME_HALFSECOND : OLE_DATETIME_HALFSECOND)) * (24 * 60 * 60));
	}

	double CPnTimeSpan::GetTotalMilliseconds() const throw()
	{
		assert(GetStatus() == valid);
		return (double)LONGLONG((m_span + (m_span < 0 ?
			-OLE_DATETIME_HALFSECOND : OLE_DATETIME_HALFSECOND)) * (24 * 60 * 60 * 1000));
	}

	int32_t CPnTimeSpan::GetDays() const throw()
	{
		assert(GetStatus() == valid);
		return int32_t(GetTotalDays());
	}

	int32_t CPnTimeSpan::GetHours() const throw()
	{
		return int32_t(GetTotalHours()) % 24;
	}

	int32_t CPnTimeSpan::GetMinutes() const throw()
	{
		return int32_t(GetTotalMinutes()) % 60;
	}

	int32_t CPnTimeSpan::GetSeconds() const throw()
	{
		return int32_t(GetTotalSeconds()) % 60;
	}

	int CPnTimeSpan::GetMilliseconds() const throw()
	{
		return int(GetTotalMilliseconds()) % 1000;
	}

	CPnTimeSpan& CPnTimeSpan::operator=(double dblSpanSrc) throw()
	{
		m_span = dblSpanSrc;
		m_status = valid;
		CheckRange();
		return *this;
	}

	bool CPnTimeSpan::operator==(const CPnTimeSpan& dateSpan) const throw()
	{
		if(GetStatus() == dateSpan.GetStatus())
		{
			if(GetStatus() == valid)
				return (m_span == dateSpan.m_span);			

			return (GetStatus() == null);
		}

		return false;
	}

	bool CPnTimeSpan::operator!=(const CPnTimeSpan& dateSpan) const throw()
	{
		return !operator==(dateSpan);
	}

	bool CPnTimeSpan::operator<(const CPnTimeSpan& dateSpan) const throw()
	{
		assert(GetStatus() == valid);
		assert(dateSpan.GetStatus() == valid);
		if( (GetStatus() == valid) && (GetStatus() == dateSpan.GetStatus()) )
			return m_span < dateSpan.m_span;

		return false;
	}

	bool CPnTimeSpan::operator>(const CPnTimeSpan& dateSpan) const throw()
	{
		assert(GetStatus() == valid);
		assert(dateSpan.GetStatus() == valid);
		if( (GetStatus() == valid) && (GetStatus() == dateSpan.GetStatus()) )
			return m_span > dateSpan.m_span ;

		return false;
	}

	bool CPnTimeSpan::operator<=(const CPnTimeSpan& dateSpan) const throw()
	{
		return operator<(dateSpan) || operator==(dateSpan);
	}

	bool CPnTimeSpan::operator>=(const CPnTimeSpan& dateSpan) const throw()
	{
		return operator>(dateSpan) || operator==(dateSpan);
	}

	CPnTimeSpan CPnTimeSpan::operator+(const CPnTimeSpan& dateSpan) const throw()
	{
		CPnTimeSpan dateSpanTemp;

		// If either operand Null, result Null
		if (GetStatus() == null || dateSpan.GetStatus() == null)
		{
			dateSpanTemp.SetStatus(null);
			return dateSpanTemp;
		}

		// If either operand Invalid, result Invalid
		if (GetStatus() == invalid || dateSpan.GetStatus() == invalid)
		{
			dateSpanTemp.SetStatus(invalid);
			return dateSpanTemp;
		}

		// Add spans and validate within legal range
		dateSpanTemp.m_span = m_span + dateSpan.m_span;
		dateSpanTemp.CheckRange();

		return dateSpanTemp;
	}

	CPnTimeSpan CPnTimeSpan::operator-(const CPnTimeSpan& dateSpan) const throw()
	{
		CPnTimeSpan dateSpanTemp;

		// If either operand Null, result Null
		if (GetStatus() == null || dateSpan.GetStatus() == null)
		{
			dateSpanTemp.SetStatus(null);
			return dateSpanTemp;
		}

		// If either operand Invalid, result Invalid
		if (GetStatus() == invalid || dateSpan.GetStatus() == invalid)
		{
			dateSpanTemp.SetStatus(invalid);
			return dateSpanTemp;
		}

		// Subtract spans and validate within legal range
		dateSpanTemp.m_span = m_span - dateSpan.m_span;
		dateSpanTemp.CheckRange();

		return dateSpanTemp;
	}

	CPnTimeSpan& CPnTimeSpan::operator+=(const CPnTimeSpan dateSpan) throw()
	{
		assert(GetStatus() == valid);
		assert(dateSpan.GetStatus() == valid);
		*this = *this + dateSpan;
		CheckRange();
		return *this;
	}

	CPnTimeSpan& CPnTimeSpan::operator-=(const CPnTimeSpan dateSpan) throw()
	{
		assert(GetStatus() == valid);
		assert(dateSpan.GetStatus() == valid);
		*this = *this - dateSpan;
		CheckRange();
		return *this;
	}

	CPnTimeSpan CPnTimeSpan::operator-() const throw()
	{
		return -this->m_span;
	}

// 	CPnTimeSpan::operator double() const throw()
// 	{
// 		return m_span;
// 	}

	void CPnTimeSpan::SetDateTimeSpan(int32_t lDays, int nHours, int nMins, int nSecs, int nMillisecs) throw()
	{
		// Set date span by breaking into fractional days (all input ranges valid)
		m_span = lDays + ((double)nHours)/24 + ((double)nMins)/(24*60) +
			((double)nSecs)/(24*60*60) + ((double)nMillisecs)/(24*60*60*1000);
		m_status = valid;
		CheckRange();
	}

	void CPnTimeSpan::CheckRange()
	{
		if(m_span < -maxDaysInSpan || m_span > maxDaysInSpan)
			m_status = invalid;
	}

	CPnTime CPnTime::GetCurrentTime() throw()
	{
		SYSTEMTIME st;
		GetSystemTime(&st);

		CPnTime time;
		time.m_status = time.ConvertSystemTimeToVariantTime(st) ? valid : invalid;

		return time;
	}

	CPnTime::CPnTime( const VARIANT& varSrc ) throw() :
	m_dt( 0 ), m_status(valid)
	{
		*this = varSrc;
	}

	
	CPnTime CPnTime::FromDATE(DATE dtSrc)
	{
		CPnTime ret;
		ret.m_dt = dtSrc;
		ret.m_status= valid;
		return ret;
	}

	CPnTime::CPnTime() throw() :
	m_dt( 0 ), m_status(valid)
	{}

	CPnTime::CPnTime( const SYSTEMTIME& systimeSrc ) throw() :
	m_dt( 0 ), m_status(valid)
	{
		*this = systimeSrc;
	}

	CPnTime::CPnTime( const FILETIME& filetimeSrc ) throw() :
	m_dt( 0 ), m_status(valid)
	{
		*this = filetimeSrc;
	}

	CPnTime::CPnTime(int nYear, int nMonth, int nDay,
		int nHour, int nMin, int nSec, int nMsec) throw()
	{
		SetDateTime(nYear, nMonth, nDay, nHour, nMin, nSec, nMsec);
	}

	CPnTime::CPnTime(uint16_t wDosDate, uint16_t wDosTime) throw()
	{
		m_status = ::DosDateTimeToVariantTime(wDosDate, wDosTime, &m_dt) ?
valid : invalid;
	}

	void CPnTime::SetStatus(DateTimeStatus status) throw()
	{
		m_status = status;
	}

	CPnTime::DateTimeStatus CPnTime::GetStatus() const throw()
	{
		return m_status;
	}

	bool VariantTimeToSystemTimeMs( const double dt, SYSTEMTIME& sysTime )  
	{
		//http://www.codeproject.com/Articles/17576/SystemTime-to-VariantTime-with-Milliseconds
		BOOL retVal = true;
		double dValue;

		retVal = VariantTimeToSystemTime(dt, &sysTime);

		if(!retVal)
			return false;

		dValue = dt - static_cast<double>(static_cast<int>(dt));

		sysTime.wHour = static_cast<WORD>(dValue*24);
		dValue -= static_cast<double>(sysTime.wHour)/static_cast<double>(24);

		sysTime.wMinute = static_cast<WORD>(dValue*24*60);
		dValue -= static_cast<double>(sysTime.wMinute)/static_cast<double>(24*60);

		sysTime.wSecond = static_cast<WORD>(dValue*24*60*60);
		dValue -= static_cast<double>(sysTime.wSecond)/static_cast<double>(24*60*60);

		sysTime.wMilliseconds = static_cast<WORD>(dValue*24*60*60*1000+0.5);

		double ms = sysTime.wMilliseconds;
		if(ms < 1.0 || ms > 999.0)
			ms = 0;

		if(ms)
			sysTime.wMilliseconds = (WORD)ms;
		else
			retVal = VariantTimeToSystemTime(dt, &sysTime);

		return true;
	}

	bool SystemTimeToVariantTimeMs( const SYSTEMTIME& systime ,double* dt )
	{
		// http://www.codeproject.com/Articles/17576/SystemTime-to-VariantTime-with-Milliseconds
		double dMillsondsPerDay;
		
		SystemTimeToVariantTime(const_cast<SYSTEMTIME*>(&systime), dt);
		dMillsondsPerDay = static_cast<double>(systime.wMilliseconds)/static_cast<double>(24*60*60*1000);
		(*dt) += dMillsondsPerDay;

		return true;
	}

	bool CPnTime::GetAsSystemTime(SYSTEMTIME& sysTime) const throw()
	{
		VariantTimeToSystemTimeMs(m_dt, sysTime);
		return GetStatus() == valid; //&& ::VariantTimeToSystemTime(m_dt, &sysTime);
	}

	bool CPnTime::GetAsUDATE(UDATE &udate) const throw()
	{
		return SUCCEEDED(::VarUdateFromDate(m_dt, 0, &udate));
	}

	int CPnTime::GetYear() const throw()
	{
		SYSTEMTIME st;
		return GetAsSystemTime(st) ? st.wYear : error;
	}

	int CPnTime::GetMonth() const throw()
	{
		SYSTEMTIME st;
		return GetAsSystemTime(st) ? st.wMonth : error;
	}

	int CPnTime::GetDay() const throw()
	{
		SYSTEMTIME st;
		return GetAsSystemTime(st) ? st.wDay : error;
	}

	int CPnTime::GetHour() const throw()
	{
		SYSTEMTIME st;
		return GetAsSystemTime(st) ? st.wHour : error;
	}

	int CPnTime::GetMinute() const throw()
	{
		SYSTEMTIME st;
		return GetAsSystemTime(st) ? st.wMinute : error;
	}

	int CPnTime::GetSecond() const throw()
	{ 
		SYSTEMTIME st;
		return GetAsSystemTime(st) ? st.wSecond : error;
	}

	int CPnTime::GetMillisecond() const throw()
	{
		SYSTEMTIME st;
		return GetAsSystemTime(st) ? st.wMilliseconds : error;
	}

	int CPnTime::GetDayOfWeek() const throw()
	{
		SYSTEMTIME st;
		return GetAsSystemTime(st) ? st.wDayOfWeek + 1 : error;
	}

	int CPnTime::GetDayOfYear() const throw()
	{
		UDATE udate;
		return GetAsUDATE(udate) ? udate.wDayOfYear : error;
	}

	CPnTime& CPnTime::operator=(const VARIANT& varSrc) throw()
	{
		if (varSrc.vt != VT_DATE)
		{
			VARIANT varDest;
			varDest.vt = VT_EMPTY;
			if(SUCCEEDED(::VariantChangeType(&varDest, const_cast<VARIANT *>(&varSrc), 0, VT_DATE)))
			{
				m_dt = varDest.date;
				m_status = valid;
			}
			else
				m_status = invalid;
		}
		else
		{
			m_dt = varSrc.date;
			m_status = valid;
		}

		return *this;
	}

// 	CPnTime& CPnTime::operator=(DATE dtSrc) throw()
// 	{
// 		m_dt = dtSrc;
// 		m_status = valid;
// 		return *this;
// 	}
// 
// 	CPnTime& CPnTime::operator=(const __time32_t& timeSrc) throw()
// 	{
// 		return operator=(static_cast<__time64_t>(timeSrc));
// 	}

// 	CPnTime& CPnTime::operator=(const __time64_t& timeSrc) throw()
// 	{
		//struct tm dumpTime;
		//errno_t err = _localtime64_s(&dumpTime, &timeSrc);
		//if (err != 0)
		//{
		//	m_status = invalid;
		//	return *this;
		//}

		//SYSTEMTIME systemTime;
		//systemTime.wYear = (WORD)(1900 + dumpTime.tm_year);
		//systemTime.wMonth = (WORD)(1 + dumpTime.tm_mon);
		//systemTime.wDayOfWeek = (WORD)dumpTime.tm_wday;
		//systemTime.wDay = (WORD)dumpTime.tm_mday;
		//systemTime.wHour = (WORD)dumpTime.tm_hour;
		//systemTime.wMinute = (WORD)dumpTime.tm_min;
		//systemTime.wSecond = (WORD)dumpTime.tm_sec;
		//systemTime.wMilliseconds = 0;

		//m_status = ConvertSystemTimeToVariantTime(systemTime) == TRUE ? valid : invalid;

		//return *this;
// 	}

	CPnTime &CPnTime::operator=(const SYSTEMTIME &systimeSrc) throw()
	{
		m_status = ConvertSystemTimeToVariantTime(systimeSrc) ?	valid : invalid;
		return *this;
	}

	CPnTime &CPnTime::operator=(const FILETIME &filetimeSrc) throw()
	{
		FILETIME ftl;
		SYSTEMTIME st;

		m_status =  ::FileTimeToLocalFileTime(&filetimeSrc, &ftl) && 
			::FileTimeToSystemTime(&ftl, &st) &&
			ConvertSystemTimeToVariantTime(st) ? valid : invalid;

		return *this;
	}

	inline BOOL PnConvertSystemTimeToVariantTime(const SYSTEMTIME& systimeSrc,double* pVarDtTm)
	{
		assert(pVarDtTm!=NULL);
		//Convert using ::SystemTimeToVariantTime and store the result in pVarDtTm then
		//convert variant time back to system time and compare to original system time.	
//		BOOL ok = ::SystemTimeToVariantTime(const_cast<SYSTEMTIME*>(&systimeSrc), pVarDtTm);
		BOOL ok = SystemTimeToVariantTimeMs(systimeSrc, pVarDtTm);
		SYSTEMTIME sysTime;
		::ZeroMemory(&sysTime, sizeof(SYSTEMTIME));

		ok = ok && VariantTimeToSystemTimeMs(*pVarDtTm, sysTime);
		ok = ok && (systimeSrc.wYear == sysTime.wYear &&
			systimeSrc.wMonth == sysTime.wMonth &&
			systimeSrc.wDay == sysTime.wDay &&
			systimeSrc.wHour == sysTime.wHour &&
			systimeSrc.wMinute == sysTime.wMinute && 
			systimeSrc.wSecond == sysTime.wSecond &&
			systimeSrc.wMilliseconds == sysTime.wMilliseconds);

		return ok;
	}

	BOOL CPnTime::ConvertSystemTimeToVariantTime(const SYSTEMTIME& systimeSrc)
	{
		return PnConvertSystemTimeToVariantTime(systimeSrc,&m_dt);	
	}
	CPnTime &CPnTime::operator=(const UDATE &udate) throw()
	{
		m_status = (S_OK == VarDateFromUdate((UDATE*)&udate, 0, &m_dt)) ? valid : invalid;

		return *this;
	}

	bool CPnTime::operator==( const CPnTime& date ) const throw()
	{
		if(GetStatus() == date.GetStatus())
		{
			if(GetStatus() == valid)
				return( m_dt == date.m_dt );

			return (GetStatus() == null);
		}
		return false;

	}

	bool CPnTime::operator!=( const CPnTime& date ) const throw()
	{
		return !operator==(date);
	}

	bool CPnTime::operator<( const CPnTime& date ) const throw()
	{
		assert(GetStatus() == valid);
		assert(date.GetStatus() == valid);
		if( (GetStatus() == valid) && (GetStatus() == date.GetStatus()) )
			return( DoubleFromDate( m_dt ) < DoubleFromDate( date.m_dt ) );

		return false;
	}

	bool CPnTime::operator>( const CPnTime& date ) const throw()
	{
		assert(GetStatus() == valid);
		assert(date.GetStatus() == valid);
		if( (GetStatus() == valid) && (GetStatus() == date.GetStatus()) )
			return( DoubleFromDate( m_dt ) > DoubleFromDate( date.m_dt ) );

		return false;		
	}

	bool CPnTime::operator<=( const CPnTime& date ) const throw()
	{
		return operator<(date) || operator==(date);
	}

	bool CPnTime::operator>=( const CPnTime& date ) const throw()
	{
		return operator>(date) || operator==(date);
	}

	CPnTime CPnTime::operator+( CPnTimeSpan dateSpan ) const throw()
	{
		assert(GetStatus() == valid);
		assert(dateSpan.GetStatus() == valid);
		return( CPnTime::FromDATE( DateFromDouble( DoubleFromDate( m_dt )+dateSpan.m_span ) ) );
	}

	CPnTime CPnTime::operator-( CPnTimeSpan dateSpan ) const throw()
	{
		assert(GetStatus() == valid);
		assert(dateSpan.GetStatus() == valid);
		return CPnTime::FromDATE( DateFromDouble( DoubleFromDate( m_dt )-dateSpan.m_span ) ) ;
	}

	CPnTime& CPnTime::operator+=( CPnTimeSpan dateSpan ) throw()
	{
		assert(GetStatus() == valid);
		assert(dateSpan.GetStatus() == valid);
		m_dt = DateFromDouble( DoubleFromDate( m_dt )+dateSpan.m_span );
		return( *this );
	}

	CPnTime& CPnTime::operator-=( CPnTimeSpan dateSpan ) throw()
	{
		assert(GetStatus() == valid);
		assert(dateSpan.GetStatus() == valid);
		m_dt = DateFromDouble( DoubleFromDate( m_dt )-dateSpan.m_span );
		return( *this );
	}

	CPnTimeSpan CPnTime::operator-(const CPnTime& date) const throw()
	{
		assert(GetStatus() == valid);
		assert(date.GetStatus() == valid);
		return DoubleFromDate(m_dt) - DoubleFromDate(date.m_dt);
	}

// 	CPnTime::operator DATE() const throw()
// 	{
// 		assert(GetStatus() == valid);
// 		return( m_dt );
// 	}

	int CPnTime::SetDateTime(int nYear, int nMonth, int nDay,
		int nHour, int nMin, int nSec, int nMillisec) throw()
	{
		SYSTEMTIME st;
		::ZeroMemory(&st, sizeof(SYSTEMTIME));

		st.wYear = uint16_t(nYear);
		st.wMonth = uint16_t(nMonth);
		st.wDay = uint16_t(nDay);
		st.wHour = uint16_t(nHour);
		st.wMinute = uint16_t(nMin);
		st.wSecond = uint16_t(nSec);
		st.wMilliseconds = uint16_t(nMillisec);

		m_status = ConvertSystemTimeToVariantTime(st) ? valid : invalid;
		return m_status;
	}

	int CPnTime::SetDate(int nYear, int nMonth, int nDay) throw()
	{
		return SetDateTime(nYear, nMonth, nDay, 0, 0, 0, 0);
	}

	int CPnTime::SetTime(int nHour, int nMin, int nSec, int nMillisec) throw()
	{
		// Set date to zero date - 12/30/1899
		return SetDateTime(1899, 12, 30, nHour, nMin, nSec, nMillisec);
	}

	double CPnTime::DoubleFromDate( DATE date ) throw()
	{
		double fTemp;

		// No problem if positive
		if( date >= 0 )
		{
			return( date );
		}

		// If negative, must convert since negative dates not continuous
		// (examples: -1.25 to -.75, -1.50 to -.50, -1.75 to -.25)
		fTemp = ceil( date );

		return( fTemp-(date-fTemp) );
	}

	DATE CPnTime::DateFromDouble( double f ) throw()
	{
		double fTemp;

		// No problem if positive
		if( f >= 0 )
		{
			return( f );
		}

		// If negative, must convert since negative dates not continuous
		// (examples: -.75 to -1.25, -.50 to -1.50, -.25 to -1.75)
		fTemp = floor( f ); // fTemp is now whole part

		return( fTemp+(fTemp-f) );
	}

	void CPnTime::CheckRange()
	{
		// About year 100 to about 9999
		if(m_dt > VTDATEGRE_MAX || m_dt < VTDATEGRE_MIN)
		{
			SetStatus(invalid);    
		}
	}

	bool CPnTime::ParseDateTime(const PNTCHAR* lpszDate, uint32_t dwFlags, LCID lcid) throw()
	{
		LPOLESTR p(NULL);
#ifdef _PNUNICODE
		p = (LPOLESTR)lpszDate;
#else
		p = (LPOLESTR)&StringA2W(lpszDate);
#endif
		if (p == NULL)
		{
			m_dt = 0;
			m_status = invalid;
			return false;
		}

		HRESULT hr;

		if (FAILED(hr = VarDateFromStr( p, lcid, dwFlags, &m_dt )))
		{
			if (hr == DISP_E_TYPEMISMATCH)
			{
				// Can't convert string to date, set 0 and invalidate
				m_dt = 0;
				m_status = invalid;
				return false;
			}
			else if (hr == DISP_E_OVERFLOW)
			{
				// Can't convert string to date, set -1 and invalidate
				m_dt = -1;
				m_status = invalid;
				return false;
			}
			else
			{
				NTTNTRACE("COleDateTime VarDateFromStr call failed.\n");
				// Can't convert string to date, set -1 and invalidate
				m_dt = -1;
				m_status = invalid;
				return false;
			}
		}

		m_status = valid;
		return true;
	}

	String CPnTimeSpan::Format(const PNTCHAR* pFormat) const
	{
		// If null, return empty string
		if (GetStatus() == null)
			return _PNT("");

		//__time64_t timeSpan(GetSeconds() + 60 * (GetMinutes() + 60 * (GetHours() + __int64(24) * GetDays())));

		struct tm tmTemp;
		tmTemp.tm_sec = GetSeconds();
		tmTemp.tm_min = GetMinutes();
		tmTemp.tm_hour = GetHours();
		tmTemp.tm_mday = GetDays();
		tmTemp.tm_mon = 0;
		tmTemp.tm_year = 0;
		tmTemp.tm_wday = 0;
		tmTemp.tm_yday = 0;
		tmTemp.tm_isdst = 0;

		String strDate;
		StrBuf strDateBuf(strDate, 256);
		_tcsftime(strDateBuf, strDate.GetLength(), pFormat, &tmTemp);

		return strDate;
	}

// 	String CPnTimeSpan::Format(UINT nFormatID) const
// 	{
// 		String strFormat;
// 		if (!strFormat.LoadString(nFormatID))
// 			AtlThrow(E_INVALIDARG);
// 		return Format(strFormat);
// 	}

// 	String CPnTime::Format(DWORD dwFlags, LCID lcid) const
// 	{
// 		// If null, return empty string
// 		if (GetStatus() == null)
// 			return _PNT("");
// 
// 		// If invalid, return DateTime global string
// 		if (GetStatus() == invalid)
// 		{
// 			String str;
// 			if(str.LoadString(ATL_IDS_DATETIME_INVALID))
// 				return str;
// 			return szInvalidDateTime;
// 		}
// 
// 		CComBSTR bstr;
// 		if (FAILED(::VarBstrFromDate(m_dt, lcid, dwFlags, &bstr)))
// 		{
// 			String str;
// 			if(str.LoadString(ATL_IDS_DATETIME_INVALID))
// 				return str;
// 			return szInvalidDateTime;
// 		}
// 
// 		String tmp = String(bstr);
// 		return tmp;
// 	}

	String CPnTime::Format(const PNTCHAR* pFormat) const
	{
		assert(pFormat != NULL);

		// If null, return empty string
		if(GetStatus() == null)
			return _PNT("");

		// If invalid, return DateTime global string
		if(GetStatus() == invalid)
		{
			String str;
// 			if(str.LoadString(ATL_IDS_DATETIME_INVALID))
// 				return str;
			return szInvalidDateTime;
		}

		UDATE ud;
		if (S_OK != VarUdateFromDate(m_dt, 0, &ud))
		{
// 			String str;
// 			if(str.LoadString(ATL_IDS_DATETIME_INVALID))
// 				return str;
			return szInvalidDateTime;
		}

		struct tm tmTemp;
		tmTemp.tm_sec	= ud.st.wSecond;
		tmTemp.tm_min	= ud.st.wMinute;
		tmTemp.tm_hour	= ud.st.wHour;
		tmTemp.tm_mday	= ud.st.wDay;
		tmTemp.tm_mon	= ud.st.wMonth - 1;
		tmTemp.tm_year	= ud.st.wYear - 1900;
		tmTemp.tm_wday	= ud.st.wDayOfWeek;
		tmTemp.tm_yday	= ud.wDayOfYear - 1;
		tmTemp.tm_isdst	= 0;

		String strDate;
		StrBuf strDateBuf(strDate,256);
		_tcsftime(strDateBuf , strDate.GetLength(), pFormat, &tmTemp);
		strDate.ReleaseBuffer();

		return strDate;
	}

// 	String CPnTime::Format(UINT nFormatID) const
// 	{
// 		String strFormat;
// 		assert(strFormat.LoadString(nFormatID));
// 		return Format(strFormat);
// 	}

#ifdef _WIN32
	CPnTime::CPnTime(const DBTIMESTAMP& dbts)
	{
		SYSTEMTIME st;
		::ZeroMemory(&st, sizeof(SYSTEMTIME));

		st.wYear = uint16_t(dbts.year);
		st.wMonth = uint16_t(dbts.month);
		st.wDay = uint16_t(dbts.day);
		st.wHour = uint16_t(dbts.hour);
		st.wMinute = uint16_t(dbts.minute);
		st.wSecond = uint16_t(dbts.second);
		st.wMilliseconds = uint16_t(dbts.fraction);

//		m_status = ::SystemTimeToVariantTime(&st, &m_dt) ? valid : invalid;
		m_status = SystemTimeToVariantTimeMs(st, &m_dt) ? valid : invalid;
	}

	bool CPnTime::GetAsDBTIMESTAMP(DBTIMESTAMP& dbts) const
	{
		UDATE ud;
		if (S_OK != VarUdateFromDate(m_dt, 0, &ud))
			return false;

		dbts.year = (SHORT) ud.st.wYear;
		dbts.month = (uint16_t) ud.st.wMonth;
		dbts.day = (uint16_t) ud.st.wDay;
		dbts.hour = (uint16_t) ud.st.wHour;
		dbts.minute = (uint16_t) ud.st.wMinute;
		dbts.second = (uint16_t) ud.st.wSecond;
		dbts.fraction = (uint16_t) ud.st.wMilliseconds;

		return true;
	}
#endif // _WIN32

#endif // __MACH__
}