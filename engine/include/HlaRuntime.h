﻿/*
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

#pragma once

#include "HlaDef.h"
#include "Message.h"
#include "Ptr.h"
#include "PNString.h"

#ifdef _MSC_VER
#pragma pack(push,8)
#endif

namespace Proud
{
	class SynchEntity_S;
	class SynchEntity_C;
	class SynchViewport_S;

#define DECLARE_HLA_MEMBER(behavior,type,name) \
private: behavior<type> m_##name##_INTERNAL; \
public: type get_##name() { return m_##name##_INTERNAL.Get(); } \
	void set_##name(type newVal) { m_##name##_INTERNAL.Set(newVal); } \
	__declspec(property(get=get_##name,put=set_##name)) type name;

	class SynchEntities_S: public map<HlaInstanceID, SynchEntity_S*>
	{
	};

	class SynchViewports_S: public map<HlaInstanceID, SynchViewport_S*>
	{
	};

	/** 
	\~korean
	SynchEntity 클라이언트측 콜렉션

	노트
	- 서버 쪽 콜렉션과 달리, 이 콜렉션은 객체 스스로 소유권을 가지고 있다. 왜냐하면 이 콜렉션 아이템은 유저의 컨트롤하에 만들어 진것이 아니기 때문이다.

	\~english
	SynchEntity client-side collection

	Note
	- Unlike the collection in server-side, this collection has the ownership of the objects. Because the collection items are
	created not under control of user.

	\~chinese
	SynchEntity client-side collection

	通知
	- 与服务器的 collection 不同，此 collection 对象本身拥有着所有权。因为此 collection 道具不是在用户的控制下创建的。

	\~japanese
	SynchEntity クライアント側コレクション

	ノート
	- サーバー側のコレクションとは違って、このコレクションはオブジェクト自体が所有権を持っています。なぜなら、このコレクションアイテムはユーザーのコントロールの下で作られるものではないからです。
	\~
	*/
	class SynchEntities_C: public map<HlaInstanceID, RefCount<SynchEntity_C> >
	{
	};

	class IHlaRuntimeDelegate_S
	{
	public:
		virtual ~IHlaRuntimeDelegate_S();

		virtual void Send(HostID dest, Protocol protocol, CMessage& message) = 0;

		// determines if one synch entity is visible (tangible) to one viewport
		virtual bool IsOneSynchEntityTangibleToOneViewport(SynchEntity_S* entity, SynchViewport_S *viewport) = 0;
	};

	/** 
	\~korean
	서버측 HLA 런타임 모듈

	사용법
	- 이 객체를 생성한다.
	- SWD compiler-generated 클래스 인스턴스를 생성하고 이 객체에 합친다.

	\~english
	Server-side HLA runtime module

	Usage
	- Create this object
	- Create SWD compiler-generated class instances and associate them to this object

	\~chinese
	服务器方 HLA 运行时间模块

	使用方法
	- 生成此对象。
	- 生成 SWD compiler-generated 类实例并与此对象合并。

	\~japanese
	サーバー側HLAランタイムモジュール

	使用方法
	- このオブジェクトを生成します。
	- SWD compiler-generated クラスのインスタンスを生成してこのオブジェクトと合わせます。
	\~
	*/

	class CHlaRuntime_S
	{
		HlaInstanceID m_instanceIDFactory;
		IHlaRuntimeDelegate_S* m_dg;
	public:
		CHlaRuntime_S(IHlaRuntimeDelegate_S* dg);
		~CHlaRuntime_S(void);

		SynchEntities_S m_synchEntities;
		SynchViewports_S m_synchViewports;

		HlaInstanceID CreateInstanceID();
		void FrameMove(double elapsedTime);

		void Check_OneSynchEntity_EveryViewport(SynchEntity_S *entity);
		void Check_EverySynchEntity_OneViewport(SynchViewport_S *viewport);
		void Check_EverySynchEntity_EveryViewport();
	private:
		void Check_OneSynchEntity_OneViewport( SynchEntity_S * entity, SynchViewport_S* viewport, CMessage appearMsg, CMessage disappearMsg );
		void Send_INTERNAL(vector<HostID> sendTo, Protocol protocol, CMessage &msg);
	public:
		void RemoveOneSynchEntity_INTERNAL(SynchEntity_S* entity);
		void RemoveOneSynchViewport_INTERNAL( SynchViewport_S* entity );
	};

	class IHlaRuntimeDelegate_C
	{
	public:
		virtual ~IHlaRuntimeDelegate_C();
		virtual SynchEntity_C* CreateSynchEntityByClassID(HlaClassID classID) = 0;
	};

	/** 
	\~korean
	클라이언트측 HLA 메인 런타임

	사용법
	- 이 객체를 생성하고 SWD 컴파일러에 의해 만들어진 하나 또는 이상의 HLA 런타임 delegate들과 합친다.
	- ProcessReceivedMessage()에 의해 이 객체로 받은 메세지를 전달한다.
	\~english
	HLA client-side main runtime

	Usage
	- Create this object and associate one or more HLA runtime delegates which are generated by SWD compiler.
	- Pass the received message to this object by ProcessReceivedMessage(). 

	\~chinese
	Client 方 HLA 主要运行时间

	使用方法
	- 生成此对象以后与由 SWD 生成器创建的一个或者一个以上的 HLA 运行时间 delegate 进行合并。
	- 由 ProcessReceivedMessage()传达从此个体接收的信息。

	\~japanese
	クライアント側HLAメインランタイム

	使用方法
	- このオブジェクトを生成してSWDコンパイラーによって作られた一つまたはそれ以上のHLAランタイムdelegateなどと合わせます。
	- ProcessReceivedMessage()によってこのオブジェクトで受けたメッセージを伝達します。
	\~
	*/
	class CHlaRuntime_C
	{
		SynchEntity_C* CreateSynchEntityByClassID( HlaClassID classID , HlaInstanceID instanceID);
		typedef vector<IHlaRuntimeDelegate_C*> DgList;
		DgList m_dgList;
		String m_lastProcessMessageReport;
	public:
		SynchEntities_C m_synchEntities;

		CHlaRuntime_C() {}
		~CHlaRuntime_C() {}

		void AddDelegate(IHlaRuntimeDelegate_C *dg);
		void ProcessReceivedMessage(CMessage& msg);
		SynchEntity_C* GetSynchEntityByID(HlaInstanceID instanceID);

		String GetLastProcessMessageReport();
		void Clear();
	};

	Protocol CombineProtocol(Protocol a, Protocol b);
}

//#pragma pack(pop)

﻿/*
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

#pragma once

#include "hladef.h"
#include "message.h"
#include "ptr.h"
#include "PNString.h"

#ifndef __GNUC__

//#pragma pack(push,8)

namespace Proud
{
	class SynchEntity_S;
	class SynchEntity_C;
	class SynchViewport_S;

#define DECLARE_HLA_MEMBER(behavior,type,name) \
private: behavior<type> m_##name##_INTERNAL; \
public: type get_##name() { return m_##name##_INTERNAL.Get(); } \
	void set_##name(type newVal) { m_##name##_INTERNAL.Set(newVal); } \
	__declspec(property(get=get_##name,put=set_##name)) type name;

	class SynchEntities_S: public map<HlaInstanceID, SynchEntity_S*>
	{
	};

	class SynchViewports_S: public map<HlaInstanceID, SynchViewport_S*>
	{
	};

	/** 
	\~korean
	SynchEntity 클라이언트측 콜렉션

	노트
	- 서버 쪽 콜렉션과 달리, 이 콜렉션은 객체 스스로 소유권을 가지고 있다. 왜냐하면 이 콜렉션 아이템은 유저의 컨트롤하에 만들어 진것이 아니기 때문이다.

	\~english
	SynchEntity client-side collection

	Note
	- Unlike the collection in server-side, this collection has the ownership of the objects. Because the collection items are
	created not under control of user.

	\~chinese
	SynchEntity client-side collection

	通知
	- 与服务器的 collection 不同，此 collection 对象本身拥有着所有权。因为此 collection 道具不是在用户的控制下创建的。

	\~japanese
	SynchEntity クライアント側コレクション

	ノート
	- サーバー側のコレクションとは違って、このコレクションはオブジェクト自体が所有権を持っています。なぜなら、このコレクションアイテムはユーザーのコントロールの下で作られたものではないからです。
	\~
	*/
	class SynchEntities_C: public map<HlaInstanceID, RefCount<SynchEntity_C> >
	{
	};

	class IHlaRuntimeDelegate_S
	{
	public:
		virtual ~IHlaRuntimeDelegate_S();

		virtual void Send(HostID dest, Protocol protocol, CMessage& message) = 0;

		// determines if one synch entity is visible (tangible) to one viewport
		virtual bool IsOneSynchEntityTangibleToOneViewport(SynchEntity_S* entity, SynchViewport_S *viewport) = 0;
	};

	/** 
	\~korean
	서버측 HLA 런타임 모듈

	사용법
	- 이 객체를 생성한다.
	- SWD compiler-generated 클래스 인스턴스를 생성하고 이 객체에 합친다.

	\~english
	Server-side HLA runtime module

	Usage
	- Create this object
	- Create SWD compiler-generated class instances and associate them to this object

	\~chinese
	服务器方 HLA 运行时间模块

	使用方法
	- 生成此对象。
	- 生成 SWD compiler-generated 类实例并与此对象合并。

	\~japanese
	サーバー側HLAランタイムモジュール

	使用方法
	- このオブジェクトを生成します。
	- SWD compiler-generated クラスインスタンスを生成してこのオブジェクトに合わせます。
	\~
	*/
	class CHlaRuntime_S
	{
		HlaInstanceID m_instanceIDFactory;
		IHlaRuntimeDelegate_S* m_dg;
	public:
		CHlaRuntime_S(IHlaRuntimeDelegate_S* dg);
		~CHlaRuntime_S(void);

		SynchEntities_S m_synchEntities;
		SynchViewports_S m_synchViewports;

		HlaInstanceID CreateInstanceID();
		void FrameMove(double elapsedTime);

		void Check_OneSynchEntity_EveryViewport(SynchEntity_S *entity);
		void Check_EverySynchEntity_OneViewport(SynchViewport_S *viewport);
		void Check_EverySynchEntity_EveryViewport();
	private:
		void Check_OneSynchEntity_OneViewport( SynchEntity_S * entity, SynchViewport_S* viewport, CMessage appearMsg, CMessage disappearMsg );
		void Send_INTERNAL(vector<HostID> sendTo, Protocol protocol, CMessage &msg);
	public:
		void RemoveOneSynchEntity_INTERNAL(SynchEntity_S* entity);
		void RemoveOneSynchViewport_INTERNAL( SynchViewport_S* entity );
	};

	class IHlaRuntimeDelegate_C
	{
	public:
		virtual ~IHlaRuntimeDelegate_C();
		virtual SynchEntity_C* CreateSynchEntityByClassID(HlaClassID classID) = 0;
	};

	/** 
	\~korean
	클라이언트측 HLA 메인 런타임

	사용법
	- 이 객체를 생성하고 SWD 컴파일러에 의해 만들어진 하나 또는 이상의 HLA 런타임 delegate들과 합친다.
	- ProcessReceivedMessage()에 의해 이 객체로 받은 메세지를 전달한다.

	\~english
	HLA client-side main runtime

	Usage
	- Create this object and associate one or more HLA runtime delegates which are generated by SWD compiler.
	- Pass the received message to this object by ProcessReceivedMessage().

	\~chinese
	Client方HLA主要运行时间

	使用方法
	- 生成此对象以后与由SWD生成器创建的一个或者一个以上的HLA运行时间delegate进行合并。
	- 由 ProcessReceivedMessage()传达从此个体接收的信息。

	\~japanese
	クライアント側HLAメインランタイム

	使用方法
	- このオブジェクトを生成してSWDコンパイラーによって作られた一つまたはそれ以上のHLAランタイムdelegateなどと合わせます。
	- ProcessReceivedMessage()によってこのオブジェクトで受けたメッセージを伝達します。
	\~
	*/
	class CHlaRuntime_C
	{
		SynchEntity_C* CreateSynchEntityByClassID( HlaClassID classID , HlaInstanceID instanceID);
		typedef vector<IHlaRuntimeDelegate_C*> DgList;
		DgList m_dgList;
		String m_lastProcessMessageReport;
	public:
		SynchEntities_C m_synchEntities;

		CHlaRuntime_C() {}
		~CHlaRuntime_C() {}

		void AddDelegate(IHlaRuntimeDelegate_C *dg);
		void ProcessReceivedMessage(CMessage& msg);
		SynchEntity_C* GetSynchEntityByID(HlaInstanceID instanceID);

		String GetLastProcessMessageReport();
		void Clear();
	};

	Protocol CombineProtocol(Protocol a, Protocol b);
}

#ifdef _MSC_VER
#pragma pack(pop)
#endif

#endif // __GNUC__
<<<<
