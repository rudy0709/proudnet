/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <new>
#if defined(_WIN32)
#include <tchar.h>
#else
#include "../include/pntchar.h"
#endif
#include "../include/sysutil.h"
#include "Upnp.h"
#include "MilisecTimerImpl.h"
#include "CriticalSectImpl.h"
#include "SocketUtil.h"
//#include "networker_c.h"

namespace Proud
{
#if defined(_WIN32)
	const int HttpResponseBufferLength = 32768;

	StringA ssdpRequest = "M-SEARCH * HTTP/1.1\r\nHost:239.255.255.250:1900\r\nST:upnp:rootdevice\r\nMan:\"ssdp:discover\"\r\nMX:3\r\n\r\n";
	CUpnp::CUpnp(CNetClientManager* manager)
	{
		m_deletePortMappingTaskCount = 0;
		m_manager = manager;
		m_heartbeatCount = 0;
		m_startTime = GetPreciseCurrentTimeMs();
		ssdpRequestTo = AddrPort::FromIPPortV4(_PNT("239.255.255.250"), 1900);
	}

	void CUpnp::Heartbeat()
	{
		CriticalSectionLock clk(m_cs, true); // 한 개의 스레드에서만 이것을 처리해야 하므로

		if(m_heartbeatCount==0)
		{
			InitSsdp();

		}
		m_heartbeatCount++;
		for(SsdpContexts::iterator i=m_ssdpContexts.begin();i!=m_ssdpContexts.end();i++)
			Heartbeat_Ssdp(*i);

		Heartbeat_Http();

		StopAllDiscoveryWorksOnNeed();
	}

	void CUpnp::OnSocketWarning(CFastSocket* /*socket*/, String /*msg*/)
	{
	}

	void CUpnp::Heartbeat_Ssdp_NoneCase(CSsdpContext* ssdp)
	{
		// 소켓 생성 및 SSDP broadcast를 한다.
		SocketCreateResult r = CFastSocket::Create(SocketType_Udp/*,ssdp*/);
		if (!r.socket)
		{
			return;
		}
		ssdp->m_ssdpSocket = r.socket;

		if(ssdp->m_ssdpSocket->EnableBroadcastOption(true) != SocketErrorCode_Ok)
		{
			ssdp->m_ssdpSocket=shared_ptr<CFastSocket>();
			ssdp->m_ssdpState=Ssdp_Finished;
			return;
		}

		if(ssdp->m_localAddr.GetLength()>0)
		{
			if (SocketErrorCode_Ok != ssdp->m_ssdpSocket->Bind(ssdp->m_localAddr.GetString(), 0))
			{
				ssdp->m_ssdpSocket=shared_ptr<CFastSocket>();
				ssdp->m_ssdpState=Ssdp_Finished;
				return;
			}
		}
		else
		{
			if (SocketErrorCode_Ok != ssdp->m_ssdpSocket->Bind())
			{
				ssdp->m_ssdpSocket=shared_ptr<CFastSocket>();
				ssdp->m_ssdpState=Ssdp_Finished;
				return;
			}
		}

		// 소켓을 넌블럭킹 모드로 바꾼 후 패킷을 쏜다.
		ssdp->m_ssdpSocket->SetBlockingMode(false);

		Ssdp_IssueRequest(ssdp);
	}

	void CUpnp::Heartbeat_Ssdp_RequestingCase(CSsdpContext* ssdp)
	{
		while(1)
		{
			OverlappedResult overlappedResult;
			if (ssdp->m_ssdpSocket->GetSendOverlappedResult(false, overlappedResult) == false)
			{
				// 아직 대기중
				return;
			}

			Proud::AssertIsLockedByCurrentThread(m_cs);

			ssdp->m_sendIssued = false;

			if (overlappedResult.m_errorCode != 0 || overlappedResult.m_completedDataLength == 0)
			{
				// 송신이 중도 잘못됐음을 의미한다. 따라서 에러 처리한다.
				ssdp->m_ssdpState = Ssdp_Finished;
				ssdp->m_ssdpSocket = shared_ptr<CFastSocket>();
				return;
			}

			ssdp->m_ssdpRequestCount++;
			if(ssdp->m_ssdpRequestCount<3)
			{
				Ssdp_IssueRequest(ssdp); // 한번 더 보낸다. UDP니까 여러번은 보내놔야 안전.
			}
			else
			{
				if(!ssdp->m_ssdpResponseWaitStartTime)
					ssdp->m_ssdpResponseWaitStartTime = GetPreciseCurrentTimeMs();
				Ssdp_IssueResponseWait(ssdp);
				return;
			}
		}
	}

	void CUpnp::Heartbeat_Ssdp_ResponseWaitingCase(CSsdpContext* ssdp)
	{
		while(1)
		{
			OverlappedResult overlappedResult;
			if (ssdp->m_ssdpSocket->GetRecvOverlappedResult(false, overlappedResult) == false)
			{
				// 아직 수신 대기중
				return;
			}

			ssdp->m_recvIssued = false;

			if (overlappedResult.m_errorCode != 0 || overlappedResult.m_completedDataLength == 0)
			{
				// 송신이 중도 잘못됐음을 의미한다. 따라서 에러 처리한다.
				ssdp->m_ssdpState = Ssdp_Finished;
				ssdp->m_ssdpSocket = shared_ptr<CFastSocket>();
				return;
			}

				// 수신 완료.
				StringA response;
				{
					StrBufA responseBuf(response,overlappedResult.m_completedDataLength + 2);
					memcpy(responseBuf,ssdp->m_ssdpSocket->GetRecvBufferPtr(),overlappedResult.m_completedDataLength);
					responseBuf[overlappedResult.m_completedDataLength] = 0;
				}

				// 여러개의 NAT일 수 있으므로 여러번 받도록 한다.
				CDiscoveringNatDevicePtr newNat(new CDiscoveringNatDevice);
				newNat->m_ownerUpnp = this;

				if(newNat->TakeResponse(response))
				{
					if(m_discoveringNatDevices.GetCount()<20)  // THIS IS NEEDED
					{
						StringA hostName = StringA(StringT2A(newNat->m_url.GetHostName()));
						if(!m_discoveringNatDevices.ContainsKey(hostName ) )
						{
							m_discoveringNatDevices.Add(hostName, newNat);
						}
					}
				}

			int64_t timeDiff = GetPreciseCurrentTimeMs() - ssdp->m_ssdpResponseWaitStartTime;
			if(timeDiff > 3000)
			{
				// no more issue. 바로 종료해도 OK.
				ssdp->m_ssdpSocket->CloseSocketOnly();
				ssdp->m_ssdpState = Ssdp_Finished;
				return;
			}
			else
			{
				Ssdp_IssueResponseWait(ssdp); // 또 다른 response가 올 수 있다. 수신을 다시 건다.
			}
		}
	}

	String CUpnp::GetNatDeviceName()
	{
		CriticalSectionLock lock(m_cs,true);
		if(m_discoveredNatDevices.GetCount()>0)
		{
			return (*m_discoveredNatDevices.begin())->m_deviceName;
		}
		else
			return String();
	}

	void CUpnp::Ssdp_IssueRequest(CSsdpContext* ssdp)
	{
		Proud::AssertIsLockedByCurrentThread(m_cs);

		ssdp->m_sendIssued = true;


		SocketErrorCode e = ssdp->m_ssdpSocket->IssueSendTo((uint8_t*)(const char*)ssdpRequest.GetString(), ssdpRequest.GetLength() + 1, ssdpRequestTo);
		if(e!=SocketErrorCode_Ok && e!=SocketErrorCode_IoPending)
		{
			ssdp->m_sendIssued = false;
			ssdp->m_ssdpSocket=shared_ptr<CFastSocket>();
			ssdp->m_ssdpState=Ssdp_Finished;
		}
		else
		{
			ssdp->m_ssdpState = Ssdp_Requesting;
		}
	}

	void CUpnp::Ssdp_IssueResponseWait(CSsdpContext* ssdp)
	{
		// UDP는 다음 송신을 걸 이유가 없다. 바로 완료이렸다.
		// 이제 수신 대기를 시작한다.

		// ikpil.choi 2016-11-04 : 사용하지 않는 버퍼 관련 코드 삭제
		//ssdp->m_ssdpResponseBuffer.SetCount(8192);
		//ZeroMemory(ssdp->m_ssdpResponseBuffer.GetData(), ssdp->m_ssdpResponseBuffer.GetCount());
		//ssdp->m_recvIssued = true;
		//if (ssdp->m_ssdpSocket->IssueRecvFrom((int)(ssdp->m_ssdpResponseBuffer.GetCount() - 1)) == SocketErrorCode_Ok)

		ssdp->m_recvIssued = true;
		if(ssdp->m_ssdpSocket->IssueRecvFrom(1024 * 16) == SocketErrorCode_Ok)
		{
			ssdp->m_ssdpState = Ssdp_ResponseWaiting;
		}
		else
		{
			ssdp->m_recvIssued = false;
			ssdp->m_ssdpSocket=shared_ptr<CFastSocket>();
			ssdp->m_ssdpState=Ssdp_Finished;
		}
	}

	void CUpnp::Heartbeat_Ssdp(CSsdpContext* ssdp)
	{
		switch(ssdp->m_ssdpState)
		{
		case Ssdp_None:
			Heartbeat_Ssdp_NoneCase(ssdp);
			break;
		case Ssdp_Requesting:
			Heartbeat_Ssdp_RequestingCase(ssdp);
			break;
		case Ssdp_ResponseWaiting:
			Heartbeat_Ssdp_ResponseWaitingCase(ssdp);
			break;
		case Ssdp_Finished:
			// 더 이상 아무것도 안함
			break;
		}
	}

	void CUpnp::Heartbeat_Http()
	{
		//assert(AtlIsValidAddress(this,sizeof(*this)));
		CriticalSectionLock clk(m_cs, true); // 한 개의 스레드에서만 이것을 처리해야 하므로

		for(DiscoveriingNatDevices::iterator i=m_discoveringNatDevices.begin();i!=m_discoveringNatDevices.end();)
		{
			CDiscoveringNatDevice* dev = i->GetSecond();

			Heartbeat_Discovering(dev);
			if(dev->GetState()== Http_Disposed)
			{
				i = m_discoveringNatDevices.erase(i);
			}
			else
				i++;
		}

		// 각 찾은 공유기들이 갖고 있는 task들을 수행한다. 가령 포트 매핑 명령을 비동기로 수행하는 task들.
		for(DiscoveredNatDevices::iterator i=m_discoveredNatDevices.begin();i!=m_discoveredNatDevices.end();i++)
		{
			CDiscoveredNatDevice* dev = *i;
			Heartbeat_Discovered(dev);
		}
	}

	void CUpnp::Heartbeat_Discovering( CDiscoveringNatDevice* dev )
	{
		switch(dev->GetState())
		{
		case Http_Connecting:
			{
				if(dev->GetCurrentStateWorkCount()==0)
				{
					SocketCreateResult r = CFastSocket::Create(SocketType_Tcp/*,dev*/);
					if(!r.socket)
					{
						dev->SetState(Http_ParsingHttpResponse);
						goto L1;
					}
					dev->m_socket = r.socket;
					dev->m_socket->Bind();
					dev->m_socket->SetBlockingMode(false);
					if(!dev->IssueConnect(dev->m_url.GetHostName(), dev->m_url.GetPortNumber()))
					{
						dev->SetState(Http_ParsingHttpResponse);
						goto L1;
					}
				}

				FastSocketSelectContext selectContext;
				selectContext.AddWriteWaiter(*dev->m_socket);
				selectContext.AddExceptionWaiter(*dev->m_socket);

				// 기다리지 않고 폴링한다. 어차피 이 함수는 타이머에 의해 일정 시간마다 호출되니까.
				selectContext.Wait(0);

				SocketErrorCode code;
				bool did = selectContext.GetConnectResult(*dev->m_socket, code);
				if (did)
				{
					if (code == SocketErrorCode_Ok)
					{
						// 오케바리. 이제 패킷을 보내기를 걸자.
						dev->SetState(Http_Requesting);
						goto L1;
					}
					else
					{
						// nonblock socket의 connect에서는 blocked socket에서는 없던
						// '아직 연결 결과가 안나왔는데도 연결 실패라고 오는' 경우가
						// 종종 있다. 이런 것도 이렇게 막아야 한다.
						if (!dev->IssueConnect(dev->m_url.GetHostName(), dev->m_url.GetPortNumber()))
						{
							// 이미 completion된 상태이므로 안전.
							dev->SetState(Http_ParsingHttpResponse);
							goto L1;
						}
					}
				}
				else
				{
					// 아직 연결 결과를 얻지 못했다. 더 기다리자.
				}
			}
			break;

		case Http_Requesting:
			{
				if(dev->GetCurrentStateWorkCount() == 0)
				{
					dev->m_socket->SetBlockingMode(true);

					dev->m_httpRequest.Format("GET %s HTTP/1.1\r\n\r\nHost: %s:%d\r\n\r\nConnection: Keep-Alive\r\n\r\n",
						StringT2A(dev->m_url.GetUrlPath()).GetString(),
						StringT2A(dev->m_url.GetHostName()).GetString(),
						dev->m_url.GetPortNumber());

					if(!dev->IssueRequestSend())
					{
						dev->SetState(Http_ParsingHttpResponse);
						goto L1;
					}
				}

				OverlappedResult ov;
				if(dev->m_socket->GetSendOverlappedResult(0,ov))
				{
					Proud::AssertIsLockedByCurrentThread(m_cs);

					dev->m_sendIssued = false;
					if(ov.m_completedDataLength < 0)
					{
						dev->SetState(Http_ParsingHttpResponse);
						goto L1;
					}
					else
					{
						dev->m_httpRequestIssueOffset += ov.m_completedDataLength;
						int length = dev->m_httpRequest.GetLength() - dev->m_httpRequestIssueOffset + 1;
						if(length<=0)
						{
							dev->SetState(Http_ReponseWaiting);
							goto L1;
						}
						else
							dev->IssueRequestSend();
					}
				}
			}
			break;
		case Http_ReponseWaiting:
			{
				Proud::AssertIsLockedByCurrentThread(m_cs);


				if(dev->GetCurrentStateWorkCount() == 0)
				{
					dev->m_recvIssued = true;
					if(dev->m_socket->IssueRecv(HttpResponseBufferLength) != SocketErrorCode_Ok)
					{
						dev->m_recvIssued = false;
						dev->SetState(Http_Disposing);
						goto L1;
					}
				}

				OverlappedResult ov;
				if(dev->m_socket->GetRecvOverlappedResult(0,ov))
				{
					dev->m_recvIssued = false;
					if(ov.m_completedDataLength <= 0)
					{
						dev->SetState(Http_ParsingHttpResponse);
						goto L1;
					}
					else
					{
						dev->m_httpResponseBuffer.AddRange(dev->m_socket->GetRecvBufferPtr(),ov.m_completedDataLength);
						dev->m_httpResponseIssueOffset += ov.m_completedDataLength;
						int length = HttpResponseBufferLength - dev->m_httpResponseIssueOffset;
						if(length<=0)
						{
							dev->SetState(Http_ParsingHttpResponse);
							goto L1;
						}
						else
						{
							dev->m_recvIssued = true;
							if(dev->m_socket->IssueRecv(length) != SocketErrorCode_Ok)
							{
								dev->m_recvIssued = false;
								dev->SetState(Http_Disposing);
								goto L1;
							}
						}
					}
				}
			}
			break;
		case Http_ParsingHttpResponse:
			// parse HTTP response
			if(dev->m_httpResponseBuffer.GetCount() == 0)
			{
				// do nothing
			}
			else
			{
				{
					// ikpil.choi 2016.11.07 : memory safe 코드로 변경
					//StrBufA httpResponseBuf(dev->m_httpResponse, (int)(dev->m_httpResponseBuffer.GetCount()+1));
					//ZeroMemory(httpResponseBuf, dev->m_httpResponseBuffer.GetCount()+1);
					//memcpy(httpResponseBuf, dev->m_httpResponseBuffer.GetData(), dev->m_httpResponseBuffer.GetCount());
					int bufferSize = (int)(dev->m_httpResponseBuffer.GetCount() + 1);
					StrBufA httpResponseBuf(dev->m_httpResponse, bufferSize);
					memset_s(httpResponseBuf.GetBuf(), bufferSize, 0, bufferSize);
					memcpy_s(httpResponseBuf.GetBuf(), bufferSize - 1, dev->m_httpResponseBuffer.GetData(), dev->m_httpResponseBuffer.GetCount());
				}

				StringA natdeviceName, controlUrl;

				// parse HTTP response

				natdeviceName = GetXmlNodeValue(dev->m_httpResponse,"FRIENDLYNAME") + StringA("##") + GetXmlNodeValue(dev->m_httpResponse,"MODELDESCRIPTION");
				controlUrl = GetXmlNodeValueAfter(dev->m_httpResponse,"CONTROLURL", "WANIPCONNECTION");

				if(natdeviceName.GetLength()>2)
				{
					// NAT device를 완전히 찾았다. 새 항목을 추가하자.
					CDiscoveredNatDevicePtr newNat(new CDiscoveredNatDevice);
					newNat->m_url = dev->m_url; // copy
					newNat->m_deviceName = StringA2T(natdeviceName);
					newNat->m_controlUrl = controlUrl;
					m_discoveredNatDevices.AddTail(newNat);
				}
			}

			dev->SetState(Http_Disposing);
			goto L1;
			break;

		case Http_Disposing:
			// goto disposed state if no issue is guaranteed
			{
				Proud::AssertIsLockedByCurrentThread(m_cs);
				if(dev->GetCurrentStateWorkCount() == 0)
				{
					if(dev->m_socket)
					{
						dev->m_socket->CloseSocketOnly();
					}
				}

				OverlappedResult ov;
				if(dev->m_socket->GetRecvOverlappedResult(0,ov))
					dev->m_recvIssued = false;

				if(dev->m_socket->GetSendOverlappedResult(0,ov))
					dev->m_sendIssued = false;

				if(!dev->m_recvIssued && !dev->m_sendIssued)
				{
					dev->SetState(Http_Disposed);
					goto L1;
				}
			}
			break;
		}

		dev->IncCurrentStateWorkCount();
L1:
		;
	}

	Proud::StringA CUpnp::GetXmlNodeValue(const StringA &xmlDoc, const StringA &nodeName)
	{
		// find by this capitalized text then trim with original one
		StringA xmlDocCap = xmlDoc; xmlDocCap.MakeUpper();
		StringA nodeNameCap = nodeName; nodeNameCap.MakeUpper();

		StringA nodeTag1Cap;
		nodeTag1Cap.Format("<%s>", (const char*)nodeNameCap.GetString());

		int valueIndex1 = xmlDocCap.Find(nodeTag1Cap.GetString());
		if (valueIndex1 >= 0)
		{
			valueIndex1 += nodeTag1Cap.GetLength();
		}

		StringA nodeTag2Cap;
		nodeTag2Cap.Format("</%s>", nodeNameCap.GetString());
		int valueIndex2 = xmlDocCap.Find(nodeTag2Cap.GetString());

		if (valueIndex1 >= 0 && valueIndex2 >= 0 && valueIndex2 > valueIndex1)
		{
			return (xmlDoc.Mid(valueIndex1, valueIndex2 - valueIndex1));
		}
		else
			return StringA();
	}

	// findAfter 문구 뒤에서부터 검색해서, 특정 node를 찾는다.
	Proud::StringA CUpnp::GetXmlNodeValueAfter( const StringA &xmlDoc, const StringA &nodeName, const StringA &findAfter )
	{
		// find by this capitalized text then trim with original one
		StringA xmlDocCap = xmlDoc; xmlDocCap.MakeUpper();
		StringA nodeNameCap = nodeName; nodeNameCap.MakeUpper();
		StringA findAfterCap = findAfter; findAfterCap.MakeUpper();

		int valueIndex0 = xmlDocCap.Find(findAfterCap.GetString());
		if(valueIndex0 < 0)
			return "";

		StringA xmlDoc2Cap = xmlDocCap.Right(xmlDocCap.GetLength() - valueIndex0);
		StringA xmlDoc2 = xmlDoc.Right(xmlDoc.GetLength() - valueIndex0);

		StringA nodeTag1Cap;
		nodeTag1Cap.Format("<%s>",(const char*)nodeNameCap.GetString());

		int valueIndex1 = xmlDoc2Cap.Find(nodeTag1Cap.GetString());
		if(valueIndex1 >= 0)
		{
			valueIndex1 += nodeTag1Cap.GetLength();
		}

		StringA nodeTag2Cap;
		nodeTag2Cap.Format("</%s>",(const char*)nodeNameCap.GetString());
		int valueIndex2 = xmlDoc2Cap.Find(nodeTag2Cap.GetString());

		if(valueIndex1 >=0 && valueIndex2>=0 && valueIndex2 > valueIndex1)
		{
			return (xmlDoc2.Mid(valueIndex1, valueIndex2 - valueIndex1));
		}
		else
			return StringA();
	}

	// 소켓을 모두 닫고 모든 pended i/o를 강제 중단시킨다.
	// WaitUntil...함수에서 이 요청에 의한 completion이 끝난 후에 dtor 실행되어야 크래시가 안남.
	void CUpnp::IssueTermination()
	{
		CriticalSectionLock lock(m_cs,true);
		/* DeletePortMapping 명령은 웬만하면 반드시 모두 실행된 후에 게임 클라 프로세스가 종료되어야 한다.
		이를 위패 DeletePortMapping task가 남아 있는 한 issue termination을 유보해야 한다.
		그러나 마냥 유보할 수는 없으므로 일정 시간 기다리도록 하자.
		*/
		int deletePortMappingTaskCount = m_deletePortMappingTaskCount;
		lock.Unlock();

		if(deletePortMappingTaskCount > 0)
			Sleep(1);

		// 이제 실제로 종료하는 과정을 수행
		lock.Lock();

		for(SsdpContexts::iterator i = m_ssdpContexts.begin();i!=m_ssdpContexts.end();i++)
		{
			if((*i)->m_ssdpSocket)
			{
				(*i)->m_ssdpSocket->CloseSocketOnly();
			}
		}

		// Discovering NAT devices에 대한 마무리 처리
		for (DiscoveriingNatDevices::iterator i = m_discoveringNatDevices.begin(); i != m_discoveringNatDevices.end(); i++)
		{
			CDiscoveringNatDevice* dev = i->GetSecond();
			dev->IssueTermination();
		}

		// upnp task도 모두 종료 지시해야.
		for(DiscoveredNatDevices::iterator i = m_discoveredNatDevices.begin();i!=m_discoveredNatDevices.end();i++)
		{
			CDiscoveredNatDevice* dev = *i;
			dev->IssueTermination();
		}
	}

	// IssueTermination의 마무리
	void CUpnp::WaitUntilNoIssueGuaranteed()
	{
		while(1)
		{
			CriticalSectionLock lock(m_cs,true);

			// 프로세스 종료중이면 아무것도 하지 않는다.
			if(Thread::m_dllProcessDetached_INTERNAL)
			{
				return;
			}

			OverlappedResult ov;
			for(SsdpContexts::iterator i = m_ssdpContexts.begin();i!=m_ssdpContexts.end();i++)
			{
				CSsdpContext* sc = *i;
				if(sc->m_ssdpSocket)
				{
					if(sc->m_ssdpSocket->GetRecvOverlappedResult(0,ov))
						sc->m_recvIssued = false;
					if(sc->m_ssdpSocket->GetSendOverlappedResult(0,ov))
						sc->m_sendIssued = false;
				}

				if(!sc->IsSafeDisposeGuaranteed())
					goto L1;
			}

			Proud::AssertIsLockedByCurrentThread(m_cs);

			// Discovering NAT devices에 대한 마무리 처리
			for(DiscoveriingNatDevices::iterator i=m_discoveringNatDevices.begin();i!=m_discoveringNatDevices.end();i++)
			{
				CDiscoveringNatDevice* dev = i->GetSecond();
				if(!dev->IsSafeDisposeGuaranteed())
					goto L1;
			}

			// Discovered NAT devices에 대한 마무리 처리
			for(DiscoveredNatDevices::iterator i=m_discoveredNatDevices.begin();i!=m_discoveredNatDevices.end();i++)
			{
				CDiscoveredNatDevice* dev = *i;
				if(!dev->IsSafeDisposeGuaranteed())
					goto L1;
			}

			return;
L1:
			lock.Unlock();
			Sleep(10);
		}
	}

	void CUpnp::StopAllDiscoveryWorksOnNeed()
	{
		CriticalSectionLock lock(m_cs,true);

		// stop all if SSDP works for a very long time
		if(GetPreciseCurrentTimeMs() - m_startTime > 60 * 10 * 1000)
		{
			for(SsdpContexts::iterator i = m_ssdpContexts.begin();i!=m_ssdpContexts.end();i++)
			{
				CSsdpContextPtr ctx = *i;
				if(ctx->m_ssdpSocket)
				{
					ctx->m_ssdpSocket->CloseSocketOnly();
				}
			}

			for(DiscoveriingNatDevices::iterator i=m_discoveringNatDevices.begin();i!=m_discoveringNatDevices.end();i++)
			{
				i->GetSecond()->SetState(Http_Disposing);
			}

		}
	}

	void CUpnp::InitSsdp()
	{
		CriticalSectionLock lock(m_cs,true);

		CFastArray<String> addrs;
		CNetUtil::GetLocalIPAddresses(addrs);

		// 설마
		if(addrs.GetCount()==0)
			addrs.Add(String());

		for(int i=0;i<(int)addrs.GetCount();i++)
		{
			CSsdpContextPtr sc(new CSsdpContext);
			sc->m_localAddr = addrs[i];
			m_ssdpContexts.Add(sc);
		}
	}

	// 내부-외부 포트 매핑 후 매핑.
	// 비동기로 수행됨.
	void CUpnp::AddTcpPortMapping( AddrPort lanAddr, AddrPort wanAddr, bool isTcp )
	{
		CriticalSectionLock lock(m_cs,true);

		CDiscoveredNatDevice* dev = GetCommandAvailableNatDevice();
		if(dev==NULL)
			return;

		// 공유기의 task queue에, TCP연결-송신-수신-완료 과정을 수행할 첫 issue를 건다.
		// 건 이슈는 netclient manager worker thread에 의해서 issue connect가 될 것이다. 이를 위한 heartbeat도 이미 작동하고 있다.
		CUpnpTaskPtr task(new CUpnpTask);

		task->m_ownerUpnp = this;
		task->m_ownerDevice = dev;
		task->m_type = UpnpTask_AddPortMapping;
		task->m_isTcp = isTcp;
		task->m_lanAddr = lanAddr;
		task->m_wanAddr = wanAddr;
		task->m_state = Http_Init;

		dev->m_tasks.AddTail(task);
	}

	void CUpnp::Heartbeat_Discovered( CDiscoveredNatDevice* dev )
	{
		for (CFastList2<CUpnpTaskPtr,int>::iterator i = dev->m_tasks.begin();i!=dev->m_tasks.end();)
		{
			CUpnpTask* task = *i;
			Heartbeat_Task(task);
			if(task->m_state == Http_Disposed)
			{
				if(task->m_type == UpnpTask_DeletePortMapping)
				{
					m_deletePortMappingTaskCount--;

					i = dev->m_tasks.erase(i);
				}
				else
					i++;
			}
		}
	}

	void CUpnp::Heartbeat_Task( CUpnpTask* task )
	{
		switch(task->m_state)
		{
		case Http_Init:
			Heartbeat_Task_InitState(task);
			break;
		case Http_Connecting:
			Heartbeat_Task_Connecting(task);
			break;
		case Http_Requesting:
			Heartbeat_Task_Requesting(task);
			break;
		case Http_ReponseWaiting:
			Heartbeat_Task_ResponseWaiting(task);
			break;
		case Http_ParsingHttpResponse:
			Heartbeat_Task_ParsingHttpResponse(task);
			break;
		case Http_Disposing:
			task->m_state = Http_Disposed;
			break;
		}
	}

	void CUpnp::Heartbeat_Task_InitState( CUpnpTask* task )
	{
		// 라우터로의 비동기 연결을 수행할 소켓을 준비 땅.
		SocketCreateResult r = CFastSocket::Create(SocketType_Tcp/*, task*/);
		if (!r.socket)
		{
			task->m_state = Http_Disposing;
			return;
		}
		task->m_socket = r.socket;
		if (SocketErrorCode_Ok  != task->m_socket->Bind())
		{
			task->m_state = Http_Disposing;
			return;
		}

		task->m_socket->SetBlockingMode(false);
		if(!task->IssueConnect(task->m_ownerDevice->m_url.GetHostName(), task->m_ownerDevice->m_url.GetPortNumber()))
		{
			task->m_state = Http_Disposing;
			return;
		}

		task->m_state = Http_Connecting;
	}

	void CUpnp::Heartbeat_Task_Connecting( CUpnpTask* task )
	{
		FastSocketSelectContext selectContext;
		selectContext.AddWriteWaiter(*task->m_socket);
		selectContext.AddExceptionWaiter(*task->m_socket);

		// 기다리지 않고 폴링한다. 어차피 이 함수는 타이머에 의해 일정 시간마다 호출되니까.
		selectContext.Wait(0);

		SocketErrorCode code;
		bool did = selectContext.GetConnectResult(*task->m_socket, code);
		if (did)
		{
			if (code == SocketErrorCode_Ok)
			{
				// 연결 성공. 이제 AddPortMapping SOAP 명령 송신을 걸자.
				task->m_socket->SetBlockingMode(true);

				task->BuildUpnpRequestText();

				if(!task->IssueRequestSend())
				{
					task->m_state = Http_Disposing;
					return;
				}

				// 첫 보내기 이슈 성공.
				task->m_state = Http_Requesting;
				return;
			}
			else
			{
				// nonblock socket의 connect에서는 blocked socket에서는 없던
				// '아직 연결 결과가 안나왔는데도 연결 실패라고 오는' 경우가
				// 종종 있다. 이런 것도 이렇게 막아야 한다.
				if (!task->IssueConnect(task->m_ownerDevice->m_url.GetHostName(), task->m_ownerDevice->m_url.GetPortNumber()))
				{
					// 이미 completion된 상태이므로 안전.
					task->m_state = Http_Disposing;
					return;
				}
			}
		}
		else
		{
			// 아직 연결 결과를 얻지 못했다. 더 기다리자.
		}

	}

	void CUpnp::Heartbeat_Task_Requesting( CUpnpTask* task )
	{
		OverlappedResult ov;
		if(task->m_socket->GetSendOverlappedResult(0,ov))
		{
			Proud::AssertIsLockedByCurrentThread(m_cs);

			task->m_sendIssued = false;
			if(ov.m_completedDataLength < 0)
			{
				// 오류 처리
				task->m_state = Http_Disposing;
				return;
			}
			else
			{
				task->m_httpRequestIssueOffset += ov.m_completedDataLength;
				int length = task->m_httpRequest.GetLength() - task->m_httpRequestIssueOffset + 1;
				if(length <= 0)
				{
					// 다 보냈다. 이제 수신을 하자.
					task->m_recvIssued = true;
					if(task->m_socket->IssueRecv(HttpResponseBufferLength) != SocketErrorCode_Ok)
					{
						task->m_recvIssued = false;

						// 오류 처리
						task->m_state = Http_Disposing;
						return;
					}

					task->m_state = Http_ReponseWaiting;
					return;
				}
				else
				{
					// 마저 더 보낼 것을 보내기
					task->IssueRequestSend();
				}
			}
		}
	}

	void CUpnp::Heartbeat_Task_ResponseWaiting( CUpnpTask* task )
	{
		OverlappedResult ov;
		if(task->m_socket->GetRecvOverlappedResult(0,ov))
		{
			task->m_recvIssued = false;
			if(ov.m_completedDataLength <= 0)
			{
				// 수신이 끝났거나 오류가 났다. 분석 단계로 들어가자.
				task->m_state = Http_ParsingHttpResponse;
				return;
			}
			else
			{
				// 다음 수신 이슈를 또 건다.
				task->m_httpResponseBuffer.AddRange(task->m_socket->GetRecvBufferPtr(),ov.m_completedDataLength);
				task->m_httpResponseIssueOffset += ov.m_completedDataLength;
				int length = HttpResponseBufferLength - task->m_httpResponseIssueOffset;
				if(length<=0)
				{
					// 수신이 끝났거나 오류가 났다. 분석 단계로 들어가자.
					task->m_state = Http_ParsingHttpResponse;
					return;
				}
				else
				{
					task->m_recvIssued = true;
					if(task->m_socket->IssueRecv(length) != SocketErrorCode_Ok)
					{
						// 수신이 끝났거나 오류가 났다. 분석 단계로 들어가자.
						task->m_state = Http_ParsingHttpResponse;
						return;
					}
				}
			}
		}
	}

	void CUpnp::Heartbeat_Task_ParsingHttpResponse( CUpnpTask* task )
	{
		//m_httpResponse에 받은 메시지를 문자열 형태로 채운다.
		{
			// ikpil.choi 2016.11.07 : memory safe 코드로 변경
			int buuferSize = (int)task->m_httpResponseBuffer.GetCount() + 1;
			StrBufA httpResponseBuf(task->m_httpResponse, buuferSize);
			//ZeroMemory(httpResponseBuf,task->m_httpResponseBuffer.GetCount()+1); // sz sure
			//memcpy(httpResponseBuf,task->m_httpResponseBuffer.GetData(),task->m_httpResponseBuffer.GetCount());
			memset_s(httpResponseBuf.GetBuf(), buuferSize, 0, buuferSize);
			memcpy_s(httpResponseBuf.GetBuf(), buuferSize - 1, task->m_httpResponseBuffer.GetData(), task->m_httpResponseBuffer.GetCount());
		}

		// 응답을 다 받았다. 이제 분석을 하자.
		// parse HTTP response

		// 현재까지는 딱히 분석할게 없다.
		task->m_state = Http_Disposing;
	}

	// upnp port mapping 항목의 이름을 지정한다.
	Proud::StringA CUpnp::GetPortMappingRandomDesc()
	{
		// 문자열이 너무 크면 IPTIME 공유기에서 bad request라는 오류를 낸다. 따라서 이정도 크기로만.
		StringA ret;
		ret.Format("PrNet_%u", GetTickCount());
		return ret;

		//		String uuidStr;
		//		Guid::ConvertUUIDToString(Win32Guid::NewGuid(), uuidStr);
		//
		//		StringA ret = "PrNet_" + StringT2A(uuidStr);
		//		return ret;
	}

	void CUpnp::DeleteTcpPortMapping( AddrPort lanAddr, AddrPort wanAddr, bool isTcp )
	{
		CriticalSectionLock lock(m_cs,true);

		// 두 개 이상의 upnp 공유기가 감지되면 하지 말자. TODO: default gateway를 찾은 후 하는 것이 정석.
		if(m_discoveredNatDevices.GetCount() != 1 )
			return;

		CDiscoveredNatDevice* dev = *(m_discoveredNatDevices.begin());

		// 공유기의 task queue에, TCP연결-송신-수신-완료 과정을 수행할 첫 issue를 건다.
		// 건 이슈는 netclient manager worker thread에 의해서 issue connect가 될 것이다. 이를 위한 heartbeat도 이미 작동하고 있다.
		CUpnpTaskPtr task(new CUpnpTask);

		task->m_ownerUpnp = this;
		task->m_ownerDevice = dev;
		task->m_type = UpnpTask_DeletePortMapping;
		task->m_isTcp = isTcp;
		task->m_lanAddr = lanAddr;
		task->m_wanAddr = wanAddr;
		task->m_state = Http_Init;

		dev->m_tasks.AddTail(task);

		m_deletePortMappingTaskCount++;
	}

	CDiscoveredNatDevice* CUpnp::GetCommandAvailableNatDevice()
	{
		// 두 개 이상의 upnp 공유기가 감지되면 하지 말자. TODO: default gateway를 찾은 후 하는 것이 정석.
		if(m_discoveredNatDevices.GetCount() != 1 )
			return NULL;

		return  *(m_discoveredNatDevices.begin());
	}

	CUpnp::~CUpnp()
	{
		Reset();
	}

	// upnp 정보 얻은 것들을 무효화하고 다시 ssdp 탐색 과정부터 시작한다.
	// local ip가 바뀐다거나 ACR 기능을 작동해야 하는 경우 이것이 콜 됨
	// 주의: async i/o가 모두 중단될 때까지 블러킹된다. 비록 밀리초 단위지만.
	void CUpnp::Reset()
	{
		CriticalSectionLock lock(m_cs,true);

		IssueTermination();
		WaitUntilNoIssueGuaranteed();

		m_heartbeatCount = 0;
		m_startTime = 0;
		m_deletePortMappingTaskCount = 0;

		m_discoveringNatDevices.Clear();
		m_discoveredNatDevices.Clear();
		m_ssdpContexts.Clear();
	}

	bool CDiscoveringNatDevice::TakeResponse( StringA response )
	{
		StringA xmlLocation;

		int left = response.Find("http");
		int right = response.Find(".xml");
		if(left<right && left>=0)
		{
			xmlLocation = response.Mid(left,right - left + 4);
		}

		// IP address 등을 추출한다.
		if(!m_url.CrackUrl(StringA2T(xmlLocation).GetString()))
			return false;

		if (String(m_url.GetHostName()) == _PNT(""))
			return false;

#ifdef _DEBUG
		String hostName = m_url.GetHostName(); // 192.168.77.1
		String path = m_url.GetUrlPath(); // /gatedesc.xml
#endif

		return true;
	}

	void CDiscoveringNatDevice::OnSocketWarning(CFastSocket* /*socket*/, String /*msg*/)
	{
	}

	CDiscoveringNatDevice::~CDiscoveringNatDevice()
	{
		/*if(m_socket)
			NTTNTRACE("CDiscoveringNatDevice.dtor. HTTP request so far: %d\n",m_httpResponse.GetLength());
		else
			NTTNTRACE("CDiscoveringNatDevice.dtor. No Socket.\n");*/
	}

	CDiscoveringNatDevice::CDiscoveringNatDevice()
	{
		m_currentStateWorkCount_USE_FUNC = 0;
		m_state_USE_FUNC = Http_Connecting;
	}

	void CDiscoveringNatDevice::SetState( HttpState val )
	{
		if(val > m_state_USE_FUNC) // 과거 상태로의 천이 금지
		{
			m_currentStateWorkCount_USE_FUNC = 0;
			m_state_USE_FUNC = val;
		}
	}

	void CDiscoveringNatDevice::IssueTermination()
	{
		if (m_socket != NULL)
		{
			m_socket->CloseSocketOnly();
			if (IsSafeDisposeGuaranteed())
				SetState(Http_Disposing);
		}
	}

	bool CDiscoveredNatDevice::IsSafeDisposeGuaranteed()
	{
		for(CFastList2<CUpnpTaskPtr,int>::iterator i = m_tasks.begin();i!=m_tasks.end();i++)
		{
			CUpnpTask* task = *i;
			if(!task->IsSafeDisposeGuaranteed())
			{
				return false;
			}
		}
		return true;
	}

	void CDiscoveredNatDevice::IssueTermination()
	{
		for(CFastList2<CUpnpTaskPtr,int>::iterator i=m_tasks.begin();i!=m_tasks.end();i++)
		{
			CUpnpTask* task = *i;
			if(task->m_socket != NULL)
			{
				task->m_socket->CloseSocketOnly();
				if(IsSafeDisposeGuaranteed())
					task->m_state = Http_Disposing;
			}
		}
	}

	bool CHttpSession::IssueConnect(String hostName, int portNumber)
	{
		AddrPort serverAddrPort;
		AddrInfo addrInfo;
		if (DnsForwardLookupAndGetPrimaryAddress(hostName.GetString(), (uint16_t)portNumber, addrInfo) != SocketErrorCode_Ok)
		{
			return false;
		}

		serverAddrPort.FromNative(addrInfo.m_sockAddr);

		SocketErrorCode r = m_socket->Connect(serverAddrPort);
		if (r != SocketErrorCode_Ok && !CFastSocket::IsWouldBlockError(r))
		{
			return false;
		}

		return true;
	}

	bool CHttpSession::IssueRequestSend()
	{
		Proud::AssertIsLockedByCurrentThread(m_ownerUpnp->m_cs);

		int offset = m_httpRequestIssueOffset;
		int length = m_httpRequest.GetLength() - offset + 1;
		m_sendIssued = true;
		if (m_socket->IssueSend((uint8_t*)(const char*)(m_httpRequest.GetString())+offset, length) != SocketErrorCode_Ok)
		{
			m_sendIssued = false;
			return false;
		}
		else
		{
			return true;
		}
	}

	bool CHttpSession::IsSafeDisposeGuaranteed()
	{
		if(m_socket==NULL)
			return true;
		if(!m_socket->IsClosed())
			return false;
		return (!m_sendIssued && !m_recvIssued);
	}

	void CSsdpContext::OnSocketWarning(CFastSocket* /*socket*/, String /*msg*/)
	{
	}

	CSsdpContext::CSsdpContext()
	{
		m_sendIssued = false;
		m_recvIssued = false;
		m_ssdpRequestCount = 0;
		m_ssdpResponseWaitStartTime = 0;
		m_ssdpState = Ssdp_None;
	}

	bool CSsdpContext::IsSafeDisposeGuaranteed()
	{
		if(m_ssdpSocket == NULL)
			return true;
		if(!m_ssdpSocket->IsClosed())
			return false;
		return (!m_sendIssued && !m_recvIssued);
	}

	void CUpnpTask::OnSocketWarning(CFastSocket* /*socket*/, String /*msg*/)
	{
	}

	void CUpnpTask::BuildUpnpRequestText()
	{
		switch(m_type)
		{
		case UpnpTask_AddPortMapping:
			BuildUpnpRequestText_AddPortMapping();
			break;
		case UpnpTask_DeletePortMapping:
			BuildUpnpRequestText_DeletePortMapping();
			break;
		default:
			assert(0);   // 이보쇼, 새 request 구문을 만들어야제!
			m_state = Http_Disposing;
			break;
	}

}
	// upnp 명령을 생성해서 송신큐에 설정
	void CUpnpTask::BuildUpnpRequestText_AddPortMapping()
	{
		CDiscoveredNatDevice* dev = m_ownerDevice;

		StringA soapCmdToken[20];
		soapCmdToken[0] =
			"<?xml version=\"1.0\"?>\r\n"
			"<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" "
			"SOAP-ENV:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><SOAP-ENV:Body>"
			"<m:AddPortMapping xmlns:m=\"urn:schemas-upnp-org:service:WANIPConnection:1\">"
			"<NewRemoteHost xmlns:dt=\"urn:schemas-microsoft-com:datatypes\" dt:dt=\"string\">"
			"</NewRemoteHost><NewExternalPort xmlns:dt=\"urn:schemas-microsoft-com:datatypes\" dt:dt=\"ui2\">";
		soapCmdToken[1].Format("%d",m_wanAddr.m_port);
		soapCmdToken[2] =
			"</NewExternalPort><NewProtocol xmlns:dt=\"urn:schemas-microsoft-com:datatypes\" dt:dt=\"string\">";
		soapCmdToken[3] = m_isTcp? "TCP" : "UDP";
		soapCmdToken[4] =
			"</NewProtocol><NewInternalPort xmlns:dt=\"urn:schemas-microsoft-com:datatypes\" dt:dt=\"ui2\">";
		soapCmdToken[5].Format("%d", m_lanAddr.m_port);
		soapCmdToken[6] =
			"</NewInternalPort><NewInternalClient xmlns:dt=\"urn:schemas-microsoft-com:datatypes\" dt:dt=\"string\">";
		soapCmdToken[7] = StringT2A(m_lanAddr.IPToString());
		soapCmdToken[8] =
			"</NewInternalClient>"
			"<NewEnabled xmlns:dt=\"urn:schemas-microsoft-com:datatypes\" dt:dt=\"boolean\">1</NewEnabled>"
			"<NewPortMappingDescription xmlns:dt=\"urn:schemas-microsoft-com:datatypes\" dt:dt=\"string\">";
		soapCmdToken[9] = CUpnp::GetPortMappingRandomDesc();
		soapCmdToken[10] =
			"</NewPortMappingDescription><NewLeaseDuration xmlns:dt=\"urn:schemas-microsoft-com:datatypes\" dt:dt=\"ui4\">0</NewLeaseDuration></m:AddPortMapping></SOAP-ENV:Body></SOAP-ENV:Envelope>";

		StringA soapCmd;
		for(int i=0;i<20;i++)
		{
			soapCmd += soapCmdToken[i];
		}

		StringA header;
		header.Format(
			"POST %s HTTP/1.1\r\n"
			"Cache-Control: no-cache\r\n"
			"Connection: Close\r\n"
			"Pragma: no-cache\r\n"
			"Content-Type: text/xml; charset=\"utf-8\"\r\n"
			"User-Agent: Microsoft-Windows/6.1 UPnP/1.0\r\n"
			"SOAPAction: \"urn:schemas-upnp-org:service:WANIPConnection:1#AddPortMapping\"\r\n"
			"Content-Length: %d\r\n"
			"Host: %s:%d\r\n\r\n",
			dev->m_controlUrl.GetString(),
			soapCmd.GetLength(),
			StringW2A(dev->m_url.GetHostName()).GetString(),
			dev->m_url.GetPortNumber());

		m_httpRequest = header + soapCmd;
	}

	// upnp 명령을 생성해서 송신큐에 설정
	void CUpnpTask::BuildUpnpRequestText_DeletePortMapping()
	{
		CDiscoveredNatDevice* dev = m_ownerDevice;

		StringA soapCmdToken[20];
		soapCmdToken[0] =
			"<?xml version=\"1.0\"?>"
			"<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" "
			"SOAP-ENV:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
			"<SOAP-ENV:Body><m:DeletePortMapping xmlns:m=\"urn:schemas-upnp-org:service:WANIPConnection:1\">"
			"<NewRemoteHost xmlns:dt=\"urn:schemas-microsoft-com:datatypes\" dt:dt=\"string\"></NewRemoteHost>"
			"<NewExternalPort xmlns:dt=\"urn:schemas-microsoft-com:datatypes\" dt:dt=\"ui2\">";

		soapCmdToken[1].Format("%d",m_wanAddr.m_port);
		soapCmdToken[2] =
			"</NewExternalPort><NewProtocol xmlns:dt=\"urn:schemas-microsoft-com:datatypes\" dt:dt=\"string\">";
		soapCmdToken[3] = m_isTcp? "TCP" : "UDP";
		soapCmdToken[4] =
			"</NewProtocol></m:DeletePortMapping></SOAP-ENV:Body></SOAP-ENV:Envelope>";

		StringA soapCmd;
		for(int i=0;i<20;i++)
		{
			soapCmd += soapCmdToken[i];
		}

		StringA header;
		header.Format(
			"POST %s HTTP/1.1\r\n"
			"Cache-Control: no-cache\r\n"
			"Connection: Close\r\n"
			"Pragma: no-cache\r\n"
			"Content-Type: text/xml; charset=\"utf-8\"\r\n"
			"User-Agent: Microsoft-Windows/6.1 UPnP/1.0\r\n"
			"SOAPAction: \"urn:schemas-upnp-org:service:WANIPConnection:1#DeletePortMapping\"\r\n"
			"Content-Length: %d\r\n"
			"Host: %s:%d\r\n\r\n",
			(const char*)dev->m_controlUrl.GetString(),
			soapCmd.GetLength(),
			(const char*)StringW2A(dev->m_url.GetHostName()).GetString(),
			dev->m_url.GetPortNumber());

		m_httpRequest = header + soapCmd;
	}
#endif
}
