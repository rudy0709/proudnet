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

namespace Nettention.Proud
{
    /** \addtogroup net_group
    *  @{
    */

    /**
    \~korean
    \brief ProudNet RMI가 참조하는 네트워크 호스트 인터페이스입니다.
    유저는 이 클래스를 직접 사용할 필요가 없습니다.

    \~english
    \brief ProudNet Network host interface that is referred by RMI.
    User does not have to use this class directly.

    \~chinese
    \brief 是ProudNet RMI所参照的网络主机Interface。
    用户没有必要直接使用此类。

    \~japanese

    \~
    */

    public interface IRmiHost
    {
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

        void AttachProxy(RmiProxy proxy);

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

        void AttachStub(RmiStub stub);

#if false
        void DetachProxy(RmiProxy proxy);

		void DetachStub(RmiStub stub);
#endif
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

        void ShowNotImplementedRmiWarning(String RMIName);

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

        void PostCheckReadMessage(Message msg, String RMIName);

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

        bool IsSimplePacketMode();

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
        void NotifyException(HostID hostID, System.Exception ex);

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
        bool IsExceptionEventAllowed();
    }
}