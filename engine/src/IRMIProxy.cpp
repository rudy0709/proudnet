/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <new>
#include "../include/IRMIProxy.h"
#include "../include/Message.h"
#include "../include/RMIContext.h"
#include "SendFragRefs.h"
#include "../include/sysutil.h"
#include "../include/MessageSummary.h"
#include "enumimpl.h"
#include "MessagePrivateImpl.h"
#include "SendOpt.h"
#include "SendFragRefs.h"
#include "FavoriteLV.h"

namespace Proud
{
	const PNTCHAR* ProxyBadSignatureErrorText = _PNT("PIDL compiler is incompatible to this version! Make sure that PIDL compiler is up to date, rebuild-all is sometimes recommended.");

	IRmiProxy::IRmiProxy()
	{
		m_signature = 1;//depot/R2/ProudNet/src/IRMIProxy.cpp
		m_core = NULL;
		m_internalUse = false;
		m_enableNotifySendByProxy = true;
	}

	IRmiProxy::~IRmiProxy(void)
	{
		if (m_core)
		{
			ShowUserMisuseError(_PNT("RMI Proxy which is still in use by ProudNet core cannot be destroyed! Destroy CNetClient or CNetServer instance first."));
		}
	}

	void IRmiProxy::NotifySendByProxy(const HostID* /*remotes*/, int /*remoteCount*/, const MessageSummary &/*summary*/, RmiContext &/*rmiContext*/, const CMessage &/*msg*/)
	{
	}

	/** NotifySendByProxy 에 RmiID추가 요청으로 RmiSend에도 RmiID를 추가한다.*/
	bool IRmiProxy::RmiSend(const HostID* remotes, int remoteCount, RmiContext &rmiContext, const CMessage &msg, const PNTCHAR* RMIName, RmiID RMIId)
	{
		bool ret;

		if(m_core==NULL)
		{
			ShowUserMisuseError(_PNT("ProudNet RMI Proxy is not attached yet!"));
			return false;
		}
		else
		{
			rmiContext.AssureValidation();

			CSmallStackAllocMessage header;
			Message_Write(header, MessageType_Rmi);

			CSendFragRefs fragRefs;
			fragRefs.Add(header);
			fragRefs.Add(msg);

			int compressedPayloadLength = 0;
			SendOpt sendOpt = SendOpt::CreateFromRmiContextAndClearRmiContextSendFailedRemotes(rmiContext);

			ret = m_core->Send(fragRefs,
				sendOpt,
				remotes, remoteCount,compressedPayloadLength);

			// RMI를 호출했다는 이벤트를 콜백한다.
			if(!m_internalUse)
			{
				MessageSummary msgSumm;
				msgSumm.m_payloadLength = fragRefs.GetTotalLength();
				msgSumm.m_rmiID = RMIId;
				msgSumm.m_rmiName = RMIName;
				msgSumm.m_encryptMode = rmiContext.m_encryptMode;
				msgSumm.m_compressMode = rmiContext.m_compressMode;
				msgSumm.m_compressedPayloadLength = compressedPayloadLength;

				if(m_enableNotifySendByProxy)
				{
					NotifySendByProxy(remotes, remoteCount, msgSumm, rmiContext, msg);
				}
				// m_core->Viz_NotifySendByProxy(remotes, remoteCount, msgSumm, rmiContext);
			}

			return ret;
		}
	}
}
