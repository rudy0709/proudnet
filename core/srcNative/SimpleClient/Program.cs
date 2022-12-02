using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;

using Nettention.Proud;
using SimpleCSharp;

namespace TestClient
{
    internal class C2SProxy : C2S.Proxy
    {
        public override void NotifySendByProxy(HostID[] remotes, MessageSummary summary, RmiContext rmiContext, Message msg)
        {
            Console.WriteLine("C2SProxy.NotifySendByProxy Call " + summary.payloadLength);
        }
    }

    public class C2CStub
    {
        public C2C.Stub stub = new C2C.Stub();

        public C2CStub()
        {
            stub.P2PChat = P2PChat;
            stub.P2PChat2 = P2PChat2;
            stub.AfterRmiInvocation = AfterRmiInvocation;
            stub.BeforeRmiInvocation = BeforeRmiInvocation;
        }

        public void AfterRmiInvocation(AfterRmiSummary summary)
        {

        }
        void BeforeRmiInvocation(BeforeRmiSummary summary)
        {

        }

        public bool P2PChat(HostID remote, RmiContext rmiContext, string a, int b, float c)
        {
            Console.WriteLine("[Client] P2PChat : a={0}, b={1}, c={2}, relay={3}\n", a, b, c, rmiContext.relayed);
            return true;
        }

        public bool P2PChat2(HostID remote, RmiContext rmiContext, SimpleCSharp.MyClass a)
        {
            Console.WriteLine("[Client] P2PChat : a={0}, b={1}, c={2} relay={3}\n", a.a, a.b, a.c, rmiContext.relayed);
            return true;
        }
    }

    class S2CStub
    {

        public S2C.Stub stub = new S2C.Stub();

        public S2CStub()
        {
            stub.ShowChat = ShowChat;
            stub.SystemChat = SystemChat;
            stub.SendByteArray = SendByteArray;
        }

        public bool ShowChat(HostID remote, RmiContext rmiContext, string a, int b, float c)
        {
            Console.WriteLine("[Client] ShowChat : a={0}, b={1}, c={2}\n", a, b, c);
            return true;
        }

        public bool SystemChat(HostID remote, RmiContext rmiContext, string txt)
        {
            Console.WriteLine("[Client] SystemChat : txt={0}\n", txt);
            return true;
        }

        public bool SendByteArray(Nettention.Proud.HostID remote,Nettention.Proud.RmiContext rmiContext, Nettention.Proud.ByteArray buffer)
        {
            Console.WriteLine("RmiStub SendByteArray : " + Encoding.Default.GetString(buffer.data));
            return true;
        }
    }

    public class ClientEventSink
    {
        public ClientEventSink(NetClient Client, C2C.Proxy Proxy)
        {
            m_Client = Client;
            m_C2CProxy = Proxy;

            //핸들 넣어주는 곳
            m_Client.JoinServerCompleteHandler = OnJoinServerComplete;
            m_Client.LeaveServerHandler = OnLeaveServer;
            m_Client.P2PMemberJoinHandler = OnP2PMemberJoin;
            m_Client.P2PMemberLeaveHandler = OnP2PMemberLeave;
            m_Client.ChangeServerUdpStateHandler = ChangeServerUdpState;
            m_Client.ChangeP2PRelayStateHandler = ChangeP2PRelayState;
            m_Client.ExceptionHandler = OnException;

            // ACR
            m_Client.ServerOfflineHandler = OnServerOffline;
            m_Client.ServerOnlineHandler = OnServerOnline;

            m_Client.P2PMemberOfflineHandler = OnP2PMemberOffline;
            m_Client.P2PMemberOnlineHandler = OnP2PMemberOnline;
            m_Client.NoRmiProcessedHandler = OnNoRmiProcessed;

            m_Client.ReceivedUserMessageHandler = OnReceiveUserMessage;
        }

        public void OnNoRmiProcessed(RmiID rmiID)
        {

        }
        public void OnException(HostID remoteID, System.Exception e)
        {
            Console.WriteLine("Exception!\n" + e.ToString() + "\n");
        }

        public void ChangeServerUdpState(ErrorType reason)
        {
            Console.WriteLine("ChangeServerUdpState : {0}", reason);
        }

        public void ChangeP2PRelayState(HostID remoteHostID, ErrorType reason)
        {
            Console.WriteLine("ChangeP2PRelayState : {0}", reason);
        }

        public NetClient m_Client;
        public C2C.Proxy m_C2CProxy;
        public bool m_IsConnectWaiting = true;
        public bool m_IsConnected = false;
        public HostID memberid = HostID.HostID_None;
        public HostID groupID = HostID.HostID_None;

        public bool IsConnectWating()
        {
            return m_IsConnectWaiting;
        }

        public bool IsConnected()
        {
            return m_IsConnected;
        }

        public void OnJoinServerComplete(Nettention.Proud.ErrorInfo info, ByteArray replyFromServer)
        {
            m_IsConnectWaiting = false;

            m_IsConnected = true;

            if (info.errorType == Nettention.Proud.ErrorType.Ok)
            {
                Console.WriteLine("Succeed to connect server. Allocated HostID={0} \n", m_Client.GetLocalHostID());
                m_IsConnected = true;
            }
            else
                Console.WriteLine("Failed to connect server.\n");
        }

        public void OnLeaveServer(Nettention.Proud.ErrorInfo errorInfo)
        {
            m_IsConnected = false;
        }

        public void OnP2PMemberJoin(HostID memberHostID, HostID groupHostID, int memberCount, ByteArray message)
        {
            Console.WriteLine("[Client] P2P member {0} joined  group{1}.\n", memberHostID, groupHostID);

            if (memberHostID != m_Client.GetLocalHostID())
            {
                m_C2CProxy.P2PChat(memberHostID, Nettention.Proud.RmiContext.ReliableSend, "Hello~~", 1, 1);

                // Send RMI with user defined class
                // 사용자 정의 class를 RMI에 넣어서 보내보자
                SimpleCSharp.MyClass myclass = new SimpleCSharp.MyClass();
                myclass.a = 1;
                myclass.b = 1.5f;
                myclass.c = 3.141592;

                m_C2CProxy.P2PChat2(memberHostID, Nettention.Proud.RmiContext.ReliableSend, myclass);

                memberid = memberHostID;
                groupID = groupHostID;
            }
        }

        public void OnP2PMemberLeave(HostID memberHostID, HostID groupHostID, int memberCount)
        {
            Console.WriteLine("[Client] P2P member {0} left group {1}.\n", memberHostID, groupHostID);
        }

        public void OnServerOffline(Nettention.Proud.RemoteOfflineEventArgs args)
        {

        }

        public void OnServerOnline(Nettention.Proud.RemoteOnlineEventArgs args)
        {

        }

        public void OnP2PMemberOffline(Nettention.Proud.RemoteOfflineEventArgs args)
        {

        }

        public void OnP2PMemberOnline(Nettention.Proud.RemoteOnlineEventArgs args)
        {

        }

        public void OnReceiveUserMessage(HostID sender, RmiContext rmiContext, ByteArray payload)
        {
            Console.WriteLine("OnReceiveUserMessage Call");
            for (int i = 0; i < payload.Count; ++i)
            {
                Console.WriteLine("{0} = {1}", i, payload.data[i]);
            }
        }
    }

    class Program
    {
        [DllImport("msvcrt")]
        static extern int _getch();

        static void Main(string[] args)
        {
            ThreadPool netWorkerthreadPool = new ThreadPool(4);
            ThreadPool userWorkerThreadPool = new ThreadPool(4);
            Vars vars = new Vars();
            C2SProxy C2Sproxy = new C2SProxy();
            C2C.Proxy C2Cproxy = new C2C.Proxy();
            C2CStub C2Cstub = new C2CStub();
            S2CStub S2Cstub = new S2CStub();
            NetClient Client = new NetClient();

            ClientEventSink EventSink = new ClientEventSink(Client, C2Cproxy);

            Client.AttachProxy(C2Sproxy);
            Client.AttachProxy(C2Cproxy);
            Client.AttachStub(C2Cstub.stub);
            Client.AttachStub(S2Cstub.stub);

            Nettention.Proud.NetConnectionParam cp = new Nettention.Proud.NetConnectionParam();
            cp.protocolVersion.Set(SimpleCSharp.Vars.m_Version);

            cp.serverIP = "127.0.0.1";
            cp.serverPort = (ushort)SimpleCSharp.Vars.m_serverPort;
            //cp.enableAutoConnectionRecovery = true;

            ByteArray userData = new ByteArray();
            userData.Add((byte)'j');
            userData.Add((byte)'o');
            userData.Add((byte)'n');
            userData.Add((byte)'g');

            cp.SetUserData(userData);

            cp.localUdpPortPool.Add(100);
            cp.localUdpPortPool.Add(200);
            cp.localUdpPortPool.Add(300);
            cp.localUdpPortPool.Add(400);

            cp.netWorkerThreadModel = ThreadModel.ThreadModel_UseExternalThreadPool;
            cp.userWorkerThreadModel = ThreadModel.ThreadModel_UseExternalThreadPool;

            cp.SetExternalNetWorkerThreadPool(netWorkerthreadPool);
            cp.SetExternalUserWorkerThreadPool(userWorkerThreadPool);

            Nettention.Proud.ErrorInfo errorInfo = new Nettention.Proud.ErrorInfo();

            if (Client.Connect(cp, errorInfo) == false)
                Console.WriteLine("Failed to connect client ~!!\n");

            while (EventSink.IsConnectWating())
            {
                System.Threading.Thread.Sleep(10);
                Client.FrameMove();
            }

            // 연결성공하자마자 패킷을 하나 보낸다.
            // 사용자 정의 class를 RMI에 넣어서 보내보자
            SimpleCSharp.MyClass myclass = new SimpleCSharp.MyClass();
            myclass.a = 1;
            myclass.b = 1.5f;
            myclass.c = 3.141592;

            // List혹은 Dictionary를 Rmi인자로 사용이 가능합니다.
            List<int> list = new List<int>();
            list.Add(4);
            list.Add(5);

            Dictionary<int, float> dic = new Dictionary<int, float>();
            dic.Add(1, 1.5f);
            dic.Add(2, 3.1f);

            ByteArray block = new ByteArray();
            for (int i = 0; i < 100; ++i)
            {
                block.Add((byte)i);
            }
            //C2Sproxy.Chat(HostID.Server, RmiContext.ReliableSend, "메롱", 22, 22.33f, myclass, list, dic, block);


            while (EventSink.IsConnected())
            {
                System.Threading.Thread.Sleep(10);

                Client.FrameMove();

                if (Console.KeyAvailable)
                {
                    ConsoleKeyInfo keyinfo = Console.ReadKey();
                    if (keyinfo.Key == ConsoleKey.Escape)
                    {
                        //DisconnectArgs disArg = new DisconnectArgs();
                        //disArg.comment = new byte[10];
                        //for (int i = 0; i < disArg.comment.Length; ++i )
                        //{
                        //    disArg.comment[i] = (byte)i;
                        //}

                        //Client.Disconnect(disArg);
                        break;
                    }
                    if (keyinfo.Key == ConsoleKey.A)
                    {
                        if (EventSink.groupID != HostID.HostID_None)
                        {
                            //C2Cproxy.P2PChat2(EventSink.groupID, RmiContext.UnreliableSend, myclass);
                            C2Cproxy.P2PChat(EventSink.groupID, RmiContext.UnreliableSend, "Welcome ProudNet!!", 1, 1);
                        }
                    }
                    if (keyinfo.Key == ConsoleKey.B)
                    {
                        //for (int i = 0; i < 500; ++i)
                        {
                            C2Sproxy.Chat(HostID.HostID_Server, RmiContext.ReliableSend, "메롱", 22, 22.33f, myclass, list, dic, block);
                        }
                    }
                    if (keyinfo.Key == ConsoleKey.C)
                    {
                        Client.Disconnect();
                    }
                    if (keyinfo.Key == ConsoleKey.D1)
                    {
                        ByteArray payload = new ByteArray();
                        payload.Add((byte)1);
                        payload.Add((byte)2);
                        payload.Add((byte)3);
                        payload.Add((byte)4);
                        payload.Add((byte)5);
                        Client.SendUserMessage(HostID.HostID_Server, RmiContext.ReliableSend, payload);
                    }
                }
            }

            Client.Dispose();
            //_getch();
        }
    }
}
