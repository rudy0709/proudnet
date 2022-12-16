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

// C++ native 객체와 C# 객체간 변환

using System;
using System.Runtime.InteropServices;

namespace Nettention.Proud
{
    internal class ConvertToNative
    {
        // C# 객체를 new C++ 객체로 변환하여 리턴한다.
        // caller는, Nettention.Proud.ProudNetClientPlugin.NativeToGuid_Delete를 통해 명시적으로 delete를 해주어야 한다.

        internal static unsafe System.IntPtr ByteArrayToNative(ByteArray arrayData)
        {
            if (arrayData.Count <= 0)
            {
                return System.IntPtr.Zero;
            }

            IntPtr nativeGuidPtr = IntPtr.Zero;

            fixed (byte* ret = arrayData.data)
            {
                nativeGuidPtr = Nettention.Proud.ProudNetClientPlugin.ByteArrayToNative(new IntPtr((void*)ret), arrayData.data.Length);
            }

            return nativeGuidPtr;
        }

        internal static unsafe void CopyByteArrayToNative(ByteArray arrayData, System.IntPtr nativeData)
        {
            if (arrayData.Count <= 0)
            {
                return;
            }

            fixed (byte* ret = arrayData.data)
            {
                Nettention.Proud.ProudNetClientPlugin.CopyManagedByteArrayToNativeByteArray(new IntPtr((void*)ret), arrayData.Count, nativeData);
            }
        }

        internal static NativeRmiContext RmiContextToNative(RmiContext rmiContext)
        {
            NativeRmiContext native = new NativeRmiContext();

            native.compressMode = rmiContext.compressMode;
            native.encryptMode = rmiContext.encryptMode;
            native.maxDirectP2PMulticastCount = rmiContext.maxDirectP2PMulticastCount;
            native.relayed = rmiContext.relayed;
            native.reliability = rmiContext.reliability;
            native.sentFrom = rmiContext.sentFrom;
            native.uniqueID = rmiContext.uniqueID;
            native.enableLoopback = rmiContext.enableLoopback;
            native.allowRelaySend = rmiContext.allowRelaySend;
            native.enableP2PJitTrigger = rmiContext.enableP2PJitTrigger;
            native.forceRelayThresholdRatio = rmiContext.forceRelayThresholdRatio;
            native.priority = rmiContext.priority;

            if (rmiContext.hostTag != null)
            {
                Nettention.Proud.ProudNetClientPlugin.RmiContext_SetHostTag(native, (long)rmiContext.hostTag);
            }

            return native;
        }
    }

    internal class ConvertToCSharp
    {
        internal static Nettention.Proud.ErrorInfo NativeErrorInfoToCSharp(System.IntPtr native)
        {
            if (native == null)
            {
                return null;
            }

            Nettention.Proud.ErrorInfo errorInfo = new Nettention.Proud.ErrorInfo();
            errorInfo.CopyFromNative(native);
            return errorInfo;
        }

        // RMI stub에서 이 함수가 사용되어, C#의 ProcessReceivedMessage를 처리하는데 사용된다.
        internal static unsafe ByteArray NativeByteArrayToCSharp(System.IntPtr nativeByteArray)
        {
            if (nativeByteArray == null)
            {
                return null;
            }

            int count = Nettention.Proud.ProudNetClientPlugin.ByteArray_GetCount(nativeByteArray);

            if (count <= 0)
            {
                return null;
            }

            ByteArray byteArray = new ByteArray();
            byteArray.SetCount(count);

            fixed (byte* ret = byteArray.data)
            {
                Nettention.Proud.ProudNetClientPlugin.CopyNativeByteArrayToManageByteArray(new IntPtr((void*)ret), nativeByteArray);
            }

            return byteArray;
        }

        internal static unsafe ByteArray NativeByteArrayToCSharp(System.IntPtr navieByte, int length)
        {
            if (length <= 0 || navieByte == null)
            {
                return null;
            }

            ByteArray byteArray = new ByteArray();
            byteArray.SetCount(length);

            fixed (byte* ret = byteArray.data)
            {
                Nettention.Proud.ProudNetClientPlugin.CopyNativeByteArrayToManageByteArray(new IntPtr((void*)ret), navieByte, length);
            }

            return byteArray;
        }

        internal static RmiContext NativeRmiContextToCSharp(System.IntPtr native)
        {
            RmiContext context = new RmiContext();

            // pa의경우 native(c++)에서 넘어온 주소이기 때문에 단순히 참조할 뿐이지, GC에 의해서 메모리가 삭제되지 않도록 합니다.
            NativeRmiContext nativeRmiContext = new NativeRmiContext(native, false);

            context.relayed = nativeRmiContext.relayed;
            context.sentFrom = nativeRmiContext.sentFrom;
            context.maxDirectP2PMulticastCount = nativeRmiContext.maxDirectP2PMulticastCount;
            context.priority = nativeRmiContext.priority;
            context.reliability = nativeRmiContext.reliability;
            context.encryptMode = nativeRmiContext.encryptMode;
            context.compressMode = nativeRmiContext.compressMode;
            context.uniqueID = nativeRmiContext.uniqueID;

            context.hostTag = (object)Nettention.Proud.ProudNetClientPlugin.RmiContext_GetHostTag(native);
            return context;
        }

        internal static ReceivedMessage NativeReceivedMessageToCSharp(System.IntPtr pa)
        {
            ReceivedMessage msg = new ReceivedMessage();

            // pa의경우 native(c++)에서 넘어온 주소이기 때문에 단순히 참조할 뿐이지, GC에 의해서 메모리가 삭제되지 않도록 합니다.
            NativeReceivedMessage native = new NativeReceivedMessage(pa, false);

            msg.unsafeMessage = new Message(ConvertToCSharp.NativeByteArrayToCSharp(native.GetMsgBuffer(), native.GetMsgBufferLength()));
            msg.unsafeMessage.ReadOffset = native.GetMsgReadOffset();

            msg.compressMode = native.compressMode;
            msg.encryptMode = native.encryptMode;
            msg.remoteHostID = native.remoteHostID;
            msg.relayed = native.relayed;

            msg.remoteAddr_onlyUdp = new System.Net.IPEndPoint(
                native.remoteAddr_onlyUdp.GetIPAddr(),
                native.remoteAddr_onlyUdp.port
                );

            return msg;
        }

        internal static unsafe System.Net.IPAddress ConvertNativeIPAddrToManagedIPAddr(System.IntPtr pa)
        {
            System.IntPtr nativeAddrBuffer = Nettention.Proud.ProudNetClientPlugin.ReceivedMessage_GetRemoteAddress(pa);
            int nativeAddrBufferSize = Nettention.Proud.ProudNetClientPlugin.AddrPort_GetAddrSize();
            byte[] managedAddrBuffer = new byte[nativeAddrBufferSize];

            fixed (byte* ret = managedAddrBuffer)
            {
                Nettention.Proud.ProudNetClientPlugin.CopyNativeAddrToManagedAddr(new IntPtr((void*)ret), nativeAddrBuffer, nativeAddrBufferSize);
            }

            return new System.Net.IPAddress(managedAddrBuffer);
        }


        internal static Nettention.Proud.RemoteOfflineEventArgs NativeRmoteOfflineEventArgsToCSharp(System.IntPtr native)
        {
            Nettention.Proud.RemoteOfflineEventArgs args = new Nettention.Proud.RemoteOfflineEventArgs();
            args.CopyFromNative(native);
            return args;
        }

        internal static Nettention.Proud.RemoteOnlineEventArgs NativeRmoteOnlineEventArgsToCSharp(System.IntPtr native)
        {
            Nettention.Proud.RemoteOnlineEventArgs args = new Nettention.Proud.RemoteOnlineEventArgs();
            args.CopyFromNative(native);
            return args;
        }

        internal static string NativeStringToCSharp(System.IntPtr native)
        {
            return Nettention.Proud.ProudNetClientPlugin.ConvertNatvieStringToManagedString(native);
        }
    }
}