using System;
using System.Collections.Generic;
using System.Text;

namespace Nettention.Proud
{
    public class Misc
    {
        private const int internalVersion = 0x00030218;
        public static bool enableLog = false;
        public static int GetInternalVersion()
        {
            return internalVersion;
        }
        public static string Base64Encode(byte[] arr)
        {
            return Convert.ToBase64String(arr);
        }

        public static byte[] Base64Decode(string str)
        {
            return Convert.FromBase64String(str);
        }

        public static void WriteLog(string content, string caller)
        {
            if(enableLog)
                Console.WriteLine(DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss.fff") + " [C#." + caller + "] - " + content);
        }
    }
    /*
    public class PreciseCurrentTime
    {
        private static object timeCritSec = new object();
        static long baseTime = 0;

        public static long GetTimeMs()
        {
            lock (timeCritSec)
            {
                long t = DateTime.Now.Ticks;
                if (baseTime == 0)
                {
                    baseTime = t;
                }
                return (t - baseTime) / 10000;
            }
        }
    }
    */

    public class Guid : ICloneable
    {
        System.Guid m_guid;

        public Guid() { }

        public Guid(Nettention.Proud.Guid guid)
        {
            Set(guid);
        }

        public Guid(System.Guid guid)
        {
            Set(guid);
        }

        public void Set(Nettention.Proud.Guid guid)
        {
            m_guid = new System.Guid(guid.m_guid.ToString());
        }

        public void Set(System.Guid guid)
        {
            m_guid = new System.Guid(guid.ToString());
        }

        public byte[] ToByteArray()
        {
            return m_guid.ToByteArray();
        }

        public object Clone()
        {
            Guid guid = new Guid(this.m_guid);

            return guid;
        }
    }

    public class NetConnectionParam : ICloneable
    {
        public Nettention.Proud.Guid protocolVersion;
        public ByteArray userData;

        public string serverIP;
        public ushort serverPort;

        public object Clone()
        {
            NetConnectionParam param = new NetConnectionParam();
            param.protocolVersion = (Guid)this.protocolVersion.Clone();
            param.userData = this.userData.Clone();
            param.serverIP = this.serverIP;
            param.serverPort = this.serverPort;

            return param;
        }
    }
    /*
    public enum RmiID
    {
        RmiID_None = 0,
        RmiID_Last = 65535
    }

    public enum HostID
    {
        HostID_None = 0,
        HostID_Server = 1,
        HostID_Last = 2
    }

    public enum EncryptMode
    {
        EM_None,
        EM_Secure,
        EM_Fast,
        EM_LAST
    }

    public enum CompressMode
    {
        CM_None,
        CM_Zip
    }

    public enum FallbackMethod
    {
        FallbackMethod_None,
        FallbackMethod_PeersUdpToTcp,
        FallbackMethod_ServerUdpToTcp,
        FallbackMethod_CloseUdpSocket
    }

    public enum MessageReliability
    {
        MessageReliability_Unreliable = 0,
        MessageReliability_Reliable,
        MessageReliability_LAST
    }

    public enum MessagePriority
    {
        MessagePriority_Ring0 = 0,
        MessagePriority_Ring1,
        MessagePriority_High,
        MessagePriority_Medium,
        MessagePriority_Low,
        MessagePriority_Ring99,
        MessagePriority_LAST
    }

    public enum ErrorType
    {
        ErrorType_Ok = 0,
        ErrorType_Unexpected,
        ErrorType_AlreadyConnected,
        ErrorType_TCPConnectFailure,
        ErrorType_InvalidSessionKey,
        ErrorType_EncryptFail,
        ErrorType_DecryptFail,
        ErrorType_ConnectServerTimeout,
        ErrorType_ProtocolVersionMismatch,
        ErrorType_InvalidLicense,
        ErrorType_NotifyServerDeniedConnection,
        ErrorType_ConnectServerSuccessful,
        ErrorType_DisconnectFromRemote,
        ErrorType_DisconnectFromLocal,
        ErrorType_DangerousArgumentWarning,
        ErrorType_UnknownAddrPort,
        ErrorType_ServerNotReady,
        ErrorType_ServerPortListenFailure,
        ErrorType_AlreadyExists,
        ErrorType_PermissionDenied,
        ErrorType_BadSessionGuid,
        ErrorType_InvalidCredential,
        ErrorType_InvalidHeroName,
        ErrorType_LoadDataPreceded,
        ErrorType_AdjustedGamerIDNotFilled,
        ErrorType_NoHero,
        ErrorType_UnitTestFailed,
        ErrorType_P2PUdpFailed,
        ErrorType_ReliableUdpFailed,
        ErrorType_ServerUdpFailed,
        ErrorType_NoP2PGroupRelation,
        ErrorType_ExceptionFromUserFunction,
        ErrorType_UserRequested,
        ErrorType_InvalidPacketFormat,
        ErrorType_TooLargeMessageDetected,
        ErrorType_CannotEncryptUnreliableMessage,
        ErrorType_ValueNotExist,
        ErrorType_TimeOut,
        ErrorType_LoadedDataNotFound,
        ErrorType_SendQueueIsHeavy,
        ErrorType_TooSlowHeartbeatWarning,
        ErrorType_CompressFail,
        ErrorType_LocalSocketCreationFailed,
        Error_NoneAvailableInPortPool,
        ErrorType_InvalidPortPool,
        ErrorType_InvalidHostID,
        ErrorType_MessageOverload,
        ErrorType_DatabaseAccessFailed,
        ErrorType_OutOfMemory,
        ErrorType_AutoConnectionRecoveryFailed
    }
    */
    public enum CallbackType
    {
        JoinServerComplete = 0,
        LeaveServer,
        SynchronizeServerTime,
        NoRmiProcessed,
        ReceiveUserMessage,
        Rmi,
        HostIDIssued, // 유저 콜백으로 이어지지는 않고, NetClient의 local HostID를 갱신하는데 쓰인다.
        CallbackType_END
    }
    /*
    // WebGL에서는 암호화와 압축을 하지 않습니다.
    // WebSocket으로 보내기 때문에 서버로 보낼때 Reliability와 Priority에 값을 설정해도 항상 Reliable로 가게 됩니다.
    public class RmiContext
    {
        public bool relayed = false;
        public HostID sentFrom = HostID.HostID_None;

        public int maxDirectP2PMulticastCount = 0;

        public Int64 uniqueID = 0;
        public MessagePriority priority = MessagePriority.MessagePriority_Medium;

        public MessageReliability reliability = MessageReliability.MessageReliability_Reliable;

        public bool enableLoopback = true;
        public Object hostTag = null;
        public bool enableP2PJitTrigger = true;
        public bool allowRelaySend = true;
        public double forceRelayThresholdRatio = 0;
        public EncryptMode encryptMode = EncryptMode.EM_None;
        public CompressMode compressMode = CompressMode.CM_None;

        public RmiContext()
        {

        }

        public RmiContext(MessagePriority priority, MessageReliability reliability, EncryptMode encryptMode)
        {
            this.priority = priority;
            this.reliability = reliability;
            this.encryptMode = encryptMode;

            this.maxDirectP2PMulticastCount = int.MaxValue;
        }

        public static readonly RmiContext ReliableSend = new RmiContext(MessagePriority.MessagePriority_High, MessageReliability.MessageReliability_Reliable, EncryptMode.EM_None);
        public static readonly RmiContext UnreliableSend = new RmiContext(MessagePriority.MessagePriority_Medium, MessageReliability.MessageReliability_Unreliable, EncryptMode.EM_None);

        public static readonly RmiContext FastEncryptedReliableSend = new RmiContext(MessagePriority.MessagePriority_High, MessageReliability.MessageReliability_Reliable, EncryptMode.EM_Fast);
        public static readonly RmiContext FastEncryptedUnreliableSend = new RmiContext(MessagePriority.MessagePriority_Medium, MessageReliability.MessageReliability_Unreliable, EncryptMode.EM_Fast);

        public static readonly RmiContext SecureReliableSend = new RmiContext(MessagePriority.MessagePriority_High, MessageReliability.MessageReliability_Reliable, EncryptMode.EM_Secure);
        public static readonly RmiContext SecureUnreliableSend = new RmiContext(MessagePriority.MessagePriority_Medium, MessageReliability.MessageReliability_Unreliable, EncryptMode.EM_Secure);
    }
    */
    public struct ErrorInfo
    {
        public ErrorType errorType;
        public ErrorType detailType;
        public HostID remote;
        public string comment;
        public string source;

        public override string ToString()
        {
            return "errorType[" + errorType + "], detailType[" + detailType + "], remote[" + remote + "], comment[" + comment + "], source[" + source + "]";
        }
    }
    public class FrameMoveResult
    {
        public uint processedMessageCount = 0;  // UserMessage, RMI 호출 횟수
        public uint processedEventCount = 0;    // NetClientEvent 호출 횟수

        public override string ToString()
        {
            return "processedMessageCount[" + processedMessageCount + "], processedEventcount[" + processedEventCount + "]";
        }
    }

    public struct CallbackEvent
    {
        public int clientInstanceID;

        public CallbackType type;

        public ErrorInfo errorInfo;

        public ByteArray replyFromServer;

        public RmiID rmiID;

        public HostID sender;
        public RmiContext rmiContext;
        public ByteArray byteArr;

        public override string ToString()
        {
            string message = "";
            switch(type)
            {
                case CallbackType.JoinServerComplete:
                    message = "JoinServerComplete: ErrorInfo[" + errorInfo + "], replyFromServerLength[" + replyFromServer.Count + "]";
                    break;
                case CallbackType.LeaveServer:
                    message = "LeaveServer: ErrorInfo[" + errorInfo + "]";
                    break;
                case CallbackType.SynchronizeServerTime:
                    message = "SynchronizeServerTime.";
                    break;
                case CallbackType.NoRmiProcessed:
                    message = "NoRmiProcessed: rmiID[" + rmiID + "]";
                    break;
                case CallbackType.ReceiveUserMessage:
                    message = "ReceiveUserMessage: sender[" + sender + "], userMessageLength[" + byteArr.Count + "]";
                    break;
                case CallbackType.Rmi:
                    message = "Rmi: sender[" + sender + "], userMessageLength[" + byteArr.Count + "]";
                    break;
                default:
                    message = "Unknown callback type[" + type + "]";
                    break;
            }

            return message;
        }
    }

    public class ServerConnectionState
    {
        public bool realUdpEnabled;

        public ServerConnectionState()
        {
            realUdpEnabled = false;
        }
    }
}
