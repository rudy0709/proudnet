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
using System.Diagnostics;
using System.Text;
using System.Runtime.InteropServices;

namespace Nettention.Proud
{
    /**
	\~korean
	PIDL 컴파일러가 생성한 Stub 클래스의 베이스 클래스

	주의 사항
	- 이 클래스를 유저가 직접 구현하지 말 것. PIDL 컴파일러에서 구현한 것을 쓰도록 해야 한다.

	\~english
	Base class of Stub class created by PIDL compiler

	Note
	- User must not create this class. Must be realized by PIDL compiler.

	\~chinese
	PIDL编译生成的Stub类的基础类。

	注意事项
	- 用户不要直接体现此类，要使用在PIDL编译中所体现的类。

	\~japanese

	\~
	 */
	public abstract class RmiStub
	{
        internal NativeInternalStub m_native_Internal = null;

        public IRmiHost m_core = null;

        public IRmiHost core
        {
            get { return m_core; }
        }

        /**
		\~korean
		true로 세팅하면 NotifyCallFromStub을 호출받을 수 있다.
		그러나, 그 댓가로 실행 속도가 현저히 떨어진다. 디버깅을 할 때만 켜는
		것을 권장한다.

		\~english
		If set as true then calls NotifyCallFromStub.
		But in return, process speed will be lowered significantly. It is recommended to use this when debugging.

		\~chinese
		如果设置为true可以接收NotifyCallFromStub的呼出。
		但运行速度将会明显下降，因此建议只在进行Debugging时开启。

		\~japanese

		\~
		*/
        public bool enableNotifyCallFromStub = false;

        /**
		\~korean
		true로 설정하면 BeforeRmiInvocation,AfterRmiInvocation를 콜백한다.
		그러나 그 댓가로 실행 속도가 약간 떨어진다. 성능 최적화를 위해 RMI 함수 종류별 실행
		시간을 체크할 때만 켜는 것을 권장한다.

		\~english
		If set as true then BeforeRmiInvocation and AfterRmiInvocation are called back.
		But in return, process speed will be lowered a little. It is recommended to use this when checking running time of RMI function of each type.

		\~chinese
		如果设置为true，将回调 BeforeRmiInvocation,AfterRmiInvocation。
		但运行速度可能会有所下降，为优化性能，建议只在检查各RMI函数种类的运行时间时开启。

		\~japanese

		\~
		*/
        public bool enableStubProfiling = false;

        public RmiStub()
        {
            try
            {
                m_native_Internal = new NativeInternalStub(this);
            }
            catch (System.TypeInitializationException ex)
            {
                // c++ ProudNetServerPlugin.dll, ProudNetClientPlugin.dll 파일이 작업 경로에 없을 때
                throw new System.Exception(ClientNativeExceptionString.TypeInitializationExceptionString);
            }
        }

        ~RmiStub()
        {
        }

        /**
		\~korean
		이 함수를 구현하지 말 것. PIDL 컴파일러의 결과물이 override한다.

		\~english
		DO NOT realize this function. The outcome of PIDL compiler will override.

		\~chinese
		不要体现此函数，PIDL的编译结果物会进行Override。

		\~japanese

		\~
		 */
        public virtual RmiID[] GetRmiIDList
        {
            get;
            set;
        }

        public virtual int GetRmiIDListCount
        {
            get;
            set;
        }

        public NativeInternalStub GetNativeInternalStub()
        {
            return m_native_Internal;
        }

        /**
		\~korean
		이 함수를 구현하지 말 것. PIDL 컴파일러의 결과물이 override한다.

		\~english
		DO NOT realize this function. The outcome of PIDL compiler will override.

		\~chinese
		不要体现此函数，PIDL的编译结果物会进行Override。

		\~japanese

		\~
		 */
        public abstract bool ProcessReceivedMessage(ReceivedMessage pa, Object hostTag);

        /**
		\~korean
		RMI가 실행된 직후 호출된다.

		\~english
		Called right after RMI is run

		\~chinese
		在RMI运行后会被立即呼出。

		\~japanese

		\~
		 */
        public delegate void AfterRmiInvocationDelegate(AfterRmiSummary summary);

        /**
		\~korean
		RMI가 실행되기 직전에 호출된다.

		\~english
		Called right after RMI is run

		\~chinese
		在RMI运行前被呼出。

		\~japanese

		\~
		*/
        public delegate void BeforeRmiInvocationDelegate(BeforeRmiSummary summary);

        /**
		\~korean
		유저가 이 함수를 override하면, RMI가 실행되면서 받은 파라메터를 문자열로 모두 표시할 수 있게 해준다.
		단, 성능이 매우 떨어지게 되므로 주의해서 쓰도록 하자.

		\~english
		If user override this function then it lets parameter received from RMI be displayed as text string.
		But in return, process speed will be lowered significantly so be careful when you use this.

		\~chinese
		用户如果 override此函数，可以用文字列形式显示在运行RMI时所接收的所有参数。
		但性能可能会明显下降，因此需注意使用。

		\~japanese

		\~
		*/
        public virtual void NotifyCallFromStub(RmiID RMIId, String methodName, String parameters)
        {
            // no impl
        }

        public void ShowUnknownHostIDWarning(HostID remoteHostID)
        {
            Console.WriteLine(String.Format("Warning: unknown HostID {0} in ProcessReceivedMessage!", (int)remoteHostID));
        }
	}

	/**	 @} */
}
