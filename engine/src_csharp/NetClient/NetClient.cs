/*
ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.

ProudNet

This program is soley copyrighted by Nettention.
Any use, correction, and distribution of this program are subject to the terms and conditions of the License Agreement.
Any violated use of this program is prohibited and will be cause of immediate termination according to the License Agreement.

** WARNING : PLEASE DO NOT REMOVE THE LEGAL NOTICE ABOVE.

ProudNet

此程序的版权归Nettention公司所有。
与此程序的修改、使用、发布相关的事项要遵守此程序的所有权者的协议。
不遵守协议时要原则性的禁止擅自使用。
擅自使用的责任明示在与此程序所有权者的合同书里。

** 注意：不要移除关于制作物的上述明示。

*/
using System;
using System.Collections.Generic;
using System.Text;
using System.Threading;
using System.Net;
using System.Net.Sockets;
using System.Diagnostics;
using System.IO;
using System.Reflection;

namespace Nettention.Proud
{
	internal class ClientNativeExceptionString
	{
		// 필요한 dll 파일들이 작업 경로에 없을 때
		public const string TypeInitializationExceptionString = @"Please check if necessary DLL files (ProudNetClientPlugin.dll, ProudNetClient.dll) exist on the working directory of your program. If there's no such files, you should copy these files to the working directory of your program.";
	}

	public class NetClient : IRmiHost, IDisposable
	{
		// C++ CNetClient가 SWIG로 바로 변환되어 쓰이지 않고 이렇게 NativeNetClient라는 클래스로 뽑아진 이후 간접적으로 쓰이는 이유는 다음과 같습니다
		// CNetClient::Create 함수를 통해 CNetClient 객체를 생성합니다. CNetClient 객체의 생성&소멸을 내부에서 관리하기 위해서 입니다.
		// RmiProxy, RmiStub, NetClientEvent 가상함수를 Unity iOS에서는 바로 사용할 수 없습니다.
		// C# <-> wrapper <-> C++로 실행이 되는데, wrapper 클래스들을 handling 해야 하기 때문입니다.(AttachProxy/AttactStub/SetEventSink API가 있기 때문에)
		// Native NetClient
		private Nettention.Proud.NativeNetClient m_native_NetClient = null;

		// Native NetClientEvent
		private NativeInternalNetClientEvent m_native_NetClientEvent = null;

		private const String DuplicatedRmiIDErrorText = "Duplicated RMI ID is found. Review RMI ID declaration in .PIDL files.";
		private const String AsyncCallbackMayOccurErrorText = "Already async callback may occur! server start or client connection should have not been done before here!";
		private const String BadRmiIDErrorText = "Wrong RMI ID is found. RMI ID should be >=1300 or < 60000.";

		private List<RmiProxy> m_proxyList = new List<RmiProxy>();
		private List<RmiStub> m_stubList = new List<RmiStub>();

		protected List<RmiID> m_proxyRmiIDList= new List<RmiID>();
		protected List<RmiID> m_stubRmiIDList = new List<RmiID>();

		private bool m_allowExceptionEvent = true;

		private bool m_disposed = false;

		private static bool IsWorkingInUnityRuntime()
		{
			bool ret = false;
			try
			{
				try // 이중 try. catch 구문 안에서도 throw가 있을 수 있으니.
				{
					var engineAssemblyName = new AssemblyName("UnityEngine");
					Assembly engineAssembly = Assembly.Load(engineAssemblyName); // 실패시 null을 리턴 안하고 throw IOException이네? 젠장.
					if (engineAssembly != null)
					{
						ret = true;
					}
				}
				catch (System.IO.FileNotFoundException)
				{
					// 유니티 안쓰는 앱에서는 여기로 온다.
					// UnityEngine.dll이 없다. 사용자가 non-unity Mono 가령 Xamarin을 쓰고 있을 수 있으므로
					// 이것을 검사하도록 하자.
					// 다행히, Unity에서도 이것은 사용 가능한 변수이지만 undocumented이기 때문에 맨아래 catch Exception으로 갈 수도 있음을 염두해야.
				}
				catch (TargetInvocationException)
				{
					// 유니티 안쓰는 앱이지만 같은 디렉토리에 UnityEditor.dll이 있는 경우(해킹 시도나 PNTest등)에는 여기로 온다.
					// 이때는 win32라고 가정해 버리자.
					// 다행히 iOS에서는 여기로 오지 않는다. (오면 큰일나지! 동접10 제한 문제에 걸리니까!)
				}
				catch (MethodAccessException)
				{
					// unity web player는 reflection을 쓸 수 없어서인지, 여기로 오게 된다.
					// 따라서 브라우저 플러그인에서 실행되는 것이라 간주하면 된다.
				}
			}
			catch (Exception)
			{
				// iPhone player에서는 Assembly.Load가 null을 리턴하거나 exception을 던진다고 함.
				// http://forum.unity3d.com/threads/78517-Assembly-Load-crashes-iOS
			}

			return ret;
		}

		// UnityEngine 에디터에서 사용할 때 에디터를 종료하는 과정에서
		// GC에 의해 메모리가 정리되는 과정에서 불규칙적으로 CriticalSection 소멸자에서 ShowUserMisuseError 함수가 호출됩니다.
		// 비 정상적인 상황이긴 하지만 C++ 코드에서는 발생하지 않으며 에디터가 종료가 될 때에만 발생하기 때문에 유니티일 경우에는
		// ErrorReactionType을 DebugOutputType으로 변경합니다. (throw가 발생해도 무시합니다. 동작에는 문제가 없습니다).
		private void ChangeErrorReactionTypeToDebugOutputType()
		{
			if (IsWorkingInUnityRuntime() == true)
			{
				ProudNetClientPlugin.ChangeErrorReactionTypeToDebugOutputTypeWhenUnityEngine();
			}
		}

		public NetClient()
		{
			ChangeErrorReactionTypeToDebugOutputType();

			try
			{
				m_native_NetClientEvent = new NativeInternalNetClientEvent(this);
				m_native_NetClient = Nettention.Proud.NativeNetClient.Create(true);
				m_native_NetClient.SetEventSink(m_native_NetClientEvent.GetNativeNetClientEvent());
			}
			catch (System.TypeInitializationException ex)
			{
				throw new System.Exception(ClientNativeExceptionString.TypeInitializationExceptionString);
			}
		}

		~NetClient()
		{
			Dispose(false);
		}

		/// <summary>
		/// NetClient 객체를 삭제할 경우, Dispose 함수를 호출하여야 합니다.
		/// </summary>
		public void Dispose()
		{
			Dispose(true);
			GC.SuppressFinalize(this);
		}

		protected virtual void Dispose(bool disposing)
		{
			if (m_disposed)
				return;

			if (disposing)
			{
				// 관리되는 자원을 해제

				if (m_native_NetClient != null)
				{
					m_native_NetClient.Disconnect();
					m_native_NetClient.Dispose();
					m_native_NetClient = null;
				}

				m_native_NetClientEvent.Dispose();

				for (int i = 0; i < m_proxyList.Count; ++i)
				{
					m_proxyList[i].m_native_Internal.Dispose();
				}

				m_proxyList.Clear();
				m_proxyRmiIDList.Clear();

				for (int i = 0; i < m_stubList.Count; ++i)
				{
					m_stubList[i].m_native_Internal.Dispose();
				}

				m_stubList.Clear();
				m_stubRmiIDList.Clear();

				/* Q: m_native_NetClientEvent는 delete 안하나요?
				* A: 안하는 것이 맞습니다. dispose pattern은 socket같은, 시스템 리소스를 사용하는 경우에 쓰는 것이고,
				* 단지 데이터만을 갖고 있는 객체 즉 시스템 리소스를 전혀 가지지 않는 것들은 dispose pattern의 대상이 아닙니다.
				* m_native_NetClient의 타입 내에서 gcobject가   garbage될 때 native C++ 객체를 delete하기는 합니다만,
				* 이것도 garbage collector가 필요할 때 할 뿐입니다. */
			}

			// 관리되지 않는 자원을 해제
			// 큰 변수들을 null로 설정한다.

			m_disposed = true;
		}

		/**
		\~korean
		내부 함수. 사용자는 호출 금지.

		\~english
		Internal function. User must not call this.

		\~chinese
		内部函数，用户禁止呼出。

		\~japanese

		\~
		*/
		public void NotifyException(HostID hostID, System.Exception ex)
		{
			if (exceptionHandler!= null)
			{
				exceptionHandler(hostID, ex);
			}
		}


		/**
		내부 함수. 사용자는 호출 금지.
		Internal function. User must not call this.
		*/
		public bool IsExceptionEventAllowed()
		{
			return m_allowExceptionEvent;
		}

		/**
		\~korean
		C# 코드에서 발생한 예외를 프라우드넷 내부에서 처리할지 유무를 설정합니다.
		Unity3D에서 Exception 에러 로그를 프라우드넷 내부 코드가 핸들링하면서 유니티에서 로그가 출력되지 않는 문제가 있을 때 요긴합니다.
		기본값은 true입니다.

		\~english Determines whether to throw exception in C# code.
		Useful if you experience problem where Unity3D log output is not printed as exception is handled by ProudNet internal code.
		Default value is true.

		\~chinese
		Determines whether to throw exception in C# code.
		Useful if you experience problem where Unity3D log output is not printed as exception is handled by ProudNet internal code.
		Default value is true.

		\~japanese
		Determines whether to throw exception in C# code.
		Useful if you experience problem where Unity3D log output is not printed as exception is handled by ProudNet internal code.
		Default value is true.
		\~
		*/
		public void AllowExceptionEvent(bool value)
		{
			m_allowExceptionEvent = value;
		}

		/**
		\~korean
		C++ CNetClient API를 사용하기 위해서 접근하는 위한 Property 입니다.
		단, namespace Nettention.Proud에 있는 클래스를 사용해야 합니다.

		\~english

		\~chinese

		\~japanese

		\~
		*/
		public Nettention.Proud.NativeNetClient NativeNetClient
		{
			get { return m_native_NetClient; }
		}

		/**
		\~korean
		PIDL 컴파일러의 결과물 중 proxy를 이 객체에 등록한다.

		\~english
		Registers proxy among the results of PIDL compiler to this object

		\~chinese
		在PIDL编译结果物中，将proxy登录到此客体。

		\~japanese

		\~
		*/
		public void AttachProxy(RmiProxy proxy)
		{
			if (proxy == null)
			{
				throw new NullReferenceException();
			}

			RmiID[] rmiIDList = proxy.GetRmiIDList();

			for (int i = 0; i < proxy.RmiIDList.Length; i++)
			{
				if ((ushort)rmiIDList[i] >= 60000)
					throw new Exception(BadRmiIDErrorText);

				// 이미 컨테이너 안에 해당값이 들어있다면 exception 발생
				if (m_proxyRmiIDList.Contains(rmiIDList[i]))
				{
					throw new Exception(DuplicatedRmiIDErrorText);
				}
				else
				{
					m_proxyRmiIDList.Add(rmiIDList[i]);
				}
			}

			proxy.m_core = this;
			m_proxyList.Add(proxy);

			m_native_NetClient.AttachProxy(proxy.m_native_Internal.GetNativeProxy());
		}

		/**
		\~korean
		PIDL 컴파일러의 결과물 중 stub을 이 객체에 등록한다.

		\~english
		Registers stub among the results of PIDL compiler to this object

		\~chinese
		在PIDL编译结果物中，将stub登录到此客体。


		\~japanese

		\~
		*/

		/// <summary>
		/// 중복된 RMI ID를 가진 RmiStub을 Attach하면 Exception을 발생시킵니다.
		/// </summary>
		public void AttachStub(RmiStub stub)
		{
			if (stub == null)
			{
				throw new NullReferenceException();
			}

			RmiID[] rmiIDList = stub.GetRmiIDList;

			for (int i = 0; i < rmiIDList.Length; i++)
			{
				if ((ushort)rmiIDList[i] >= 60000)
					throw new Exception(BadRmiIDErrorText);

				// 이미 컨테이너 안에 해당값이 들어있다면 exception 발생
				if (m_stubRmiIDList.Contains(rmiIDList[i]))
				{
					throw new Exception(DuplicatedRmiIDErrorText);
				}
				else
				{
					m_stubRmiIDList.Add(rmiIDList[i]);
				}
			}

			stub.m_core = this;
			m_stubList.Add(stub);

			m_native_NetClient.AttachStub(stub.m_native_Internal.GetNativeStub());
		}

		/**
		\~korean
		내부 함수. 사용자는 호출 금지.

		\~english
		Internal function. User must not call this.

		\~chinese
		内部函数，用户禁止呼出。

		\~japanese

		\~
		*/
		public void ShowNotImplementedRmiWarning(String RMIName)
		{

		}

		/**
		\~korean
		내부 함수. 사용자는 호출 금지.

		\~english
		Internal function. User must not call this.

		\~chinese
		内部函数，用户禁止呼出。

		\~japanese

		\~
		*/
		public void PostCheckReadMessage(Message msg, String RMIName)
		{

		}

		/*
		\~korean
		내부 함수. 사용자는 호출 금지.

		\~english
		Internal function. User must not call this.

		\~chinese
		内部函数，用户禁止呼出。

		\~japanese

		\~
		*/
		public bool IsSimplePacketMode()
		{
			return false;
		}

		public HostID LocalHostID
		{
			get { return GetLocalHostID(); }
		}

		/**
		\~korean
		서버 연결 요청을 시작한다.
		NetConnectionParam 에 필요한 값들을 채워 넣어야 한다.
		- 이 함수는 즉시 리턴한다. 이 함수가 리턴했다고 해서 서버와의 연결이 모두 끝난 것은 아니다.
		- 이 함수 호출 후 joinServerCompleteHandler에 세팅을 하고, 이벤트가 도착한 후에야 서버 연결의 성사 여부를 파악할 수 있다.
		\param param 서버에 연결시 NetConnectionParam 객체를 인자로 받는다.
		\return 이미 다른 서버에 연결된 상태이면 false를 리턴한다. 성공적으로 연결 요청을 시작했으면 true가 리턴.

		\~english TODO:translate needed.

		\~chinese
		开始连接服务器请求。
		要在NetConnectionParam中填充所需值。
		- 此函数立即Return。此函数Return并不代表结束与服务器的连接。
		- 呼出此函数后，在joinServerCompleteHandler中进行设置。在Event到达后才能了解服务器连接的成功与否。
		\param param 连接到服务器时，用因子接收 NetConnectionParam%客体。
		\return 如果已连接至其他服务器则Return  false。成功开始连接请求的话将Return true。

		\~japanese

		\~
		*/
		public bool Connect(Nettention.Proud.NetConnectionParam param)
		{
			string comment = "";
			if (IsValidConnectionParam(param, ref comment) == false)
			{
				throw new System.Exception(comment);
			}

			return m_native_NetClient.Connect(param);
		}

		public bool Connect(Nettention.Proud.NetConnectionParam param, Nettention.Proud.ErrorInfo outError)
		{
			string comment = "";
			if (IsValidConnectionParam(param, ref comment) == false)
			{
				throw new System.Exception(comment);
			}

			return m_native_NetClient.Connect(param, outError);
		}

		private bool IsValidConnectionParam(Nettention.Proud.NetConnectionParam param, ref string comment)
		{
			if (param == null)
			{
				comment = "Nettention.Proud.NetConnectionParam is null";
				return false;
			}

			if (param.serverPort <= 0)
			{
				comment = string.Format("{0}, serverPort = {1}", "NetConnectionParam.serverPort is invalid value", param.serverPort);
				return false;
			}

			return true;
		}

		/**
		\~korean
		서버와의 연결을 해제한다. 아울러 모든 P2P 그룹에서도 탈퇴한다.
		- 자세한 것은 동명 메서드 참조

		\~english
		Terminates the connection to server and withdraws from all P2P group.
		- Please refer to same name method for more detail.

		\~chinese
		解除与服务器的连接，随之退出所有P2P组。
		- 详细内容请参考同名Mathod。

		\~japanese

		\~
		*/

		/// <summary>
		/// NetClient.Disconnect 함수를 호출하면 내부적으로 현재 사용하고 있는 리소스(스레드, 기타 객체)를 정리하는 작업을 수행합니다.
		/// </summary>
		public void Disconnect()
		{
			m_native_NetClient.Disconnect();
		}

		/**
		\~korean
		서버와의 연결을 해제한다. 아울러 모든 P2P 그룹에서도 탈퇴한다.
		\param gracefulDisconnectTimeout 서버와의 연결을 해제하는 과정을 처리하기 위해 클라이언트는 일정 시간의
		시간을 요구한다. 이 값은 서버와의 연결을 해제하는 데까지 허락하는 최대 시간(초)이다.
		이 값은 통상적으로 1 이내가 적당하지만, 너무 지나치게 작은 값을 잡는 경우, 클라이언트는 서버와의
		연결을 종료했지만 서버측에서 클라이언트의 연결 해제를 즉시 감지하지 못하는 경우가 있을 수 있다.
		\param comment 여기에 채워진 데이터는 INetServerEvent.OnClientLeave에서 받아진다.
		즉 클라이언트가 서버와의 연결을 해제하되 서버에게 마지막으로 데이터를 보내고자 할 때(예: 접속을 끊는 이유를 보내기) 유용하다.
		gracefulDisconnectTimeout가 너무 짧으면 못 갈수 있다.

		\~english
		Terminates the connection to server and withdraws from all P2P group.
		\param gracefulDisconnectTimeout Client requires a certain amount of time in order to process the steps to terminate the connection to server. This value is the maximum time(in second) that allowed to complete the termination.
		Usually 1 is reasonable for the value but if it is too small then there can be some cases that server cannot detect clinet's diconnection immediately after client terminated the connection.
		\param comment The data filled in here will be received at INetServerEvent.OnClientLeave.
		In other words, it is useful when client need to send its last data to server before terminating its connection to server.(e.g. sending why terminating the connection)
		If gracefulDisconnectTimeout is too short then there is a chance the sending fails.

		\~chinese
		解除与服务器的连接。之后退出所有P2P组。
		\param gracefulDisconnectTimeout 为了处理与服务器连接的解除过程，client 要求一定时间的时间。此值是解除与服务器的连接的允许的最大时间（秒）。
		此值一般1以内为适当，用过小的值的话可能会发生client与服务器的连接虽已终止，服务器方不能立即感知client的连接解除的情况。
		\param comment 往这里填充的数据在 INetServerEvent.OnClientLeave%接收。即client解除与服务器的连接，想最后给服务器传送数据的时候有用（例：发送断开连接的理由）。
		gracefulDisconnectTimeout 太短的话可能不能到达。

		\~japanese
		\~
		*/

		public void Disconnect(DisconnectArgs args)
		{
			if (args == null)
			{
				throw new System.ArgumentNullException("args");
			}
			m_native_NetClient.Disconnect(args);
		}

		/**
		\~korean
		Added for emergency use if Disconnect() never returns though it has automatic non-wait functionality.

		\~english
		Added for emergency use if Disconnect() never returns though it has automatic non-wait functionality.

		\~chinese
		Added for emergency use if Disconnect() never returns though it has automatic non-wait functionality.

		\~japanese
		* Added for emergency use if Disconnect() never returns though it has automatic non-wait functionality.
		\~
		*/

		public void DisconnectAsync()
		{
			if (m_native_NetClient == null)
			{
				return;
			}
			m_native_NetClient.DisconnectAsync();
		}

		/**
		\~korean
		이 호스트의 local HostID를 구한다. HostID_None이면 서버에 연결 안됐다는 뜻.

		\~english
		Gets local HostID of this host. If HostID_None then means not connected to server

		\~chinese
		求此主机的local Host ID。HostID_None 的话表示没有与服务器连接。

		\~japanese
		\~
		*/
		public HostID GetLocalHostID()
		{
			if (m_native_NetClient == null)
			{
				return HostID.HostID_None;
			}

			return (HostID)m_native_NetClient.GetLocalHostID();
		}

		/**
		\~korean
		시작 이래 수집된 처리량 통계를 얻는다.

		\~english
		Gets the statistics of process collected since beginning

		\~chinese
		获取在开始之后所收集的处理量统计。

		\~japanese

		\~
		*/
		public Nettention.Proud.NetClientStats GetStats()
		{
			return m_native_NetClient.GetStats();
		}

		/**
		\~korean
		서버와의 소켓 연결 여부를 리턴합니다.
		- 서버와 소켓이 연결 되었는지의 여부만 리턴합니다.

		\~english
		Return connect socket to server or not.
		- Only return connected socket to server or not.

		\~chinese
		Return与服务器的Socket连接与否。
		- 只Return与服务器的连接与否。

		\~japanese

		\~
		*/
		public bool HasServerConnection()
		{
			return m_native_NetClient.HasServerConnection();
		}

		/**
		\~korean
		수신된 RMI나 이벤트를 처리합니다.
		가장 마지막에 FrameMove을 호출한 이후부터 지금까지 서버로부터 수신된 RMI나 INetClientEvent의 콜백 이벤트는 클라이언트 메모리에 누적되어 있습니다.
		그리고 그것들을 이 메서드에 의해 일제히 콜백이 발생하게 합니다.
		- 사용자는 일정 시간마다 이를 호출해서 RMI 수신 처리 및 발생 이벤트를 처리하도록 해야 합니다. 일반적인 경우 사용자는 이 메서드를 매 렌더 프레임마다 호출합니다.
		- 이 메서드를 장시간 호출 안한다고 해서 타 호스트나 서버와의 연결이 끊어지는 일은 없습니다.

		\param maxWaitTimeMs 처리할 이벤트나 수신 메시지가 있을 때까지, 얼마나 오래 기다릴 것인지에 대한 값입니다. 0이면 기다리지 않고 즉시 리턴합니다.
		게임 등 렌더링 루프 안에서는 0이 일반적이며, 렌더링 루프가 없는 일반 앱에서는 적당한 값 (가령 200ms)를 넣습니다.
		\param outResult FrameMove 호출시 처리 결과 보고를 얻습니다. 생략 가능한 파라메터입니다.

		\~english
		Handles received RMI or event
		All of RMI from server or callbcak event of INetClientEvent occurred since the last FrameMove call are stacked within memory.
		And those let callbacks happen by this method.
		- User is to call this periodically in order to handle RMI reception process and event occurred. Usually, user calls this method at every render frame.
		- By not calling this method for a long time does not cause the connection between server or other hosts terminated.

		\param maxWaitTimeMs  It is the value of how long it will wait until there is a processed event or in-coming message,  If it is 0, do not wait and return immediately.
		0 is general in the rendering loop including game, and etc.  Put the appropriate value (for example, 200ms) in the general app without the rendering loop.
		\param outResult Gets the report to process result when FrameMove is called. Ignorable parameter.

		\~chinese
		处理收信的RMI或者event。
		从最后呼叫FrameMove开始到现在从服务器收信的RMI或者INetClientEvent的回拨event累计在client内存里。
		然后利用此方法一齐发生回调。
		- 用户要在每一段时间呼叫它进行RMI收信处理及event发生处理。一般的情况下用户把此方法在每个render frame呼叫。
		- 不会发生因没有长时间呼叫此方法而与其他主机或者服务器的连接断开的事情。

		\param maxWaitTimeMs  是对要处理的活动或者到有收信信息的时候为止,以及需要等多长时间的价格。如果是 0 的话,则立即返回,不将等待。
		在游戏等绘制圈里,0是一般性的, 在没有绘制圈的应用软件里,放入适当的价格(例如200ms)。
		\param outResult FrameMove呼叫时获得处理结果报告。可以省略的参数。

		\~japanese

		\param maxWaitTimeMs  処理するイベントや受信メッセージがあるまで、どれほど長く待つのかについた値です。0には待たずに直ちにリターンします。
		ゲームなどレンダリングループの中では0が一般的であり、レンダリングループがない一般アプリでは適当な値(例えば、200ms)を入れます。

		\~
		*/
		public void FrameMove()
		{
			try
			{
				m_native_NetClient.FrameMove();
			}
			catch(System.Exception ex)
			{
				NotifyException(GetLocalHostID(),ex);
			}

		}

		public Nettention.Proud.FrameMoveResult FrameMove(int maxWaitTime)
		{
			Nettention.Proud.FrameMoveResult result = new Proud.FrameMoveResult();

			try
			{
				m_native_NetClient.FrameMove(maxWaitTime, result);
			}
			catch(System.Exception ex)
			{
				if (result != null)
				{
					result.Dispose();
				}
				NotifyException(GetLocalHostID(), ex);
			}

			return result;
		}

		/**
		\~korean
		이 호스트에 연결된 다른 호스트(서버의 경우 클라이언트들, 클라이언트의 경우 피어들)들 각각에의 tag를 지정합니다. \ref host_tag  기능입니다.
		- 자기 자신(서버 혹은 클라이언트)에 대한 tag도 지정할 수 있습니다.

		\~english
		Desigantes tag to each of other hosts(client for server, peer for client) aht are connected to this host. A \ref host_tag function.
		- Can designate tag to itself (server or client)

		\~chinese
		指定连接到此主机（如果是服务器则指定各客户端，如果是客户端则指定各Peer）的其他各个主机的tag。是\ref host_tag%功能。
		- 也可以指定自身（服务器或客户端）tag。

		\~japanese

		\~
		*/
		public bool SetHostTag(Proud.HostID remote, long hostTag)
		{
			return m_native_NetClient.SetHostTag((Proud.HostID)remote, new IntPtr(hostTag));
		}

		/**
		\~korean
		Remote peer의 마지막 레이턴시를 얻는다.
		\param remoteHostID 찾으려는 remote peer의 HostID. HostID_Server를 지정하면 서버와의 레이턴시를 얻는다.
		\return ping time을 밀리초단위로 리턴한다. 단 없으면 -1을 리턴한다.

		\~english
		Gets the last latency of Remote peer
		\param remoteHostID HostID of remote peer to find. Gets the latency with server when set HostID_Server.
		\return returns ping time(in millisecond). Returns -1 if there is none.

		\~chinese
		获得Remote peer的最后latency。
		\param remoteHostID 要找的remote peer的HostID。指定HostID_Server的话获得与服务器的latency。
		\return ping把time以毫秒单位返回。没有的话返回-1。

		\~japanese
		Remote peerの最後のレイテンシを得る。
		\param remoteHostID 探そうとするremote peerのHostID. HostID_Serverを指定するとサーバとのレイテンシを得る。
		\return ping timeをミリ秒単位でリターンする。ただし、 ping timeが測定されていなければ-1をリターンする。

		\~
		*/
		public int GetLastUnreliablePingMs(Proud.HostID remote)
		{
			return m_native_NetClient.GetLastUnreliablePingMs((Proud.HostID)(remote));
		}

		/**
		\~korean
		Remote peer의 마지막 레이턴시를 얻는다.
		\param remoteHostID 찾으려는 remote peer의 HostID. HostID_Server를 지정하면 서버와의 레이턴시를 얻는다.
		\param error 에러 여부를 리턴한다. 정상적인 경우 ErrorType_Ok, Ping을 얻지 못한 경우 ErrorType_ValueNotExist가 저장된다.
		\return ping time을 초단위로 리턴한다. 단 없으면 -1을 리턴한다.

		\~english
		Gets the last latency of Remote peer
		\param remoteHostID HostID of remote peer to find. Gets the latency with server when set HostID_Server.
		\param error It returns whether an error occurs or not. In case of normal case, ErrorType_Ok will be saved, however, ErrorType_ValueNotExist will be saved in case Ping is not received.
		\return returns ping time(in second). Returns -1 if there is none.

		\~chinese
		获得remote peer的最后latency。
		\param remoteHostID 要找的remote peer的HostID。指定HostID_Server的话获得与服务器的latency。
		\param error 返回错误与否。如果正常则为ErrorType_Ok，如果没有获得Ping则会储存ErrorType_ValueNotExist。
		\return ping把time以秒单位返回。没有的话返回-1。

		\~japanese
		Remote peerの最後のレイテンシを得る。
		\param remoteHostID 探そうとするremote peerのHostID. HostID_Serverを指定するとサーバとのレイテンシを得る。
		\param error エラーがあるかないかをリターンする。正常的な場合はErrorType_Ok, Pingを得なかった場合にはErrorType_ValueNotExistが保存される。
		\return ping timeを秒単位でリターンする。ただし、 ping timeが測定されていなければ-1をリターンする。

		\~
		*/
		public double GetLastUnreliablePingSec(HostID remoteHostID)
		{
			return m_native_NetClient.GetLastUnreliablePingSec(remoteHostID);
		}

		/**
		\~korean
		Remote peer의 마지막 레이턴시를 얻는다. GetLastUnreliablePing값은 Unreliable로 핑값을 구하는 반면 해당함수는 ReliableUdp로 측정한다.
		\param remoteHostID 찾으려는 remote peer의 HostID. HostID_Server를 지정하면 서버와의 레이턴시를 얻는다.
		\param error 에러 여부를 리턴한다. 정상적인 경우 ErrorType_Ok, Ping을 얻지 못한 경우 ErrorType_ValueNotExist가 저장된다.
		\return ping time을 밀리초단위로 리턴한다. 단 없으면 -1을 리턴한다.

		\~english
		Gets the last latency of Remote peer. GetLastUnreliablePing value is found by Unreliable but the relevant function is measured by ReliableUdp.
		\param remoteHostID HostID of remote peer to find. Gets the latency with server when set HostID_Server.
		\param error It returns whether an error occurs or not. In case of normal case, ErrorType_Ok will be saved, however, ErrorType_ValueNotExist will be saved in case Ping is not received.
		\return returns ping time(in millisecond). Returns -1 if there is none.

		\~chinese
		获得Remote peer的最后Latency。GetLastUnreliablePing 值用Unreliable求ping值，但相应函数用ReliableUdp测定。
		\param remoteHostID 要找的remote peer的HostID。指定HostID_Server的话获得与服务器的latency。
		\param error 返回错误与否。如果正常则为ErrorType_Ok，如果没有获得Ping则会储存ErrorType_ValueNotExist。
		\return ping把time以毫秒单位返回。没有的话返回-1。

		\~japanese
		Remote peerの最後のレイテンシを得る。 GetLastUnreliablePing値はUnreliableでPING値を求める反面、該当関数は ReliableUdpで測定する。
		\param remoteHostID 探そうとするremote peerのHostID. HostID_Serverを指定するとサーバとのレイテンシを得る。
		\param error エラーがあるかないかをリターンする。正常的な場合はErrorType_Ok, Pingを得なかった場合にはErrorType_ValueNotExistが保存される。
		\return ping timeをミリ秒単位でリターンする。ただし、 ping timeが測定されていなければ-1をリターンする。

		\~
		*/
		public int GetLastReliablePingMs(HostID remoteHostID)
		{
			return m_native_NetClient.GetLastReliablePingMs(remoteHostID);
		}

		/**
		\~korean
		Remote peer의 마지막 레이턴시를 얻는다. GetLastUnreliablePing값은 Unreliable로 핑값을 구하는 반면 해당함수는 ReliableUdp로 측정한다.
		\param remoteHostID 찾으려는 remote peer의 HostID. HostID_Server를 지정하면 서버와의 레이턴시를 얻는다.
		\param error 에러 여부를 리턴한다. 정상적인 경우 ErrorType_Ok, Ping을 얻지 못한 경우 ErrorType_ValueNotExist가 저장된다.
		\return ping time을 초단위로 리턴한다. 단 없으면 -1을 리턴한다.

		\~english
		Gets the last latency of Remote peer. GetLastUnreliablePing value is found by Unreliable but the relevant function is measured by ReliableUdp.
		\param remoteHostID HostID of remote peer to find. Gets the latency with server when set HostID_Server.
		\param error It returns whether an error occurs or not. In case of normal case, ErrorType_Ok will be saved, however, ErrorType_ValueNotExist will be saved in case Ping is not received.
		\return returns ping time(in second). Returns -1 if there is none.

		\~chinese
		获得Remote peer的最后Latency。GetLastUnreliablePing 值用Unreliable求Ping值，但相应函数用ReliableUDP测定。
		\param remoteHostID 要找的remote peer的HostID。指定HostID_Server的话获得与服务器的latency。输入\ref p2p_group%的Host ID的话获得P2P组内的所有成员的平均latency。
		\param error 返回错误与否。如果正常则为ErrorType_Ok，如果没有获得Ping则会储存ErrorType_ValueNotExist。
		\return ping把time以秒单位返回。没有的话返回-1。

		\~japanese
		Remote peerの最後のレイテンシを得る。 GetLastUnreliablePing値はUnreliableでPING値を求める反面、該当関数は ReliableUdpで測定する。
		\param remoteHostID 探そうとするremote peerのHostID. HostID_Serverを指定するとサーバとのレイテンシを得る。
		\param error エラーがあるかないかをリターンする。正常的な場合はErrorType_Ok, Pingを得なかった場合にはErrorType_ValueNotExistが保存される。
		\return ping timeを秒単位でリターンする。ただし、 ping timeが測定されていなければ-1をリターンする。

		\~
		*/
		public double GetLastReliablePingSec(HostID remoteHostID)
		{
			return m_native_NetClient.GetLastReliablePingSec(remoteHostID);
		}

		/**
		\~korean
		Remote peer 의 최근 레이턴시를 얻는다. GetRecentUnreliablePing 값은 Unreliable로 핑값을 구하는 반면 해당함수는 ReliableUdp 로 측정한다.
		\param remoteHostID 찾으려는 remote peer의 HostID. HostID_Server를 지정하면 서버와의 레이턴시를 얻는다.
		\param error 에러 여부를 리턴한다. 정상적인 경우 ErrorType_Ok, Ping을 얻지 못한 경우 ErrorType_ValueNotExist가 저장된다.
		\return ping time을 밀리초단위로 리턴한다. 단 없으면 -1을 리턴한다.

		\~english
		Gets the recent latency of Remote peer. GetRecentUnreliablePing value is found by Unreliable but the relevant function is measured by ReliableUdp.
		\param remoteHostID HostID of remote peer to find. Gets the latency with server when set HostID_Server.
		\param error It returns whether an error occurs or not. In case of normal case, ErrorType_Ok will be saved, however, ErrorType_ValueNotExist will be saved in case Ping is not received.
		\return returns ping time(in millisecond). Returns -1 if there is none.

		\~chinese
		获得Remote peer的最近Latency。GetRecentUnreliablePing 值用Unreliable求Ping值，但相应函数则用ReliableUDP测定。
		\param remoteHostID 要找的remote peer的HostID。指定HostID_Server的话获得与服务器的latency。
		\param error 返回错误与否。如果正常则为ErrorType_Ok，如果没有获得Ping则会储存ErrorType_ValueNotExist。
		\return ping把time以毫秒单位返回。没有的话返回-1。

		\~japanese
		Remote peerの最近のレイテンシを得る。 GetRecentUnreliablePing値はUnreliableでPING値を求める反面、該当関数は ReliableUdpで測定する。
		\param remoteHostID 探そうとするremote peerのHostID. HostID_Serverを指定するとサーバとのレイテンシを得る。
		\param error エラーがあるかないかをリターンする。正常的な場合はErrorType_Ok, Pingを得なかった場合にはErrorType_ValueNotExistが保存される。
		\return ping timeをミリ秒単位でリターンする。ただし、 ping timeが測定されていなければ-1をリターンする。

		\~
		*/
		public int GetRecentReliablePingMs(HostID remoteHostID)
		{
			return m_native_NetClient.GetRecentReliablePingMs(remoteHostID);
		}

		/**
		\~korean
		Remote peer 의 최근 레이턴시를 얻는다. GetRecentUnreliablePing 값은 Unreliable로 핑값을 구하는 반면 해당함수는 ReliableUdp 로 측정한다.
		\param remoteHostID 찾으려는 remote peer의 HostID. HostID_Server를 지정하면 서버와의 레이턴시를 얻는다.
		\param error 에러 여부를 리턴한다. 정상적인 경우 ErrorType_Ok, Ping을 얻지 못한 경우 ErrorType_ValueNotExist가 저장된다.
		\return ping time을 초단위로 리턴한다. 단 없으면 -1을 리턴한다.

		\~english
		Gets the recent latency of Remote peer. GetRecentUnreliablePing value is found by Unreliable but the relevant function is measured by ReliableUdp.
		\param remoteHostID HostID of remote peer to find. Gets the latency with server when set HostID_Server.
		\param error It returns whether an error occurs or not. In case of normal case, ErrorType_Ok will be saved, however, ErrorType_ValueNotExist will be saved in case Ping is not received.
		\return returns ping time(in second). Returns -1 if there is none.

		\~chinese
		获得Remote peer的最近Latency。GetRecentUnreliablePing 值用Unreliable求Ping值，但相应函数则用ReliableUDP测定。
		\param remoteHostID 要找的remote peer的HostID。指定HostID_Server的话获得与服务器的latency。
		\param error 返回错误与否。如果正常则为ErrorType_Ok，如果没有获得Ping则会储存ErrorType_ValueNotExist。
		\return ping把time以秒单位返回。没有的话返回-1。

		\~japanese
		Remote peerの最近のレイテンシを得る。 GetRecentUnreliablePing値はUnreliableでPING値を求める反面、該当関数は ReliableUdpで測定する。
		\param remoteHostID 探そうとするremote peerのHostID. HostID_Serverを指定するとサーバとのレイテンシを得る。
		\param error エラーがあるかないかをリターンする。正常的な場合はErrorType_Ok, Pingを得なかった場合にはErrorType_ValueNotExistが保存される。
		\return ping timeを秒単位でリターンする。ただし、 ping timeが測定されていなければ-1をリターンする。

		\~

		*/
		public double GetRecentReliablePingSec(HostID remoteHostID)
		{
			return m_native_NetClient.GetRecentReliablePingSec(remoteHostID);
		}

		/**
		\~korean
		Remote peer의 최근 레이턴시를 얻는다.
		\param remoteHostID 찾으려는 remote peer의 HostID. HostID_Server를 지정하면 서버와의 레이턴시를 얻는다.
		\param error 에러 여부를 리턴한다. 정상적인 경우 ErrorType_Ok, Ping을 얻지 못한 경우 ErrorType_ValueNotExist가 저장된다.
		\return ping time을 밀리초단위로 리턴한다. 단 없으면 -1을 리턴한다.

		\~english
		Gets the recent latency of Remote peer
		\param remoteHostID HostID of remote peer to find. Gets the latency with server when set HostID_Server.
		\param error It returns whether an error occurs or not. In case of normal case, ErrorType_Ok will be saved, however, ErrorType_ValueNotExist will be saved in case Ping is not received.
		\return returns ping time(in millisecond). Returns -1 if there is none.

		\~chinese
		获得Remote peer的最近latency。
		\param remoteHostID 要找的remote peer的HostID。指定HostID_Server的话获得与服务器的latency。
		\param error 返回错误与否。如果正常则为ErrorType_Ok，如果没有获得Ping则会储存ErrorType_ValueNotExist。
		\return ping把time以毫秒单位返回。没有的话返回-1。

		\~japanese
		Remote peerの最近のレイテンシを得る。
		\param remoteHostID 探そうとするremote peerのHostID. HostID_Serverを指定するとサーバとのレイテンシを得る。
		\param error エラーがあるかないかをリターンする。正常的な場合はErrorType_Ok, Pingを得なかった場合にはErrorType_ValueNotExistが保存される。
		\return ping timeをミリ秒単位でリターンする。ただし、 ping timeが測定されていなければ-1をリターンする。

		\~
		*/
		public int GetRecentUnreliablePingMs(HostID remoteHostID)
		{
			return m_native_NetClient.GetRecentUnreliablePingMs(remoteHostID);
		}

		/**
		\~korean
		이 호스트가 가입한 모든 P2P 그룹 리스트를 얻는다.

		\~english
		Gets the entire P2P group list that this host is participating

		\~chinese
		获取主机加入的所有P2P组列表。

		\~japanese

		\~
		*/
		public Nettention.Proud.HostIDArray GetLocalJoinedP2PGroups()
		{
			NativeHostIDArray native = m_native_NetClient.GetLocalJoinedP2PGroups();

			if ((native == null) || (native.GetCount() == 0))
			{
				return null;
			}

			Proud.HostIDArray idArray = new HostIDArray();
			int count = native.GetCount();
			for (int i = 0; i < count; ++i)
			{
				idArray.Add(native.At(i));
			}
			return idArray;
		}

		/**
		\~korean
		\brief 이 객체에 연결된 P2P 피어의 정보를 얻습니다.

		\~english TODO:translate needed.

		\~chinese
		\brief 获取连接到此客体的P2P Peer的情报。

		\~japanese

		*/
		public Nettention.Proud.NetPeerInfo GetPeerInfo(Proud.HostID remote)
		{
			return m_native_NetClient.GetPeerInfo((Proud.HostID)remote);
		}

		/**
		\~korean
		서버와의 소켓 연결 상태를 리턴합니다.
		- 소켓의 연결 상태만 리턴합니다. 완전히 연결되었는지는 OnJoinServerComplete로 콜백됩니다.
		\param output 서버와의 연결 상태가 채워지는 공간
		\return 서버와의 연결 상태

		\~english
		Returns connection status to server
		\param output where the status is filled
		\return connection status to server

		\~chinese
		返回服务器和socket连接状态。
		- 值返回socket连接状态。是否完全连接用OnJoinServerComplete回调。
		\param output 填充与服务器的连接状态的空间。
		\return 与服务器的连接状态。

		\~japanese
		\~
		*/
		public ConnectionState GetServerConnectionState(ServerConnectionState output)
		{
			if (output == null)
			{
				throw new System.NullReferenceException("ServerConnectionState output");
			}

			ConnectionState ret = m_native_NetClient.GetServerConnectionState(output);
			return ret;
		}

		/**
		\~korean
		이 호스트가 remotePeerID가 가리키는 타 Peer와의 통신을 위해 홀펀칭된 정보를 얻는다.
		- \ref use_alternative_p2p  에서 사용되기도 한다.

		\param remotePeerID 타 피어의 ID
		\param outInfo 타 피어와의 통신을 위한 홀펀칭 정보가 채워질 곳
		\return 홀펀칭된 Peer인 경우 true, 그 외의 경우 false를 리턴한다. 만약 false를 리턴한 경우 아직 홀펀칭되지 않은 peer인 경우에는 0.3~1초 간격으로
		이 메서드를 지속적으로 호출하다 보면 true를 리턴할 때가 있다. 왜냐하면 홀펀칭이 성사되는 시간이 항상 다르기 때문이다.

		\~english
		Gets the hole-punched information for this host to communicate with other peer that remotePeerID points
		- Also used in \ref use_alternative_p2p

		\param remotePeerID ID of other peer
		\param outInfo where the hole-punching information for communicating with other peer to be filled
		\return True if hole-punched peer, otherwise returns false.
		For the peer that is not hole-punched yet, there are some occasions that it returns true when calling this method in every 0.3 ~ 1 seconds.
		This happens since the moment when hole-punching is completed differs all the time.
		\~chinese
		此主机为了与remotePeerID所指的其他peer的通信获得打洞信息。
		- 也在\ref use_alternative_p2p%使用。

		\param remotePeerID 其他peer的ID。
		\param outInfo 为了与其他peer的通信的打洞信息要被填充的地方。
		\return 打洞的peer的话true，之外的情况返回false。如果返回false的话，还没有打洞的peer的情况是以0.3~1间隔，持续呼叫此方法的话有返回true的时候。因为打洞成功的时间总是不同。

		\~japanese
		\~
		*/
		public bool GetDirectP2PInfo(HostID remotePeerID, DirectP2PInfo outInfo)
		{
			if (outInfo == null)
			{
				throw new System.NullReferenceException("DirectP2PInfo outInfo");
			}

			return m_native_NetClient.GetDirectP2PInfo(remotePeerID, outInfo);
		}

		/**
		\~korean
		사용자가 지정한 다른 P2P peer와의 통신을 강제로 relay로 되게 할지를 지정하는 함수입니다.

		이 기능이 요긴하게 쓰이는 경우는 다음과 같습니다.
		- 클라이언트측의 P2P 통신량이 과다해서, 몇몇 P2P peer와의 통신은 relay로 전환하고자 할 경우
		\param remotePeerID Relay 전환시킬 Peer 의 HostID값입니다.
		\param enable true이면 강제 relay 를 켭니다.

		\~english TODO:translate needed.

		\~chinese
		是指定是否将用户指定的与其他P2P Peer间的通信强制进行relay的函数。
		能够有效使用其功能的状况如下。
		客户端的P2P通信量过多时，想将几个P2P Peer间的通信转换成relay的时候。
		\param remotePeerID Relay要转换的Peer的HostID值。
		\param enable 如果是true则开启强制relay。

		\~japanese
		\~
		*/
		public ErrorType ForceP2PRelay(HostID remotePeerID, bool enable)
		{
			return m_native_NetClient.ForceP2PRelay(remotePeerID, enable);
		}

		public void TEST_SetPacketTruncatePercent(Proud.HostType hostType, int percent)
		{
			m_native_NetClient.TEST_SetPacketTruncatePercent((Proud.HostType)hostType, percent);
		}

		public void TEST_FallbackUdpToTcp(Proud.FallbackMethod fallbackMethod)
		{
			m_native_NetClient.TEST_FallbackUdpToTcp((Proud.FallbackMethod)fallbackMethod);
		}

		/**
		\~korean
		일반적인 온라인 게임에서는 unreliable 메시징이 전체 트래픽의 대부분을 차이지함에 따라 기존 unreliable 메시징 기능에 트래픽 로스율을 얻는 기능을 추가합니다.
		패킷 로스율 측정기
		\param remoteHostID 어느 remote 와의 통신에 대한 로스율을 얻을 걷인지. 자기 자신 peer, server 뭐든지 입력 가능합니다. 자기 자신이면 0% 입니다
		\param outputPercent 패킷 로스율을 %단위로 채워짐(즉 0~100)

		\~english TODO:translate needed.

		\~chinese
		在一般在线游戏中当Unreliable信息的总体通信量占据绝大部分时，在原有Unreliable信息功能上添加获得通信量Loss率的功能。
		数据包Loss率测定仪。
		\param remoteHostID 要获得对哪一个remote间的通信Loss率，可以输入如自身Peer，server 等。如果是自己的话则为0%。
		\param outputPercent  数据包的Loss率将以%单位填充，（即0~100）

		\~japanese
		\~
		*/
		public Proud.ErrorType GetUnreliableMessagingLossRatioPercent(Proud.HostID remotePeerID, out int outputPercent)
		{
			Proud.ErrorType errorType = (Proud.ErrorType)m_native_NetClient.GetUnreliableMessagingLossRatioPercentErrorType((Proud.HostID)remotePeerID);
			outputPercent = m_native_NetClient.GetUnreliableMessagingLossRatioPercent((Proud.HostID)remotePeerID);
			return errorType;
		}

		/**
		\~korean
		서버의 현재 시간을 구한다. (밀리초 단위)
		이 값은 일정 시간마다 레이턴시도 고려되어 계산되는 서버의 시간이다.

		\~english
		Gets current time of server (in millisecond)
		This value is the server time that considered periodic latency

		\~chinese
		求服务器现时间。（毫秒单位）
		此值是每在一定时间内，考虑到Latency而算出的服务器时间。

		\~japanese

		\~
		*/
		public long GetServerTimeMs()
		{
			if (m_native_NetClient == null)
			{
				return 0;
			}
			return m_native_NetClient.GetServerTimeMs();
		}


		/**
		\~korean
		사용자 정의 메시지를 전송합니다. 자세한 것은 \ref send_user_message 를 참고하십시오.

		\~english
		Send user defined message. Please refer to \ref send_user_message for more detail

		\~chinese
		传送用户定义信息。详细请参考\ref send_user_message

		\~japanese
		*/
		public bool SendUserMessage(HostID remote, RmiContext rmiContext, ByteArray payload)
		{
			if (payload == null || payload.Count <= 0)
			{
				return false;
			}

			return NativeInternalNetClient.SendUserMessage(m_native_NetClient, remote, rmiContext, payload);
		}

		/**
		\~korean
		서버에서 인식된, 이 클라이언트의 네트워크 주소를 얻는다. 즉, 즉 공인 인터넷 주소(external addr)이다.
		- 서버와 연결되어 있을때만 유효한 값을 리턴한다.

		\~english
		Gets the network address of this client that is recognized by server. In other words, this is a public Internet address(external address).
		- Returns effective values only when connected to server

		\~chinese
		获得从服务器识别的，此client的网络地址。即公认互联网地址（external addr）。
		- 只与服务器连接的时候返回有效值。

		\~japanese
		\~
		*/
		public bool GetPublicAddress(ref IPEndPoint ipEndPoint)
		{
			if (m_native_NetClient == null)
			{
				return false;
			}

			AddrPort addrPort = null;

			try
			{
				addrPort = m_native_NetClient.GetPublicAddress();
				ipEndPoint = new IPEndPoint(IPAddress.Parse(addrPort.IPToString()), addrPort.port);
			}
			finally
			{
				if (addrPort != null)
				{
					addrPort.Dispose();
					addrPort = null;
				}
			}

			return true;
		}

		public void HolsterMoreCallbackUntilNextFrameMove()
		{
			if (m_native_NetClient == null)
				return;

			m_native_NetClient.HolsterMoreCallbackUntilNextFrameMove();
		}

		public void SetApplicationHint(ApplicationHint hint)
		{
			if (m_native_NetClient == null)
				return;

			m_native_NetClient.SetApplicationHint(hint);
		}


		///////////////////////////////////////////////////////////////////
		//event handler
		public delegate void ErrorInfoDelegate(Nettention.Proud.ErrorInfo errorInfo);

		public delegate void ExceptionDelegate(HostID remoteID, Exception e);

		public delegate void NoRmiProcessedDelegate(RmiID rmiID);

		public delegate void ReceiveUserMessageDelegate(HostID sender, RmiContext rmiContext, ByteArray payload);

		public delegate void JoinServerCompleteDelegate(Nettention.Proud.ErrorInfo info, ByteArray replyFromServer);

		public delegate void LeaveServerDelegate(Nettention.Proud.ErrorInfo info);

		public delegate void P2PMemberJoinDelegate(HostID memberHostID, HostID groupHostID, int memberCount, ByteArray message);

		public delegate void P2PMemberLeaveDelegate(HostID memberHostID, HostID groupHostID, int memberCount);

		public delegate void ChangeP2PRelayStateDelegate(HostID remoteHostID, ErrorType reason);

		public delegate void ChangeServerUdpStateDelegate(ErrorType reason);

		public delegate void SynchronizeServerTimeDelegate();

		public delegate void ServerOfflineDelegate(Nettention.Proud.RemoteOfflineEventArgs args);

		public delegate void ServerOnlineDelegate(Nettention.Proud.RemoteOnlineEventArgs args);

		public delegate void P2PMemberOfflineDelegate(Nettention.Proud.RemoteOfflineEventArgs args);

		public delegate void P2PMemberOnlineDelegate(Nettention.Proud.RemoteOnlineEventArgs args);

		internal ErrorInfoDelegate errorHandler = null;

		public ErrorInfoDelegate ErrorHandler
		{
			set { errorHandler = value; }
		}

		internal ErrorInfoDelegate warningHandler = null;
		public ErrorInfoDelegate WarningHandler
		{
			set { warningHandler = value; }
		}

		internal ErrorInfoDelegate informationHandler = null;
		public ErrorInfoDelegate InformationHandler
		{
			set { informationHandler = value; }
		}

		internal ExceptionDelegate exceptionHandler = null;
		public ExceptionDelegate ExceptionHandler
		{
			set { exceptionHandler = value; }
		}

		internal NoRmiProcessedDelegate noRmiProcessedHandler = null;
		public NoRmiProcessedDelegate NoRmiProcessedHandler
		{
			set { noRmiProcessedHandler = value; }
		}

		internal ReceiveUserMessageDelegate receivedUserMessageHandler = null;
		public ReceiveUserMessageDelegate ReceivedUserMessageHandler
		{
			set { receivedUserMessageHandler = value; }
		}

		internal JoinServerCompleteDelegate joinServerCompleteHandler = null;
		public JoinServerCompleteDelegate JoinServerCompleteHandler
		{
			set { joinServerCompleteHandler = value; }
		}

		internal LeaveServerDelegate leaveServerHandler = null;
		public LeaveServerDelegate LeaveServerHandler
		{
			set { leaveServerHandler = value; }
		}

		internal P2PMemberJoinDelegate p2pMemberJoinHandler = null;
		public P2PMemberJoinDelegate P2PMemberJoinHandler
		{
			set { p2pMemberJoinHandler = value; }
		}

		internal P2PMemberLeaveDelegate p2pMemberLeaveHandler = null;
		public P2PMemberLeaveDelegate P2PMemberLeaveHandler
		{
			set { p2pMemberLeaveHandler = value; }
		}

		internal ChangeP2PRelayStateDelegate changeP2PRelayStateHandler = null;
		public ChangeP2PRelayStateDelegate ChangeP2PRelayStateHandler
		{
			set { changeP2PRelayStateHandler = value; }
		}

		internal ChangeServerUdpStateDelegate changeServerUdpStateHandler = null;
		public ChangeServerUdpStateDelegate ChangeServerUdpStateHandler
		{
			set { changeServerUdpStateHandler = value; }
		}

		internal SynchronizeServerTimeDelegate synchronizeServerTimeHandler = null;
		public SynchronizeServerTimeDelegate SynchronizeServerTimeHandler
		{
			set { synchronizeServerTimeHandler = value; }
		}

		internal ServerOfflineDelegate serverOfflineHandler = null;
		public ServerOfflineDelegate ServerOfflineHandler
		{
			set { serverOfflineHandler = value; }
		}

		internal ServerOnlineDelegate serverOnlineHandler = null;
		public ServerOnlineDelegate ServerOnlineHandler
		{
			set { serverOnlineHandler = value; }
		}

		internal P2PMemberOfflineDelegate p2pMemberOfflineHandler = null;
		public P2PMemberOfflineDelegate P2PMemberOfflineHandler
		{
			set { p2pMemberOfflineHandler = value; }
		}

		internal P2PMemberOnlineDelegate p2pMemberOnlineHandler = null;
		public P2PMemberOnlineDelegate P2PMemberOnlineHandler
		{
			set { p2pMemberOnlineHandler = value; }
		}
	}
}
