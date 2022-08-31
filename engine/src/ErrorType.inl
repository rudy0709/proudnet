
namespace Proud
{
	const PNTCHAR* ErrorInfo::TypeToPlainString(ErrorType e)
	{
		switch (e)
		{
				case ErrorType_Ok:
			return _PNT("Ok");
				case ErrorType_Unexpected:
			return _PNT("Unexpected");
				case ErrorType_AlreadyConnected:
			return _PNT("AlreadyConnected");
				case ErrorType_TCPConnectFailure:
			return _PNT("TCPConnectFailure");
				case ErrorType_InvalidSessionKey:
			return _PNT("InvalidSessionKey");
				case ErrorType_EncryptFail:
			return _PNT("EncryptFail");
				case ErrorType_DecryptFail:
			return _PNT("DecryptFail");
				case ErrorType_ConnectServerTimeout:
			return _PNT("ConnectServerTimeout");
				case ErrorType_ProtocolVersionMismatch:
			return _PNT("ProtocolVersionMismatch");
				case ErrorType_InvalidLicense:
			return _PNT("InvalidLicense");
				case ErrorType_NotifyServerDeniedConnection:
			return _PNT("NotifyServerDeniedConnection");
				case ErrorType_Reserved01:
			return _PNT("Reserved01");
				case ErrorType_DisconnectFromRemote:
			return _PNT("DisconnectFromRemote");
				case ErrorType_DisconnectFromLocal:
			return _PNT("DisconnectFromLocal");
				case ErrorType_Reserved02:
			return _PNT("Reserved02");
				case ErrorType_UnknownAddrPort:
			return _PNT("UnknownAddrPort");
				case ErrorType_Reserved03:
			return _PNT("Reserved03");
				case ErrorType_ServerPortListenFailure:
			return _PNT("ServerPortListenFailure");
				case ErrorType_AlreadyExists:
			return _PNT("AlreadyExists");
				case ErrorType_PermissionDenied:
			return _PNT("PermissionDenied");
				case ErrorType_BadSessionGuid:
			return _PNT("BadSessionGuid");
				case ErrorType_InvalidCredential:
			return _PNT("InvalidCredential");
				case ErrorType_InvalidHeroName:
			return _PNT("InvalidHeroName");
				case ErrorType_Reserved06:
			return _PNT("Reserved06");
				case ErrorType_Reserved07:
			return _PNT("Reserved07");
				case ErrorType_Reserved08:
			return _PNT("Reserved08");
				case ErrorType_UnitTestFailed:
			return _PNT("UnitTestFailed");
				case ErrorType_P2PUdpFailed:
			return _PNT("P2PUdpFailed");
				case ErrorType_ReliableUdpFailed:
			return _PNT("ReliableUdpFailed");
				case ErrorType_ServerUdpFailed:
			return _PNT("ServerUdpFailed");
				case ErrorType_NoP2PGroupRelation:
			return _PNT("NoP2PGroupRelation");
				case ErrorType_ExceptionFromUserFunction:
			return _PNT("ExceptionFromUserFunction");
				case ErrorType_UserRequested:
			return _PNT("UserRequested");
				case ErrorType_InvalidPacketFormat:
			return _PNT("InvalidPacketFormat");
				case ErrorType_TooLargeMessageDetected:
			return _PNT("TooLargeMessageDetected");
				case ErrorType_NoNeedWebSocketEncryption:
			return _PNT("NoNeedWebSocketEncryption");
				case ErrorType_ValueNotExist:
			return _PNT("ValueNotExist");
				case ErrorType_TimeOut:
			return _PNT("TimeOut");
				case ErrorType_LoadedDataNotFound:
			return _PNT("LoadedDataNotFound");
				case ErrorType_SendQueueIsHeavy:
			return _PNT("SendQueueIsHeavy");
				case ErrorType_TooSlowHeartbeatWarning:
			return _PNT("TooSlowHeartbeatWarning");
				case ErrorType_CompressFail:
			return _PNT("CompressFail");
				case ErrorType_LocalSocketCreationFailed:
			return _PNT("LocalSocketCreationFailed");
				case ErrorType_NoneAvailableInPortPool:
			return _PNT("NoneAvailableInPortPool");
				case ErrorType_InvalidPortPool:
			return _PNT("InvalidPortPool");
				case ErrorType_InvalidHostID:
			return _PNT("InvalidHostID");
				case ErrorType_MessageOverload:
			return _PNT("MessageOverload");
				case ErrorType_DatabaseAccessFailed:
			return _PNT("DatabaseAccessFailed");
				case ErrorType_OutOfMemory:
			return _PNT("OutOfMemory");
				case ErrorType_AutoConnectionRecoveryFailed:
			return _PNT("AutoConnectionRecoveryFailed");
				case ErrorType_NotImplementedRmi:
			return _PNT("NotImplementedRmi");
				default:
			break;
		}
		return _PNT("<none>");
	}

	const PNTCHAR* ErrorInfo::TypeToString_Kor(ErrorType e)
	{
		switch (e)
		{
				case ErrorType_Ok:
			return _PNT("성공. 문제 없음.");
				case ErrorType_Unexpected:
			return _PNT("의도되지 않은 상황이 발생했습니다.");
				case ErrorType_AlreadyConnected:
			return _PNT("이미 연결되어 있음.");
				case ErrorType_TCPConnectFailure:
			return _PNT("TCP 연결 실패");
				case ErrorType_InvalidSessionKey:
			return _PNT("잘못된 대칭키");
				case ErrorType_EncryptFail:
			return _PNT("암호화가 실패했음");
				case ErrorType_DecryptFail:
			return _PNT("해커가 깨진 데이터를 전송했거나 복호화가 실패했음");
				case ErrorType_ConnectServerTimeout:
			return _PNT("서버 연결 과정이 너무 오래 걸려서 실패 처리됨");
				case ErrorType_ProtocolVersionMismatch:
			return _PNT("서버 연결을 위한 프로토콜 버전이 다름");
				case ErrorType_InvalidLicense:
			return _PNT("서버 쪽 인증에 문제가 있음.");
				case ErrorType_NotifyServerDeniedConnection:
			return _PNT("서버가 연결을 의도적으로 거부했음");
				case ErrorType_Reserved01:
			return _PNT("서버 연결 성공!");
				case ErrorType_DisconnectFromRemote:
			return _PNT("상대측 호스트에서 연결을 해제했음");
				case ErrorType_DisconnectFromLocal:
			return _PNT("이쪽 호스트에서 연결을 해제했음");
				case ErrorType_Reserved02:
			return _PNT("위험한 상황을 불러올 수 있는 인자가 있음");
				case ErrorType_UnknownAddrPort:
			return _PNT("알 수 없는 인터넷 주소");
				case ErrorType_Reserved03:
			return _PNT("서버 준비 부족");
				case ErrorType_ServerPortListenFailure:
			return _PNT("서버 소켓의 listen을 시작할 수 없습니다. TCP 또는 UDP 소켓이 이미 사용중인 포트인지 확인하십시오.");
				case ErrorType_AlreadyExists:
			return _PNT("이미 개체가 존재합니다.");
				case ErrorType_PermissionDenied:
			return _PNT("접근이 거부되었습니다. <a target=\"_blank\" href=\"http://guide.nettention.com/cpp_ko#dbc2_nonexclusive_overview\" >비독점적 데이터 접근하기</a> 기능에서, DB cache server가 비독점적 접근 기능을 쓰지 않겠다는 설정이 되어 있으면 발생할 수 있습니다. 윈도우 비스타이상의 OS에서 관리자권한이 없이 CNetUtil::EnableTcpOffload( bool enable ) 함수가 호출되면 발생할 수 있습니다.");
				case ErrorType_BadSessionGuid:
			return _PNT("잘못된 session Guid입니다.");
				case ErrorType_InvalidCredential:
			return _PNT("잘못된 credential입니다.");
				case ErrorType_InvalidHeroName:
			return _PNT("잘못된 hero name입니다.");
				case ErrorType_Reserved06:
			return _PNT("로딩 과정이 unlock 후 lock 한 후 꼬임이 발생했다");
				case ErrorType_Reserved07:
			return _PNT("출력 파라메터 AdjustedGamerIDNotFilled가 채워지지 않았습니다.");
				case ErrorType_Reserved08:
			return _PNT("플레이어 캐릭터가 존재하지 않습니다.");
				case ErrorType_UnitTestFailed:
			return _PNT("유닛 테스트 실패");
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
			return _PNT("너무 큰 크기의 메시징이 시도되었습니다. 기술지원부에 문의하십시오. 필요시 <a target=\"_blank\" href=\"http://guide.nettention.com/cpp_ko#message_length\" >통신 메시지의 크기 제한</a>  를 참고하십시오.");
				case ErrorType_NoNeedWebSocketEncryption:
			return _PNT("websocket 메세지는 encrypt 할수 없습니다.");
				case ErrorType_ValueNotExist:
			return _PNT("존재하지 않는 값입니다.");
				case ErrorType_TimeOut:
			return _PNT("타임아웃 입니다.");
				case ErrorType_LoadedDataNotFound:
			return _PNT("로드된 데이터를 찾을수 없습니다.");
				case ErrorType_SendQueueIsHeavy:
			return _PNT("<a target=\"_blank\" href=\"http://guide.nettention.com/cpp_ko#send_queue_heavy\" >송신량 과다시 경고 발생 기능</a>. 송신 queue가 지나치게 커졌습니다.");
				case ErrorType_TooSlowHeartbeatWarning:
			return _PNT("Heartbeat가 평균적으로 너무 느림");
				case ErrorType_CompressFail:
			return _PNT("메시지 압축이 실패 하였습니다.");
				case ErrorType_LocalSocketCreationFailed:
			return _PNT("클라이언트 소켓의 listen 혹은 UDP 준비를 할 수 없습니다. TCP 또는 UDP 소켓이 이미 사용중인 포트인지, 프로세스당 소켓 갯수 제한이 있는지 확인하십시오.");
				case ErrorType_NoneAvailableInPortPool:
			return _PNT("Socket을 생성할 때 Port Pool 내 port number로의 bind가 실패했습니다. 대신 임의의 port number가 사용되었습니다. Port Pool의 갯수가 충분한지 확인하십시요.");
				case ErrorType_InvalidPortPool:
			return _PNT("Port pool 내 값들 중 하나 이상이 잘못되었습니다. 포트를 0(임의 포트 바인딩)으로 하거나 중복되지 않았는지 확인하십시요.");
				case ErrorType_InvalidHostID:
			return _PNT("유효하지 않은 HostID 값입니다.");
				case ErrorType_MessageOverload:
			return _PNT("사용자가 소화하는 메시지 처리 속도보다 내부적으로 쌓이는 메시지의 속도가 더 높습니다. 지나치게 너무 많은 메시지를 송신하려고 했는지, 혹은 사용자의 메시지 수신 함수가 지나치게 느리게 작동하고 있는지 확인하십시오.");
				case ErrorType_DatabaseAccessFailed:
			return _PNT("Accessing database failed. For example, query statement execution failed. You may see the details from comment variable.");
				case ErrorType_OutOfMemory:
			return _PNT("메모리가 부족합니다.");
				case ErrorType_AutoConnectionRecoveryFailed:
			return _PNT("서버와의 연결이 끊어져서 연결 복구 기능이 가동되었지만, 이것 마저도 실패했습니다.");
				case ErrorType_NotImplementedRmi:
			return _PNT("RMI 함수를 호출 했지만, 함수 구현부가 없습니다.");
				default:
			break;
		}
		return _PNT("<none>");
	}

	const PNTCHAR* ErrorInfo::TypeToString_Eng(ErrorType e)
	{
		switch (e)
		{
				case ErrorType_Ok:
			return _PNT("Success. No problem.");
				case ErrorType_Unexpected:
			return _PNT("Exception occurred.");
				case ErrorType_AlreadyConnected:
			return _PNT("Already connected.");
				case ErrorType_TCPConnectFailure:
			return _PNT("TCP connection failed");
				case ErrorType_InvalidSessionKey:
			return _PNT("Wrong matching key");
				case ErrorType_EncryptFail:
			return _PNT("Encoding failed");
				case ErrorType_DecryptFail:
			return _PNT("Either the hacker transmitted corrupt data or decoding failed.");
				case ErrorType_ConnectServerTimeout:
			return _PNT("Processed as failure since the server connection process took too much time.");
				case ErrorType_ProtocolVersionMismatch:
			return _PNT("Protocol version for server connection is different");
				case ErrorType_InvalidLicense:
			return _PNT("Problem occured while license validation task.");
				case ErrorType_NotifyServerDeniedConnection:
			return _PNT("Server intentionally rejected connection");
				case ErrorType_Reserved01:
			return _PNT("Server connection success!");
				case ErrorType_DisconnectFromRemote:
			return _PNT("The correspondent host removed the connection");
				case ErrorType_DisconnectFromLocal:
			return _PNT("This side host removed the connection");
				case ErrorType_Reserved02:
			return _PNT("There is a factor that can cause a dangerous situation");
				case ErrorType_UnknownAddrPort:
			return _PNT("Unknown internet address");
				case ErrorType_Reserved03:
			return _PNT("Insufficient server readiness");
				case ErrorType_ServerPortListenFailure:
			return _PNT("You cannot start listen of the server socket. Please check whether either the TCP or UDP socket is a port already being used.");
				case ErrorType_AlreadyExists:
			return _PNT("Already an object exists.");
				case ErrorType_PermissionDenied:
			return _PNT("Access denied. At <a target=\"_blank\" href=\"http://guide.nettention.com/cpp_en#dbc2_nonexclusive_overview\" >Accessing non-exclusive data</a> function, it could occur if a function that DB cache server does not use exclusive access function has been set. At OS over window vista, It could occur when CNetUtil::EnableTcpOffload( bool enable ) function is called without administration authority.");
				case ErrorType_BadSessionGuid:
			return _PNT("This is a wrong session Guid");
				case ErrorType_InvalidCredential:
			return _PNT("This is a wrong credential.");
				case ErrorType_InvalidHeroName:
			return _PNT("This is a wrong hero name.");
				case ErrorType_Reserved06:
			return _PNT("After the loading is locked after being unlocked, a problem occurred.");
				case ErrorType_Reserved07:
			return _PNT("The output parameter AdjustedGamerIDNotFilled has not been filled.");
				case ErrorType_Reserved08:
			return _PNT("The player character does not exist.");
				case ErrorType_UnitTestFailed:
			return _PNT("Unit test failed.");
				case ErrorType_P2PUdpFailed:
			return _PNT("Peer-to-peer UDP communication is blocked.");
				case ErrorType_ReliableUdpFailed:
			return _PNT("P2P reliable UDP failed.");
				case ErrorType_ServerUdpFailed:
			return _PNT("Client-server UDP communication is blocked.");
				case ErrorType_NoP2PGroupRelation:
			return _PNT("There is no longer a P2P group that belongs together.");
				case ErrorType_ExceptionFromUserFunction:
			return _PNT("An exception is thrown in the user defined function (an RMI receiving routine or an event handler).");
				case ErrorType_UserRequested:
			return _PNT("This is a failure due to a user request.");
				case ErrorType_InvalidPacketFormat:
			return _PNT("This is a wrong packet type. Either the correspondent host is hacked or it is a bug.");
				case ErrorType_TooLargeMessageDetected:
			return _PNT("A messaging with too big size has been attempted. Please contact the technical support team. If necessary, refer to \ref message_length");
				case ErrorType_NoNeedWebSocketEncryption:
			return _PNT("You cannot encrypt an websocket message.");
				case ErrorType_ValueNotExist:
			return _PNT("This value does not exist");
				case ErrorType_TimeOut:
			return _PNT("This is a time-out.");
				case ErrorType_LoadedDataNotFound:
			return _PNT("The loaded data cannot be located.");
				case ErrorType_SendQueueIsHeavy:
			return _PNT("<a target=\"_blank\" href=\"http://guide.nettention.com/cpp_en#send_queue_heavy\" >Warning function for traffic overload</a> \ref send_queue_heavy. Transmission queue became too big.");
				case ErrorType_TooSlowHeartbeatWarning:
			return _PNT("The heartbeat is too slow on average.");
				case ErrorType_CompressFail:
			return _PNT("The message compression has failed.");
				case ErrorType_LocalSocketCreationFailed:
			return _PNT("Unable to listen client socket or prepare UDP. Must check if there is limitation in number of socket per process or if either TCP or UDP socket is already in use.");
				case ErrorType_NoneAvailableInPortPool:
			return _PNT("Failed binding to local port that defined in Port Pool. Please check number of values in Port Pool are sufficient.");
				case ErrorType_InvalidPortPool:
			return _PNT("Range of user defined port is wrong. Set port to 0(random port binding) or check if it is overlaped.");
				case ErrorType_InvalidHostID:
			return _PNT("Invalid HostID.");
				case ErrorType_MessageOverload:
			return _PNT("The speed of stacking messages are higher than the speed of processing them. Check that you are sending too many messages, or your message processing routines are running too slowly.");
				case ErrorType_DatabaseAccessFailed:
			return _PNT("Accessing database failed. For example, query statement execution failed. You may see the details from comment variable.");
				case ErrorType_OutOfMemory:
			return _PNT("Out of memory.");
				case ErrorType_AutoConnectionRecoveryFailed:
			return _PNT("서버와의 연결이 끊어져서 연결 복구 기능이 가동되었지만, 이것 마저도 실패했습니다.");
				case ErrorType_NotImplementedRmi:
			return _PNT("RMI 함수를 호출 했지만, 함수 구현부가 없습니다.");
				default:
			break;
		}
		return _PNT("<none>");
	}

	const PNTCHAR* ErrorInfo::TypeToString_Chn(ErrorType e)
	{
		switch (e)
		{
				case ErrorType_Ok:
			return _PNT("成功。没有问题。");
				case ErrorType_Unexpected:
			return _PNT("发生例外情况。");
				case ErrorType_AlreadyConnected:
			return _PNT("已经是连接状态。");
				case ErrorType_TCPConnectFailure:
			return _PNT("TCP 连接失败。");
				case ErrorType_InvalidSessionKey:
			return _PNT("错误的对称key。");
				case ErrorType_EncryptFail:
			return _PNT("加密失败。");
				case ErrorType_DecryptFail:
			return _PNT("黑客可能传送了乱码数据，或者是解码失败。");
				case ErrorType_ConnectServerTimeout:
			return _PNT("因服务器连接时间过长而失败。");
				case ErrorType_ProtocolVersionMismatch:
			return _PNT("服务器连接的网络协议版本不同。");
				case ErrorType_InvalidLicense:
			return _PNT("Problem occured while license validation task.");
				case ErrorType_NotifyServerDeniedConnection:
			return _PNT("服务器故意拒绝连接。");
				case ErrorType_Reserved01:
			return _PNT("服务器连接成功！");
				case ErrorType_DisconnectFromRemote:
			return _PNT("对方主机解除了连接。");
				case ErrorType_DisconnectFromLocal:
			return _PNT("这边主机解除了连接。");
				case ErrorType_Reserved02:
			return _PNT("存在可能引起危险状况的因素。");
				case ErrorType_UnknownAddrPort:
			return _PNT("未知的互联网地址。");
				case ErrorType_Reserved03:
			return _PNT("服务器准备不足。");
				case ErrorType_ServerPortListenFailure:
			return _PNT("无法开始玩家socket的listen或者准备UDP。请确认TCP或者UDP socket端口是否已在使用中还是有限量在每个process的socket数量。");
				case ErrorType_AlreadyExists:
			return _PNT("对象已存在。");
				case ErrorType_PermissionDenied:
			return _PNT("接近被拒绝。在<a target=\"_blank\" href=\"http://guide.nettention.com/cpp_zh#dbc2_nonexclusive_overview\" >访问共享性数据</a>%功能上，如果DB cache server被设置成不使用共享接近功能的话便会发生此状况。在window，vista 以上的OS中，没有管理员权限呼出 CNetUtil::EnableTcpOffload ( bool enable )函数的话，可能会发生此状况。");
				case ErrorType_BadSessionGuid:
			return _PNT("错误的session Guid。");
				case ErrorType_InvalidCredential:
			return _PNT("错误的credential。");
				case ErrorType_InvalidHeroName:
			return _PNT("错误的hero name。");
				case ErrorType_Reserved06:
			return _PNT("加载过程unlock以后，lock 后发生云集。");
				case ErrorType_Reserved07:
			return _PNT("输出参数AdjustedGamerIDNotFilled没有被填充。");
				case ErrorType_Reserved08:
			return _PNT("玩家角色不存在。");
				case ErrorType_UnitTestFailed:
			return _PNT("单元测试失败。");
				case ErrorType_P2PUdpFailed:
			return _PNT("peer-to-peer UDP 通信堵塞。");
				case ErrorType_ReliableUdpFailed:
			return _PNT("P2P reliable UDP失败。");
				case ErrorType_ServerUdpFailed:
			return _PNT("玩家-服务器的UDP通信堵塞。");
				case ErrorType_NoP2PGroupRelation:
			return _PNT("再没有一同所属的P2P组。");
				case ErrorType_ExceptionFromUserFunction:
			return _PNT("从用户定义函数（RMI 收信例程或者事件handler）exception 被throw。");
				case ErrorType_UserRequested:
			return _PNT("用户邀请造成的失败。");
				case ErrorType_InvalidPacketFormat:
			return _PNT("错误的数据包形式。可能是对方主机被黑或者是存在漏洞。");
				case ErrorType_TooLargeMessageDetected:
			return _PNT("试图使用了过大的messaging。请咨询技术支援部。必要时请参考<a target=\"_blank\" href=\"http://guide.nettention.com/cpp_zh#message_length\" >限制通信信息大小</a>%。");
				case ErrorType_NoNeedWebSocketEncryption:
			return _PNT("websocket 信息不能encrypt。");
				case ErrorType_ValueNotExist:
			return _PNT("不存在的值。");
				case ErrorType_TimeOut:
			return _PNT("超时。");
				case ErrorType_LoadedDataNotFound:
			return _PNT("无法找到加载的数据。");
				case ErrorType_SendQueueIsHeavy:
			return _PNT("<a target=\"_blank\" href=\"http://guide.nettention.com/cpp_zh#send_queue_heavy\" >送信量过多时发生警告的功能</a>。传送queue过大。");
				case ErrorType_TooSlowHeartbeatWarning:
			return _PNT("Heartbeat 平均太慢。");
				case ErrorType_CompressFail:
			return _PNT("信息压缩失败。");
				case ErrorType_LocalSocketCreationFailed:
			return _PNT("无法开始玩家socket的listen或者准备UDP。请确认TCP或者UDP socket端口是否已在使用中还是有限量在每个process的socket数量。");
				case ErrorType_NoneAvailableInPortPool:
			return _PNT("生成socket的时候往Port Pool内port number的bind失败。任意port number被代替使用。请确认Port Pool的个数是否充分。");
				case ErrorType_InvalidPortPool:
			return _PNT("Port pool 中一个以上的内值出错。确认把端口设为0（任意端口binding）或者是否重复。");
				case ErrorType_InvalidHostID:
			return _PNT("无效的HostID。");
				case ErrorType_MessageOverload:
			return _PNT("消息的堆叠速度高于处理它们的速度。检查您要发送的邮件太多，或消息处理程序运行过慢。");
				case ErrorType_DatabaseAccessFailed:
			return _PNT("Accessing database failed. For example, query statement execution failed. You may see the details from comment variable.");
				case ErrorType_OutOfMemory:
			return _PNT("内存不足.");
				case ErrorType_AutoConnectionRecoveryFailed:
			return _PNT("서버와의 연결이 끊어져서 연결 복구 기능이 가동되었지만, 이것 마저도 실패했습니다.");
				case ErrorType_NotImplementedRmi:
			return _PNT("RMI 함수를 호출 했지만, 함수 구현부가 없습니다.");
				default:
			break;
		}
		return _PNT("<none>");
	}

	const PNTCHAR* ErrorInfo::TypeToString_Jpn(ErrorType e)
	{
		switch (e)
		{
				case ErrorType_Ok:
			return _PNT("成功。問題なし。");
				case ErrorType_Unexpected:
			return _PNT("例外状況発生。");
				case ErrorType_AlreadyConnected:
			return _PNT("既に接続されています。");
				case ErrorType_TCPConnectFailure:
			return _PNT("TCP接続失敗");
				case ErrorType_InvalidSessionKey:
			return _PNT("間違った対称キー");
				case ErrorType_EncryptFail:
			return _PNT("暗号化が失敗しました。");
				case ErrorType_DecryptFail:
			return _PNT("ハッカーが壊れたデータを転送したか復号化に失敗しました。");
				case ErrorType_ConnectServerTimeout:
			return _PNT("サーバー接続過程がとてもかかり過ぎて失敗処理になりました。");
				case ErrorType_ProtocolVersionMismatch:
			return _PNT("サーバー接続のためのプロトコルバージョンが違います。");
				case ErrorType_InvalidLicense:
			return _PNT("Problem occured while license validation task.");
				case ErrorType_NotifyServerDeniedConnection:
			return _PNT("サーバーが接続を意図的に拒否しました。");
				case ErrorType_Reserved01:
			return _PNT("サーバー接続成功！");
				case ErrorType_DisconnectFromRemote:
			return _PNT("相手側のホストが接続を解除しました。");
				case ErrorType_DisconnectFromLocal:
			return _PNT("内側のホストが接続を解除しました。");
				case ErrorType_Reserved02:
			return _PNT("危険な状況を引き起こす可能性のある因子があります。");
				case ErrorType_UnknownAddrPort:
			return _PNT("知られていないインターネットアドレス");
				case ErrorType_Reserved03:
			return _PNT("サーバー準備不足");
				case ErrorType_ServerPortListenFailure:
			return _PNT("サーバーソケットのlistenを開始することができません。TCPまたはUDPソケットが既に使用中であるポートなのかを確認してください。");
				case ErrorType_AlreadyExists:
			return _PNT("既に個体が存在します。");
				case ErrorType_PermissionDenied:
			return _PNT("接近が拒否されました。<a target=\"_blank\" href=\"http://guide.nettention.com/cpp_jp#dbc2_nonexclusive_overview\" >Accessing non-exclusive data</a> 機能で, DB cache serverが非独占的接近機能を使用しないと設定されていると発生する可能性があります。Window Vista以上のOSで管理者権限なしに CNetUtil::EnableTcpOffload( bool enable ) 関数が呼び出されると発生する可能性があります。");
				case ErrorType_BadSessionGuid:
			return _PNT("間違ったSession Guidです。");
				case ErrorType_InvalidCredential:
			return _PNT("間違ったCredentialです。");
				case ErrorType_InvalidHeroName:
			return _PNT("間違ったHero nameです。");
				case ErrorType_Reserved06:
			return _PNT("ローディング過程がアンロック後にロックし、ねじれが発生しました。");
				case ErrorType_Reserved07:
			return _PNT("出力パラメーターのAdjustedGamerIDNotFilledが満たされていません。");
				case ErrorType_Reserved08:
			return _PNT("プレーヤーキャラクターが存在しません。");
				case ErrorType_UnitTestFailed:
			return _PNT("ユニットテスト失敗");
				case ErrorType_P2PUdpFailed:
			return _PNT("peer-to-peer UDP通信が詰まりました。");
				case ErrorType_ReliableUdpFailed:
			return _PNT("P2P reliable UDPが失敗しました。");
				case ErrorType_ServerUdpFailed:
			return _PNT("クライアントサーバーUDP通信が詰まりました。");
				case ErrorType_NoP2PGroupRelation:
			return _PNT("これ以上一緒に所属されたP2Pグループはありません。");
				case ErrorType_ExceptionFromUserFunction:
			return _PNT("ユーザー定義関数(RMI受信ルーチンまたはイベントハンドラー)でexceptionがthrowされました。");
				case ErrorType_UserRequested:
			return _PNT("ユーザーの要請による失敗です。");
				case ErrorType_InvalidPacketFormat:
			return _PNT("間違ったパケット形式です。相手側のホストがハッキングされたか、バグである可能性があります。");
				case ErrorType_TooLargeMessageDetected:
			return _PNT("あまりにも大きいサイズのメッセージングが試されました。技術支援部までお問合せください。必要によって、\ref message_lengthをご参照ください。");
				case ErrorType_NoNeedWebSocketEncryption:
			return _PNT("websocketメッセージはencryptできません。");
				case ErrorType_ValueNotExist:
			return _PNT("存在しない値です。");
				case ErrorType_TimeOut:
			return _PNT("タイムアウトです。");
				case ErrorType_LoadedDataNotFound:
			return _PNT("ロードされたデータが見つかりません。");
				case ErrorType_SendQueueIsHeavy:
			return _PNT("\ref send_queue_heavy . 送信キューがあまりにも大き過ぎです。");
				case ErrorType_TooSlowHeartbeatWarning:
			return _PNT("Heartbeatが平均的にとても遅いです。");
				case ErrorType_CompressFail:
			return _PNT("メッセージ圧縮に失敗しました。");
				case ErrorType_LocalSocketCreationFailed:
			return _PNT("クライアントソケットのlistenを開始することができません。TCPまたはUDPソケットが既に使用中であるポートなのかをご確認ください。");
				case ErrorType_NoneAvailableInPortPool:
			return _PNT("Socketを生成する時、Port Pool内のPort Numberへのバインドに失敗しました。代わりに任意のPort Numberが使われました。Port Poolの数が十分なのかをご確認ください。");
				case ErrorType_InvalidPortPool:
			return _PNT("Port pool内の値のうち一つ以上が間違っています。ポートを0(任意ポートバインディング)にするか重複されていないかをご確認ください。");
				case ErrorType_InvalidHostID:
			return _PNT("有効ではないHostID値です。");
				case ErrorType_MessageOverload:
			return _PNT("ユーザーが消化するメッセージ処理速度より内部的に積もるメッセージの速度がもっと速いです。あまりにも多過ぎるメッセージを送信しようとしていないか、またはユーザーのメッセージ受信関数があまりにも遅く作動してはいないかを確認してください。");
				case ErrorType_DatabaseAccessFailed:
			return _PNT("Accessing database failed. For example, query statement execution failed. You may see the details from comment variable.");
				case ErrorType_OutOfMemory:
			return _PNT("メモリーが足りません。");
				case ErrorType_AutoConnectionRecoveryFailed:
			return _PNT("서버와의 연결이 끊어져서 연결 복구 기능이 가동되었지만, 이것 마저도 실패했습니다.");
				case ErrorType_NotImplementedRmi:
			return _PNT("RMI 함수를 호출 했지만, 함수 구현부가 없습니다.");
				default:
			break;
		}
		return _PNT("<none>");
	}
}
