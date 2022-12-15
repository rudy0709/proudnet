#pragma once


#include "../include/Ptr.h"
#include "FastSocket.h"
#include "FastMap2.h"
#include "FastList2.h"
#include "URI.h"

namespace Proud
{
#if defined(_WIN32)
	class CNetClientManager;
	class CUpnpTask;
	class CHttpSession;

	typedef RefCount<CUpnpTask> CUpnpTaskPtr;

	enum SsdpState
	{
		Ssdp_None,
		Ssdp_Requesting,
		Ssdp_ResponseWaiting,
		Ssdp_Finished,
	};

	enum HttpState
	{
		Http_Init,
		Http_Connecting,
		Http_Requesting,
		Http_ReponseWaiting,
		Http_ParsingHttpResponse,
		Http_Disposing,
		Http_Disposed,
	};

	class CDiscoveringNatDevice;
	typedef RefCount<CDiscoveringNatDevice> CDiscoveringNatDevicePtr;

	class CSsdpContext//:public IFastSocketDelegate
	{
	public:
		String m_localAddr;
		shared_ptr<CFastSocket> m_ssdpSocket;
		SsdpState m_ssdpState;
		int m_ssdpRequestCount;
		// ikpil.choi 2016-11-04 : 사용하지 않는 버퍼 관련 코드 삭제
		//ByteArray m_ssdpResponseBuffer;

		volatile bool m_sendIssued; // true-false-true 문제를 피하기 위해 선 true후 issue fail시 false해야 하겠다.
		volatile bool m_recvIssued;
		int64_t m_ssdpResponseWaitStartTime;
	private:
		virtual void OnSocketWarning(CFastSocket* socket, String msg);
	public:
		CSsdpContext();
		bool IsSafeDisposeGuaranteed();
	};

	typedef RefCount<CSsdpContext> CSsdpContextPtr;

	class CDiscoveredNatDevice;

	typedef RefCount<CDiscoveredNatDevice> CDiscoveredNatDevicePtr;

	class CUpnp
	{
		friend CHttpSession;
	public:
		void Heartbeat();

		static StringA GetPortMappingRandomDesc();
	private:
		void InitSsdp();
		void StopAllDiscoveryWorksOnNeed();
		void Heartbeat_Http();
		void Heartbeat_Ssdp(CSsdpContext* ssdp);
		void Heartbeat_Discovering(CDiscoveringNatDevice* dev);
		void Heartbeat_Discovered(CDiscoveredNatDevice* dev);
		void Heartbeat_Task(CUpnpTask* task);

		void Heartbeat_Task_InitState( CUpnpTask* task );
		void Heartbeat_Task_Connecting( CUpnpTask* task );
		void Heartbeat_Task_Requesting( CUpnpTask* task );

		StringA GetXmlNodeValue(const StringA &xmlDoc, const StringA &nodeName);
		StringA GetXmlNodeValueAfter(const StringA &xmlDoc, const StringA &nodeName, const StringA &findAfter);
		CNetClientManager* m_manager;

		CriticalSection m_cs;

		int m_heartbeatCount;

		typedef CFastMap2<StringA, CDiscoveringNatDevicePtr, int> DiscoveriingNatDevices;
		DiscoveriingNatDevices m_discoveringNatDevices;

		typedef CFastList2<CDiscoveredNatDevicePtr, int> DiscoveredNatDevices;
		DiscoveredNatDevices m_discoveredNatDevices;

		typedef CFastArray<CSsdpContextPtr> SsdpContexts;
		SsdpContexts m_ssdpContexts;

		int64_t m_startTime;

		// DeletePortMapping 명령이 실행되는 task가 몇 개인지
		int m_deletePortMappingTaskCount;

		// 전에는 전역 변수였으나, 어차피 성능에 큰 영향을 주는 것도 아니고, 이것의 전역 생성시 다른 전역변수가 아직 안 만들어져 있어서 생기는 문제 때문에, 멤버 변수다.
		AddrPort ssdpRequestTo;

		virtual void OnSocketWarning(CFastSocket* socket, String msg);

		void Heartbeat_Ssdp_NoneCase(CSsdpContext* ssdp);

		void Ssdp_IssueRequest(CSsdpContext* ssdp);
		void Heartbeat_Ssdp_RequestingCase(CSsdpContext* ssdp);

		void Ssdp_IssueResponseWait(CSsdpContext* ssdp);
		void Heartbeat_Ssdp_ResponseWaitingCase(CSsdpContext* ssdp);

	public:
		CUpnp(CNetClientManager* manager);
		~CUpnp();

		void Reset();

		void IssueTermination();
		void WaitUntilNoIssueGuaranteed();
		String GetNatDeviceName();

		void AddTcpPortMapping( AddrPort lanAddr, AddrPort wanAddr, bool isTcp );
		void DeleteTcpPortMapping( AddrPort lanAddr, AddrPort wanAddr, bool isTcp );

		void Heartbeat_Task_ResponseWaiting( CUpnpTask* task );
		void Heartbeat_Task_ParsingHttpResponse( CUpnpTask* task );
		CDiscoveredNatDevice* GetCommandAvailableNatDevice();
	};

	class CUpnpTask;

	class CHttpSession
	{
	public:
		CUpnp* m_ownerUpnp;
		shared_ptr<CFastSocket> m_socket;
		StringA m_httpRequest;

		volatile bool m_sendIssued; // true-false-true 문제를 피하기 위해 선 true후 issue fail시 false해야 하겠다.
		volatile bool m_recvIssued;
		int m_httpRequestIssueOffset;
		int m_httpResponseIssueOffset;
		CFastArray<uint8_t> m_httpResponseBuffer; // not B﻿yteArray which uses FastHeap
		StringA m_httpResponse;

		bool IssueConnect(String hostName, int portNumber);
		bool IssueRequestSend();
		bool IsSafeDisposeGuaranteed();

		CHttpSession()
		{
			m_sendIssued = false;
			m_recvIssued = false;
			m_httpResponseIssueOffset = 0;
			m_httpRequestIssueOffset = 0;
		}
	};

	// 이 클래스가 가진 CUrl 클래스는 dbghelp.h와 conflict를 일으킨다. 따라서 CPP로 이동.
	class CDiscoveringNatDevice:/*public IFastSocketDelegate,*/ public CHttpSession
	{
	public:
		StringA m_name; // NAT device name

		CUri m_url;
	private:
		HttpState m_state_USE_FUNC;
		int m_currentStateWorkCount_USE_FUNC;
	public:
		HttpState GetState() { return m_state_USE_FUNC;  }
		int GetCurrentStateWorkCount() { return m_currentStateWorkCount_USE_FUNC; }
		void IncCurrentStateWorkCount() { m_currentStateWorkCount_USE_FUNC++; }

		void SetState(HttpState val);
		bool TakeResponse(StringA response);

		void IssueTermination();

		virtual void OnSocketWarning(CFastSocket* socket, String msg);

		CDiscoveringNatDevice();
		~CDiscoveringNatDevice();
	};

	// NAT device 찾기가 완전히 끝난 객체. CDiscoveringNatDevice는 찾기 진행 중인 것과 상반된다.
	class CDiscoveredNatDevice
	{
	public:
		// 포트매핑 명령 수행 등을 하는 작업 큐
		CFastList2<CUpnpTaskPtr,int> m_tasks;

		// 발견된 device의 이름. LAN 내 같은 모델의 공유기가 있을 수 있으므로 key로 잡으면 곤란.
		String m_deviceName;

		// 공유기에 명령을 던질 때 공유기 측에서 받아 처리할 웹 페이지 주소
		StringA m_controlUrl;

		// 발견된 공유기의 연결 주소
		CUri m_url;

		bool IsSafeDisposeGuaranteed();
		void IssueTermination();
	};

	enum UpnpTaskType
	{
		UpnpTask_AddPortMapping,
		UpnpTask_DeletePortMapping,
	};

	/* upnp 사용 가능한 공유기를 찾았으면 거기에 명령을 던질 수 있다.
	이것은 공유기에 명령 수행중인 상태 객체이다.
	소켓 핸들과 io 진행 상태 값을 가짐.
	*/
	class CUpnpTask:/*public IFastSocketDelegate,*/public CHttpSession
	{
		virtual void OnSocketWarning(CFastSocket* socket, String msg);
	public:
		HttpState m_state;
		UpnpTaskType m_type;

		// 이 명령 처리 대상의 공유기.
		CDiscoveredNatDevice* m_ownerDevice;

		// AddPortMapping 전용
		bool m_isTcp;

		// Add, DeletePortMapping 전용
		AddrPort m_lanAddr, m_wanAddr;

		void BuildUpnpRequestText();
	private:
		void BuildUpnpRequestText_AddPortMapping();
		void BuildUpnpRequestText_DeletePortMapping();
	};

	class CUpnpTcpPortMappingState
	{
	public:
		AddrPort m_lanAddr, m_wanAddr;
	};
#endif
}