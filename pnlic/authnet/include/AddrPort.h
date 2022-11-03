/*
ProudNet HERE_SHALL_BE_EDITED_BY_BUILD_HELPER

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

#include "BasicTypes.h"
#include "FakeClr.h"

#if defined(__unix__)
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <winsock2.h>
#pragma comment(lib,"Ws2_32.lib")
#else
#include <arpa/inet.h>
#include <errno.h>
#endif

#ifdef PF_MAX
#undef PF_MAX
#endif

#if (_MSC_VER>=1400)
#pragma managed(push, off)
#endif

//#pragma pack(push,8)

namespace Proud
{
	/** \addtogroup net_group
	*  @{
	*/
	class CFastSocket;

	struct NamedAddrPort;

	/**

	\~korean
	소켓 에러 코드

	\~english
	Socket Error Code

	\~chinese
	Socket 错误代码

	\~japanese
	\~
	*/
	enum SocketErrorCode
	{
		SocketErrorCode_Ok = 0,
		// EINTR와 WSAEINTR은 서로 다른 값이다. 따라서 Win32에서는 EINTR을 쓰면 안된다.
		// 따라서, 어쩔 수 없이 아래와 같이 서로 다르게 구별될 수밖에 없다.
#if !defined(_WIN32)
		SocketErrorCode_Error = -1,
		SocketErrorCode_Timeout = ETIMEDOUT,
		SocketErrorCode_ConnectionRefused = ECONNREFUSED,
		SocketErrorCode_ConnectResetByRemote = ECONNRESET,
		SocketErrorCode_AddressNotAvailable = EADDRNOTAVAIL,
		SocketErrorCode_NotSocket = ENOTSOCK,
		SocketErrorCode_WouldBlock = EWOULDBLOCK,
		SocketErrorCode_AccessError = EACCES,
		SocketErrorCode_InvalidArgument = EINVAL,
		SocketErrorCode_Intr = EINTR, // EINTR in linux
		SocketErrorCode_InProgress = EINPROGRESS, // operation이 이미 진행중이다.
		SocketErrorCode_Again = EAGAIN, // win32에서는 would-block이 대체.
		SocketErrorCode_AlreadyIsConnected = EISCONN,
		SocketErrorCode_AlreadyAttempting = EALREADY, // The socket is nonblocking and a previous connection attempt has not yet been completed.
		SocketErrorCode_NetUnreachable = ENETUNREACH,
#else
		SocketErrorCode_Error = SOCKET_ERROR,
		SocketErrorCode_Timeout = WSAETIMEDOUT,
		SocketErrorCode_ConnectionRefused = WSAECONNREFUSED,
		SocketErrorCode_ConnectResetByRemote = WSAECONNRESET,
		SocketErrorCode_AddressNotAvailable = WSAEADDRNOTAVAIL,
		SocketErrorCode_NotSocket = WSAENOTSOCK,
		SocketErrorCode_ShutdownByRemote = WSAEDISCON,	// FD_CLOSE or FD_SEND에서의 이벤트
		SocketErrorCode_WouldBlock = WSAEWOULDBLOCK,
		SocketErrorCode_IoPending = WSA_IO_PENDING,
		SocketErrorCode_AccessError = WSAEACCES,
		SocketErrorCode_OperationAborted = ERROR_OPERATION_ABORTED,
		SocketErrorCode_InvalidArgument = WSAEINVAL,
		SocketErrorCode_Intr = WSAEINTR, // EINTR in linux
		SocketErrorCode_InProgress = WSAEINPROGRESS, // operation이 이미 진행중이다.
		SocketErrorCode_AlreadyIsConnected = WSAEISCONN,
		SocketErrorCode_AlreadyAttempting = WSAEALREADY,
		SocketErrorCode_Cancelled = WSAECANCELLED,
		SocketErrorCode_NetUnreachable = WSAENETUNREACH,
#endif
	};

	enum ShutdownFlag
	{
#if !defined(_WIN32)
		ShutdownFlag_Send = SHUT_WR,
		ShutdownFlag_Receive = SHUT_RD,
		ShutdownFlag_Both = SHUT_RDWR,
#else
		ShutdownFlag_Send = SD_SEND,
		ShutdownFlag_Receive = SD_RECEIVE,
		ShutdownFlag_Both = SD_BOTH,
#endif
	};

	/**
	\~korean
	IP, Port 식별자

	\~english
	IP, Port Identifier

	\~chinese
	IP, Port识别者

	\~japanese
	\~
	*/
	struct PROUD_API AddrPort
	{
		/**
		\~korean
		호스트 IP이다. 111.111.111.111 형태이다.
		- ipv4 기준이라 uint32_t이다. 이것을 문자열화하려면 ToDottedIP 또는 ToString을 사용하면 된다.
		- 미지정시 0xffffffff이다. 0x00000000인 경우 Any IP임을 의미한다.

		\~english
		his is host IP in 111.111.111.111 format.
		- It's uint32_t based on ipv4. To convert them to string, use ToDottedIP or ToString.
		- If not appointed, it's 0xffffffff. If it's 0x00000000, it means Any IP.

		\~chinese
		主机IP。111.111.111.111形态。
		- 以ipv4为基准的uint32_t。要想把这个字符串化需要使用ToDottedIP或ToString即可。
		- 未指定时是0xffffffff。的话意味着Any IP。

		\~japanese
		\~
		*/
		uint32_t m_binaryAddress;

		/**
		\~korean
		포트 번호
		- native endian이고, network endian가 아니다. 즉 socket 함수 htons나 ntohs를 통해 변환할 필요가 없이 그대로 사용해도 된다.

		\~english
		Port Number
		- It's native endian, not network endian. So it can be used as-is without converting it through htons or ntohs.

		\~chinese
		端口编号
		- 是native endian，而不是network endian。也就是说，没有必要通过socket函数htons或ntohs转变，也可以直接使用。

		\~japanese
		\~
		*/
		uint16_t m_port;

		inline uint8_t* GetIPv4() const { return (uint8_t*)&m_binaryAddress; }

		/**
		\~korean
		두 주소가 같은 LAN에 있는 주소인지 /24 범위 내에서 검사한다.
		물론 이는 편법이다. 같은 /24 동일 주소도 서로 다른 LAN일 수 있고(가령 VLAN 트렁킹)
		반대로 /16인데도 같은 LAN일 수도 있다. (대규모 LAN)
		이러한 경우까지는 잡아내지 못한다. 따라서 이 함수가 무조건 같은 LAN인지 판별해주는 것이 아님을 주의할 것. 확률적으로 같은 LAN인지 검사해줄 뿐이다.
		이 함수는 외부 주소가 서로 같은지 결과와 같이 검사할 때 사용할 것. 이 함수만 쓰지 말고. 외부 주소는 다르나 내부 주소가 같은 경우가 흔하기 때문이다.

		\~english TODO:translate needed.

		\~chinese
		在/24范围内检查两个地址是否是在相同LAN的地址。
		当然这是简便的方法。但是相同的/24范围内的地址有可能是不同的LAN ， 相反即使是/16范围内的地址也有可能是相同的LAN。（大规模LAN）
		这种情况是检测不到的。 所以这个涵数不能完全能分辨出是否是相同的LAN, 只是以概率检查是否有相同的LAN而已
		此涵数在外部地址是否相同跟结果一起检查时使用。不要只使用此函数。因为外部地址虽然不同，内部地址相同的情况很多。

		\~japanese
		\~
		*/
		inline bool IsSameSubnet24(const AddrPort &a) const
		{
			return a.GetIPv4()[0] == GetIPv4()[0] &&
				a.GetIPv4()[1] == GetIPv4()[1] &&
				a.GetIPv4()[2] == GetIPv4()[2];
		}

		inline bool IsSameHost(const AddrPort &a) const
		{
			return m_binaryAddress == a.m_binaryAddress;
		}

		/**
		\~korean
		생성자

		\~english
		Generator

		\~chinese
		生成者

		\~japanese
		\~
		*/
		inline AddrPort()
		{
			m_binaryAddress = 0; // any
			m_port = 0;
		}

		/**
		\~korean
		생성자
		\param binAddr host ID
		\param port 포트 번호

		\~english
		Generator
		\param binAddr host ID
		\param port port number

		\~chinese
		生成者
		\param binAddr host ID
		\param port 端口号码

		\~japanese
		\~
		*/
		inline AddrPort(uint32_t binAddr, uint16_t port)
		{
			m_binaryAddress = binAddr;
			m_port = port;
		}

		/**
		\~korean
		Socket API 파라메터 sockaddr_in 구조체로부터 값을 가져온다.

		\~english
		This method gets a value from Socket API Parameter, sockaddr_in structure.

		\~chinese
		从Socket API参数的sockaddr_in构造体来取值。

		\~japanese
		\~
		*/
		void ToNative(sockaddr_in& out);
		/**
		\~korean
		Socket API 파라메터 sockaddr_in 구조체에게 값을 준다.

		\~english
		This method passes a value to Socket API Parameter, sockaddr_in structure.

		\~chinese
		给予Socket API参数sockaddr_in构造体赋值。

		\~japanese
		\~
		*/
		void FromNative(const sockaddr_in& in);

		/**
		\~korean
		로컬 컴퓨터의 IP를 얻습니다. 하지만 로컬 IP가 여럿인 호스트인 경우를 위해, Proud.CNetUtil.GetLocalIPAddresses 를 더 권장합니다.

		\~english
		This method obtains an IP of local computer. But in case there is a host with several IPs, we recommend you to use Proud.CNetUtil.GetLocalIPAddresses.

		\~chinese
		取得本地主机的IP。更提倡使用 Proud.CNetUtil.GetLocalIPAddresses%。

		\~japanese
		\~
		*/
		static AddrPort GetOneLocalAddress();

		/**
		\~korean
		xxx.xxx.xxx.xxx:xxxx 문자열 추출

		\~english
		Extract xxx.xxx.xxx.xxx:XXXX string

		\~chinese
		抽出字符串 xxx.xxx.xxx.xxx:XXXX

		\~japanese
		\~
		*/
		virtual String ToString() const;

		/**
		\~korean
		xxx.xxx.xxx.xxx:xxxx 문자열 추출

		\~english
		Extract xxx.xxx.xxx.xxx:XXXX string

		\~chinese
		抽出字符串 xxx.xxx.xxx.xxx:XXXX

		\~japanese
		\~
		*/
		virtual String IPToString() const;

		/**
		\~korean
		IP address 문자열과 port를 입력받아, AddrPort 객체를 리턴합니다.
		host name은 처리할 수 없습니다. 대신 FromHostNamePort()를 사용하세요.
		\param ipAddress IP 주소 값입니다. 예를 들어 "11.22.33.44"입니다.
		\param port 포트 값입니다.

		\~english
		Input an IP address string and a port to get the return value from AddrPort object.
		Host name can’t be processed. Use FromHostNamePort() instead.
		\param ipAddress IP is the address value. Ex. “11.22.33.44”
		\param port is the port value.

		\~chinese
		\~

		\~japanese
		\~
		*/
		static AddrPort FromIPPort(const String& ipAddress, uint16_t port);

		/**
		\~korean
		hostName에 대한 ip address를 얻고 port를 채웁니다.
		FromIPPort()와 다음 차이가 있습니다.
		- DNS로 인식 못하는 주소는 처리 못합니다. 예: "11.22.33.44", "239.255.255.0"
		- DNS를 액세스하므로 블러킹이 발생할 수 있습니다. 파일 캐시되면 빠르겠지만 그래도 소량 블러킹은 생깁니다.
		- IP 주소가 아닌 문자열을 변환해줍니다. 가령 "myhost.mygame.com"을 변환합니다.
		\param hostName 호스트 이름입니다. "11.22.33.44"같은 IP주소나 "myhost.mygame.com" 같은 값도 사용 가능합니다.
		\param port 포트 값입니다.

		\~english
		Gain Ip address from the hostname and fills up the port.
		There is an alteration in the FromIPPort().
		- Incompatible IP address which does not detect as the DNS can’t be processed. Ex. “11.22.33.44”, “239.255.255.0”
		- When accesing the DNS, blocking can be occured. It would be faster if the file cache in process, however, minimum blocking might occur.
		- Convert the string which is not an IP address. Ex. Convert ”myhost.mygame.com"
		\param hostname is hostname. An IP address like "11.22.33.44" and value such as "myhost.mygame.com" are possible to use.
		\param port is port value.

		\~chinese
		\~

		\~japanese
		\~
		*/
		static AddrPort FromHostNamePort(const String& hostName, uint16_t port);

		/**
		\~korean
		hostName을 입력하면 그것의 xxx.xxx.xxx.xxx:xxxx 형식의 IP address를 변환해서 리턴합니다.

		\~english
		If hostName is typed in, this method converts and returns it in the format of IP address; xxx.xxx.xxx.xxx:xxxx.

		\~chinese
		输入hostName会把它的 xxx.xxx.xxx.xxx:xxxx 形式IP address变换并返回。

		\~japanese
		\~
		*/
		static AddrPort GetIPAddress(String hostName);

		/**
		\~korean
		이 컴퓨터의 IP address 문자열 구하기. 단, port는 제외한다.

		\~english
		Gets IP address string of this computer except port.

		\~chinese
		求此电脑的IP address字符串。但，除了port。

		\~japanese
		\~
		*/
		String ToDottedIP() const;

		/**
		\~korean
		NamedAddrPort 객체로부터 값을 가져온다.
		- NamedAddrPort의 host name이 "my.somename.net" 형태인 경우 이를 IP address로 변환해서 가져온다.

		\~english
		This method gets a value from NamedAddrPort object.
		- If host name of NamedAddrPort is in "my.somename.net" format, then this method converts the host name to IP address and returns it.

		\~chinese
		从NamedAddrPort对象取值。
		- 当NamedAddrPort的host name是"my.somename.net"形态的时，把它转换成IP address后取回。

		\~japanese
		\~
		*/
		static AddrPort From(const NamedAddrPort& src);

		/**
		\~korean
		미지정 IP 객체. 디폴트 값이다.

		\~english
		Undesignated IP object. It is default value.

		\~chinese
		未指定的IP对象，是默认值。

		\~japanese
		\~
		*/
		static AddrPort Unassigned;

		/**
		\~korean
		Any IP로 지정되어 있는가?

		\~english
		Is it designated Any IP?

		\~chinese
		是否被指定为Any IP？

		\~japanese
		\~
		*/
		inline bool IsAnyIP()
		{
			return m_binaryAddress == 0;
		}

		/**
		\~korean
		로컬 컴퓨터를 가리키는 IP인가?

		\~english
		Is this IP pointing local computer?

		\~chinese
		是指定本地电脑的IP吗？

		\~japanese
		\~
		*/
		bool IsLocalIP();

		/**
		\~korean
		브로드캐스트 주소도 아니고, null 주소도 아닌, 1개 호스트의 1개 포트를 가리키는 정상적인 주소인가?

		\~english
		Is it correct address that point 1 port of 1 host instead of broadcast address, null address?

		\~chinese
		不是broadcast地址，也不是null地址，是否是指一个主机上一个端口的正常地址？

		\~japanese
		\~
		*/
		bool IsUnicastEndpoint();

		inline bool operator<( const AddrPort& rhs )
		{
			if (m_binaryAddress < rhs.m_binaryAddress)
				return true;
			else if (m_binaryAddress > rhs.m_binaryAddress)
				return false;
			else
				return m_port < rhs.m_port;
		}

		inline uint32_t Hash() const
		{
			return m_binaryAddress ^ m_port;
		}
	};

	/**
	\~korean
	AddrPort와 비슷하지만 m_addr에 문자열이 들어간다.
	- 문자열에는 111.222.111.222 형태 또는 game.aaa.com 형식의 이름이 들어갈 수 있다.
	- AddrPort은 111.222.111.222 형태만이 저장될 수 있는 한계 때문에 이 구조체가 필요한거다.

	\~english
	It's similar to AddrPort but string can be inserted in m_addr.
	- At string, either 111.222.111.222 format or game.aaa.com format can be added.
	- This structure is needed since AddrPort can only store 111.222.111.222 format.

	\~chinese
	与AddrPortt相似，但是m_addr里有字符串。
	- 字符串里会有111.222.111.222格式，或者game.aaa.com格式的名称。
	- 因为AddrPort只能存储111.222.111.222格式，所以需要此构造。

	\~japanese
	\~
	*/
	struct NamedAddrPort
	{
		/**
		\~korean
		호스트 이름
		- 문자열에는 111.222.111.222 형태 또는 game.aaa.com 형식의 이름이 들어갈 수 있다.

		\~english
		Host name
		- At string, either 111.222.111.222 format or game.aaa.com format can be added.

		\~chinese
		主机名
		- 字符串里可以添加111.222.111.222或者game.aaa.com格式的名称。

		\~japanese
		\~
		*/
		String m_addr;

		/**
		\~korean
		포트 값

		\~english
		Port Value

		\~chinese
		端口值

		\~japanese
		\~
		*/
		uint16_t m_port;

		/**
		\~korean
		빈 주소

		\~english
		Empty Address

		\~chinese
		空地址

		\~japanese
		\~
		*/
		static NamedAddrPort PROUD_API Unassigned;

		/**
		\~korean
		특정 hostname, 포트 번호로부터 이 객체를 생성한다.

		\~english
		Generate this object from a specific hostname and port number.

		\~chinese
		特定的hostname，从端口号码生成此对象。

		\~japanese
		\~
		*/
		static NamedAddrPort PROUD_API FromAddrPort(String addr, uint16_t port);

		/**
		\~korean
		AddrPort 객체로부터 이 객체를 생성한다

		\~english
		Generate this object from AddrPort object

		\~chinese
		从AddrPort对象生成这个对象。

		\~japanese
		\~
		*/
		static NamedAddrPort PROUD_API From(const AddrPort &src);

		/**
		\~korean
		hostname이 들어있는 문자열인 경우 들어있는 문자열로 새 호스트 이름을 지정한다.

		\~english
		Appoint a new host name with the string inside of hostname.

		\~chinese
		有hostname字符串的情况，使用内在字符串指定新的主机名。

		\~japanese
		\~
		*/
		PROUD_API void OverwriteHostNameIfExists(String hostName);

		/**
		\~korean
		비교 연산자

		\~english
		Comparison Operator

		\~chinese
		比较运算符

		\~japanese
		\~
		*/
		inline bool operator==( const NamedAddrPort &rhs ) const
		{
			return m_addr == rhs.m_addr && m_port == rhs.m_port;
		}

		/**
		\~korean
		비교 연산자

		\~english
		Comparison Operator

		\~chinese
		比较运算符

		\~japanese
		\~
		*/
		inline bool operator!=( const NamedAddrPort &rhs ) const
		{
			return !( m_addr == rhs.m_addr && m_port == rhs.m_port );
		}

		/**
		\~korean
		내용물을 문자열로 만든다.

		\~english
		This method makes the contents into string.

		\~chinese
		把内容改成字符串。

		\~japanese
		\~
		*/
		String PROUD_API ToString() const;

		/**
		\~korean
		로컬 컴퓨터를 가리키는 IP인가?

		\~english
		Is this IP pointing local computer?

		\~chinese
		是指定本地计算机的IP吗？

		\~japanese
		\~
		*/
		bool IsLocalIP();

		/**
		\~korean
		브로드캐스트 주소도 아니고, null 주소도 아닌, 1개 호스트의 1개 포트를 가리키는 정상적인 주소인가?

		\~english
		Is it correct address that point 1 port of 1 host instead of broadcast address, null address?

		\~chinese
		不是broadcast地址，也不是null地址，是否是指定一个主机上一个端口的正常地址？

		\~japanese
		\~
		*/
		bool IsUnicastEndpoint();
	};

	inline void AppendTextOut(String& a, AddrPort &b)
	{
		a += b.ToString();
	}

	inline void AppendTextOut(String& a, NamedAddrPort &b)
	{
		a += b.ToString();
	}

	inline bool operator!=( const AddrPort &a, const AddrPort &b )
	{
		return a.m_binaryAddress != b.m_binaryAddress || a.m_port != b.m_port;
	}
	inline bool operator==( const AddrPort &a, const AddrPort &b )
	{
		return a.m_binaryAddress == b.m_binaryAddress && a.m_port == b.m_port;
	}

	inline bool operator<( const AddrPort& a, const AddrPort& b )
	{
		if (a.m_binaryAddress < b.m_binaryAddress)
			return true;
		if (a.m_binaryAddress > b.m_binaryAddress)
			return false;
		return a.m_port < b.m_port;
	}

	/**
	\~korean
	네트워크 프로그래밍 관련 유틸리티 클래스

	\~english
	Network programming related utility class

	\~chinese
	网络编程相关的应用程序类

	\~japanese
	\~
	*/
	class CNetUtil
	{
	public:
//#if !defined __MARMALADE__ && !defined __FreeBSD__
		/**
		\~korean
		호스트가 갖고 있는 로컬 IP 주소를 모두 얻어냅니다.

		\~english
		Gets every local IP addresses that host has.

		\~chinese
		取得主机拥有的所有本地IP地址。

		\~japanese
		\~
		*/
		PROUD_API static void GetLocalIPAddresses(CFastArray<String> &output);
//#endif // !defined(__MARMALADE__)

		/**
		\~korean
		호스트가 갖고 있는 로컬 IP 주소 중 하나를 아무거나 얻어냅니다.
		이 메서드는 비권장됩니다. 가능하면 GetLocalIPAddresses()을 쓰시기 바랍니다.

		\~english
		Get any one of local IP addresses among all that host has.
		This method is not recommended. If possible, please use GetLocalIPAddresses()

		\~chinese
		取得主机拥有的任意一个本地IP地址 。
		不推荐此方法。尽量使用GetLocalIPAddresses()。

		\~japanese
		\~
		*/
		PROUD_API static String GetOneLocalAddress();

#if defined(_WIN32)

		/**
		\~korean
		일부 온보드 저가형 네트워크 아답타의 경우 하드웨어에서 지원하는 TCP offload 기능에 문제가 있을 수 있습니다.
		이 메서드는 윈도우 레지스트리에서 해당 기능을 제어하며 재부팅을 필요로 합니다.
		\return ErrorType_Ok 이면 성공적으로 적용했으며, 재부팅을 요합니다. Windows Vista 이상의 OS 에서 관리자 권한을 획득하지 않으면 ErrorType_PermissionDenied 에러가 발생할 수 있습니다. 기타 오류의 경우 ErrorType_Unexpected 가 리턴됩니다.

		\~english
		TCP offload function will occur problem with some cheap network adapter that intergrated on the mother board.
		This method is controlled by Windows registry and it require restart a machine.
		It successfully applied, if \return ErroType_Ok is true. and it require restart. ErrorType_PermissionDenied will occur, if you do not get aminitrator permission on Windows Vista or above version. ErrorType_Unexpected will return with other errors

		\~chinese
		一些廉价的网络适配器集成，可能会在硬件支持的TCP offload技能产生问题。这个方法会在Windows注册表上控制有关技能，并需要重新启动。、
		\return ErrorType_Ok的话表示应用成功，需重新启动。在Windows Vista以上的OS里不获得管理权限，会发生ErrorType_PermissionDenied错误。其他错误的时候会返回ErrorType_Unexpected。

		\~japanese
		\~
		*/
		PROUD_API static ErrorType EnableTcpOffload(bool enable);
#endif
	};

	/**  @}  */
}

template<>
class CPNElementTraits < Proud::AddrPort >
{
public:
	typedef const Proud::AddrPort& INARGTYPE;
	typedef Proud::AddrPort& OUTARGTYPE;

	inline static uint32_t Hash(INARGTYPE a)
	{
		return a.m_binaryAddress ^ a.m_port;
	}

	inline static bool CompareElements(INARGTYPE element1, INARGTYPE element2)
	{
		return element1 == element2;
	}

	inline static int CompareElementsOrdered(INARGTYPE element1, INARGTYPE element2)
	{
		if (element1 < element2)
			return -1;
		if (element1 == element2)
			return 0;
		return 1;
	}
};

// template<>
// class CPNElementTraits<Proud::AddrPort>
// {
// public:
// 	typedef const Proud::AddrPort& INARGTYPE;
// 	typedef Proud::AddrPort& OUTARGTYPE;
//
// 	inline static ULONG Hash( INARGTYPE element )
// 	{
// 		assert(sizeof(element.m_binaryAddress) == 4);
// 		return element.m_binaryAddress ^ element.m_port;
//
// 		return ret;
// 	}
//
// 	inline static bool CompareElements( INARGTYPE element1, INARGTYPE element2 ) throw()
// 	{
// 		return ( element1 == element2 ) ? true:false;
// 	}
//
// 	inline static int CompareElementsOrdered( INARGTYPE element1, INARGTYPE element2 ) throw()
// 	{
// 		if(element1.m_binaryAddress < element2.m_binaryAddress)
// 			return -1;
// 		else if(element1.m_binaryAddress < element2.m_binaryAddress)
// 			return 1;
// 		if(element1.m_port < element2.m_port)
// 			return -1;
// 		else if(element1.m_port > element2.m_port)
// 			return 1;
// 		return 0;
// 	}
// };

#if (_MSC_VER>=1400)
#pragma managed(pop)
#endif

//#pragma pack(pop)
