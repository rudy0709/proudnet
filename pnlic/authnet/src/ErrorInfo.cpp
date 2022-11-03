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

			if((m_remoteAddr.m_binaryAddress != 0xffffffff) && (m_remoteAddr.m_binaryAddress != 0x00000000))
			{
				String tempret;
				tempret.Format(_PNT(", remoteAddr=%s"), m_remoteAddr.ToString().GetString());
				ret += tempret;
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
		switch(CLocale::Instance().m_languageID)
		{
		case 82:
			return TypeToString_Kor(e);
		case 86:
			return TypeToString_Chn(e);
		default:
			return TypeToString_Eng(e);
		}
#endif
	}

	const PNTCHAR* ErrorInfo::TypeToString_Kor(ErrorType e)
	{
		switch (e)
		{
		case ErrorType_Unexpected:
			return _PNT("의도되지 않은 상황이 발생했습니다.");
		case ErrorType_AlreadyConnected:
			return _PNT("이미 연결되어 있었습니다.");
		case ErrorType_TCPConnectFailure:
			return _PNT("TCP 연결이 실패했습니다.");
		case ErrorType_InvalidSessionKey:
			return _PNT("잘못된 세션 암호 키입니다.");
		case ErrorType_EncryptFail:
			return _PNT("암호화가 실패했습니다.");
		case ErrorType_DecryptFail:
			return _PNT("복호화 실패 혹은 해커에 의한 조작된 데이터입니다.");
		case ErrorType_ConnectServerTimeout:
			return _PNT("서버와의 연결 시도가 타임 아웃하였습니다.");
		case ErrorType_ProtocolVersionMismatch:
			return _PNT("서버와 프로토콜 버전이 맞지 않습니다.");
		case ErrorType_NotifyServerDeniedConnection:
			return _PNT("서버에서 연결을 거부했습니다.");
		case ErrorType_ConnectServerSuccessful:
			return _PNT("서버와의 연결이 성공했습니다.");
		case ErrorType_DisconnectFromRemote:
			return _PNT("상대측 호스트가 연결을 끊었습니다.");
		case ErrorType_DisconnectFromLocal:
			return _PNT("로컬 호스트에서 능동적으로 연결을 끊었습니다.");
		case ErrorType_DangerousArgumentWarning:
			return _PNT("위험한 호출 파라메터가 있습니다.");
		case ErrorType_UnknownAddrPort:
			return _PNT("알 수 없는 인터넷 주소입니다.");
		case ErrorType_ServerNotReady:
			return _PNT("서버가 준비되지 않았습니다.");
		case ErrorType_ServerPortListenFailure:
			return _PNT("서버 소켓의 listen을 시작할 수 없습니다. TCP 또는 UDP 소켓이 이미 사용중인 포트인지 확인하십시오.");
		case ErrorType_AlreadyExists:
			return _PNT("이미 개체가 존재합니다.");
		case ErrorType_PermissionDenied:
			return _PNT("접근이 거부되었습니다.");
		case ErrorType_BadSessionGuid:
			return _PNT("잘못된 session Guid입니다.");
		case ErrorType_InvalidCredential:
			return _PNT("잘못된 credential입니다.");
		case ErrorType_InvalidHeroName:
			return _PNT("잘못된 hero name입니다.");
		case ErrorType_LoadDataPreceded:
			return _PNT("로딩 과정이 unlock 후 lock 한 후 꼬임이 발생했습니다.");
		case ErrorType_AdjustedGamerIDNotFilled:
			return _PNT("출력 파라메터 AdjustedGamerIDNotFilled가 채워지지 않았습니다.");
		case ErrorType_NoHero:
			return _PNT("플레이어 캐릭터가 존재하지 않습니다.");
		case ErrorType_UnitTestFailed:
			return _PNT("UnitTestFailed");
		case ErrorType_P2PUdpFailed:
			return _PNT("peer-to-peer UDP 통신이 막혔습니다.");
		case ErrorType_ReliableUdpFailed:
			return _PNT("P2P reliable UDP가 실패했습니다.");
		case ErrorType_ServerUdpFailed:
			return _PNT("클라이언트-서버 UDP 통신이 막혔습니다.");
		case ErrorType_NoP2PGroupRelation:
			return _PNT("더 이상 같이 소속된 P2P 그룹이 없습니다.");
		case ErrorType_ExceptionFromUserFunction:
			return _PNT("사용자 정의 함수(RMI 수신 루틴 혹은 이벤트 핸들러)에서 exception이 throw되었습니다.");
		case ErrorType_UserRequested:
			return _PNT("사용자의 요청에 의한 실패입니다.");			
		case ErrorType_InvalidPacketFormat:
			return _PNT("잘못된 패킷 형식입니다. 상대측 호스트가 해킹되었거나 버그일 수 있습니다.");
		case ErrorType_TooLargeMessageDetected:
			return _PNT("너무 큰 크기의 메시징이 시도되었습니다. 기술지원부에 문의하십시오.");
		case ErrorType_CannotEncryptUnreliableMessage:
			return _PNT("Unreliable 메세지는 암호화할 수 없습니다.");
		case ErrorType_ValueNotExist:
			return _PNT("존재하지 않는 값입니다.");
		case ErrorType_TimeOut:
			return _PNT("타임 아웃입니다.");
		case ErrorType_LoadedDataNotFound:
			return _PNT("로드된 데이터를 찾을 수 없습니다.");
		case ErrorType_SendQueueIsHeavy:
			return _PNT("송신 queue가 너무 많이 쌓여 있습니다. 송신량을 조절하는 것을 권장합니다.");
		case ErrorType_TooSlowHeartbeatWarning:
			return _PNT("Heartbeat가 너무 늦게 호출되고 있습니다.기아화를 의심하세요.");
		case ErrorType_CompressFail:
			return _PNT("메시지 압축을 푸는데 실패 하였습니다.");
		case ErrorType_LocalSocketCreationFailed:
			return _PNT("클라이언트 소켓의 listen을 시작할 수 없습니다. TCP 또는 UDP 소켓이 이미 사용중인 포트인지 확인하십시오.");
		case Error_NoneAvailableInPortPool:
			return _PNT("Socket을 생성할 때 Port Pool 내 port number로의 bind가 실패했습니다. 대신 임의의 port number가 사용되었습니다. Port Pool의 갯수가 충분한지 확인하십시요.");
		case ErrorType_InvalidPortPool:
			return _PNT("Port pool 내 값들 중 하나 이상이 잘못되었습니다. 포트를 0(임의 포트 바인딩)으로 하거나 중복되지 않았는지 확인하십시요.");
		case ErrorType_InvalidHostID:
			return _PNT("유효하지 않은 HostID 입니다.");
		case ErrorType_MessageOverload:
			return _PNT("패킷 처리 속도 보다 들어오는 패킷의 양이 많습니다. 패킷의 양을 줄여주세요");
		case ErrorType_AutoConnectionRecoveryFailed:
			return _PNT("서버와의 연결이 끊어져 연결 복구를 시도했으나 이 또한 실패했습니다.");
			
        default:
            break;
		}

		return _PNT("<none>");
	}

	const PNTCHAR* ErrorInfo::TypeToString_Eng( ErrorType e )
	{
		switch (e)
		{
		case ErrorType_Unexpected:
			return _PNT("Unexpected Error.");
		case ErrorType_AlreadyConnected:
			return _PNT("Already connected.");
		case ErrorType_TCPConnectFailure:
			return _PNT("TCP connection failure.");
		case ErrorType_InvalidSessionKey:
			return _PNT("Invalid session key.");
		case ErrorType_EncryptFail:
			return _PNT("Encryption failed.");
		case ErrorType_DecryptFail:
			return _PNT("Decryption failed or hack suspected.");
		case ErrorType_ConnectServerTimeout:
			return _PNT("Connect to server timed out.");
		case ErrorType_ProtocolVersionMismatch:
			return _PNT("Mispatched protocol between hosts.");
		case ErrorType_NotifyServerDeniedConnection:
			return _PNT("Server denied connection attempt.");
		case ErrorType_ConnectServerSuccessful:
			return _PNT("Connecting to server successful.");
		case ErrorType_DisconnectFromRemote:
			return _PNT("Remote host disconnected.");
		case ErrorType_DisconnectFromLocal:
			return _PNT("Local host disconnected.");
		case ErrorType_DangerousArgumentWarning:
			return _PNT("Dangerous parameters are detected.");
		case ErrorType_UnknownAddrPort:
			return _PNT("Unknown Internet address.");
		case ErrorType_ServerNotReady:
			return _PNT("Server is not ready.");
		case ErrorType_ServerPortListenFailure:
			return _PNT("Server socket listen failure. Make sure that the TCP or UDP listening port is not already in use.");
		case ErrorType_AlreadyExists:
			return _PNT("Object already exists.");
		case ErrorType_PermissionDenied:
			return _PNT("Permission denied.");
		case ErrorType_BadSessionGuid:
			return _PNT("Bad session Guid.");
		case ErrorType_InvalidCredential:
			return _PNT("Invalid credential.");
		case ErrorType_InvalidHeroName:
			return _PNT("Invalid player character name.");
		case ErrorType_LoadDataPreceded:
			return _PNT("Corruption occurred while unlocked loading and locking.");
		case ErrorType_AdjustedGamerIDNotFilled:
			return _PNT("Output parameter AdjustedGamerIDNotFilled is not filled.");
		case ErrorType_NoHero:
			return _PNT("No Player Character(Hero) Found.");
		case ErrorType_UnitTestFailed:
			return _PNT("UnitTestFailed");
		case ErrorType_P2PUdpFailed:
			return _PNT("peer-to-peer UDP comm is blocked.");
		case ErrorType_ReliableUdpFailed:
			return _PNT("P2P reliable UDP failed.");
		case ErrorType_ServerUdpFailed:
			return _PNT("Client-server UDP comm is blocked.");
		case ErrorType_NoP2PGroupRelation:
			return _PNT("No common P2P group exists anymore.");
		case ErrorType_ExceptionFromUserFunction:
			return _PNT("An exception is thrown from user function. It may be an RMI function or event handler.");
		case ErrorType_UserRequested:
			return _PNT("By user request.");
		case ErrorType_InvalidPacketFormat:
			return _PNT("Invalid packet format. Remote host is hacked or has a bug.");
		case ErrorType_TooLargeMessageDetected:
			return _PNT("Too large message is detected. Contact technical supports.");
		case ErrorType_CannotEncryptUnreliableMessage:
			return _PNT("An unreliable message cannot be encrypted.");
		case ErrorType_ValueNotExist:
			return _PNT("Not exist value.");
		case ErrorType_TimeOut:
			return _PNT("Working is timeout.");
		case ErrorType_LoadedDataNotFound:
			return _PNT("Can not found loaddata.");
		case ErrorType_SendQueueIsHeavy:
			return _PNT("SendQueue has Accumulated too much.");
		case ErrorType_TooSlowHeartbeatWarning:
			return _PNT("Heartbeat Call in too slow.Suspected starvation");
		case ErrorType_CompressFail:
			return _PNT("Message uncompress fail.");
		case ErrorType_LocalSocketCreationFailed:
			return _PNT("Unable to start listening of client socket. Must check if either TCP or UDP socket is already in use.");
		case Error_NoneAvailableInPortPool:
			return _PNT("Failed binding to local port that defined in Port Pool. Please check number of values in Port Pool are sufficient.");
		case ErrorType_InvalidPortPool:
			return _PNT("Range of user defined port is wrong. Set port to 0(random port binding) or check if it is overlaped.");
		case ErrorType_InvalidHostID:
			return _PNT("Invalid HostID.");
		case ErrorType_MessageOverload:
			return _PNT("Process speed of packet cannot handle incoming packets. Please reduce amount of incoming packet.");
		case ErrorType_AutoConnectionRecoveryFailed:
			return _PNT("Connection with the server has been disconnected, attempt to repair this connection also failed.");
        default:
            break;
		}	
		return _PNT("<none>");
	}

	const PNTCHAR* ErrorInfo::TypeToString_Chn(ErrorType e)
	{
		switch (e)
		{
		case ErrorType_Unexpected:
			return _PNT("发生了以外的情况.");
		case ErrorType_AlreadyConnected:
			return _PNT("已连接.");
		case ErrorType_TCPConnectFailure:
			return _PNT("TCP/IP 连接失败.");
		case ErrorType_InvalidSessionKey:
			return _PNT("session key不正确.");
		case ErrorType_EncryptFail:
			return _PNT("暗号化失败.");
		case ErrorType_DecryptFail:
			return _PNT("符号化失败或被操作的数据.");
		case ErrorType_ConnectServerTimeout:
			return _PNT("服务器连接超时.");
		case ErrorType_ProtocolVersionMismatch:
			return _PNT("跟服务器的版本不同 需要上级.");
		case ErrorType_NotifyServerDeniedConnection:
			return _PNT("被据否了服务器连接.");
		case ErrorType_ConnectServerSuccessful:
			return _PNT("服务器连接成功.");
		case ErrorType_DisconnectFromRemote:
			return _PNT("Remote host disconnected.");
		case ErrorType_DisconnectFromLocal:
			return _PNT("Local host disconnected.");
		case ErrorType_DangerousArgumentWarning:
			return _PNT("Dangerous parameters are detected.");
		case ErrorType_UnknownAddrPort:
			return _PNT("在网上没有地址.");
		case ErrorType_ServerNotReady:
			return _PNT("服务器还没准备.");
		case ErrorType_ServerPortListenFailure:
			return _PNT("Server socket listen failure. Make sure that the TCP or UDP listening port is not already in use.");
		case ErrorType_AlreadyExists:
			return _PNT("个体已存在.");
		case ErrorType_PermissionDenied:
			return _PNT("访问被拒绝.");
		case ErrorType_BadSessionGuid:
			return _PNT("session Guid不正确.");
		case ErrorType_InvalidCredential:
			return _PNT("credential不正确.");
		case ErrorType_InvalidHeroName:
			return _PNT("hero name不正确.");
		case ErrorType_LoadDataPreceded:
			return _PNT("加载过程unlock后与lock后发生隔阂.");
		case ErrorType_AdjustedGamerIDNotFilled:
			return _PNT("Output parameter AdjustedGamerIDNotFilled is not filled.");
		case ErrorType_NoHero:
			return _PNT("No Player Character(Hero) Found.");
		case ErrorType_UnitTestFailed:
			return _PNT("UnitTestFailed");
		case ErrorType_P2PUdpFailed:
			return _PNT("peer-to-peer UDP comm is blocked.");
		case ErrorType_ReliableUdpFailed:
			return _PNT("P2P reliable UDP failed.");
		case ErrorType_ServerUdpFailed:
			return _PNT("Client-server UDP comm is blocked.");
		case ErrorType_NoP2PGroupRelation:
			return _PNT("No common P2P group exists anymore.");
		case ErrorType_ExceptionFromUserFunction:
			return _PNT("An exception is thrown from user function. It may be an RMI function or event handler.");
		case ErrorType_UserRequested:
			return _PNT("By user request.");			
		case ErrorType_InvalidPacketFormat:
			return _PNT("Invalid packet format. Remote host is hacked or has a bug.");
		case ErrorType_TooLargeMessageDetected:
			return _PNT("Too large message is detected. Contact technical supports.");
		case ErrorType_CannotEncryptUnreliableMessage:
			return _PNT("An unreliable message cannot be encrypted.");
		case ErrorType_ValueNotExist:
			return _PNT("Not exist value.");
		case ErrorType_TimeOut:
			return _PNT("Working is timeout.");
		case ErrorType_LoadedDataNotFound:
			return _PNT("Can not found loaddata.");
		case ErrorType_SendQueueIsHeavy:
			return _PNT("SendQueue has Accumulated too much.");
		case ErrorType_TooSlowHeartbeatWarning:
			return _PNT("Heartbeat Call in too slow.Suspected starvation");
		case ErrorType_CompressFail:
			return _PNT("Message uncompress fail.");
		case ErrorType_LocalSocketCreationFailed:
			return _PNT("Unable to start listening of client socket. Must check if either TCP or UDP socket is already in use.");
		case Error_NoneAvailableInPortPool:
			return _PNT("Failed binding to local port that defined in Port Pool. Please check number of values in Port Pool are sufficient.");
		case ErrorType_InvalidPortPool:
			return _PNT("Range of user defined port is wrong. Set port to 0(random port binding) or check if it is overlaped.");
		case ErrorType_InvalidHostID:
			return _PNT("无效的HostID");
		case ErrorType_MessageOverload:
			return _PNT("消息的堆叠速度高于处理它们的速度。检查您要发送的邮件太多，或消息处理程序运行过慢。");
		case ErrorType_AutoConnectionRecoveryFailed:
			return _PNT("因与服务器的连接断开所以试图恢复连接，但又失败了。");
        default:
            break;
		}
		return _PNT("<none>");
	}
}
