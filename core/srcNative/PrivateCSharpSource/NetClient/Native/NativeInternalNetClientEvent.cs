/*

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
using System.Diagnostics;
using System.Text;
using System.Runtime.InteropServices;

namespace Nettention.Proud
{
    // Swig에서 제공하는 C++ 가상 클래스를 C# 클래스에서 상속받아서 사용하는 기능을 사용하지 못하는 이유는 Unity IL2CPP에서 C++ 코드를 생성할 때 C++ -> C# 콜백에 대한 코드에 문제가 있기 때문에
    // 각 이벤트에 대해서 콜백 함수를 만들어서 처리합니다.
    internal class NativeInternalNetClientEvent : GCHandleList
    {
        private NetClient m_netclient = null;
        private System.IntPtr m_netEventWrap = System.IntPtr.Zero;

        private bool disposed = false;

        internal NativeInternalNetClientEvent(NetClient netclient)
        {
            m_netclient = netclient;

            GCHandle handle = GCHandle.Alloc(this);

            m_netEventWrap = Nettention.Proud.ProudNetClientPlugin.NativeToNetClientEventWrap_New();

            // C# -> C++로 넘겨진 Delegate도 가비지 수집이 되기 때문에 GCHandle.Alloc을 해주어야 합니다.
            Nettention.Proud.ProudNetClientPlugin.NetClientEvent_SetCSharpHandle(m_netEventWrap, GCHandle.ToIntPtr(handle));
            base.AddHandle(handle);

            ProudDelegate.Delegate_8 del1 = OnJoinServerComplete;
            handle = GCHandle.Alloc(del1);
            Nettention.Proud.ProudNetClientPlugin.NetClientEvent_SetCallbackJoinServerComplete(m_netEventWrap, del1);
            base.AddHandle(handle);

            ProudDelegate.Delegate_0 del2 = OnLeaveServer;
            handle = GCHandle.Alloc(del2);
            Nettention.Proud.ProudNetClientPlugin.NetClientEvent_SetCallbackLeaveServer(m_netEventWrap, del2);
            base.AddHandle(handle);

            ProudDelegate.Delegate_9 del3 = OnP2PMemberJoin;
            handle = GCHandle.Alloc(del3);
            Nettention.Proud.ProudNetClientPlugin.NetClientEvent_SetCallbackP2PMemberJoin(m_netEventWrap, del3);
            base.AddHandle(handle);

            ProudDelegate.Delegate_10 del4 = OnP2PMemberLeave;
            handle = GCHandle.Alloc(del4); ;
            Nettention.Proud.ProudNetClientPlugin.NetClientEvent_SetCallbackP2PMemberLeave(m_netEventWrap, del4);
            base.AddHandle(handle);

            ProudDelegate.Delegate_11 del5 = OnChangeP2PRelayState;
            handle = GCHandle.Alloc(del5);
            Nettention.Proud.ProudNetClientPlugin.NetClientEvent_SetCallbackChangeP2PRelayState(m_netEventWrap, del5);
            base.AddHandle(handle);

            ProudDelegate.Delegate_12 del6 = OnChangeServerUdpState;
            handle = GCHandle.Alloc(del6);
            Nettention.Proud.ProudNetClientPlugin.NetClientEvent_SetCallbackChangeServerUdpState(m_netEventWrap, del6);
            base.AddHandle(handle);

            ProudDelegate.Delegate_5 del7 = OnSynchronizeServerTime;
            handle = GCHandle.Alloc(del7);
            Nettention.Proud.ProudNetClientPlugin.NetClientEvent_SetCallbackSynchronizeServerTime(m_netEventWrap, del7);
            base.AddHandle(handle);

            ProudDelegate.Delegate_0 del8 = OnError;
            handle = GCHandle.Alloc(del8);
            Nettention.Proud.ProudNetClientPlugin.NetClientEvent_SetCallbackError(m_netEventWrap, del8);
            base.AddHandle(handle);

            ProudDelegate.Delegate_0 del9 = OnWarning;
            handle = GCHandle.Alloc(del9);
            Nettention.Proud.ProudNetClientPlugin.NetClientEvent_SetCallbackWarning(m_netEventWrap, del9);
            base.AddHandle(handle);

            ProudDelegate.Delegate_0 del10 = OnInformation;
            handle = GCHandle.Alloc(del10);
            Nettention.Proud.ProudNetClientPlugin.NetClientEvent_SetCallbackInformation(m_netEventWrap, del10);
            base.AddHandle(handle);

            ProudDelegate.Delegate_3 del11 = OnException;
            handle = GCHandle.Alloc(del11);
            Nettention.Proud.ProudNetClientPlugin.NetClientEvent_SetCallbackException(m_netEventWrap, del11);
            base.AddHandle(handle);

            ProudDelegate.Delegate_12 del12 = OnNoRmiProcessed;
            handle = GCHandle.Alloc(del12);
            Nettention.Proud.ProudNetClientPlugin.NetClientEvent_SetCallbackNoRmiProcessed(m_netEventWrap, del12);
            base.AddHandle(handle);

            ProudDelegate.Delegate_7 del13 = OnReceiveUserMessage;
            handle = GCHandle.Alloc(del13);
            Nettention.Proud.ProudNetClientPlugin.NetClientEvent_SetCallbackReceiveUserMessage(m_netEventWrap, del13);
            base.AddHandle(handle);

            Nettention.Proud.ProudDelegate.Delegate_0 del14 = OnServerOffline;
            handle = GCHandle.Alloc(del14);
            Nettention.Proud.ProudNetClientPlugin.NetClientEvent_SetCallbackServerOffline(m_netEventWrap, del14);
            base.AddHandle(handle);

            Nettention.Proud.ProudDelegate.Delegate_0 del15 = OnServerOnline;
            handle = GCHandle.Alloc(del15);
            Nettention.Proud.ProudNetClientPlugin.NetClientEvent_SetCallbackServerOnline(m_netEventWrap, del15);
            base.AddHandle(handle);

            Nettention.Proud.ProudDelegate.Delegate_0 del16 = OnP2PMemberOffline;
            handle = GCHandle.Alloc(del16);
            Nettention.Proud.ProudNetClientPlugin.NetClientEvent_SetCallbackP2PMemberOffline(m_netEventWrap, del16);
            base.AddHandle(handle);

            Nettention.Proud.ProudDelegate.Delegate_0 del17 = OnP2PMemberOnline;
            handle = GCHandle.Alloc(del17);
            Nettention.Proud.ProudNetClientPlugin.NetClientEvent_SetCallbackP2PMemberOnline(m_netEventWrap, del17);
            base.AddHandle(handle);
        }

        ~NativeInternalNetClientEvent()
        {
            Dispose(false);
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing)
        {
            if (disposed)
                return;

            if (disposing == true)
            {
                Nettention.Proud.ProudNetClientPlugin.NativeToNetClientEventWrap_Delete(m_netEventWrap);
                m_netEventWrap = IntPtr.Zero;

                base.FreeAllHandle();
            }

            disposed = true;
        }

        internal System.IntPtr GetNativeNetClientEvent()
        {
            return m_netEventWrap;
        }

        // natvie -> managed callback 되어 수행되는 로직에서 exception 발생시에 managed단에서 try~catch로 예외처리를 해주어야 합니다.
        private static void Internal_HandleException(NativeInternalNetClientEvent native, HostID hostID, System.Exception ex)
        {
            // exceptionHandler 등록되어 있지 않으면, 무시합니다. networker thread or user worker thread의 루프가 중단되어 버리는 문제를 피하기 위해서 입니다.
            if (native.m_netclient.exceptionHandler != null)
            {
                native.m_netclient.exceptionHandler(hostID, ex);
            }
        }

#if (UNITY_ENGINE)
    [AOT.MonoPInvokeCallback(typeof(ProudDelegate.Delegate_8))]
#endif
        internal static void OnJoinServerComplete(System.IntPtr obj, System.IntPtr info, System.IntPtr replyFromServer)
        {
            GCHandle gch = (GCHandle)obj;
            NativeInternalNetClientEvent native = (NativeInternalNetClientEvent)gch.Target;
            Nettention.Proud.ErrorInfo errorInfo = ConvertToCSharp.NativeErrorInfoToCSharp(info);

            if (native.m_netclient.IsExceptionEventAllowed())
            {
                try
                {
                    if (native.m_netclient.joinServerCompleteHandler != null)
                    {
                        native.m_netclient.joinServerCompleteHandler(errorInfo, ConvertToCSharp.NativeByteArrayToCSharp(replyFromServer));
                    }
                }
                catch (System.Exception ex)
                {
                    Internal_HandleException(native, ((errorInfo != null) ? errorInfo.remote : HostID.HostID_None), ex);
                }
            }
            else
            {
                if (native.m_netclient.joinServerCompleteHandler != null)
                {
                    native.m_netclient.joinServerCompleteHandler(errorInfo, ConvertToCSharp.NativeByteArrayToCSharp(replyFromServer));
                }
            }
        }

#if (UNITY_ENGINE)
    [AOT.MonoPInvokeCallback(typeof(ProudDelegate.Delegate_0))]
#endif
        internal static void OnLeaveServer(System.IntPtr obj, System.IntPtr errorinfo)
        {
            GCHandle gch = (GCHandle)obj;
            NativeInternalNetClientEvent native = (NativeInternalNetClientEvent)gch.Target;

            Nettention.Proud.ErrorInfo errorInfo = ConvertToCSharp.NativeErrorInfoToCSharp(errorinfo);;

            if (native.m_netclient.IsExceptionEventAllowed())
            {
                try
                {
                    if (native.m_netclient.leaveServerHandler != null)
                    {
                        native.m_netclient.leaveServerHandler(errorInfo);
                    }
                }
                catch (System.Exception ex)
                {
                    Internal_HandleException(native, ((errorInfo != null) ? errorInfo.remote : HostID.HostID_None), ex);
                }
            }
            else
            {
                if (native.m_netclient.leaveServerHandler != null)
                {
                    native.m_netclient.leaveServerHandler(errorInfo);
                }
            }
        }

#if (UNITY_ENGINE)
    [AOT.MonoPInvokeCallback(typeof(ProudDelegate.Delegate_9))]
#endif
        internal static void OnP2PMemberJoin(System.IntPtr obj, int memberHostID, int groupHostID, int memberCount, System.IntPtr message)
        {
            GCHandle gch = (GCHandle)obj;
            NativeInternalNetClientEvent native = (NativeInternalNetClientEvent)gch.Target;

            if (native.m_netclient.IsExceptionEventAllowed())
            {
                try
                {
                    if (native.m_netclient.p2pMemberJoinHandler != null)
                    {
                        native.m_netclient.p2pMemberJoinHandler((HostID)memberHostID, (HostID)groupHostID, memberCount, ConvertToCSharp.NativeByteArrayToCSharp(message));
                    }
                }
                catch (System.Exception ex)
                {
                    Internal_HandleException(native, (HostID)memberHostID, ex);
                }
            }
            else
            {
                if (native.m_netclient.p2pMemberJoinHandler != null)
                {
                    native.m_netclient.p2pMemberJoinHandler((HostID)memberHostID, (HostID)groupHostID, memberCount, ConvertToCSharp.NativeByteArrayToCSharp(message));
                }
            }
        }

#if (UNITY_ENGINE)
    [AOT.MonoPInvokeCallback(typeof(ProudDelegate.Delegate_10))]
#endif
        internal static void OnP2PMemberLeave(System.IntPtr obj, int memberHostID, int groupHostID, int memberCount)
        {
            GCHandle gch = (GCHandle)obj;
            NativeInternalNetClientEvent native = (NativeInternalNetClientEvent)gch.Target;

            if (native.m_netclient.IsExceptionEventAllowed())
            {
                try
                {
                    if (native.m_netclient.p2pMemberLeaveHandler != null)
                    {
                        native.m_netclient.p2pMemberLeaveHandler((HostID)memberHostID, (HostID)groupHostID, memberCount);
                    }
                }
                catch (System.Exception ex)
                {
                    Internal_HandleException(native, (HostID)memberHostID, ex);
                }
            }
            else
            {
                if (native.m_netclient.p2pMemberLeaveHandler != null)
                {
                    native.m_netclient.p2pMemberLeaveHandler((HostID)memberHostID, (HostID)groupHostID, memberCount);
                }
            }
        }

#if (UNITY_ENGINE)
    [AOT.MonoPInvokeCallback(typeof(ProudDelegate.Delegate_11))]
#endif
        internal static void OnChangeP2PRelayState(System.IntPtr obj, int remoteHostID, int reason)
        {
            GCHandle gch = (GCHandle)obj;
            NativeInternalNetClientEvent native = (NativeInternalNetClientEvent)gch.Target;

            if (native.m_netclient.IsExceptionEventAllowed())
            {
                try
                {
                    if (native.m_netclient.changeP2PRelayStateHandler != null)
                    {
                        native.m_netclient.changeP2PRelayStateHandler((HostID)remoteHostID, (ErrorType)reason);
                    }
                }
                catch (System.Exception ex)
                {
                    Internal_HandleException(native, (HostID)remoteHostID, ex);
                }
            }
            else
            {
                if (native.m_netclient.changeP2PRelayStateHandler != null)
                {
                    native.m_netclient.changeP2PRelayStateHandler((HostID)remoteHostID, (ErrorType)reason);
                }
            }
        }

#if (UNITY_ENGINE)
    [AOT.MonoPInvokeCallback(typeof(ProudDelegate.Delegate_12))]
#endif
        internal static void OnChangeServerUdpState(System.IntPtr obj, int reason)
        {
            GCHandle gch = (GCHandle)obj;
            NativeInternalNetClientEvent native = (NativeInternalNetClientEvent)gch.Target;

            if (native.m_netclient.IsExceptionEventAllowed())
            {
                try
                {
                    if (native.m_netclient.changeServerUdpStateHandler != null)
                    {
                        native.m_netclient.changeServerUdpStateHandler((ErrorType)reason);
                    }
                }
                catch (System.Exception ex)
                {
                    Internal_HandleException(native, native.m_netclient.GetLocalHostID(), ex);
                }
            }
            else
            {
                if (native.m_netclient.changeServerUdpStateHandler != null)
                {
                    native.m_netclient.changeServerUdpStateHandler((ErrorType)reason);
                }
            }
        }

#if (UNITY_ENGINE)
    [AOT.MonoPInvokeCallback(typeof(ProudDelegate.Delegate_5))]
#endif
        internal static void OnSynchronizeServerTime(System.IntPtr obj)
        {
            GCHandle gch = (GCHandle)obj;
            NativeInternalNetClientEvent native = (NativeInternalNetClientEvent)gch.Target;

            if (native.m_netclient.IsExceptionEventAllowed())
            {
                try
                {
                    if (native.m_netclient.synchronizeServerTimeHandler != null)
                    {
                        native.m_netclient.synchronizeServerTimeHandler();
                    }
                }
                catch (System.Exception ex)
                {
                    Internal_HandleException(native, native.m_netclient.GetLocalHostID(), ex);
                }
            }
            else
            {
                if (native.m_netclient.synchronizeServerTimeHandler != null)
                {
                    native.m_netclient.synchronizeServerTimeHandler();
                }
            }
        }

#if (UNITY_ENGINE)
    [AOT.MonoPInvokeCallback(typeof(ProudDelegate.Delegate_0))]
#endif
        internal static void OnError(System.IntPtr obj, System.IntPtr errorInfo)
        {
            GCHandle gch = (GCHandle)obj;
            NativeInternalNetClientEvent native = (NativeInternalNetClientEvent)gch.Target;

            Nettention.Proud.ErrorInfo managedErrorInfo = ConvertToCSharp.NativeErrorInfoToCSharp(errorInfo);;

            if (native.m_netclient.IsExceptionEventAllowed())
            {
                try
                {
                    if (native.m_netclient.errorHandler != null)
                    {
                        native.m_netclient.errorHandler(managedErrorInfo);
                    }
                }
                catch (System.Exception ex)
                {
                    Internal_HandleException(native, ((managedErrorInfo != null) ? managedErrorInfo.remote : HostID.HostID_None), ex);
                }
            }
            else
            {
                if (native.m_netclient.errorHandler != null)
                {
                    native.m_netclient.errorHandler(managedErrorInfo);
                }
            }
        }

#if (UNITY_ENGINE)
    [AOT.MonoPInvokeCallback(typeof(ProudDelegate.Delegate_0))]
#endif
        internal static void OnWarning(System.IntPtr obj, System.IntPtr errorInfo)
        {
            GCHandle gch = (GCHandle)obj;
            NativeInternalNetClientEvent native = (NativeInternalNetClientEvent)gch.Target;

            Nettention.Proud.ErrorInfo managedErrorInfo = ConvertToCSharp.NativeErrorInfoToCSharp(errorInfo);;

            if (native.m_netclient.IsExceptionEventAllowed())
            {
                try
                {
                    if (native.m_netclient.warningHandler != null)
                    {
                        native.m_netclient.warningHandler(managedErrorInfo);
                    }
                }
                catch (System.Exception ex)
                {
                    Internal_HandleException(native, ((managedErrorInfo != null) ? managedErrorInfo.remote : HostID.HostID_None), ex);
                }
            }
            else
            {
                if (native.m_netclient.warningHandler != null)
                {
                    native.m_netclient.warningHandler(managedErrorInfo);
                }
            }
        }

#if (UNITY_ENGINE)
    [AOT.MonoPInvokeCallback(typeof(ProudDelegate.Delegate_0))]
#endif
        internal static void OnInformation(System.IntPtr obj, System.IntPtr errorInfo)
        {
            GCHandle gch = (GCHandle)obj;
            NativeInternalNetClientEvent native = (NativeInternalNetClientEvent)gch.Target;

            Nettention.Proud.ErrorInfo managedErrorInfo = ConvertToCSharp.NativeErrorInfoToCSharp(errorInfo);

            if (native.m_netclient.IsExceptionEventAllowed())
            {
                try
                {
                    if (native.m_netclient.informationHandler != null)
                    {
                        native.m_netclient.informationHandler(managedErrorInfo);
                    }
                }
                catch (System.Exception ex)
                {
                    Internal_HandleException(native, ((managedErrorInfo != null) ? managedErrorInfo.remote : HostID.HostID_None), ex);
                }
            }
            else
            {
                if (native.m_netclient.informationHandler != null)
                {
                    native.m_netclient.informationHandler(managedErrorInfo);
                }
            }
        }

#if (UNITY_ENGINE)
    [AOT.MonoPInvokeCallback(typeof(ProudDelegate.Delegate_3))]
#endif
        internal static void OnException(System.IntPtr obj, int remote, System.IntPtr what)
        {
            GCHandle gch = (GCHandle)obj;
            NativeInternalNetClientEvent native = (NativeInternalNetClientEvent)gch.Target;
            if (native.m_netclient.exceptionHandler != null)
            {
                native.m_netclient.exceptionHandler((HostID)remote, new Exception(ConvertToCSharp.NativeStringToCSharp(what)));
            }
        }

#if (UNITY_ENGINE)
    [AOT.MonoPInvokeCallback(typeof(ProudDelegate.Delegate_12))]
#endif
        internal static void OnNoRmiProcessed(System.IntPtr obj, int rmiID)
        {
            GCHandle gch = (GCHandle)obj;
            NativeInternalNetClientEvent native = (NativeInternalNetClientEvent)gch.Target;

            if (native.m_netclient.IsExceptionEventAllowed())
            {
                try
                {
                    if (native.m_netclient.noRmiProcessedHandler != null)
                    {
                        native.m_netclient.noRmiProcessedHandler((RmiID)rmiID);
                    }
                }
                catch (System.Exception ex)
                {
                    Internal_HandleException(native, HostID.HostID_None, ex);
                }
            }
            else
            {
                if (native.m_netclient.noRmiProcessedHandler != null)
                {
                    native.m_netclient.noRmiProcessedHandler((RmiID)rmiID);
                }
            }
        }

#if (UNITY_ENGINE)
    [AOT.MonoPInvokeCallback(typeof(ProudDelegate.Delegate_7))]
#endif
        internal static void OnReceiveUserMessage(System.IntPtr obj, int sender, System.IntPtr rmiContext, System.IntPtr payload, int payloadLength)
        {
            GCHandle gch = (GCHandle)obj;
            NativeInternalNetClientEvent native = (NativeInternalNetClientEvent)gch.Target;

            if (native.m_netclient.IsExceptionEventAllowed())
            {
                try
                {
                    if (native.m_netclient.receivedUserMessageHandler != null)
                    {
                        native.m_netclient.receivedUserMessageHandler((HostID)sender, ConvertToCSharp.NativeRmiContextToCSharp(rmiContext), ConvertToCSharp.NativeByteArrayToCSharp(payload, payloadLength));
                    }
                }
                catch (System.Exception ex)
                {
                    Internal_HandleException(native, (HostID)sender, ex);
                }
            }
            else
            {
                if (native.m_netclient.receivedUserMessageHandler != null)
                {
                    native.m_netclient.receivedUserMessageHandler((HostID)sender, ConvertToCSharp.NativeRmiContextToCSharp(rmiContext), ConvertToCSharp.NativeByteArrayToCSharp(payload, payloadLength));
                }
            }
        }

#if (UNITY_ENGINE)
    [AOT.MonoPInvokeCallback(typeof(ProudDelegate.Delegate_0))]
#endif
        internal static void OnServerOffline(System.IntPtr obj, System.IntPtr args)
        {
            GCHandle gch = (GCHandle)obj;
            NativeInternalNetClientEvent native = (NativeInternalNetClientEvent)gch.Target;

            Nettention.Proud.RemoteOfflineEventArgs managedArgs = ConvertToCSharp.NativeRmoteOfflineEventArgsToCSharp(args);

            if (native.m_netclient.IsExceptionEventAllowed())
            {
                try
                {
                    if (native.m_netclient.serverOfflineHandler != null)
                    {
                        native.m_netclient.serverOfflineHandler(managedArgs);
                    }
                }
                catch (System.Exception ex)
                {
                    Internal_HandleException(native, ((managedArgs != null) ? managedArgs.remoteHostID : HostID.HostID_None), ex);
                }
            }
            else
            {
                if (native.m_netclient.serverOfflineHandler != null)
                {
                    native.m_netclient.serverOfflineHandler(managedArgs);
                }
            }
        }

#if (UNITY_ENGINE)
    [AOT.MonoPInvokeCallback(typeof(ProudDelegate.Delegate_0))]
#endif
        internal static void OnServerOnline(System.IntPtr obj, System.IntPtr args)
        {
            GCHandle gch = (GCHandle)obj;
            NativeInternalNetClientEvent native = (NativeInternalNetClientEvent)gch.Target;

            Nettention.Proud.RemoteOnlineEventArgs managedArgs = ConvertToCSharp.NativeRmoteOnlineEventArgsToCSharp(args);

            if (native.m_netclient.IsExceptionEventAllowed())
            {
                try
                {
                    if (native.m_netclient.serverOnlineHandler != null)
                    {
                        native.m_netclient.serverOnlineHandler(managedArgs);
                    }
                }
                catch (System.Exception ex)
                {
                    Internal_HandleException(native, ((managedArgs != null) ? managedArgs.remoteHostID : HostID.HostID_None), ex);
                }
            }
            else
            {
                if (native.m_netclient.serverOnlineHandler != null)
                {
                    native.m_netclient.serverOnlineHandler(managedArgs);
                }
            }
        }

#if (UNITY_ENGINE)
    [AOT.MonoPInvokeCallback(typeof(ProudDelegate.Delegate_0))]
#endif
        internal static void OnP2PMemberOffline(System.IntPtr obj, System.IntPtr args)
        {
            GCHandle gch = (GCHandle)obj;
            NativeInternalNetClientEvent native = (NativeInternalNetClientEvent)gch.Target;

            Nettention.Proud.RemoteOfflineEventArgs managedArgs = ConvertToCSharp.NativeRmoteOfflineEventArgsToCSharp(args);

            if (native.m_netclient.IsExceptionEventAllowed())
            {
                try
                {
                    if (native.m_netclient.p2pMemberOfflineHandler != null)
                    {
                        native.m_netclient.p2pMemberOfflineHandler(managedArgs);
                    }
                }
                catch (System.Exception ex)
                {
                    Internal_HandleException(native, ((managedArgs != null) ? managedArgs.remoteHostID : HostID.HostID_None), ex);
                }
            }
            else
            {
                if (native.m_netclient.p2pMemberOfflineHandler != null)
                {
                    native.m_netclient.p2pMemberOfflineHandler(managedArgs);
                }
            }
        }

#if (UNITY_ENGINE)
    [AOT.MonoPInvokeCallback(typeof(ProudDelegate.Delegate_0))]
#endif
        internal static void OnP2PMemberOnline(System.IntPtr obj, System.IntPtr args)
        {
            GCHandle gch = (GCHandle)obj;
            NativeInternalNetClientEvent native = (NativeInternalNetClientEvent)gch.Target;

            Nettention.Proud.RemoteOnlineEventArgs managedArgs = ConvertToCSharp.NativeRmoteOnlineEventArgsToCSharp(args);

            if (native.m_netclient.IsExceptionEventAllowed())
            {
                try
                {
                    if (native.m_netclient.p2pMemberOnlineHandler != null)
                    {
                        native.m_netclient.p2pMemberOnlineHandler(managedArgs);
                    }
                }
                catch (System.Exception ex)
                {
                    Internal_HandleException(native, ((managedArgs != null) ? managedArgs.remoteHostID : HostID.HostID_None), ex);
                }
            }
            else
            {
                if (native.m_netclient.p2pMemberOnlineHandler != null)
                {
                    native.m_netclient.p2pMemberOnlineHandler(managedArgs);
                }
            }
        }
    }
}