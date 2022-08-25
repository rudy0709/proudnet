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
using System.Diagnostics;
using System.Text;
using System.Runtime.InteropServices;
using Nettention.Proud;

namespace Nettention.Proud
{
    // Swig에서 제공하는 C++ 가상 클래스를 C# 클래스에서 상속받아서 사용하는 기능을 사용하지 못하는 이유는 Unity IL2CPP에서 C++ 코드를 생성할 때 C++ -> C# 콜백에 대한 코드에 문제가 있기 때문에
    // 각 이벤트에 대해서 콜백 함수를 만들어서 처리합니다.
    public class NativeInternalProxy : GCHandleList
    {
        private RmiProxy m_proxy;
        private System.IntPtr m_proxyWrap = System.IntPtr.Zero;

        private bool disposed = false;

        internal NativeInternalProxy(RmiProxy clrObj)
        {
            m_proxy = clrObj;

            GCHandle handle = GCHandle.Alloc(this);
            m_proxyWrap = Nettention.Proud.ProudNetClientPlugin.NativeToRmiProxyWrap_New();

            Nettention.Proud.ProudNetClientPlugin.RmiProxy_SetCSharpHandle(m_proxyWrap, GCHandle.ToIntPtr(handle));
            base.AddHandle(handle);

            // C# -> C++로 넘겨진 Delegate도 가비지 수집이 되기 때문에 GCHandle.Alloc을 해주어야 합니다.
            ProudDelegate.Delegate_6 del1 = GetRmiIDList;
            handle = GCHandle.Alloc(del1);
            Nettention.Proud.ProudNetClientPlugin.RmiProxy_SetCallbackGetRmiIDList(m_proxyWrap, del1);
            base.AddHandle(handle);

            ProudDelegate.Delegate_1 del2 = GetRmiIDListCount;
            handle = GCHandle.Alloc(del2);
            Nettention.Proud.ProudNetClientPlugin.RmiProxy_SetCallbackGetRmiIDListCount(m_proxyWrap, del2);
            base.AddHandle(handle);
        }

        ~NativeInternalProxy()
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
                Nettention.Proud.ProudNetClientPlugin.NativeToRmiProxyWrap_Delete(m_proxyWrap);
                m_proxyWrap = IntPtr.Zero;

                base.FreeAllHandle();
            }

            disposed = true;
        }

        public System.IntPtr GetNativeProxy()
        {
            return m_proxyWrap;
        }

#if (UNITY_ENGINE)
    [AOT.MonoPInvokeCallback(typeof(ProudDelegate.Delegate_6))]
#endif
        internal static unsafe System.IntPtr GetRmiIDList(System.IntPtr obj)
        {
            GCHandle gch = (GCHandle)obj;
            NativeInternalProxy native = (NativeInternalProxy)gch.Target;

            fixed (RmiID* ret = (&native.m_proxy.RmiIDList[0]))
            {
                return new IntPtr((void*)ret);
            }
        }

#if (UNITY_ENGINE)
    [AOT.MonoPInvokeCallback(typeof(ProudDelegate.Delegate_1))]
#endif
        internal static int GetRmiIDListCount(System.IntPtr obj)
        {
            GCHandle gch = (GCHandle)obj;
            NativeInternalProxy native = (NativeInternalProxy)gch.Target;

            return native.m_proxy.GetRmiIDListCount;
        }

        internal static unsafe bool RmiSend(NativeInternalProxy nativeProxy, HostID[] remotes, RmiContext rmiContext, Message msg, String rmiName, RmiID rmiID)
        {
            if (remotes.Length <= 0 || msg.Length <= 0)
            {
                return false;
            }

#if true
            bool ret = false;

            fixed (HostID* remote = remotes)
            {
                fixed (byte* data = msg.Data.data)
                {
                    NativeRmiContext nativeRmiContext = null;

                    try
                    {
                        nativeRmiContext = ConvertToNative.RmiContextToNative(rmiContext);

                        ret = Nettention.Proud.ProudNetClientPlugin.RmiProxy_RmiSend(
                        nativeProxy.GetNativeProxy(),
                        new IntPtr((void*)remote), remotes.Length,
                        nativeRmiContext,
                        new IntPtr((void*)data), msg.Data.Count,
                        rmiName,
                        (int)rmiID);
                    }
                    catch (System.Exception ex)
                    {
                        nativeProxy.m_proxy.core.NotifyException(HostID.HostID_None, ex);
                    }
                    finally
                    {
                       if (nativeRmiContext != null)
                       {
                           nativeRmiContext.Dispose();
                       }
                    }
                }
            }
            return ret;
#else
            return false;
#endif
        }
    }
}