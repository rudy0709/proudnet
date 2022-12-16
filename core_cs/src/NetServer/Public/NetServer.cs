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

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
//using System.Threading.Tasks;
using System.Runtime.InteropServices;
using System.Net;

namespace Nettention.Proud
{
    internal class ServerNativeExceptionString
    {
        // 필요한 dll 파일들이 작업 경로에 없을 때
        public const string TypeInitializationExceptionString = @"Please check if necessary DLL files (ProudNetClientPlugin.dll, ProudNetClient.dll, ProudNetServerPlugin.dll, ProudNetServer.dll, libcrypto-1_1-x64.dll, libssl-1_1-x64.dll) exist on the working directory of your program. If there's no such files, you should copy these files to the working directory of your program.";
    }

    public class NetServer : IRmiHost, IDisposable
    {
        // SWIG에서 만들어진 C++ NetServer 래핑 클래스.
        private Proud.NativeNetServer m_native_NetServer = null;

        // C++ INetServerEvent의 콜백을 처리하는 클래스.
        // NOTE: 사용자가 넣는게 아니라 여기서 자체 구현본을 넣는다.
        // 사용자에게는 이것의 delegate function들만 노출된다.
        private NetServerEvent m_netServer_Event = null;

        private bool m_allowExceptionEvent = true;

        private bool m_disposed = false;

        // C# -> C++로 넘기는 포인터(객체, 함수 등)에 대해서는 가비지 수집이 되지 않도록 하기 위해서
        // 별도의 리스트 자료구조에 GCHandle를 저장합니다.
        // 여기에 attach된 proxy, stub들의 handle이 여기 보관된다.
        List<GCHandle> m_handleList = new List<GCHandle>();

        public NetServer()
        {
            try
            {
                m_native_NetServer = Nettention.Proud.NativeNetServer.Create(true);
                m_netServer_Event = new NetServerEvent(this);
                m_native_NetServer.SetEventSink(m_netServer_Event);
            }
            catch(System.TypeInitializationException ex)
            {
                // c++ ProudNetServerPlugin.dll, ProudNetClientPlugin.dll 파일이 작업 경로에 없을 때
                throw new System.Exception(ServerNativeExceptionString.TypeInitializationExceptionString);
            }
        }

        ~NetServer()
        {
            Dispose(false);
        }

        // IDisposable 인터페이스 구현
        // NetServer의 경우 프로그램 종료 또는 NetServer 객체 파괴시 명시적으로 NetServer.Dispose() 를 호출해주어야 합니다.
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
                m_native_NetServer.Dispose();
                m_native_NetServer = null;
            }

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
            if (ExceptionHandler != null)
            {
                ExceptionHandler(ex);
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
        \copy Nettention.Proud.NetClient.AllowExceptionEvent
        */
        public void AllowExceptionEvent(bool value)
        {
            m_allowExceptionEvent = value;
        }

        public Nettention.Proud.NativeNetServer NativeNetServer
        {
            get { return m_native_NetServer; }
        }

        public void ShowNotImplementedRmiWarning(String RMIName)
        {
        }

        public void PostCheckReadMessage(Message msg, string RMIName)
        {
        }

        public bool IsSimplePacketMode()
        {
            return false;
        }

        /**
		\~korean
		서버를 시작합니다. 서버 시작을 하지 못할 경우 Proud.Exception이 throw 됩니다.
		\param 서버 시작을 위한 설정 내용입니다. TCP listen port 등을 지정합니다.

		\~english
		The server starts. If the server does not start, Proud.Exception will throw.
		Setting for the initiating the \param server. Able to indicate TCP listen port.

		\~chinese
		开始启动服务器，如果无法启动则 Proud.Exception 将被 throw。
		\param 服务器启动相关设置内容，指定 TCP listen port 等。


		\~japanese
		サーバーを起動します。 サーバが起動していない場合はProud.Exceptionがthrowされます。
		\Param サーバを起動するための確立の内容です。 TCP listen port等を指定します。
		*/
        public void Start(Nettention.Proud.StartServerParameter param)
        {
            m_native_NetServer.Start(param);
        }

        /**
		\~korean
		서버를 종료한다.
		-주의!! 이 함수는 rmi등 UserWorkerThread에서 호출하는 함수내에서는 호출해서는 안됩니다.

		\~english
		Stop server.
		- Warning!! This function never call in function that call from UserWorkerThread such as rmi.

		\~chinese
		终止服务器。
		- 注意！不能把此函数在rmi等UserWorkerThread呼叫的函数内呼叫。

		\~japanese
		\~
		*/
        public void Stop()
        {
            m_native_NetServer.Stop();
        }

        public void AttachProxy(RmiProxy proxy)
        {
            GCHandle handle = GCHandle.Alloc(proxy);
            m_handleList.Add(handle);

            proxy.m_core = this;
            m_native_NetServer.AttachProxy(proxy.GetNativeInternalProxy().GetNativeProxy());
        }

        public void AttachStub(RmiStub stub)
        {
            stub.m_core = this;

            GCHandle handle = GCHandle.Alloc(stub);
            m_handleList.Add(handle);


            m_native_NetServer.AttachStub(stub.GetNativeInternalStub().GetNativeStub());
        }

        /**
		\~korean
		클라이언트간 P2P 홀펀칭을 시작하기 위한 조건을 지정합니다. 자세한 것은 \ref jit_p2p 를 참고하십시오.
		- 이 메서드를 호출한 이후에 접속한 클라이언트부터 본 값이 적용됩니다.
		- 기본값은 JIT 입니다.

		\param newVal 클라이언트간 P2P 통신을 위한 홀펀칭을 시작하는 조건입니다.
		\return 성공적으로 반영시 true를 리턴합니다.

		\~english

		\todo retranslate required.

		Designates the condition to begin P2P hole-punching among clients
		- The clients that the designated value will be applied to are the client that connected after this method is called.
		- Default is the value that is designated at Proud.CNetConfig.DefaultDirectP2PStartCondition.

		\param newVal the condition to begin P2P hole-punching among clients
		\return returns true if successfully applied

		\~chinese
		为了开始P2P打洞而指定条件。详细请参考\ref jit_p2p%。
		- 呼叫此方法以后连接的client开始使用此值。
		- 默认值是JIT。
		\param newVal 开始为了client之间P2P通信的条件。
		\return 反映成功时返回true。

		\~japanese
		\~
		*/
        public bool SetDirectP2PStartCondition(DirectP2PStartCondition newVal)
        {
            return m_native_NetServer.SetDirectP2PStartCondition((Proud.DirectP2PStartCondition)newVal);
        }

        /**
		\~korean
		클라이언트 HostID 목록을 얻는다.
		- 이 메서드로 얻는 목록은 호출할 시점의 스냅샷일 뿐이다. 이 함수가 리턴된 후에도
		클라이언트 목록의 내용이 실제 서버의 상태와 동일함을 보장하지는 않는다.
		따라서 정확한 클라이언트 리스트를 얻고자 하면 INetServerEvent.OnClientJoin,
		INetServerEvent.OnClientLeave 에서 유저가 필요로 하는 클라이언트 리스트를 따로
		만들어 관리하는 것을 권장한다.

		\~english
		Gets client HostID list
		- The list acquired by this method is only a snapshot. After this function is returned, there is no guarantee that the contents in the list matche the status of real server.
		Therefore, in order to get precise client list, it is recommended to create and manage a separate client list needed by user at INetServerEvent.OnClientJoin, INetServerEvent.OnClientLeave.

		\~chinese
		获得client Host ID目录。
		- 用此方法获得的目录只是要呼叫的时点的snapshot而已。此函数返回以后不保证client目录的内容与实际服务器状态相同。
		所以想获得正确的client list的话，建议在 INetServerEvent.OnClientJoin,
		INetServerEvent.OnClientLeave%另外创建并管理用户需要的client list。

		\~japanese
		\~
		*/
        public Proud.HostID[] GetClientHostIDs()
        {
            Proud.NativeHostIDArray nativeHostIDArray = new Proud.NativeHostIDArray();
            m_native_NetServer.GetClientHostIDs(nativeHostIDArray);

            int count = nativeHostIDArray.GetCount();

            Proud.HostID[] clientHostIDs = new Proud.HostID[count];
            for (int i = 0; i < count; ++i)
            {
                clientHostIDs[i] = (Proud.HostID)nativeHostIDArray.At(i);
            }

            nativeHostIDArray.Dispose();

            return clientHostIDs;
        }

        /**
		\~korean
		\ref p2p_group 을 생성합니다.
		- 이렇게 생성한 그룹 내의 피어들끼리는 즉시 서로간 메시징을 해도 됩니다.  (참고: \ref robust_p2p)
		- 클라이언트 목록(clientHostIDs,clientHostIDNum)을 받을 경우 이미 멤버가 채워져 있는 P2P group을 생성한다.
		텅 빈 배열을 넣을 경우 멤버 0개짜리 P2P 그룹을 생성할 수 있다. (단, AllowEmptyP2PGroup 에 따라 다르다.)
		- 이 메서드 호출 후, CNetClient 에서는 P2P 그룹의 각 멤버에 대해(자기 자신 포함)
		INetClientEvent.OnP2PMemberJoin 이 연이어서 호출된다.
		- HostID_Server 가 들어가도 되는지 여부는 \ref server_as_p2pgroup_member 를 참고하십시오.

		\~english TODO:translate needed.
		Creates \ref p2p_group
		- When receiving client list(clientHostIDs,clientHostIDNum), creates P2P group that is already filled with members.
		When entering an empty array, it is possible to create P2P group of 0 member. (But, it depends on AllowEmptyP2PGroup.)
		- After calling this method, for each member of P2P group(including itself), INetClientEvent.OnP2PMemberJoin will be called one after another to CNetClient.
		- Please refer \ref server_as_p2pgroup_member to find out whether HostID_Server can enter.

		\~chinese
		生成\ref p2p_group%。
		- 这样生成的组内peer之间可以立即进行相互之间的messaging。（参考：\ref robust_p2p）
		- 收到client目录（clientHostIDs,clientHostIDNum）的话生成已经填充成员的P2P group。
		- 输入空数组的话能生成0个成员的P2P组。（但根据AllowEmptyP2Pgroup会有所不同）
		- 呼叫此方法后对P2P组的各个成员（包括自己本身）在 CNetClient%连续呼叫 INetClientEvent.OnP2PmemberJoin。
		- HostID_Server 是否能进入请参考\ref server_as_p2pgroup_member%。
         *
		\~japanese
		\~
		*/
        public Proud.HostID CreateP2PGroup(Proud.HostID[] managedHostIDArray, Proud.ByteArray message)
        {
            using (Proud.NativeHostIDArray nativeHostIDArray = new Proud.NativeHostIDArray())
            using (Proud.NativeByteArray nativeByteArray = new Proud.NativeByteArray())
            {
                foreach (var hostID in managedHostIDArray)
                {
                    nativeHostIDArray.Add(hostID);
                }

                // 주의 : ByteArray는 내부적으로 Capacity로 넉넉히 reserve하기 때문에 message.data를 foreach하면 안된다.
                for (int i = 0; i < message.Count; ++i)
                {
                    nativeByteArray.Add(message[i]);
                }

                Proud.HostID groupID = (Proud.HostID)m_native_NetServer.CreateP2PGroup(nativeHostIDArray, nativeByteArray);

                return groupID;
            }
        }

        /**
		\~korean
		이 객체에 연결된 peer 1개의 정보를 얻는다.
		\param peerHostID 찾으려는 peer의 HostID
		\param output 찾은 peer의 정보. 못 찾으면 아무것도 채워지지 않는다.
		\return true이면 찾았다는 뜻

		\~english
		Gets the information of 1 peer connected to this object
		\param peerHostID HostID of peer to find
		\param output Infoof foudn peer. Nothing will be filled when found nothing.
		\return true when found

		\~chinese
		获得连接与此对象的一个peer信息。
		\param peerHostID 要找的peer的HostID。
		\param output 找到的peer信息。没有找到的话不填充任何东西。
		\return true是找到了的意思。

		\~japanese
		\~
		*/
        public Proud.NetClientInfo GetClientInfo(Proud.HostID hostID)
        {
            Proud.NetClientInfo nativeInfo = new Proud.NetClientInfo();

            bool ret = m_native_NetServer.GetClientInfo((Proud.HostID)hostID, nativeInfo);

            if (ret == false)
            {
                nativeInfo.Dispose();
                nativeInfo = null;
                return null;
            }

            return nativeInfo;
        }

        /**
		\~korean
		\ref p2p_group 을 파괴한다.
		- 안에 있던 모든 member들은 P2P 그룹에서 탈퇴된다는 이벤트(Proud.INetClientEvent.OnP2PMemberLeave)를 노티받는다.
		물론, 서버와의 연결은 유지된다.
		\param groupHostID 파괴할 P2P 그룹의 ID
		\return 해당 P2P 그룹이 없으면 false.

		\~english
		Destructs \ref p2p_group
		- All members within will receive an event(Proud.INetClientEvent.OnP2PMemberLeave) that they will be withdrawn from P2P group. Of course, the connection to server will be sustained.
		\param groupHostID ID of P2P group to be destructed
		\return false if there is no P2P group related

		\~chinese
		破坏\ref p2p_group%。
		- 在里面的所有member被通知从P2P组退出的event（Proud.INetClientEvent.OnP2PMemberLeave）。
		当然，继续维持与服务器的连接。
		\param groupHostID 要破坏的P2P组ID。
		\return 没有相关P2P组的话false。

		\~japanese
		\~
		*/
        public bool DestroyP2PGroup(Proud.HostID groupID)
        {
            return m_native_NetServer.DestroyP2PGroup(groupID);
        }

        /**
         \~korean
         서버의 현재 시간을 얻는다. CNetClient.GetServerTimeMs()과 거의 같은 값이다.

         \~english
         Gets current time of server. Has almost same value as CNetClient.GetServerTimeMs().

         \~chinese
         获得服务器的现在时间。与 CNetClient.GetServerTimeMs()基本相同的值。

         \~japanese
         サーバーの現在時間を得ます。CNetClient.GetServerTimeMs()とほぼ同じ値です。
         \~
         */

        public long GetTimeMs()
        {
            return m_native_NetServer.GetTimeMs();
        }

        /**
         \~korean
         너무 오랫동안 서버와 통신하지 못하면 연결을 해제하기 위한 타임 아웃 임계값(밀리초)를 지정한다.
         - CNetServer.Start 호출 전에만 사용할 수 있다. 그 이후부터 호출할 경우 무시된다.
         - 무한으로 잡는 것은 추천하지 않는다. 비정상 연결 해제시 무한히 남는 연결로 자리잡기 때문이다. 그러느니 차라리 1시간을 거는 것이 낫다.
         반면, 1초 등 너무 작은 값을 잡으면 정상적인 클라이언트도 쫓겨날 위험이 있으므로 피해야 한다.
         - 기본값은 ProudNetConfig.DefaultNoPingTimeoutTime 이다.
         - 참고: <a target="_blank" href="http://guide.nettention.com/cpp_ko#debug_pause_problem" >디버거에 의해 일시정지하면 서버와의 연결이 끊어지는 문제 해결법</a>

         \~english
         Sets the critical value of time out(millisecond) to disconnect the connection with server when it has been long not communicating with it
         - Can only be used before calling CNetServer.Start. Caliing after that point will be ignored.
         - It is not recommended setting it as permanent since it will remain as an infinite connection after unexpected disconnection. It is better setting it as an hour than the case.
         - Default is ProudNetConfig.DefaultNoPingTimeoutTime.
         - Please refer <a target="_blank" href="http://guide.nettention.com/cpp_en#debug_pause_problem" >How to maintain the connection with server when the program stops due to debugger</a>.

         \~chinese
         过长时间不能与服务器通信的话，指定为了解除连接的超时临界值（millisecond）。
         - 只能在 CNetServer.Start t 呼叫之前使用。之后开始呼叫的话会被忽视。
         - 不建议设为无限。因为非正常连接时会成为无限剩余的连接。所以不如设置为1小时。
         反面，设太小的值的话，client 可能会有被赶出去的危险。
         - 默认值是 ProudNetConfig.DefaultNoPingTimeoutTime%。
         - 参考：<a target="_blank" href="http://guide.nettention.com/cpp_zh#debug_pause_problem" >因Debug一时终止而与服务器断开连接时的解决方法</a>。

         \~japanese
		*/
        public void SetDefaultTimeoutTimeMs(int newValInMs)
        {
            m_native_NetServer.SetDefaultTimeoutTimeMs(newValInMs);
        }

        /**
        \~korean
        너무 오랫동안 서버와 통신하지 못하면 연결을 해제하기 위한 타임 아웃 임계값(밀리초)를 지정합니다.
        SetDefaultTimeoutTimeMs 와 다른점은 특정 호스트에 대해서만 런타임중에도 타임아웃을 변경 할 수 있습니다.
        - 서버의 시작 전, 후 상관 없이 사용 할 수 있습니다.
        - 특정 호스트(클라이언트)에만 적용 시킵니다.
        - 변경된 타임아웃 값이 적용되는 데는 몇 초간 걸릴 수 있습니다.

        \~english
        Specify timeout threshold(msec.) for disconnection in case of disconnected from server too long.
        Difference from SetDefaultTimeoutTimeMS is that you can change timeout during runtime for specific host.
        -You can use it regardless of before or after starting the server.
        -Applied to specific host(client).
        -It may take some seconds to apply the time out value change.


        \~chinese TODO:translate needed.
        Specify timeout threshold(msec.) for disconnection in case of disconnected from server too long.
        Difference from SetDefaultTimeoutTimeMS is that you can change timeout during runtime for specific host.
        -You can use it regardless of before or after starting the server.
        -Applied to specific host(client).
        -It may take some seconds to apply the time out value change.


        \~japanese TODO:translate needed.
        Specify timeout threshold(msec.) for disconnection in case of disconnected from server too long.
        Difference from SetDefaultTimeoutTimeMS is that you can change timeout during runtime for specific host.
        -You can use it regardless of before or after starting the server.
        -Applied to specific host(client).
        -It may take some seconds to apply the time out value change.
        */
        public void SetTimeoutTimeMs(HostID host, int newValInMs)
        {
            m_native_NetServer.SetTimeoutTimeMs(host, newValInMs);
        }

        /**
        \~korean
        m_enableAutoConnectionRecovery가 true이면 SetDefaultTimeoutSec이나 SetDefaultTimeouMs으로 지정한 타임아웃 시간이 유효하지 않게 됩니다.
        따라서 ACR의 연결 유지 시간을 대신 지정해줘야 하는데, 이 함수를 통하여 지정해줄 수 있습니다.
        서버 시작 전에 설정할 수 있습니다.
        - 기본값은 60 * 1000 입니다. 즉, 1분입니다.

        \~english
        If m_enableAutoConnectionRecovery is true, The time of TimeOut is not valid.
        So, You should set the ACR time by using this method.
        - Default is 60 * 1000. ( 1 minute )

        \~chinese TODO:translate needed.

        If m_enableAutoConnectionRecovery is true, The time of TimeOut is not valid.
        So, You should set the ACR time by using this method.
        - Default is 60 * 1000. ( 1 minute )

        \~japanese TODO:translate needed.

        If m_enableAutoConnectionRecovery is true, The time of TimeOut is not valid.
        So, You should set the ACR time by using this method.
        - Default is 60 * 1000. ( 1 minute )
        */
        public void SetDefaultAutoConnectionRecoveryTimeoutTimeMs(int newValInMs)
        {
            m_native_NetServer.SetDefaultAutoConnectionRecoveryTimeoutTimeMs(newValInMs);
        }

        /**
        \~korean
        m_enableAutoConnectionRecovery가 true이면 SetDefaultTimeoutSec이나 SetDefaultTimeouMs으로 지정한 타임아웃 시간이 유효하지 않게 됩니다.
        따라서 ACR의 연결 유지 시간을 대신 지정해줘야 하는데, 이 함수를 통하여 지정해줄 수 있습니다.
        - 서버의 시작 전, 후 상관 없이 사용 할 수 있습니다.
        - 특정 호스트(클라이언트)에만 적용 시킵니다.
        - 변경된 타임아웃 값이 적용되는 데는 몇 초간 걸릴 수 있습니다.

        \~english
        If m_enableAutoConnectionRecovery is true, The time of TimeOut is not valid.
        So, You should set the ACR time by using this method.
        - It may take some seconds to apply the time out value change.
        - You can use it regardless of before or after starting the server.
        - Applied to specific host(client).
        - It may take some seconds to apply the time out value change.

        \~chinese TODO:translate needed.

        If m_enableAutoConnectionRecovery is true, The time of TimeOut is not valid.
        So, You should set the ACR time by using this method.
        - It may take some seconds to apply the time out value change.
        - You can use it regardless of before or after starting the server.
        - Applied to specific host(client).
        - It may take some seconds to apply the time out value change.


        \~japanese TODO:translate needed.

        If m_enableAutoConnectionRecovery is true, The time of TimeOut is not valid.
        So, You should set the ACR time by using this method.
        - It may take some seconds to apply the time out value change.
        - You can use it regardless of before or after starting the server.
        - Applied to specific host(client).
        - It may take some seconds to apply the time out value change.

        */
        public void SetAutoConnectionRecoveryTimeoutTimeMs(HostID host, int newValInMs)
        {
            m_native_NetServer.SetAutoConnectionRecoveryTimeoutTimeMs(host, newValInMs);
        }

        /**
		\~korean
		사용자 정의 메시지를 전송합니다. 자세한 것은 \ref send_user_message 를 참고하십시오.
         *
		\~english
		Sending user defined message. Please refer to \ref send_user_message for more detail.

		\~chinese
		传送用户定义信息。详细请参考\ref send_user_message%。

		\~japanese
		\~
		*/
        public bool SendUserMessage(HostID remote, RmiContext rmiContext, ByteArray payload)
        {
            if ((payload == null) || (payload.Count <= 0))
            {
                return false;
            }
            return NativeInternalNetServer.SendUserMessage(m_native_NetServer, remote, rmiContext, payload);
        }

        /**
		\~korean
		사용자 정의 메시지를 전송합니다. 자세한 것은 \ref send_user_message 를 참고하십시오.
		\param remotes 수신자 배열입니다. RMI와 마찬가지로, 클라이언트, 서버(HostID_Server), \ref p2p_group  등을 넣을 수 있습니다.
		\param rmiContext 송신 부가정보. 자세한 것은 \ref protocol_overview 를 참고하십시오.
		\param payload 보낼 메시지 데이터

		\~english
		Send user defined message. Please refer to \ref send_user_message for more detail
		\param remotes Receiver array. You can put client, server(HostID_Server), \ref p2p_group, etc like RMI.
		\param rmiContext Additional information of sending. Please refer to \ref protocol_overview.
		\param payload Message data for sending.

		\~chinese
		传送用户定义信息。详细请参考\ref send_user_message%。
		\param remotes 收信者数组。与RMI相同可以输入client，服务器（HostID_Server），\ref p2p_group%等。
		\param rmiContext 传送信息的附加信息。详细请参考\ref protocol_overview%。
		\param payload 要发送的信息数据。

		\~japanese
		\~
		*/
        public bool SendUserMessage(HostID[] remotes, RmiContext rmiContext, ByteArray payload)
        {
            if ((remotes == null) || (remotes.Length <= 0))
            {
                return false;
            }

            if ((payload == null) || (payload.Count <= 0))
            {
                return false;
            }

            return NativeInternalNetServer.SendUserMessage(m_native_NetServer, remotes, rmiContext, payload);
        }

        /**
		\~korean
		클라이언트 memberHostID가 기 존재 \ref p2p_group  groupHostID에 중도 참여하게 합니다.
		- 참여 시도된 피어들과 기존에 참여되어 있던 피어들 끼리는 즉시 서로간 메시징을 해도 됩니다.  (참고: \ref robust_p2p)

		\param memberHostID P2P 그룹에 참여할 멤버의 Host ID. HostID_Server 가 들어가도 되는지 여부는 \ref server_as_p2pgroup_member 를 참고하십시오.
		\param groupHostID 이미 만들어진 P2P 그룹의 Host ID입니다.
		\param message P2P 그룹을 추가하면서 서버에서 관련 클라이언트들에게 보내는 메시지입니다. INetClientEvent.OnP2PMemberJoin 에서 그대로 받아집니다.
		\return 성공시 true를 리턴합니다.

		\~english
		Lets client memberHostID enter into existing \ref p2p_group groupHostID
		- Messaging can do between existed peer and new peer. (Reference: \ref robust_p2p)

		\param memberHostID HostID of member to join P2P group. Please refer \ref server_as_p2pgroup_member to find out if HostID_Server can join.
		\param groupHostID HostID of existing P2P group
		\param message Message to be sent to related clients from server when adding P2P group. Will be accepted as it is at INetClientEvent.OnP2PMemberJoin.
		\return returns true if successful

		\~chinese
		让Client的memberHostID往已存在的 \ref p2p_group groupHostID中途参与。
		- 试图参与的peer和之前参与的peer之间可以立即进行messaging（参考 \ref robust_p2p）。

		\param memberHostID 参与P2P组的成员Host ID。 HostID_Server 是否也能进入请参考\ref server_as_p2pgroup_member%。
		\param groupHostID 已经创建的P2P组的Host ID。
		\param message 添加P2P组的时候服务器往相关client发送的信息。在 INetClientEvent.OnP2PMemberJoin%原封不动的被接收。
		\return 成功的时候返回true。

		\~japanese
		\~
		*/
        public bool JoinP2PGroup(HostID memberHostID, HostID groupHostID, ByteArray message)
        {
            return NativeInternalNetServer.JoinP2PGroup(m_native_NetServer, memberHostID, groupHostID, message);
        }

        public bool JoinP2PGroup(HostID memberHostID, HostID groupHostID)
        {
            return m_native_NetServer.JoinP2PGroup(memberHostID, groupHostID);
        }

        /**
		\~korean
		클라이언트 memberHostID가 \ref p2p_group  groupHostID 를 나가게 한다.
		- 추방되는 클라이언트에서는 P2P 그룹의 모든 멤버에 대해(자기 자신 포함) INetClientEvent.OnP2PMemberLeave 이 연속으로 호출된다.
		한편, P2P group의 나머지 멤버들은 추방되는 클라이언트에 대한 INetClientEvent.OnP2PMemberLeave 를 호출받는다.
		- 멤버가 전혀 없는, 즉 비어있는 P2P group가 될 경우 AllowEmptyP2PGroup의 설정에 따라 자동 파괴되거나 잔존 후 재사용 가능하다.

		\param memberHostID 추방할 멤버의 Host ID
		\param groupHostID 어떤 그룹으로부터 추방시킬 것인가
		\return 성공적으로 추방시 true를 리턴

		\~english
		Client memberHostID lets \ref p2p_group groupHostID go out
		- At the client being kicked out, INetClientEvent.OnP2PMemberLeave will be called one after another for all members of P2P group(including itself).
		On the other hand, INetClientEvent.OnP2PMemberLeave will be called for other members of P2P group that are to be kicked out.
		- For those of no members, in other words for an empty P2P group, they will be either auto-destructed by AllowEmptyP2PGroup setting or can be re-used after its survival.

		\param memberHostID HostID of member to be kicked out
		\param groupHostID to be kicked out from which group?
		\return returns true if successfully kicked out

		\~chinese
		Client 的memberHostID让 \ref p2p_group groupHostID退出。
		- 在被踢出去的client对P2P组的所有成员（包括自己本身）INetClientEvent.OnP2PmemberLeave%连续呼叫。
		一方面P2P group的剩下成员会被呼叫对踢出的client的 INetClientEvent.OnP2PMemberLeave。
		- 没有成员的话，即成为空的P2P group的时候根据AllowEmptyP2Pgroup的设置可以自动破坏或者残留后再使用。

		\param memberHostID 要踢出的Host ID。
		\param groupHostID 要从哪个组踢出。
		\return 踢出成功的话返回true。

		\~japanese
		\~
        */
        public bool LeaveP2PGroup(HostID memberHostID, HostID groupHostID)
        {
            return m_native_NetServer.LeaveP2PGroup(memberHostID, groupHostID);
        }

        /**
		\~korean
		클라이언트 또는 채팅 그룹에 속한 모든 클라이언트를 쫓아낸다.
		\return 해당 클라이언트가 없으면 false.

		\~english
		Kicks out all clients in client group and/or chatting group
		\return false if there is no realted client.

		\~chinese
		踢出属于Client或者聊天组的所有client。
		\return 没有相关client的话false。

		\~japanese
		\~
		*/
        public bool CloseConnection(HostID clientHostID)
        {
            return m_native_NetServer.CloseConnection(clientHostID);
        }

        /**
		\~korean
		P2P group 1개의 정보를 얻는다.

		\~english
		Gets the information of 1 P2P group

		\~chinese
		获得P2P group一个信息。

		\~japanese
		\~
		*/
        public P2PGroup GetP2PGroupInfo(HostID groupHostID)
        {
            return m_native_NetServer.GetP2PGroupInfo(groupHostID);
        }

        public int GetMemberCountOfP2PGroup(HostID groupHostID)
        {
            return m_native_NetServer.GetMemberCountOfP2PGroup(groupHostID);
        }

        /**
		\~korean
		TCP listener port의 로컬 주소 리스트를 얻는다. 예를 들어 서버 포트를 동적 할당한 경우 이것을 통해 실제 서버의 리스닝 포트 번호를 얻을 수 있다.

		\~english
		Gets local address of TCP listener port. For an example, it is possible to acquire the listening port number of real server through this when the server ports are dynamically allocated.

		\~chinese
		获得TCP listener port的本地地址list。例如动态分配服务器端口的时候，通过这个可以获得实际服务器的listening端口号码。

		\~japanese
		\~
		*/
        public List<IPEndPoint> GetTcpListenerLocalAddrs()
        {
            List<IPEndPoint> addrs = new List<IPEndPoint>();

            AddrPortArray nativeAddrPortArray = m_native_NetServer.GetTcpListenerLocalAddrs();
            if (nativeAddrPortArray != null)
            {
                int count = nativeAddrPortArray.GetCount();
                for (int i = 0; i < count; ++i)
                {
                    AddrPort nativeAddrPort = nativeAddrPortArray.Get(i);
                    if (nativeAddrPort != null)
                    {
                        addrs.Add(new IPEndPoint(nativeAddrPort.GetIPAddr(), nativeAddrPort.port));

                        // C++ AddrPort 객체 삭제 합니다.(명시적)
                        nativeAddrPort.Dispose();
                    }
                }

                // C++ FastArray<AddrPort> 객체 삭제 합니다.(명시적)
                nativeAddrPortArray.Dispose();
            }

            return addrs;
        }

        /**
		\~korean
		해킹된 클라이언트에서 망가뜨려놓은 메시지 헤더를 보낼 경우(특히 메시지 크기 부분)
		이를 차단하기 위해서는 서버측에서 받는 메시지 크기의 상한선을 지정해야 한다.
		이 메서드는 그 상한선을 지정하는 역할을 한다.
		초기값은 64KB이다.
		\param serverLength serverMessageMaxLength
		\param clientLength clientMessageMaxLength

		\~english
		To stop the damaged message header sent by a hacked client(especially the size part), it is crucial to set the maximum size of messages received by server.
		This method sets the limit and the default value is 64KB.
		\param serverLength serverMessageMaxLength
		\param clientLength clientMessageMaxLength

		\~chinese
		从被黑的client发送毁坏的信息header的时候（特别是信息大小部分），为了中断这个要指定在服务器方接收的信息大小的上限。
		此方法起着指定那个上限的作用。
		初始值是64KB。
		\param serverLength serverMessageMaxLength
		\param clientLength clientMessageMaxLength

		\~japanese
		\~
		*/
        public void SetMessageMaxLength(int serverLength, int clientLength)
        {
            m_native_NetServer.SetMessageMaxLength(serverLength, clientLength);
        }

        /**
        \~korean
        클라이언트가 UDP 통신을 어느 정도까지만 쓰도록 제한할 것인지를 설정하는 메서드다.
        - 사용자의 의도적인 이유로 클라이언트가 서버와의 또는 peer간의 UDP 통신을 차단하고자 할 때
        이 메서드를 쓸 수 있다.
        - 극악의 상황, 예를 들어 UDP 통신 장애가 심각하게 발생하는 서버에서 임시방편으로 이 옵션을 켜서
        문제를 일시적으로 줄일 수 있다. 하지만 UDP 통신은 게임 서비스에서 중요하기 때문에
        최대한 빨리 문제를 해결 후 이 옵션을 다시 켜는 것이 좋다.

        - 이 값을 변경한 후 새로 접속하는 클라이언트에 대해서만 적용된다.
        - 초기 상태는 FallbackMethod_None 이다.

        \param newValue 제한 정책. Proud.FallbackMethod_CloseUdpSocket 는 쓸 수 없다.

        \~english
        The method to set limits to how far the client uses UDP communication
        - This method can be used to block UDP communication between client and server or client and peer due to intentional reason by user.
        - The worst case, for an example, if there occurs a serious UDP communication hurdle at a server then the problem can be temporarily reduced by turning this option up.
        However, since UDP communication is crucial to game service so it is recommended to solve the problems ASAP then thrun this option back on.

        - Only applied to those newly connected clients after this value is changed
        - The initial status is FallbackMethod_None.

        \param newValue a limitation policy. Proud.FallbackMethod_CloseUdpSocket cannot be used.

        \~chinese
        设置client要限制UDP通信至什么程度的方法。
        - 因用户故意想要中断client和服务器或者peer的UDP通信的时候，可以使用此方法。
        - 最坏的情况，例如在UDP通信障碍严重发生的服务器里以应急方法开启此选项的话可以临时减少此问题。但是UDP通信在游戏服务里很重要，尽快把问题解决后重新开启此选项为好。

        - 只对修改此值以后重新连接的client适用。
        - 初始状态是FallbackMethod_None。

        \param newValue 限制政策。不能使用 Proud.FallbackMethod_CloseUdpSocket%。

        \~japanese
        \~
        */
        public void SetDefaultFallbackMethod(FallbackMethod newValue)
        {
            m_native_NetServer.SetDefaultFallbackMethod(newValue);
        }

        /**
		\~korean
		수퍼 피어로서 가장 적격의 역할을 할 클라이언트를 찾는다.
		\ref super_peer  형식의 게임을 개발중인 경우, 이 메서드는 groupID가 가리키는 P2P그룹 에 있는 멤버들 중
		가장 최적의 수퍼 피어를 찾아서 알려준다.
		- 이 메서드는 P2P 그룹을 생성하거나 변경한 직후 바로 얻을 경우 제대로 찾지 못할 수 있다.
		- 처음 이 메서드를 호출한 후부터 약 2~5초 정도 지난 후에 다시 호출하면 더 정확한 수퍼피어를 찾는다.
		\param groupID 이 \ref p2p_group 에서 찾는다
		\param policy 수퍼 피어를 선정하는 정책. 자세한 설명은 CSuperPeerSelectionPolicy 참고.
		\param exclusdees groupID가 가리키는 \ref p2p_group 의 멤버 중 exclusdees에 들어있는 멤버들은 제외하고 선별한다.
		예를 들어 이미 사용하던 수퍼피어가 자격 박탈된 경우 다시 이것이 재 선발됨을 의도적으로 막고자 할 때 유용하다.
		\return 수퍼피어로서 가장 적격인 클라이언트의 HostID. P2P 그룹을 찾지 못하거나 excludees에 의해 모두가 필터링되면 HostID_None 을 리턴합니다.

		\~english
		Finds the clients that can perform best as super peer
		When developing game of \ref super_peer format, this method finds and notifies the best possible super peer among members of the P2P group that is pointed by groupID.
		- There are possibilities that this method cannot find properly right after P2P group is eiter created or modified.
		- It will find more suitable super peer when called about 2 to 5 seconds after this method is called.
		\param groupID finds within this \ref p2p_group
		\param policy policy that designates super peer. Please refer CSuperPeerSelectionPolicy.
		\param exclusdees Selects excluding the members within excludees among \ref p2p_group that is pointed by groupID
		For an example, it is useful to prevent intentionally the super peer to be re-selected when the super peer wass once disqualified.
		\return HostID of the best possible client as super peer. Returns HostID_None either when no P2P group was found or all filtered by excludees.

		\~chinese
		寻找作为super peer起着最合适作用的client。
		正在开发\ref super_peer%形式游戏的话，此方法会找到在groupID所指的P2P组成员中最适合的superpeer并告知。
		- 此方法在生成P2P组或者修改后立即获得的时候可能会找得不够准确。
		- 开始呼叫此方法以后大概2~5秒后重新呼叫的话能找到正确的superpeer。
		\param groupID 在此\ref p2p_group%寻找。
		\param policy 选拔superpeer的政策。详细请参考 CSuperPeerSelectionPolicy%。
		\param exclusdees groupID所指的\ref p2p_group%成员中把exclusdees里的成员除外之后选拔。
		例如已使用的superpeer被剥夺资格以后，想故意阻止它重新被选定的时候有用。
		\return 作为superpeer最适合的clientHost ID。找不到P2P组或者被excludees全部过滤的话返回HostID_None。

		\~japanese
		\~
		*/
        public HostID GetMostSuitableSuperPeerInGroup(HostID groupID, SuperPeerSelectionPolicy policy, HostID[] excludees)
        {
            return NativeInternalNetServer.GetMostSuitableSuperPeerInGroup(m_native_NetServer, groupID, policy, excludees);
        }

        /**
		\~korean
		동명 메서드 설명을 참고바람

		\~english
		Please refer same name method.

		\~chinese
		请参考同名方法说明。

		\~japanese
		\~
		*/
        public HostID GetMostSuitableSuperPeerInGroup(HostID groupID, SuperPeerSelectionPolicy policy, HostID excludee)
        {
            return m_native_NetServer.GetMostSuitableSuperPeerInGroup(groupID, policy, excludee);
        }

        /**
		\~korean
		동명 메서드 설명을 참고바람

		\~english
		Please refer same name method.

		\~chinese
		请参考同名方法说明。

		\~japanese
		\~
		*/
        public HostID GetMostSuitableSuperPeerInGroup(HostID groupID, SuperPeerSelectionPolicy policy)
        {
            return m_native_NetServer.GetMostSuitableSuperPeerInGroup(groupID, policy);
        }

        /**
		\~korean
		동명 메서드 설명을 참고바람

		\~english
		Please refer same name method.

		\~chinese
		请参考同名方法说明。

		\~japanese
		\~
		*/
        public HostID GetMostSuitableSuperPeerInGroup(HostID groupID)
        {
            return m_native_NetServer.GetMostSuitableSuperPeerInGroup(groupID);
        }

        /**
		\~korean
		엔진 프로토콜 버전을 얻는다. 이 값이 클라이언트에서 얻는 엔진 프로토콜 버전과 다르면 접속이 불허된다.

		\~english
		Gets engine protocol version. Connection will be rejected if this value is different to engine protocol version from client.

		\~chinese
		获得引擎协议版本。此值与从client获得的引擎协议版本不同的话不允许连接。

		\~japanese
		\~
		*/
        public int GetInternalVersion()
        {
            return m_native_NetServer.GetInternalVersion();
        }

        /**
		\~korean
		서버 처리량 통계를 얻는다.

		\~english
		Gets server process statistics

		\~chinese
		获得服务器处理量统计。

		\~japanese
		\~
		*/
        public NetServerStats GetStats()
        {
            return m_native_NetServer.GetStats();
        }

        /**
		\~korean
		내부 실행 로그를 파일로 남기는 기능을 켠다.
		- 이 기능이 필요한 경우는 별로 없다. Nettention에서 문제를 수집하기 위한 목적으로 사용된다.
		- 이미 EnableLog 를 한 상태에서 이 함수를 또 호출하면 이 함수는 아무것도 하지 않는다. 만약 다른 파일명으로
		로그를 계속해서 쌓고 싶으면 DisableLog를 호출 후 EnableLog를 호출해야 한다.
		\param logFileName 로그 파일 이름

		\~english
		Turns on the option that leaves internal execution log as file
		- There are not many cases that require this function. Used to collect problems at Nettention.
		- Calling this function again while already 'EnableLog'ed, this function will not do anything. If you need to keep stacking the log with other file name the you must call DisableLog then call EnableLog.
		\param logFileName log file name

		\~chinese
		开启把内部实行log保留为文件的功能。
		- 一般没有需要此功能的情况。 Nettention 以收集问题为目的使用此功能。
		- 已EnableLog的状态下呼叫此函数的话，此函数不会做任何事情。如果想用其他文件名继续堆积log的话，要呼叫DisableLog以后再呼叫EnableLog。
		\param logFileName log文件名。

		\~japanese
		\~
		*/
        public void EnableLog(string logFileName)
        {
            if (m_native_NetServer == null)
            {
                return;
            }
            m_native_NetServer.EnableLog(logFileName);
        }

        /**
		\~korean
		EnableLog로 켠 로그를 끈다.

		\~english
		Turns off the log that is thrned on by EnableLog

		\~chinese
		关闭EnableLog的log。

		\~japanese
		\~
		*/
        public void DisableLog()
        {
            if (m_native_NetServer == null)
            {
                return;
            }
            m_native_NetServer.DisableLog();
        }

        /**
		\~korean
		두 피어간의 최근 핑 정보를 받습니다.

		\~english
		Gets recent ping information between two peers

		\~chinese
		接收两个peer之间的最近ping信息。

		\~japanese
		\~
		*/
        public int GetP2PRecentPingMs(HostID HostA, HostID HostB)
        {
            return m_native_NetServer.GetP2PRecentPingMs(HostA, HostB);
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
            return m_native_NetServer.SetHostTag(remote, new IntPtr(hostTag));
        }

        /**
        \~korean
        클라이언트의 마지막 레이턴시를 얻는다.
        \param clientID 찾으려는 클라이언트의 HostID.
        \return ping time을 밀리초단위로 리턴한다. 단 없으면 -1을 리턴한다.

        \~english
        Gets the last latency of client
        \param clientID HostID of client to find
        \return Returns ping time in millisecond.Returns -1 when there is none.

        \~chinese
        获得client的最后latency。
        \param clientID 要找的client的Host ID。
        \return ping把time以毫秒单位返回。但没有的话返回-1。

        \~japanese
        \~
        */
        public int GetLastUnreliablePingMs(HostID peerHostID)
        {
            return m_native_NetServer.GetLastUnreliablePingMs(peerHostID);
        }

        /**
		\~korean
		클라이언트의 최근 레이턴시를 얻는다.
		\param clientID 찾으려는 클라이언트의 HostID.
		\return ping time을 밀리초단위로 리턴한다. 단 없으면 -1을 리턴한다.

		\~english
		Gets the recent latency of client
		\param clientID HostID of client to find
		\return Returns ping time in millisecond.Returns -1 when there is none.

		\~chinese
		获得client的最近latency。
		\param clientID 要找的client的Host ID。
		\return ping把time以毫秒单位返回。但没有的话返回-1。

		\~japanese
		\~
		*/
        public int GetRecentUnreliablePingMs(HostID peerHostID)
        {
            return m_native_NetServer.GetRecentUnreliablePingMs(peerHostID);
        }

        /**
		\~korean
		멤버가 전혀 없는 \ref p2p_group 이 유지될 수 있도록 설정하는 옵션. 기본적으로는 true로 설정되어 있다.
		- true로 설정될 경우: CreateP2PGroup 호출시 멤버 0개짜리 P2P 그룹을 생성해도 성공한다. 멤버가 0개짜리 P2P 그룹이 계속 존재한다.
		이러한 경우 사용자는 멤버가 0개인 P2P 그룹이 계속해서 서버에 남지 않도록, 필요한 경우 DestroyP2PGroup 또는 DestroyEmptyP2PGroups 을 호출해야 한다.
		- false로 설정될 경우: CreateP2PGroup 호출시 멤버 0개짜리 P2P 그룹을 생성할 수 없다. P2P 그룹의 멤버가 0개가 되면 그 P2P 그룹은 자동 파괴된다.

		\~english
		Option to set \ref p2p_group without member can be sustained. Default is true.
		- When set as true: Creating P2P group of 0 member will succeed when CreateP2PGroup is called. The P2P group of 0 member will sustain.
		In this case, user must call DestroyP2PGroup or DestroyEmptyP2PGroups to prevent the P2P group of 0 member to be left in server.
		- When set as false: Creating P2P group of 0 member will fail when CreateP2PGroup is called. If the number of P2P group becomes 0 then the P2P group will be destructed automatically.

		\~chinese
		设置成\ref p2p_group%没有成员时也能维持的选项。默认设置为true。
		- 设置为true的话：呼叫CreateP2Pgroup的话，即使生成0个成员的P2P组也会成功。0个成员的P2P组继续存在。
		这时候用户不要让0个成员的P2P组继续存在于服务器，并且需要的话呼叫DestroyP2PGroup或者DestroyEmptyP2PGroups。
		- 设置为false的话：呼叫CreateP2PGroup时不能生成0个成员的P2P组。P2P 组的成员变成0个的话那个P2P组会自动破坏。

		\~japanese
		\~
		*/
        public void AllowEmptyP2PGroup(bool enable)
        {
            m_native_NetServer.AllowEmptyP2PGroup(enable);
        }

        /**
       \~korean
       멤버가 전혀 없는 \ref p2p_group 이 유지될 수 있도록 설정하는 옵션값을 얻습니다.
       \return 멤버가 전혀 없는 \ref p2p_group 이 유지될 수 있도록 설정하는 옵션값입니다.

       \~english
       Gets option value to set \ref p2p_group without member can be sustained.
       \return Option value to set \ref p2p_group without member can be sustained.

       \~chinese
       Gets option value to set \ref p2p_group without member can be sustained.
       \return Option value to set \ref p2p_group without member can be sustained.

       \~japanese
       \~
       */
        public bool IsEmptyP2PGroupAllowed()
        {
            return m_native_NetServer.IsEmptyP2PGroupAllowed();
        }

        /**
		\~korean
		지정한 대상과 관련된 모든 P2P 연결의 상태 통계를 얻습니다.
		가령 Direct P2P와 relayed P2P의 비율을 얻을 때 이 메서드가 유용합니다.
		\param remoteHostID 클라이언트 또는 \ref p2p_group 의 HostID 입니다.
		- 1개 클라이언트의 HostID 인 경우, 이 메서드는 그 클라이언트가 P2P 통신하는 다른 피어들과의 연결 통계를 반환합니다.
		- 1개 P2P 그룹의 HostID 인 경우, 이 메서드는 그 P2P 그룹 내의 모든 클라이언트끼리의 P2P 통신에 대한 통계를 반환합니다.
		\param status P2P커넥션 관련 변수들이 여기에 채워집니다.
		\return remoteHostID가 유효하면 true,유효하지 않으면 false를 리턴 합니다.

		\~english
		Gets status statistics of all P2P connections related to designated target
		This method is useful to get the proportion of Direct P2P and relayed P2P.
		\param remoteHostID HostID of client or \ref p2p_group
		- If this is HostID of 1 client then this method returns connection statistics of other peers that the client is P2P communicating with.
		- If this is HostID of 1 P2Pgroup then this method returns the statistics of all P2P communications among all clients in the P2P group.
		\param status P2P connection related vairables are to be filled in here.
		\return Returns true if remoteHostID is valid, false if not valid

		\~chinese
		获得与指定的对象相关的所有P2P连接状态统计。
		如果获得Direct P2P和relayed P2P的比率的时候此方法有用。
		\param remoteHostID client或者\ref p2p_group%的Host ID。
		- 一个client的Host ID的话，此方法返回那个client与其他peer进行P2P通信的连接统计。
		- 一个P2P组的Host ID的话，此方法返回对那个P2P组内的所有client之间P2P通信的统计。
		\param status 这里填充P2P connection相关变数。
		\return remoteHostID有效的话true，无效的话返回false。

		\~japanese
		\~
		*/
        public bool GetP2PConnectionStats(HostID remoteHostID, ref P2PConnectionStats status)
        {
            if (status == null)
            {
                throw new System.ArgumentNullException("status");
            }

            return m_native_NetServer.GetP2PConnectionStats(remoteHostID, status);
        }

        /**
        \~korean
        remoteA 와 remoteB 사이의 udp message 시도횟수와 성공횟수를 반환해줍니다.
        - 사용 자는 이를 바탕으로 udp 손실율 등을 계산할수있습니다.

        \~english
        Return attempted number and succeed number of udp message between remoteA and remoteB
        - User can calculate udp loss rate based on it.

        \~chinese
        返回remoteA与remoteB之间udp message试图次数和成功次数。
        - 用户可以以此为基础计算UDP损失率等。

        \~japanese
        \~
        */
        public bool GetP2PConnectionStats(HostID remoteA, HostID remoteB, ref P2PPairConnectionStats status)
        {
            if (status == null)
            {
                throw new System.ArgumentNullException("status");
            }

            return m_native_NetServer.GetP2PConnectionStats(remoteA, remoteB, status);
        }

 
        // INetCoreEvent Delegate
        public delegate void ErrorDelegate(Nettention.Proud.ErrorInfo errorInfo);
        public delegate void WarningDelegate(Nettention.Proud.ErrorInfo errorInfo);
        public delegate void InformationDelegate(Nettention.Proud.ErrorInfo errorInfo);

        public delegate void ExceptionDelegate(System.Exception e);
        public delegate void NoRmiProcessedDelegate(Proud.RmiID rmiID);
        public delegate void ReceiveUserMessageDelegate(Proud.HostID sender, Nettention.Proud.RmiContext rmiContext, Proud.ByteArray payload);

        // INetServerEvent Delegate

        /**
        \~korean
        클라이언트가 연결을 들어오면 발생하는 이벤트
        - OnConnectionRequest에서 진입 허가를 한 클라이언트에 대해서만 콜백된다.
        \param clientInfo 연결이 들어온 클라이언트
        \~english
        Event that occurs when client attempts to connect
        - Only called back for the client that received access permission from OnConnectionRequest
        \param clientInfo = client requested to connect
        \~
        */
        public delegate void ClientJoinDelegate(Nettention.Proud.NetClientInfo clientInfo);

        /**
        \~korean
        클라이언트가 연결을 해제하면 발생하는 이벤트입니다.
        \param clientInfo 연결이 해제된 클라이언트의 정보입니다.
        \param errorInfo 연결이 해제된 클라이언트의 연결 해제 정보입니다.
        ErrorInfo.m_type 값은 연결 해제의 경위를, ErrorInfo.
        자세한 것은 ErrorInfo 도움말에 있습니다.
        \param comment 클라이언트가 CNetClient.Disconnect의 파라메터 comment 를 통해 보내온 문자열입니다.
        \~english
        Event that occurs when client terminates connection
        \param clientInfo = client info that is disconnected
        \param errorInfo = disconnection info of client that is disconneced
        ErrorInfo.m_type value shows how disconnection processed
        Please refer ErrorInfo help section.
        \param comment = text string sent by client via parameter comment of CNetClient.Disconnect
        \~
        */
        public delegate void ClientLeaveDelegate(Nettention.Proud.NetClientInfo clientInfo, Nettention.Proud.ErrorInfo errorinfo, Proud.ByteArray comment);

        /**
        \~korean
        클라이언트가 서버로 처음 연결을 시도하면 콜백되는 이벤트
        - 이 이벤트에서는 서버가 클라의 연결 시도를 수용할 것인지 거절할 것인지를 결정한다.
        만약 거절할 경우 클라이언트는 서버와의 연결이 실패하며 클라이언트는
        ErrorType_NotifyServerDeniedConnection를 받게 된다.
        만약 수용할 경우 클라이언트는 서버와의 연결이 성공하게 되며 클라이언트는 새 HostID를
        할당받게 된다. 이때 서버는 OnClientJoin 이벤트가 콜백된다.

        주의사항
        - 절대! 여기에서 유저의 로그온 정보(아이디,비밀번호)를 주고받지 말아야 한다.
        왜냐하면 여기서 클라이언트가 서버로 보내는 커스텀 데이터는 비 암호화되어 있기
        때문이다.

        \param clientAddr 클라이언트의 인터넷 주소
        \param userDataFromClient 클라이언트에서 보낸 커스텀 데이터. 이 값은 클라이언트에서 서버로 연결시
        넣었던 파라메터 CNetConnectionParam::m_userData에서 넣었던 값이다.
        \param reply 이 이벤트 콜백에서 이 필드에 값을 채우면 클라이언트는 서버와의 연결 결과
        INetClientEvent::OnJoinServerComplete에서 받게 된다.
        \return 클라이언트로부터의 연결을 수용할 것이라면 이 함수는 true를 리턴하면 된다.
        \~english
        Event called back when client attempts to connect to server for the first time
        - This event is where to decide if server accepts the attempt or not
        If refused then client fails to connect to server and client receives ErrorType_NotifyServerDeniedConnection.
        If accepted then client succeeds to connect to server and a new HostID will be allocated to client.
        This is when OnClientJoin event is called back to server

        Note
        - NEVER! to send/receive user logon info(user account name and/or password) in here.
        That is because the custom data from cliet to server is not encrypted.

        \param clientAddr = Internet address of client
        \param userDataFromClient = The custom data from client
        This value is the value that was entered at parameter CNetConnectionParam::m_userData when client was connected to server.
        \param reply = If this event callback fills a value to this field then client will receive the result of connection to server from INetClientEvent::OnJoinServerComplete.
        \return = If to accept the connection reqeust from client then this function should return true.
        \~
        */
        public delegate bool ConnectionRequestDelegate(Nettention.Proud.AddrPort clientAddr, Proud.ByteArray userDataFromClient, Proud.ByteArray reply);

        /**
        \~korean
        \ref p2p_group 에 새 멤버가 추가되는 과정이 모든 클라이언트에서도 완료되면 발생하는 이벤트입니다.
        - 만약 그룹 G에 기 멤버 M1,M2,M3가 있다고 가정할 경우 신규 멤버 M4를 넣는다고 합시다.
        이때 JoinP2PGroup()에 의해 M4가 추가되더라도 즉시 완료되는 것은 아닙니다. 각 클라이언트
        M1,M2,M3에서는 아직 'M4의 추가되었음' 보고(INetClientEvent.OnP2PMemberJoin)를 받지 않았고 M4에서도 M1,M2,M3,M4 모두에 대한 '추가되었음 보고'(INetClientEvent.OnP2PMemberJoin)를 받지 않았기 때문입니다.
        이 이벤트는 M4에 대해, M1,M2,M3,M4에서 이들을 모두 받은 것을 서버에서 확인한 후에야 발생됩니다.
        - CreateP2PGroup에 대해서도 마찬가지로 이 이벤트가 발생합니다.

        \param groupHostID \ref 그룹의 HostID
        \param memberHostID 그룹 멤버의 HostID
        \param result ErrorType_Ok이면 성공적으로 추가됐음을 의미하며,ErrorType_AlreadyExists이면 이미 그룹내 들어가있는 멤버임을 의미 합니다.(실패 했다는 의미는 아님)
        그외 다른 값이면 추가가 실패했음을 의미합니다.
        예를 들어 CNetServer.CreateP2PGroup 혹은 CNetServer.JoinP2PGroup 호출 후에 이 콜백이 있기 전에
        CNetServer.LeaveP2PGroup 혹은 CNetServer.DestroyP2PGroup를 호출 하거나, 해당 클라이언트가 동시에 서버와의 연결을 끊고 있는 중이었다면
        다른 값이 여기에 들어갈 수 있습니다.
        \~english
        The event that occurs when the process adding a new member to \ref p2p_group is completed in all client
        - Assuming that there are existing member M1, M2 and M3 in group G and we try to add a new member M4.
        Even if M4 is added by JoinP2PGroup(), the process is not to be completed immediately.
        This is because each client M1, M2 and M3 did not yet receive the report(INetClientEvent.OnP2PMemberJoin) that 'M4 is added',
        and M4 did not yet receive the report(INetClientEvent.OnP2PMemberJoin) that M4 is added to M1, M2, M3 and M4.
        This event, for M4, occurs only after server acknowledges that M1, M2, M3 and M4 received necesary info.
        - The event also occurs for CreateP2PGroup.

        \param groupHostID = HostId of \ref group
        \param memberHostID = HostID of group member
        \param result = If ErrorType_Ok then it means successfully added, if ErrorType_AlreadyExists then it is a member that already in group. (does not mean it falied to)
        If has other values then it means failed to add.
        For an example,
        Case1. Calling CNetServer.LeaveP2PGroup or CNetServer.DestroyP2PGroup between the moment after calling CNetServer.CreateP2PGroup or CNetServer.JoinP2PGroup and the moment before this callback occurs, OR
        Case2. the client was terminating its connection to server at the same time,
        For each case, an other value can enter here.
        \~
        */
        public delegate void P2PGroupJoinMemberAckCompleteDelegate(Proud.HostID groupHostID, Proud.HostID memberHostID, Nettention.Proud.ErrorType result);

        /**
        \~korean
        user work thread pool의 스레드가 시작할 때 이 메서드가 호출된다.
        \~english
        This method is called when thread of user work thread pool starts.
        \~
        */
        public delegate void UserWorkerThreadBeginDelegate();

        /**
        \~korean
        user work thread pool의 스레드가 종료할 때 이 메서드가 호출된다.
        \~english
        This method is called when thread of user work thread pool terminates.
        \~
        */
        public delegate void UserWorkerThreadEndDelegate();

        /**
        \~korean
        클라이언트가 해킹당했다는 의혹이 있을 때 이 메서드가 호출된다.
        \param clinetID 의심되는 client의 HostID
        \param hackType 해킹의 종류
        \~english
        This method is called when there is any suspicion that client is hacked.
        \~
        */
        public delegate void ClientHackSuspectedDelegate(Proud.HostID clientID, Nettention.Proud.HackType hackType);

        /**
        \~korean
        P2P group이 제거된 직후 이 메서드가 호출됩니다.
        - P2P group이 제거되는 조건은 Proud.CNetServer.DestroyP2PGroup를 호출하거나 P2P group에 소속된 마지막 멤버가
        서버와의 접속을 종료할 때 등입니다.
        \~english
        This method is called right after P2P group is removed.
        - The condition P2P froup is removed is EITHER calling Proud.CNetServer.DestroyP2PGroup OR the last member of P2P group terminates its connection to server.
        \~
        */
        public delegate void P2PGroupRemovedDelegate(Proud.HostID groupID);

        /**
        \~korean
        RMI 호출 또는 이벤트 발생으로 인해 user worker에서 callback이 호출되기 직전에 호출됩니다.
        프로그램 실행 성능 측정을 위해 사용하셔도 됩니다.
        \~english
        Called right before calling callback by either RMI calling or user worker due to event occur. Also can be used for performance test purpose.
        \~
        */
        //delegate void UserWorkerThreadCallbackBeginDelegate(UserWorkerThreadCallbackContext^ ctx);
        /**
        \~korean
        RMI 호출 또는 이벤트 발생으로 인해 user worker에서 callback이 리턴한 직후에 호출됩니다.
        프로그램 실행 성능 측정을 위해 사용하셔도 됩니다.
        \~english
        Called right before calling callback by either RMI calling or user worker due to event occur. Also can be used for performance test purpose.
        \~
        */
        //delegate void UserWorkerThreadCallbackEndDelegate(UserWorkerThreadCallbackContext^ ctx);

        /**
        \~korean
        일정 시간마다 user worker thread pool에서 콜백되는 함수입니다. \ref server_timer_callback  기능입니다.
        \param context CStartServerParameter.m_timerCallbackContext에서 입력된 값과 동일합니다.
        \~english
        Function called back at user worker thread pool periodically. This is a \ref server_timer_callback function.
        \param context = same as the value entered at CStartLanServerParameter.m_timerCallbackContext
        \~
        */
        public delegate void TickDelegate(Object context);

        public delegate void ClientOfflineDelegate(Nettention.Proud.RemoteOfflineEventArgs args);

        public delegate void ClientOnlineDelegate(Nettention.Proud.RemoteOnlineEventArgs args);

        // INetCoreEvent Handler
        public ErrorDelegate ErrorHandler = null;
        public WarningDelegate WarningHandler = null;
        public InformationDelegate InformationHandler = null;
        public ExceptionDelegate ExceptionHandler = null;
        public NoRmiProcessedDelegate NoRmiProcessedHandler = null;
        public ReceiveUserMessageDelegate ReceiveUserMessageHandler = null;

        // INetServerEvent Handler
        public ClientJoinDelegate ClientJoinHandler = null;
        public ClientLeaveDelegate ClientLeaveHandler = null;
        public ConnectionRequestDelegate ConnectionRequestHandler = null;
        public P2PGroupJoinMemberAckCompleteDelegate P2PGroupJoinMemberAckCompleteHandler = null;
        public UserWorkerThreadBeginDelegate UserWorkerThreadBeginHandler = null;
        public UserWorkerThreadEndDelegate UserWorkerThreadEndHandler = null;
        public ClientHackSuspectedDelegate ClientHackSuspectedHandler = null;
        public P2PGroupRemovedDelegate P2PGroupRemovedHandler = null;
        //public UserWorkerThreadCallbackBeginDelegate UserWorkerThreadCallbackBeginHandler= null;
        //public UserWorkerThreadCallbackEndDelegate UserWorkerThreadCallbackEndHandler= null;
        public TickDelegate TickHandler = null;

        public ClientOfflineDelegate ClientOfflineHandler = null;
        public ClientOnlineDelegate ClientOnlineHandler = null;
    }
}
