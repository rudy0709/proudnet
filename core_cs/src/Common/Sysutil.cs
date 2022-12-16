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

*/
using System;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;
using System.Net.Sockets;
using System.Net;

namespace Nettention.Proud
{
    internal class Sysutil
    {
        public static readonly double PROUDNET_PI = 3.14159265358979323846;

        public static double Lerp(double v1, double v2, double ratio)
        {
            return v1 + (v2 - v1) * ratio;
        }

        public static long LerpInt(long v1, long v2, long ratio1, long ratio2)
        {
            return v1 + ((v2 - v1) * ratio1) / ratio2;
        }

#if false
        public static void ShowUserMisuseError(String text)
        {
            if (NetConfig.UserMisuseErrorReaction == ErrorReaction.Assert)
            {
                //Debug.WriteLine(text);
                Debug.Assert(false);

                // 대화 상자를 표시한다.
                //MessageBox.Show(text);
            }
            //else if(NetConfig.UserMisuseErrorReaction == ErrorReaction.DebugOutput)
            //{
            //    // DebugOutput으로 출력한다.
            //    //Debug.WriteLine(text);
            //}
            else
            {
                //Debug.WriteLine(text);

                // 크래시를 유발한다.
                Object a = null;

                int b = (int)a;

                b = 50;
            }
        }


        public static void Swap<T>(ref T lhs, ref T rhs)
        {
            T temp;
            temp = lhs;
            lhs = rhs;
            rhs = temp;
        }

        public static bool IsCombinationEmpty<T>(T min1, T max1, T min2, T max2) where T : IComparable<T>
        {
            if (min1.CompareTo(max1) > 0)
                Swap<T>(ref min1, ref max1);
            if (min2.CompareTo(max2) > 0)
                Swap<T>(ref min2, ref max2);

            if (min2.CompareTo(min1) <= 0 && max1.CompareTo(max2) <= 0)
                return false;
            if (min1.CompareTo(min2) <= 0 && max2.CompareTo(max1) <= 0)
                return false;
            if (min1.CompareTo(min2) <= 0 && min2.CompareTo(max1) <= 0 && max1.CompareTo(max2) <= 0)
                return false;
            if (min2.CompareTo(min1) <= 0 && min1.CompareTo(max2) <= 0 && max2.CompareTo(max1) <= 0)
                return false;

            return true;
        }

        public static void CauseAccessViolation()
        {
            /* C++에서는 명시적으로 덤프 파일을 만드려고 AV를 일으키지만
            C#에서는 아래 코드가 obfuscator+IL2CPP에서 컴파일 자체를 못하게 한다.
            
            int[] a = null; // 크래시 유발
            a[0] = 1;
            
            이 코드를 IL2Spy로 열면 이렇게 생성되니 당연히 컴파일이 안됨.
            if (this.b != null && (this.b.b || this.b.c))
            {
                null[0] = 1;
            }

            어차피 C#에서는 널참조하면 throw를 하므로 */
            throw new NullReferenceException("CauseAccessViolation is called.");
        }

        //internal static byte[] ObjectToByteArray(object _Object)
        //{
        //    try
        //    {
        //        System.IO.MemoryStream _MemoryStream = new System.IO.MemoryStream();

        //        System.Runtime.Serialization.Formatters.Binary.BinaryFormatter _BinaryFormatter
        //                    = new System.Runtime.Serialization.Formatters.Binary.BinaryFormatter();

        //        _BinaryFormatter.Serialize(_MemoryStream, _Object);

        //        return _MemoryStream.ToArray();
        //    }
        //    catch (Exception _Exception)
        //    {
        //        Debug.WriteLine("Exception caught in process: {0}", _Exception.ToString());
        //    }

        //    return null;
        //} 


        /*
                internal unsafe static void UnsafeMemCopy(Byte* src, Byte* dst, int length)
                {
                    Byte* ps = src;
                    Byte* pd = dst;

                    int pad = length / 4;
                    int blsize = length % 4;

                    for (int i = 0; i < pad; ++i)
                    {
                        *((int*)pd) = *((int*)ps);
                        pd += 4;
                        ps += 4;
                    }

                    for (int i = 0; i < blsize; i++)
                    {
                        *pd = *ps;
                        pd++;
                        ps++;
                    }
                }
        unsafe code 퇴역*/
        internal static List<T> UnionDuplicates<T>(List<T> obj, IComparer<T> comparer)
        {
            obj.Sort(comparer);

            // 중복 항목을 옮기기
            List<T> obj2 = new List<T>();

            int c = 0;
            int objSize = obj.Count;

            for (int i = 0; i < objSize; i++)
            {
                if (obj[i] != null)
                {
                    if (c == 0 || comparer.Compare(obj[i], obj2[c - 1]) != 0)
                    {
                        obj2.Add(obj[i]);
                        c++;
                    }
                }
            }

            return obj2;
        }
    }

    internal class NetUtil
    {
        //public static IPEndPoint OneLocalAdress = GetOneLocalAdress();

        public static IPEndPoint UnassignedIPEndPoint = MakeUnassignedIPEndPoint;

        public static IPEndPoint MakeUnassignedIPEndPoint
        {
            get { return new IPEndPoint(0xffffffff, 0xffff); }
        }

        //public static IPEndPoint GetOneLocalAdress()
        //{
        //    IPHostEntry entry = Dns.GetHostEntry(Dns.GetHostName());

        //    foreach (IPAddress addr in entry.AddressList)
        //    {
        //        return new IPEndPoint(addr,0);
        //    }

        //    return new IPEndPoint(0,0);
        //}

        public static bool IsUnicastEndpoint(IPEndPoint addrPort)
        {
            if (addrPort.Port == 0 /*|| (ushort)addrPort.Port == 0xffff*/)
                return false;

            uint binaryAdress = BitConverter.ToUInt32(addrPort.Address.GetAddressBytes(), 0);

            if (binaryAdress == 0 || binaryAdress == 0xffffffff)
                return false;

            return true;
        }

        public static bool IsSameHost(IPEndPoint a, IPEndPoint b)
        {
            return (a.Address.Equals(b.Address));
        }

        public static bool IsSameLan(IPEndPoint a, IPEndPoint b)
        {
            Byte[] aBytes = a.Address.GetAddressBytes();
            Byte[] bBytes = b.Address.GetAddressBytes();

            return (aBytes[0] == bBytes[0] &&
                aBytes[1] == bBytes[1] &&
                aBytes[2] == bBytes[2]);
        }

        static void SetSocketSendAndRecvBufferLength(Socket socket, int sendBufferLength, int recvBufferLength)
        {
            socket.SendBufferSize = sendBufferLength;
            socket.ReceiveBufferSize = recvBufferLength;
        }

        public static void SetTcpDefaultBehavior_Client(Socket socket)
        {
            SetSocketSendAndRecvBufferLength(socket,
                NetConfig.TcpSendBufferLength,
                NetConfig.TcpRecvBufferLength);

            if (NetConfig.EnableSocketTcpKeepAliveOption)
            {
                socket.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.KeepAlive, true);
            }
        }

        public static void SetUdpDefaultBehavior_Client(bool weborpc, Socket socket)
        {
			// 이게 있어야 제대로 동작 ..
			if (!weborpc)
				socket.Blocking = false;

			// 수정전에는 weborpc 값에 따라 size를 다르게 셋팅하여 주었었는데 그렇게 하지 않아도 문제가 없는지..
            SetSocketSendAndRecvBufferLength(socket,
                NetConfig.UdpSendBufferLength_Client,
                NetConfig.UdpRecvBufferLength_Client);
        }

        // 0-byte send를 테스트해서 TCP 연결 상태를 확인한다.
        // Connected,Connecting,Disconnected 중 하나를 리턴한다.
        // Connecting인 경우: nonblock connect를 호출했지만 아직 연결 상태가 완료되기 전인 경우 혹은 아예 connect 자체를 호출 안한 경우.
        public static ConnectionState TestConnectionByZeroByteSend(Socket s, out SocketError code)
        {
            try
            {
                byte[] tmp = new byte[1];

                s.Send(tmp, 0, 0);
                code = SocketError.Success;
            }
            catch (SocketException e)
            {
                code = e.SocketErrorCode;
            }

            switch (code)
            {
                case SocketError.Success:
                case SocketError.WouldBlock:
                    return ConnectionState.Connected;
                case SocketError.NotConnected: // 아직 서버와의 연결 진행중이면 이 값이 리턴된다.
                    return ConnectionState.Connecting;
                default:
                    return ConnectionState.Disconnected;
            }
        }
#endif
        public static void ThrowInvalidArgumentException()
        {
            throw new Exception("An Invalid argument is detected!");
        }

        public static void ThrowArrayOutOfBoundException()
        {
            throw new Exception("Array index out of range!");
        }
    }
}
