using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;
using System.Net;

using System.IO;
using Nettention.Proud;
using SimpleCSharp;

namespace TestServer
{
#if true
    public class C2SStub
    {
        internal C2S.Stub stub = new C2S.Stub();
        public S2C.Proxy m_S2Cproxy = null;

        public C2SStub(S2C.Proxy proxy)
        {
            m_S2Cproxy = proxy;
            stub.Chat = Chat;
            stub.AfterRmiInvocation = AfterRmiInvocation;
            stub.BeforeRmiInvocation = BeforeRmiInvocation;
            stub.enableStubProfiling = true;
        }

        public bool Chat(Nettention.Proud.HostID remote, Nettention.Proud.RmiContext rmiContext, System.String a, int b, float c, MyClass d, List<int> f, Dictionary<int, float> g, ByteArray block)
        {
            Console.WriteLine("[Server] Received message : a={0}, b={1}, c={2}, d.a={3}, d.b={4}, d.c={5}", a, b, c, d.a, d.b, d.c);

            int count = 0;
            foreach (int i in f)
            {
                Console.WriteLine("f[{0}]={1}", count, i);
                count++;
            }

            foreach (KeyValuePair<int, float> pair in g)
            {
                Console.WriteLine("Pair({0},{1}) ", pair.Key, pair.Value);
            }

            int length = block.Count;
            if (length != 100)
                Console.WriteLine("ByteArray length does not match.");

            for (int i = 0; i < length; ++i)
            {
                if ((byte)i != block[i])
                    Console.WriteLine("ByteArray does not match. index : {0} data : {1}", i, (int)block[i]);
            }

            // 서버에서 받은 채팅 메시지를 다시 클라이언트에게 에코한다.
            // Echo chatting message that received from server to client again.
            m_S2Cproxy.ShowChat(remote, Nettention.Proud.RmiContext.ReliableSend, a, b + 1, c + 1);

            return true;
        }

        public void AfterRmiInvocation(Nettention.Proud.AfterRmiSummary summary)
        {
            Console.WriteLine("AfterRmiSummary : hostid : {0}, RmiName : {1} Rmiid : {2} ElapsedTime : {3}",
                summary.hostID, summary.rmiName, summary.rmiID, summary.elapsedTime);
        }

        public void BeforeRmiInvocation(Nettention.Proud.BeforeRmiSummary summary)
        {
            Console.WriteLine("BeforeRmiSummary : hostid : {0}, RmiName : {1} Rmiid : {2}",
                summary.hostID, summary.rmiName, summary.rmiID);
        }

    }

    public class ServerEventSink
    {
        private NetServer _server = null;
        public HostID _sendUserMsgDestID = HostID.HostID_None;

        public ServerEventSink(NetServer server)
        {
            _server = server;

            // handler 추가
            _server.ClientJoinHandler = OnClientJoin;
            _server.ClientLeaveHandler = OnClientLeave;
            _server.ConnectionRequestHandler = OnConnectionRequest;
            _server.ExceptionHandler = OnException;
            _server.ClientOnlineHandler = OnClientOnline;
            _server.ClientOfflineHandler = OnClientOffline;
            _server.ReceiveUserMessageHandler = OnReceiveUserMessage;
        }
        public void OnClientJoin(Nettention.Proud.NetClientInfo clientInfo)
        {
            if (_sendUserMsgDestID == HostID.HostID_None)
            {
                _sendUserMsgDestID = clientInfo.hostID;

            }
            Console.WriteLine("Client {0} is connected.\n", clientInfo.hostID);
        }

        public void OnClientLeave(Nettention.Proud.NetClientInfo clientInfo, Nettention.Proud.ErrorInfo errorInfo, ByteArray comment)
        {
            if (_sendUserMsgDestID == clientInfo.hostID)
            {
                _sendUserMsgDestID = HostID.HostID_None;
            }
            Console.WriteLine("Client {0} is disconnected.\n", clientInfo.hostID);
        }

        public bool OnConnectionRequest(Nettention.Proud.AddrPort clientAddr, ByteArray userDataFromClient, ByteArray reply)
        {
            reply = new ByteArray();
            reply.Clear();
            return true;
        }

        public void OnException(System.Exception e)
        {
            Console.WriteLine("Exception!\n" + e.ToString() + "\n");
        }

        public void OnClientOnline(Nettention.Proud.RemoteOnlineEventArgs args)
        {

        }

        public void OnClientOffline(Nettention.Proud.RemoteOfflineEventArgs args)
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
#endif

    class Program
    {
        [DllImport("msvcrt")]
        static extern int _getch();

        internal static void StartServer(NetServer server, Nettention.Proud.StartServerParameter param)
        {
            if ((server == null) || (param == null))
            {
                throw new NullReferenceException();
            }

            try
            {
                server.Start(param);
            }
            catch (System.Exception ex)
            {
                Console.WriteLine("Failed to start server~!!" + ex.ToString());
            }

            Console.WriteLine("Succeed to start server~!!\n");
        }

        static void Main(string[] args)
        {
            String dump = Directory.GetCurrentDirectory() + @"\" + System.DateTime.Now.ToString("yyyy-MM-dd-hh-mm-ss") + ".DMP";

            switch (MiniDumper.StartUp(dump, MinidumpType.MiniDumpNormal))
            {
                case MiniDumpAction.MiniDumpAction_AlarmCrash:
                    {
                    }return;
                case MiniDumpAction.MiniDumpAction_DoNothing:
                    {

                    }return;
                default:
                    break;
            }

            NetServer server = new NetServer();
            ThreadPool netWorkerThreadPool = new ThreadPool(8);
            ThreadPool userWorkerThreadPool = new ThreadPool(8);

            S2C.Proxy S2Cproxy = new S2C.Proxy();
            C2SStub C2Sstub = new C2SStub(S2Cproxy);
			HostID groupHostID = HostID.HostID_None;

            ServerEventSink eventSink = new ServerEventSink(server);

            server.AttachStub(C2Sstub.stub);
            server.AttachProxy(S2Cproxy);
            server.SetDirectP2PStartCondition(DirectP2PStartCondition.DirectP2PStartCondition_Always);

            SimpleCSharp.Vars vars = new SimpleCSharp.Vars();

            Nettention.Proud.StartServerParameter param = new Nettention.Proud.StartServerParameter();

            param.protocolVersion = new Nettention.Proud.Guid(Vars.m_Version);
            param.tcpPorts = new Nettention.Proud.IntArray();
            param.tcpPorts.Add(Vars.m_serverPort);
            param.enableP2PEncryptedMessaging = true;
            param.fastEncryptedMessageKeyLength = Nettention.Proud.FastEncryptLevel.FastEncryptLevel_Middle;
            param.encryptedMessageKeyLength = Nettention.Proud.EncryptLevel.EncryptLevel_Middle;
            param.localNicAddr = "127.0.0.1";

            param.SetExternalNetWorkerThreadPool(netWorkerThreadPool);
            param.SetExternalUserWorkerThreadPool(userWorkerThreadPool);

            server.SetDefaultTimeoutTimeMs(1000 * 60 * 60);
            server.SetMessageMaxLength(100000, 100000);

            StartServer(server, param);



#if true
            while (true)
            {
                if (Console.KeyAvailable)
                {
                    ConsoleKeyInfo keyinfo = Console.ReadKey();

                    if (keyinfo.Key == ConsoleKey.Escape)
                        break;
                    else if (keyinfo.Key == ConsoleKey.D1)
                    {
                        // 1 키를 눌렀을때 서버에 연결된 모든 클라이언트를 P2P 그룹으로 묶이게한다.
                        // When you press "1" key, bind all client who connected with server to P2P group.
                        Nettention.Proud.HostID[] list = server.GetClientHostIDs();
                        groupHostID = server.CreateP2PGroup(list, new Nettention.Proud.ByteArray());

                        foreach (HostID hostid in list)
                        {
                            Nettention.Proud.NetClientInfo info = server.GetClientInfo(hostid);
                            if (info != null)
                            {
                                Console.WriteLine("Client HostID : {0}, IP:Port : {1}", info.hostID, info.tcpAddrFromServer.ToString());
                            }
                        }

                        if (groupHostID == Nettention.Proud.HostID.HostID_None)
                        {
                            Console.WriteLine("Failed to create group!!!\n");
                        }
                        else
                        {
                            Console.WriteLine("Succeed to create group!! GroupHostID : {0}\n", groupHostID);
                        }

                    }
                    else if (keyinfo.Key == ConsoleKey.D2)
                    {
                        S2Cproxy.SystemChat(groupHostID, Nettention.Proud.RmiContext.ReliableSend, "Hello~~!!!");
                    }
                    else if (keyinfo.Key == ConsoleKey.D3)
                    {
                        server.DestroyP2PGroup(groupHostID);
                        groupHostID = HostID.HostID_None;
                    }
                    else if (keyinfo.Key == ConsoleKey.D4)
                    {
                        server.Stop();
                    }
                    else if (keyinfo.Key == ConsoleKey.D5)
                    {
                        StartServer(server, param);
                    }
                    else if (keyinfo.Key == ConsoleKey.D6)
                    {
                        ByteArray payload = new ByteArray();
                        payload.Add((byte)'j');
                        payload.Add((byte)'j');
                        payload.Add((byte)'j');
                        payload.Add((byte)'j');
                        payload.Add((byte)'j');

                        HostID[] idArray = new HostID[1];
                        idArray[0] = eventSink._sendUserMsgDestID;

                        server.SendUserMessage(idArray, RmiContext.ReliableSend, payload);
                    }
                    else if (keyinfo.Key == ConsoleKey.D7)
                    {
                        List<IPEndPoint> endPoints = server.GetTcpListenerLocalAddrs();
                        foreach (IPEndPoint v in endPoints)
                        {
                            Console.WriteLine("\n" + v.Address.ToString() + " " + v.Port.ToString());
                        }

                    }
                    else if (keyinfo.Key == ConsoleKey.D8)
                    {


                        ByteArray buffer = new ByteArray();
                        for (int i = 0; i < 94000; ++i)
                        {
                            buffer.Add((byte)'a');
                        }


                        S2Cproxy.SendByteArray(eventSink._sendUserMsgDestID, Nettention.Proud.RmiContext.ReliableSend, buffer);
                    }
                    else if (keyinfo.Key == ConsoleKey.D9)
                    {
                        var emptyGroupHostID = server.CreateP2PGroup(new HostID[] { }, new Nettention.Proud.ByteArray());

                        if (HostID.HostID_None == emptyGroupHostID)
                        {
                            Console.WriteLine("Create an empty P2P group failed.");
                        }
                        else
                        {
                            Console.WriteLine("Create an empty P2P group succeeded.");
                        }
                    }
                }
                System.Threading.Thread.Sleep(10);
            }

            server.Dispose();
#endif
            //_getch();
        }
    }
}
