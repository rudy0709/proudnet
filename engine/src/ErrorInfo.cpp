/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <new>
#include "../include/ErrorInfo.h"
#include "../include/sysutil.h"
#include "ErrorType.inl"

namespace Proud
{
	ErrorInfo::ErrorInfo() : m_lastReceivedMessage()
	{
		m_errorType = ErrorType_Ok; // unexpected로 바꾸지 말것! 생성자 값을 그대로 쓰는데가 여기저기 있으니!
		m_detailType = ErrorType_Ok;
		m_socketError = SocketErrorCode_Ok;
		m_remote = HostID_None;
		m_remoteAddr = AddrPort::Unassigned;
#if defined(_WIN32)
		m_hResult = S_OK;
#endif
	}

	ErrorInfoPtr ErrorInfo::FromSocketError( ErrorType code, SocketErrorCode se )
	{
		ErrorInfoPtr e(new ErrorInfo());
		e->m_errorType = code;
		e->m_socketError = se;
		return e;
	}

	ErrorInfoPtr ErrorInfo::From(ErrorType errorType, HostID remote , const String &comment , const ByteArray &lastReceivedMessage )
	{
		ErrorInfoPtr e (new ErrorInfo());
		e->m_errorType = errorType;
		e->m_remote = remote;
		e->m_comment = comment;
		e->m_lastReceivedMessage = lastReceivedMessage;
		return e;
	}

	ErrorInfo* ErrorInfo::Clone() 
	{
		ErrorInfo* ret = new ErrorInfo;
		*ret = *this;

		return ret;

		/*ErrorType m_errorType;
		ErrorType m_detailType;
		SocketErrorCode m_socketError;
		HostID m_remote;
		String m_comment;
		AddrPort m_remoteAddr;
		ByteArray m_lastReceivedMessage;
		HRESULT m_hResult;
		String m_source;*/

	}

	String ErrorInfo::ToString() const
	{
		String ret;
// 		ret.Format(_PNT("Error=%s,Detail=%s,SocketError=%d,HostID=%d,Comment='%s'"), 
// 			(LPCWSTR)TypeToString(m_errorType), 
// 			(LPCWSTR)TypeToString(m_detailType), 
// 			m_socketError, 
// 			(int)m_remote, 
// 			(LPCWSTR)m_comment);

		//modify By Seungwhan 			
		ret.Format(_PNT("Error=%s"), TypeToString(m_errorType));
		
		if(m_errorType != m_detailType && m_detailType != ErrorType_Ok )
		{
			String tempret;
			tempret.Format(_PNT(", Detail=%s"), TypeToString(m_detailType));
			ret += tempret;
		}
		if(m_socketError != SocketErrorCode_Ok)
		{
			String tempret;
			tempret.Format(_PNT(", SocketError=%d"), m_socketError);
			ret += tempret;
		}
		if(m_remote != HostID_None)
		{
			String tempret;
			tempret.Format(_PNT(", HostID=%d"), m_remote);
			ret += tempret;

			if(m_remoteAddr.IsUnicastEndpoint() == true)
			{
				String tempret_;
				tempret_.Format(_PNT(", remoteAddr=%s"), m_remoteAddr.ToString().GetString());
				ret += tempret_;
			}
		}
		if(m_comment.GetLength() > 0)
		{
			String tempret(m_comment.GetString());
			ret += _PNT(", ");
			ret += tempret;
		}
#if defined(_WIN32)
		if(m_hResult != S_OK)
		{
			String tempret;
			tempret.Format(_PNT(", HRESULT = %d"),(int)m_hResult);
			ret += tempret;
		}
#endif
		if(m_source.GetLength() > 0)
		{
			String tempret(m_source.GetString());
			ret += _PNT(", ");
			ret += tempret;
		}

		return ret;
	}

	const PNTCHAR* ErrorInfo::TypeToString( ErrorType e )
	{
#if !defined(_WIN32)
		return TypeToString_Eng(e);
#else
		return TypeToStringByLangID(e, CLocale::GetUnsafeRef().m_languageID);
#endif

	}

	const PNTCHAR* ErrorInfo::TypeToStringByLangID(ErrorType e, int languageID)
	{
#if defined(_WIN32)
		if (0 > languageID)
			languageID = CLocale::GetUnsafeRef().m_languageID;
#endif

		switch (languageID)
		{
		case 81:
			return TypeToString_Jpn(e);
		case 82:
			return TypeToString_Kor(e);
		case 86:
			return TypeToString_Chn(e);
		default:
			return TypeToString_Eng(e);
		}
	}

}
