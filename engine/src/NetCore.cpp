/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <new>
#include <stack>
#include "../include/INetCoreEvent.h"
#include "../include/MessageSummary.h"
//#include "../include/ErrorInfo.h"
#include "../include/IRMIProxy.h"
#include "../include/IRMIStub.h"
#include "../include/quicksort.h"
#include "../include/BasicTypes.h"
#include "NetCore.h"
//#include "ThreadPoolImpl.h"
//#include "TimeEventThreadPool.h"
#include "NetUtil.h"
#ifdef VIZAGENT
	#include "VizAgent.h"
#endif
//222-원본: #include "./zlib/zlib.h"
#include "../OpenSources/zlib-v1.2.8/zlib.h"
#include "UseZlib.h"
#include "Functors.h"
#include "ExceptionImpl.h"

//#include "./zlib/compress.c"

#include "NetS2C_common.cpp"
#include "NetC2S_common.cpp"
#include "NetC2C_common.cpp"

// #include "LanS2C_common.cpp"
// #include "LanC2S_common.cpp"
// #include "LanC2C_common.cpp"
//#include "SendOpt.h"
//#include "SuperSocket.h"
//#include "TimeAlarm.h"
#include "UseCount.h"
#include "SendFragRefs.h"
#include "RemoteClient.h"
#include "MessagePrivateImpl.h"
#include "NumUtil.h"
//#include "../UtilSrc/DebugUtil/DebugCommon.h"
//#include "../UtilSrc/DebugUtil/NetworkAnalyze.h"

namespace Proud
{
//	marker_series g_mySeries;

	const PNTCHAR* DuplicatedRmiIDErrorText = _PNT("Duplicated RMI ID is found. Review RMI ID declaration in .PIDL files.");
	// 내부 RMI는 60000 이상 값을 쓴다. 사용자는 이외 범위를 써야 한다.
	const PNTCHAR* BadRmiIDErrorText = _PNT("Wrong RMI ID is found. RMI ID should be < 60000.");
	const PNTCHAR* AsyncCallbackMayOccurErrorText = _PNT("Already async callback may occur! server start or client connection should have not been done before here!");
	const PNTCHAR* RmiInterfaceErrorText = _PNT("IRmiProxy or IRmiStub pointer is invalid.");

	void CNetCoreImpl::AttachProxy( IRmiProxy *proxy )
	{
		if(AsyncCallbackMayOccur())
			throw Exception(AsyncCallbackMayOccurErrorText);

		// 만약 중복되는 ID가 있으면 에러 처리한다.
		//CWriterLock_NORECURSE clk(m_callbackMon, true);

		if(proxy == nullptr)
			throw Exception(RmiInterfaceErrorText);

		const RmiID* rmiIDList = proxy->GetRmiIDList();

		for(int ii = 0; ii < proxy->GetRmiIDListCount(); ii++)
		{
			if((uint16_t)rmiIDList[ii] >= 60000 && !proxy->m_internalUse)
				throw Exception(BadRmiIDErrorText);

			// Add가 실패한다면 이미 똑같은 값이 들어가있다는 의미.
			if(m_proxyRmiIDList_NOCSLOCK.Add(rmiIDList[ii]) == false)
				throw Exception(DuplicatedRmiIDErrorText);
		}

		proxy->m_core = this;
		m_proxyList_NOCSLOCK.Add(proxy);
	}

	void CNetCoreImpl::AttachStub( IRmiStub *stub )
	{
		// 만약 중복되는 ID가 있으면 에러 처리한다.
		//CWriterLock_NORECURSE clk(m_callbackMon, true);

		if(stub == nullptr)
			throw Exception(RmiInterfaceErrorText);

		const RmiID* rmiIDList = stub->GetRmiIDList();

		for(int ii = 0; ii < stub->GetRmiIDListCount(); ii++)
		{
			if ((uint16_t)rmiIDList[ii] >= 60000 && !stub->m_internalUse)
				throw Exception(BadRmiIDErrorText);

			if(m_stubRmiIDList_NOCSLOCK.Add(rmiIDList[ii]) == false)
				throw Exception(DuplicatedRmiIDErrorText);
		}

		stub->m_core = this;
		m_stubList_NOCSLOCK.Add(stub);
	}

	void CNetCoreImpl::ShowError_NOCSLOCK( ErrorInfoPtr errorInfo )
	{
		AssertIsNotLockedByCurrentThread();

		if (Get_HasCoreEventFunctionObjects().OnError.m_functionObject)
			Get_HasCoreEventFunctionObjects().OnError.m_functionObject->Run(errorInfo);

		if (GetEventSink_NOCSLOCK() != nullptr)
			GetEventSink_NOCSLOCK()->OnError(errorInfo);
	}


	void CNetCoreImpl::ShowNotImplementedRmiWarning(const PNTCHAR* RMIName)
	{
		// ikpil.choi 2017-01-18 : ShowNotImplementedRmiWarning 이벤트 발생시, OnWarning 으로 핸들링 할수 있게 수정 (N3698)
		EnqueWarning(ErrorInfo::From(ErrorType_NotImplementedRmi, Proud::HostID_None, String::NewFormat(_PNT("RMI (name=%s)"), RMIName)));
	}

	void CNetCoreImpl::PostCheckReadMessage(CMessage &msg, const PNTCHAR* RMIName)
	{
		// ikpil.choi 2017-01-18 : 가상함수는 인라인이 안되므로, cpp 쪽으로 이동시킵니다.
		if (msg.GetReadOffset() != msg.GetLength())
		{
			_pn_unused(RMIName);
			NTTNTRACE("** 경고: RMI 수신 메시지 %s를 다 읽지 못했음!\n", StringT2A(RMIName).GetString());
		}
	}

	void CNetCoreImpl::DetachProxy(IRmiProxy *proxy)
	{
		if(AsyncCallbackMayOccur())
			throw Exception(AsyncCallbackMayOccurErrorText);

		//CWriterLock_NORECURSE clk(m_callbackMon, true);

		for (int ii = 0; ii < (int)m_proxyList_NOCSLOCK.GetCount(); ii++)
		{
			IRmiProxy *i = m_proxyList_NOCSLOCK[ii];
			if (i == proxy)
			{
				const RmiID* rmiIDList = i->GetRmiIDList();
				for (int jj = 0; jj < i->GetRmiIDListCount(); ++jj)
				{
					RmiID const rmiIDValue = rmiIDList[jj];
					m_proxyRmiIDList_NOCSLOCK.Remove(rmiIDValue);
				}

				m_proxyList_NOCSLOCK.RemoveAt(ii);
				proxy->m_core = nullptr;
				return;
			}
		}
	}

	void CNetCoreImpl::DetachStub( IRmiStub *stub )
	{
		//CWriterLock_NORECURSE clk(m_callbackMon, true);

		for (int ii = 0; ii < (int)m_stubList_NOCSLOCK.GetCount(); ii++)
		{
			IRmiStub *i = m_stubList_NOCSLOCK[ii];
			if (i == stub)
			{
				const RmiID* rmiIDList = i->GetRmiIDList();
				for (int jj = 0; jj < i->GetRmiIDListCount(); ++jj)
				{
					RmiID const rmiIDValue = rmiIDList[jj];
					m_stubRmiIDList_NOCSLOCK.Remove(rmiIDValue);
				}

				m_stubList_NOCSLOCK.RemoveAt(ii);
				stub->m_core = nullptr;
				return;
			}
		}
	}

	bool CNetCoreImpl::Send_CompressLayer(
		const CSendFragRefs &payload,
		const SendOpt& sendContext,
		const HostID *sendTo,
		int numberOfsendTo,
		int &compressedPayloadLength)
	{
		if(sendContext.m_compressMode != CM_None && payload.GetTotalLength() > 50 && !m_simplePacketMode ) // 12Byte의 헤더가 붙기 때문에 최소 50Byte 이하는 오히려 사이즈가 늘기 쉽다.
		{
			/*
			여기서는 ExpandHostIDArray() 를 하지 않는다.
			Convert 과정이 많은 시간을 소요하고 다음 Layer 에서 하기때문.
			loopback 의 경우 압축 및 암호화를 하면 안되지만 여기서는 그것을 판별할 수 없으므로 원본과 압축 메시지를 함께 넘긴다.
			*/

			// fragment들을 1개의 데이터로 이어붙인다. 그래야 압축이 되니까.
			CMessage payloadMessage;
			payloadMessage.UseInternalBuffer();
			Message_AppendFragments(payloadMessage, payload);

			int payloadLength = payloadMessage.GetLength();
			// 최대 압축 바운드 얻기
			uLongf actualCompressedLegnth = compressBound(payloadLength);

			// 압축할 메시지가 들어갈 공간 준비
			CMessage compressedMessage;
			compressedMessage.UseInternalBuffer();
			compressedMessage.SetLength(actualCompressedLegnth);

			// 압축 실패시 즐
			int compressResult = ZlibCompress((Bytef*)compressedMessage.GetData(), &actualCompressedLegnth, (Bytef*)payloadMessage.GetData(),(uLongf)payloadLength);
			if( compressResult != Z_OK )
			{
				String errorText;
				errorText.Format(_PNT("Packet compression failed! Error code=%d"), compressResult);
				EnqueError(ErrorInfo::From(ErrorType_CompressFail, *sendTo, errorText.GetString() ));				

				// FillSendFailListOnNeed를 건너뛰자. 압축 안하고 보내긴 할테니까.
				goto L1;
			}

			// 압축 효율이 없어도 즐
			if(actualCompressedLegnth + 9 < (uLongf)payload.GetTotalLength())	// 압축 후 크키가 더 커질 수 있다. ((MessageType, scalar,scalar),payload)
			{
				// 압축된 실제 크기로 재조절
				compressedMessage.SetLength(actualCompressedLegnth);

				// 압축된 메시지 전에 헤더를 넣자
				CSmallStackAllocMessage header;
				Message_Write(header, MessageType_Compressed);
				header.WriteScalar((int)compressedMessage.GetLength());
				header.WriteScalar((int)payload.GetTotalLength());

				// 이제 압축 본문을 추가하자
				CSendFragRefs compressedPayload;
				compressedPayload.Add(header);
				compressedPayload.Add(compressedMessage);

				compressedPayloadLength = actualCompressedLegnth;

				// 보내기
				return Send_SecureLayer(payload, &compressedPayload, sendContext, sendTo, numberOfsendTo);
			}
		}
L1:
		// 비압축 상태로 보내기.
		compressedPayloadLength = 0;
		return Send_SecureLayer(payload, nullptr, sendContext,sendTo, numberOfsendTo);
	}


	bool CNetCoreImpl::ProcessMessage_Compressed( CReceivedMessage& receivedInfo, CMessage& uncompressedOutput)
	{
		CMessage &compressedMessage = receivedInfo.m_unsafeMessage;

		int orgReadOffset = compressedMessage.GetReadOffset();
		int compressedPayloadLength;
		int uncompressedPayloadLength;

		// 헤더를 읽기
		if(!compressedMessage.ReadScalar(compressedPayloadLength) ||
			!compressedMessage.ReadScalar(uncompressedPayloadLength) )
		{
			// 오류. 상태 롤백하자.
			compressedMessage.SetReadOffset(orgReadOffset);
			return false;
		}

		// 해킹된 크기이거나 버그로 인해 메모리를 왕창 잡으면 곤란하므로 먼저 체크한다.
		if (uncompressedPayloadLength < 0 || uncompressedPayloadLength > GetMessageMaxLength())
		{
			// 오류. 상태 롤백하자.
			compressedMessage.SetReadOffset(orgReadOffset);
			return false;
		}
		if (compressedPayloadLength < 0 || compressedPayloadLength > GetMessageMaxLength())
		{
			// 오류. 상태 롤백하자.
			compressedMessage.SetReadOffset(orgReadOffset);
			return false;
		}

		// 압축 해제된 결과물이 들어갈 곳 버퍼 준비
		uLongf actualUncompressedLength = (uLongf)uncompressedPayloadLength;

		uncompressedOutput.UseInternalBuffer();
		uncompressedOutput.SetLength(uncompressedPayloadLength);

		// stub 에서 압축정보를 넘겨주기 위해 셋팅.
		receivedInfo.m_compressMode = CM_Zip;

		// 주의 actualUncompressedLength는 Input Output으로 모두 쓰인다
		if(ZlibUncompress(uncompressedOutput.GetData(), &actualUncompressedLength,
					compressedMessage.GetData()+compressedMessage.GetReadOffset(), compressedPayloadLength) != Z_OK
			|| actualUncompressedLength!=(uLongf)uncompressedPayloadLength)
		{
			// 오류. 상태 롤백하자.
			compressedMessage.SetReadOffset(orgReadOffset);
			return false;
		}

		// 압축해제 끝. uncompressedOutput에 잘 저장되었다.
		return true;
	}

	bool CNetCoreImpl::Send_SecureLayer(
		const CSendFragRefs& payload,
		const CSendFragRefs* compressedPayload,
		const SendOpt& sendContext,
		const HostID* sendTo, int numberOfsendTo)
	{
		bool ret = true;	// 하나라도 보내기 실패한 Host가 있으면 false
		if( m_settings.m_enableEncryptedMessaging == true && sendContext.m_encryptMode != EM_None && sendContext.m_reliability != MessageReliability_LAST && !m_simplePacketMode)
		{
			//EnqueError(ErrorInfo::From(ErrorType_CannotEncryptUnreliableMessage));
			CriticalSectionLock clk(GetCriticalSection(), true);			// 이것이 없으면 ExpandHostIDArray에서 AssertIsLockedByCurrentThread 실패가 난다!

			CHECK_CRITICALSECTION_DEADLOCK(this);

			// ungroup send dest 후 각각에 대해 crypt_count 증가, unicast를 한다.
			// 참고: 암호화된 메시지는 reliable만 가능하다. 2010.8.13일 unreliable도 가능하게 수정
			// 암호화를 위해서는 각각의 peer 에 대한 암호화 키의 정보를 가져와야 하기에 HostIDs 를 분리해야한다.
			POOLED_LOCAL_VAR(HostIDArray, ungroupedSendDestList);
			ExpandHostIDArray(numberOfsendTo, sendTo, ungroupedSendDestList);

			// modify by ulelio : main unlock
			// 이슈 814 by sopar
			//clk.Unlock();

			String errorOut;

			// 실제로 보낼 메시지.
			const CSendFragRefs* realPayload = nullptr;

			if (compressedPayload != nullptr)
			{
				// compressedPayload 가 nullptr 이 아니라면 압축된 메시지이다.
				realPayload = compressedPayload;
			}
			else
			{
				// compressedPayload 가 nullptr 이면 압축되지 않는 메시지이다.
				realPayload = &payload;
			}

			for (int i = 0; i < (int)ungroupedSendDestList.GetCount(); i++)
			{
				HostID sendDest = ungroupedSendDestList[i];

				LogLevel outLogLevel = LogLevel_Ok;
				shared_ptr<CSessionKey> sessionKey;
				if (GetLocalHostID() == sendDest)
				{
					// 자신에게 보내는 메시지의 경우 loopback 되기 때문에 압축, 암호화를 하지 않는다. 원본 메시지만 보낸다.
					if(!Send_BroadcastLayer(payload, nullptr, sendContext, &sendDest, 1))
						ret = false;
				}
				else if ((TryGetCryptSessionKey(sendDest, sessionKey, errorOut, outLogLevel)))		// NextEncryptCount는 나중에 실행해야 하므로 &&의 우변항에 있어야 한다! 안그러면 GetCryptSessionKey가 실패시 NextEncryptCount는 +1 된 상태이므로 상대측에서 패킷 해킹으로 오인함!
				{
					// modify by ulelio : NextEncryptCount의 순서를 보장하기위해 여기서 main lock필요.
					// 이슈 814 by sopar
					CriticalSectionLock mainlock(GetCriticalSection(), true);
					CHECK_CRITICALSECTION_DEADLOCK(this);

					CMessage encryptedPayload;   // CMessage는 이제 fast head을 쓰므로, 지나친 stack alloc으로 인한 호환성 문제가 더 크다.

					encryptedPayload.UseInternalBuffer();
					if (sendContext.m_reliability == MessageReliability_Reliable)
					{
						CryptCount encryptCount = 0; // #DECRYPT_COUNT_DISABLED
// 						if (!NextEncryptCount(sendDest, encryptCount))
// 						{
// 							EnqueError(ErrorInfo::From(ErrorType_EncryptFail, sendDest, _PNT("Failed to Gain NextEncryptCount!!")));
// 							return false;
// 						}
						encryptedPayload.Write(encryptCount);
					}
					Message_AppendFragments(encryptedPayload, *realPayload);

					CMessage outputPayload;
					outputPayload.UseInternalBuffer();

					bool result = false;
					ErrorInfoPtr errorInfo;
					if (sendContext.m_encryptMode == EM_Secure)
					{
						result = (CCryptoAes::EncryptMessage(sessionKey->m_aesKey, encryptedPayload, outputPayload, 0));
						if (result == false) errorInfo = ErrorInfo::From(ErrorType_EncryptFail, sendDest, _PNT("Please Check Encrypt Error CStartServerParameter"));
					}
					else
					{
						result = (CCryptoFast::EncryptMessage(sessionKey->m_fastKey, encryptedPayload, outputPayload, 0, errorInfo));
					}

					if (result)
					{
						CSmallStackAllocMessage header;
						// reliable or not & secure or not 여부에 따라 message type을 얻는다.
						MessageType msgType;
						if (sendContext.m_reliability == MessageReliability_Reliable)
						{
							if (sendContext.m_encryptMode == EM_Secure)
								msgType = MessageType_Encrypted_Reliable_EM_Secure;
							else
								msgType = MessageType_Encrypted_Reliable_EM_Fast;
						}
						else
						{
							if (sendContext.m_encryptMode == EM_Secure)
								msgType = MessageType_Encrypted_UnReliable_EM_Secure;
							else
								msgType = MessageType_Encrypted_UnReliable_EM_Fast;
						}

						header.Write((int8_t)msgType);
						//header.WriteScalar((int)outputPayload.GetLength());

						CSendFragRefs sendData;
						sendData.Add(header);
						sendData.Add(outputPayload);

						//send_broadcastlayer이 실패할 상황이라면, 이미 nextencryptcount에서 실패.
						//그러므로, 여기서는 PrevEncryptCount를 하지 않는다.
						if (!Send_BroadcastLayer(payload, &sendData, sendContext, &sendDest, 1))
							ret = false;
					}
					else
					{
						//위에서 하므로.
						//CriticalSectionLock mainlock(GetCriticalSection(), true);
						//CHECK_CRITICALSECTION_DEADLOCK(this);

						errorInfo->m_remote = sendDest;
						EnqueError(errorInfo);

						FillSendFailListOnNeed(sendContext, &sendDest, 1, errorInfo->m_errorType);
						ret = false;

						//여기까지 내려왔으면 nextencrypt가 되었을터, --를 하자.
						if (sendContext.m_reliability == MessageReliability_Reliable)
						{
//							PrevEncryptCount(sendDest); #DECRYPT_COUNT_DISABLED
						}
					}
				}
				else
				{
					// 세션키를 못 얻었거나 remote가 없다.
					if (outLogLevel == LogLevel_Error)
					{
						CriticalSectionLock mainlock(GetCriticalSection(), true);

						CHECK_CRITICALSECTION_DEADLOCK(this);

						// CStartServerParameter.m_enableP2PEncryptedMessaging=false
						if (errorOut.GetLength() > 0)
						{
							EnqueError(ErrorInfo::From(ErrorType_EncryptFail, sendDest, errorOut));
						}
						else
						{
							EnqueError(ErrorInfo::From(ErrorType_EncryptFail, sendDest,
								_PNT("CStartServerParameter.m_enableP2PEncryptedMessaging=false. P2P Messaging can not encrypted!!")));
						}
						FillSendFailListOnNeed(sendContext, &sendDest, 1, ErrorType_EncryptFail);
						ret = false;
					}
					else if (outLogLevel == LogLevel_Warning)
					{
						// 키를 못 얻었지만 보내긴 보낼 수 있는 상황이다.
						if (errorOut.GetLength() > 0)
						{
							EnqueWarning(ErrorInfo::From(ErrorType_NoNeedWebSocketEncryption, sendDest, errorOut));
						}
						else
						{
							EnqueWarning(ErrorInfo::From(ErrorType_NoNeedWebSocketEncryption, sendDest, 
								_PNT("No need to encrypt Websocket message. Use wss protocol.")));
						}

						// WebGL클라이언트에게 보낼때에는 Secure옵션을 True로 주어도 Warning을 Enqueue하고 암호화를 하지 않는다.
						SendOpt webGLSendContext =  sendContext;
						webGLSendContext.m_encryptMode = EM_None;
						if (!Send_BroadcastLayer(payload, compressedPayload, webGLSendContext, &sendDest, 1))
						{
							ret = false;
						}
					}
					else if (outLogLevel == LogLevel_InvalidHostID)
					{
						EnqueWarning(ErrorInfo::From(ErrorType_InvalidHostID, sendDest, errorOut));
						FillSendFailListOnNeed(sendContext, &sendDest, 1, ErrorType_InvalidHostID);
						ret = false;
					}
				}
			}
		}
		else
		{
			/*
			암호화 하지 않을경우 그냥 보낸다.
			이 경우 HostIDs를 분리하지 않기 때문에 자신에게 보내는 것인지 타 peer 에게 보내는것인지 확인할 수 없다.(compressLayer 에서 HostIDs 를 분리하지 않는다.)
			따라서 원본 메시지를 넘겨줄지 압축메시지를 넘겨줄지 판단할 수 없다.
			원본 메시지와 압축된 메시지를 같이 넘겨주어 Send_BroadcastLayer() 에서 처리되도록 한다.
			*/
			return Send_BroadcastLayer(
				payload,
				compressedPayload,
				sendContext,
				sendTo, numberOfsendTo);
		}
		return ret;
	}

	bool CNetCoreImpl::ProcessMessage_Encrypted( MessageType msgType, CReceivedMessage& receivedInfo, CMessage& decryptedOutput )
	{
		//netclient에서도 사용한다.
		//GetCriticalSection().AssertIsNotLockedByCurrentThread();

		CMessage &msg =receivedInfo.GetReadOnlyMessage();

		int orgReadOffset = msg.GetReadOffset();

		// modify by ulelio : 이제 decrypt를 할때 mainlock이 필요치 않음.
		// 복호화를 한다.				  
		String errorOut;
		LogLevel outLogLevel = LogLevel_Ok;
		shared_ptr<CSessionKey> sessionKey;
		if (!TryGetCryptSessionKey(receivedInfo.m_remoteHostID, sessionKey, errorOut, outLogLevel))
			sessionKey = shared_ptr<CSessionKey>();

		bool decryptedResult = false;
		if (sessionKey.get() == nullptr)
		{
			// 암호화된 메시지를 받았으나 그것을 처리할 remote 가 없는 경우.
			// 비암호화 메시지의 경우도 remote 객체가 이미 사라졌는데 받는 경우가 있을 수 있다. 그때는 무시하고 있다.
			// sesstionKey 가 nullptr 인 경우는 2가지가 있따. remote 가 없는경우. 서버 시작시 암호화 사용 셋팅을 하지 않은경우.
			// remote 가 없는 경우는 무시하면 되지만, 셋팅을 하지 않은 경우는 error 를 날려주어야 한다.
			if (outLogLevel == LogLevel_Error)
			{
				if (errorOut.IsEmpty())
					errorOut = _PNT("Make sure that enableP2PEncryptedMessaging is true.");

				EnqueError(ErrorInfo::From(ErrorType_DecryptFail, receivedInfo.m_remoteHostID, errorOut));
			}

			msg.SetReadOffset(orgReadOffset);
			return false;
		}

		assert(sessionKey.get() != nullptr); // 뻔한 방어 코딩이지만 넣어두자. 유지보수하다 실수하기도.

		ErrorInfoPtr errorInfo;
		if(msgType == MessageType_Encrypted_Reliable_EM_Secure || msgType == MessageType_Encrypted_UnReliable_EM_Secure)
		{
			// 일반 암호화
			receivedInfo.m_encryptMode = EM_Secure;
			decryptedOutput.UseInternalBuffer();
			decryptedResult = CCryptoAes::DecryptMessage(sessionKey->m_aesKey, msg, decryptedOutput, msg.GetReadOffset());
			if (decryptedResult == false)
			{
				errorInfo = ErrorInfo::From(ErrorType_DecryptFail, receivedInfo.m_remoteHostID, _PNT("decryption failure 1"));
			}
		}
		else if(msgType == MessageType_Encrypted_Reliable_EM_Fast || msgType == MessageType_Encrypted_UnReliable_EM_Fast)
		{
			// 빠른 암호화
			receivedInfo.m_encryptMode = EM_Fast;
			decryptedOutput.UseInternalBuffer();
			decryptedResult = CCryptoFast::DecryptMessage(sessionKey->m_fastKey, msg, decryptedOutput, msg.GetReadOffset(), errorInfo);
		}

		if (decryptedResult == false)
		{
			CriticalSectionLock clk(GetCriticalSection(),true);
			CHECK_CRITICALSECTION_DEADLOCK(this);

			if (errorInfo == nullptr)
			{
				errorInfo = ErrorInfo::From(ErrorType_DecryptFail, receivedInfo.m_remoteHostID, _PNT("decryption failure without encryption"));
			}

			errorInfo->m_remote = receivedInfo.m_remoteHostID;
			EnqueError(errorInfo);
			msg.SetReadOffset(orgReadOffset);
			return false;
		}

		if(msgType == MessageType_Encrypted_Reliable_EM_Secure || msgType == MessageType_Encrypted_Reliable_EM_Fast)
		{
			// 시리얼 값의 정상 여부를 체크한다.
			CryptCount decryptCount1;
//			CryptCount decryptCount2;  #DECRYPT_COUNT_DISABLED
			if ( !decryptedOutput.Read(decryptCount1))
			{
				CriticalSectionLock clk(GetCriticalSection(),true);
				CHECK_CRITICALSECTION_DEADLOCK(this);

				EnqueError(ErrorInfo::From(ErrorType_DecryptFail, receivedInfo.m_remoteHostID, _PNT("decryptCount1 read failed!!")));
				decryptedOutput.SetReadOffset(orgReadOffset);
				return false;
			}
			// #DECRYPT_COUNT_DISABLED
// 			if(!GetExpectedDecryptCount(receivedInfo.m_remoteHostID, decryptCount2) )
// 			{
// // 				CriticalSectionLock clk(GetCriticalSection(),true);
// //
// // 				CHECK_CRITICALSECTION_DEADLOCK(this);
// //
// // 				String errtxt;
// // 				errtxt.Format(_PNT("GetExpectedDecryptCount failed!!"));
// // 				EnqueError(ErrorInfo::From(ErrorType_DecryptFail, receivedInfo.m_remoteHostID, errtxt));
// 				// 암호화된 메시지를 받았으나 그것을 처리할 remote 가 없는 경우.
// 				// 비암호화 메시지의 경우도 remote 객체가 이미 사라졌는데 받는 경우가 있을 수 있다. 그때는 무시하고 있다.
// 				decryptedOutput.SetReadOffset(orgReadOffset);
// 				return false;
// 			}
// 			if (decryptCount1 != decryptCount2)
// 			{
// 				CriticalSectionLock clk(GetCriticalSection(),true);
//
// 				CHECK_CRITICALSECTION_DEADLOCK(this);
//
// 				String errtxt;
// 				errtxt.Format(_PNT("decryptCount1(%d) != decryptCount2(%d)"), (int)decryptCount1, (int)decryptCount2); // 이 에러가 떴을 경우, 사용자가 패킷 캡처&복제 테스트를 하고 있거나, 해커가 같은 수작을 부리거나.
// 				EnqueError(ErrorInfo::From(ErrorType_DecryptFail, receivedInfo.m_remoteHostID, errtxt));
// 				decryptedOutput.SetReadOffset(orgReadOffset);
// 				return false;
// 			}
//
// 			// 시리얼 값 이동 ( 내부에서 mainlock검 )
// 			NextDecryptCount(receivedInfo.m_remoteHostID);

		}

		return true;
	}


	CNetCoreImpl::CNetCoreImpl()
		: m_destructorIsRunning(false)
		, m_HolsterMoreCallbackUntilNextProcessCall_flagged(false)
		, m_userTaskQueue(this)
		, m_disposeGarbagedHosts_Timer(CNetConfig::DisposeGarbagedHostsTimeoutMs)
		, m_DisconnectRemoteClientOnTimeout_Timer(CNetConfig::UnreliablePingIntervalMs)
		, m_acceptedPeerdispose_Timer(CNetConfig::DisposeGarbagedHostsTimeoutMs)
		, m_lanRemotePeerHeartBeat_Timer(CNetConfig::LanRemotePeerHeartBeatTimerIntervalMs)
		{
		// code profile 결과 이거 무시 못하므로.
		m_socketToHostsMap.SetOptimalLoad_BestLookup();
		m_authedHostMap.SetOptimalLoad_BestLookup();
		m_candidateHosts.SetOptimalLoad_BestLookup();

		m_simplePacketMode = false;

		m_timerCallbackInterval = 0;
		m_timerCallbackParallelMaxCount = 1;
		m_timerCallbackContext = nullptr;

		m_netThreadPool = nullptr;
		m_userThreadPool = nullptr;
		m_netThread_useExternal = false;
		m_userThread_useExternal = false;

		m_DoGarbageCollect_lastTime = 0;
		m_tag = 0;

		// AllocPreventOutOfMemory와 순서 바뀌면 안됩니다.
		m_preventOutOfMemoryDisconnectError = nullptr;
		AllocPreventOutOfMemory();
	}


	CNetCoreImpl::~CNetCoreImpl()
	{
		FreePreventOutOfMemory();
		// 이미 상속 클래스에서 제거해둔 상태이어야 한다.
		//CWriterLock_NORECURSE clk(m_callbackMon, true);
		assert(m_proxyList_NOCSLOCK.GetCount() == 0);
		assert(m_stubList_NOCSLOCK.GetCount() == 0);
		assert(m_proxyRmiIDList_NOCSLOCK.GetCount() == 0);
		assert(m_stubRmiIDList_NOCSLOCK.GetCount() == 0);
#ifdef VIZAGENT
		assert(!m_vizAgent);
#endif
		// 확인 사살
		m_garbagedHosts.Clear();
		m_garbagedSockets.Clear();

		// value를 모두 지운다.
		for (SocketsToHostsMap::iterator i = m_socketToHostsMap.begin(); i != m_socketToHostsMap.end(); i++)
		{
			delete i.GetSecond();
		}
	}


	void CNetCoreImpl::CleanupEveryProxyAndStub()
	{
		//CWriterLock_NORECURSE clk(m_callbackMon, true);

		for(int i=0;i<(int)m_proxyList_NOCSLOCK.GetCount();i++)
		{
			m_proxyList_NOCSLOCK[i]->m_core = nullptr;
		}
		m_proxyList_NOCSLOCK.Clear();

		for(int i=0;i<(int)m_stubList_NOCSLOCK.GetCount();i++)
		{
			m_stubList_NOCSLOCK[i]->m_core = nullptr;
		}
		m_stubList_NOCSLOCK.Clear();

		m_proxyRmiIDList_NOCSLOCK.Clear();
		m_stubRmiIDList_NOCSLOCK.Clear();
	}


	bool CNetCoreImpl::SendUserMessage(const HostID* remotes, int remoteCount, RmiContext &rmiContext, uint8_t* payload, int payloadLength)
	{
		rmiContext.AssureValidation();

		CSmallStackAllocMessage header;
		Message_Write(header, MessageType_UserMessage);

		CSendFragRefs fragRefs;
		fragRefs.Add(header);
		fragRefs.Add(payload, payloadLength);

		int compressedPayloadLength = 0;
		SendOpt sendOpt = SendOpt::CreateFromRmiContextAndClearRmiContextSendFailedRemotes(rmiContext); // callee에서 변경을 일으킬 수 있으므로 이렇게 복사를 하는게 안전.
		bool ret = Send(fragRefs, 
			sendOpt, 
			remotes, 
			remoteCount, 
			compressedPayloadLength);

		// 사용자 정의 메시지를 호출했다는 이벤트를 콜백한다.
		/*MessageSummary msgSumm;
		msgSumm.m_payloadLength = fragRefs.GetTotalLength();
		msgSumm.m_rmiID = RmiID_None;
		msgSumm.m_rmiName = _PNT("<User Defined Message>");
		msgSumm.m_encryptMode = rmiContext.m_encryptMode;
		msgSumm.m_compressMode = rmiContext.m_compressMode;
		msgSumm.m_compressedPayloadLength = compressedPayloadLength;

		Viz_NotifySendByProxy(remotes, remoteCount, msgSumm, rmiContext);*/

		return ret;
	}

	/* stopIoacked=true로 세팅하고, 여기 목록에 넣어버린다.
	remote가 socket을 버릴 때 호출하는 루틴.

	과거에는 여기서 socket을 close했지만 OS가 socket fd 값 재사용해버리는 문제 때문에
	garbage collector에서 delete를 할 때까지 close socket을 보류한다.

	이걸 호출 후 더 이상 overlapped send/receive를 수행 안하게 된다.
	unix에서는 epoll avail i/o signal이 한동안 더 계속 오겠지만
	CIoReactorEventNotifier에서 이미 등록 해제되어 있으므로 이벤트를 무시할 것이다.

	주의: 이 함수를 호출한 스레드는 이후부터 socket을 액세스하면 안된다.
	socket을 참고하고 있던 곳들도 모두 이를 참고하지 않게 바꾸어야 한다.
	변수가 null을 가지게 바꾸던지, 아니면 remove from collection을 해주어야 한다.
	왜냐하면, main lock이 해제되는 직후부터 불시에 delete되기 때문이다. */
	void CNetCoreImpl::GarbageSocket(const shared_ptr<CSuperSocket>& socket)
	{
		// caller에서 이미 main lock 한 상태이어야 한다.
		// 이 함수를 호출한다음 garbage되는 객체를 가리키던 변수들이 null로 세팅되는 과정이
		// main lock atomic해야 할테니까.
		// 만약 여기서 실패한다는 것은, 잠재적인 data race를 발생하는 것이므로 꼭 고치자.
		LockMain_AssertIsLockedByCurrentThread();

		TEST_CaptureCallStack();

#ifdef TEST_CloseSocketOnly_CallStack
		printf("[ProudNetClient] active close at : %s\n", __FUNCTION__);
#endif


		CriticalSectionLock lock(m_garbageSocketQueueCS, true);

		m_garbageSocketQueue.PushBack(socket);
		socket->RequestStopIo();

		// socket에서 데이터 수신을 해도 그것을 처리할 host는 더 이상 없다고 처리해 버린다.
		SocketToHostsMap_RemoveForAnyAddr(socket);
	}

	/* 파괴해야 하는 remote에 대해서 issue dispose를 건다.
	단, authed에서는 아직 사라지지 않는다.
	처리해야 할 user work item이 남아있는 경우 authed에 유효하게 있어야
	validation이 잔존하기 때문이다.
	이것을 delete host를 하지는 않는다. 이 remote에 대한 user task가 아직 실행중인 경우 때문이다.
	이 함수는 마구 호출해도 괜찮다. 즉 멱등성임. */
	void CNetCoreImpl::GarbageHost(
		const shared_ptr<CHostBase>& remoteBase,
		ErrorType errorType,
		ErrorType detailType,
		const ByteArray& comment,
		const PNTCHAR*,
		SocketErrorCode socketErrorCode)
	{
		// caller에서 이미 main lock 한 상태이어야 한다.
		// 이 함수를 호출한다음 garbage되는 객체를 가리키던 변수들이 null로 세팅되는 과정이
		// main lock atomic해야 할테니까.
		// 만약 여기서 실패한다는 것은, 잠재적인 data race를 발생하는 것이므로 꼭 고치자.
		AssertIsLockedByCurrentThread();

		TEST_CaptureCallStack();

#ifdef TEST_CloseSocketOnly_CallStack
		printf("[ProudNetClient] active close at : %s\n", __FUNCTION__);
#endif


		if (!m_garbagedHosts.ContainsKey(remoteBase.get()))
		{
			m_garbagedHosts.Add(remoteBase.get(), remoteBase);
			OnHostGarbaged(remoteBase, errorType, detailType, comment, socketErrorCode);
		}
	}

	// NetCore 레벨에서의 각종 custom value 이벤트를 처리한다.
	// 주의! 이 객체를 상속받은 클래스는 OnCustomValueEvent에서 반드시 이것을 호출해야 한다
	void CNetCoreImpl::ProcessCustomValueEvent(const ThreadPoolProcessParam& /*param*/, CWorkResult*, CustomValueEvent customValue)
	{
		try
		{
			switch (customValue)
			{
			case CustomValueEvent_GarbageCollect:
				DoGarbageCollect();
				break;
			case CustomValueEvent_OnTick:
				Run_OnTick();
				break;
				//#ifdef _WIN32
				//		case CustomValueEvent_IssueFirstRecv:
				//			IssueFirstRecv();
				//			break;
				//#endif
				// 		case CustomValueEvent_End:
				// 			EndCompletion();
				// 			break;
			default:
				//assert(0);
				break;
			}
		}
		catch (std::bad_alloc &ex)
		{
			Exception e(ex);
			BadAllocException(e);
		}
	}

	// delete is safe인 garbage socket을 목록에서 제거 및 파괴를 한다.
	// socket, remote에 대해 처리한다.
	void CNetCoreImpl::DoGarbageCollect()
	{
		// 동접 많은 서버에서, 클라이언트들이 대거 나간 경우, 아래 구문이 많은 CPU 시간을 먹는 것으로 보고되었다.
		// 이유는, 동접 많을 때, 20밀리초마다 DoGarbageCollect 이벤트를 post하는 것이 계속 누적되어서 그런 것이다.
		// 따라서, 누적되더라도, 최근 20밀리초에 한 적이 있었으면, 그냥 건너뛰는 것이 경제적이다.
		// 그래서 이렇게 한다.
		int64_t currTime = GetPreciseCurrentTimeMs();
		if (currTime - m_DoGarbageCollect_lastTime >= CNetConfig::GarbageCollectIntervalMs)
		{
			// 여기서 미리 main lock을 걸어야 garbageCS를 lock하는 것이 데드락으로 안 이어짐.
			CriticalSectionLock lock(GetCriticalSection(), true);

			DoGarbageCollect_Host();
			DoGarbageCollect_Socket();

			m_DoGarbageCollect_lastTime = currTime;
		}
	}

	// 모든 send ready socket들을 순회하면서 send call을 수행한다.
	// 1회라도 issue send를 하면 즉 syscall을 하면 true를 리턴한다.
	bool CNetCoreImpl::EveryRemote_IssueSendOnNeed(int64_t currTime)
	{
		AssertIsNotLockedByCurrentThread(); // 여기는 꽤 오래 걸린다. 병렬 병목이 있으면 안된다.

		// 고객사 중 크래쉬 이슈로 인하여 로컬 변수에 copy 한 후, 작업 한다.
		// 이 함수가 끝나기 전까지 이 객체가 돌연 사라지 말아야 하기 때문이다.
		shared_ptr<CSendReadySockets> sendReadyList = m_sendReadyList;

		// stale 문제가 있지만, 아래 본 루틴을 들어가기 전에 사전검사한다.
		// 아래 본 루틴은 상대적으로 많은 시간을 차지하고, 이 함수는 매우 자주 호출되기 때문이다.
		// stale 문제로 인해 '보낼게 있어서 안 보내버리는'상황이 발생하더라도, OO ms밖에 안되는 다음 턴에서
		// 보내지므로 괜찮다.
		if (!sendReadyList ||
			sendReadyList->GetCount() == 0)
		{
			return false;
		}

		//한 스레드만 작업하는것을 개런티
		TemporaryAtomicCompareAndSwap CASChecker(0, 1, &m_issueSendOnNeedWorking);

		if (CASChecker.GetReturnValue() == 0)
		{
			// TODO: 주석 기재하기.
 			if (m_lastIssueSendOnNeedTimeMs == currTime)
 				return false;

			m_lastIssueSendOnNeedTimeMs = currTime;

			POOLED_LOCAL_VAR(CSuperSocketArray, sendIssuedPool);

			if (!sendReadyList) {
				return false;
			}

			// 1개씩 꺼내지 말고 한꺼번에 다 꺼내자. 이유는 PopKeys 주석에.
			sendReadyList->PopKeys(sendIssuedPool);

			// 이 안에서는 오랜 시간 per-remote lock을 수행한다. 따라서 unlock main 후 수행한다
			// 그래도 안전하다. 왜냐하면 이미 use count+1씩 해놨으니까.
			// 이 함수 안에서 각각에 대해 issue-send를 한 후에 use count-1을 한다.
			IssueSendFunctor<CNetCoreImpl> functor(this);
			LowContextSwitchingLoop(
				sendIssuedPool.GetData(),
				sendIssuedPool.GetCount(),
				functor);

			bool ret = sendIssuedPool.GetCount() > 0;

			// 이게 있어야 한다. 안그러면 free list에 오래 방치된 array 객체의 shared_ptr들이 계속 남아 버린다.
			sendIssuedPool.Clear();

			return ret;
		}
		return false;
	}

	// super socket에 의해 send ready list에 뭔가를 넣어야 할 일이 생기면 콜백되는 함수.
	// TCP의 경우 항상 콜백되지만 UDP의 경우 coalesce 등으로 인해 바로 호출되지 않는 경우도 있다.
	// send ready list에 넣고, thread pool이 CustomValueEvent_SendEnqueued를 처리하게 깨운다.
	// CustomValueEvent_SendEnqueued 안에서 send ready list를 처리한다.
	// issueSendNow: 즉시 send call이 일어나게 하려면(예:i/o completion이나 TCP) true,
	// UDP coalesce 때가 될 때만 send call이 일어나게 하려면 false.
	void CNetCoreImpl::SendReadyList_Add(const shared_ptr<CSuperSocket>& socket, bool issueSendNow)
	{
		Proud::AssertIsLockedByCurrentThread(socket->GetSendQueueCriticalSection());

		// dangle pointer access 문제가 있는 객체인데 멤버 포인터 변수를 바로 액세스 하는것이 문제 될 수 있다.
		// 따라서 sendReadyList는 아래와 같이 copy 후 작업 한다.
		shared_ptr<CSendReadySockets> sendReadyList = m_sendReadyList;
		if (sendReadyList == nullptr)
		{
			return;
		}

		// send ready list에도 추가한다. main lock 상태가 아닐 수 있음에 주의.
		// code profile 결과 이 부분이 무시못하는 허당(정작 할 일이 없음) 99%이므로.
		sendReadyList->AddOrSet(socket);

		// 예전에는 여기서 '빈 리스트에의 추가'이면 PQCS를 해서 worker thread가 깨게 했었다.
		// 이제는 낮은 CPU 처리를 위해, 1ms의 딜레이를 감수하고 batching시켰기 때문에, 그렇게 하지 않는다.

		// #FAST_REACTION case 104로 테스트 결과 이게 없으면 안됨. 자세한 사항은 sole/doc 내 P2P RMI reaction speed.pptx에 있다.
		if (issueSendNow)
		{
#ifdef _WIN32
			m_netThreadPool->m_completionPort->PostCompletionStatus();
#else 
			// #UNIX_POST_EVENT
			_pn_unused(issueSendNow); // clang warning fix
			throw Exception("Unsupported yet!"); // 이게 되려면 eventfd or socketpair로 작동하게 해야 한다. PS4에서는 socketpair가 없어서 다른 방법을 써야 한다. 나중에 구현하도록 하자.
#endif
		}
	}

	// delete is safe인 garbage socket을 목록에서 제거 및 파괴를 한다.
	void CNetCoreImpl::DoGarbageCollect_Socket()
	{
		CriticalSectionLock lock(GetCriticalSection(), true);

		// MOVE garbage pending socket queue TO garbage socket list
		{
			CriticalSectionLock garbageSocketQueueLock(m_garbageSocketQueueCS, true);

			while (!m_garbageSocketQueue.empty())
			{
				shared_ptr<CSuperSocket> socket = m_garbageSocketQueue.PopFront();

				if (!m_garbagedSockets.ContainsKey(socket.get()))
				{
					m_garbagedSockets.Add(socket.get(), socket);

					// dangle pointer access 문제가 있는 객체인데 멤버 포인터 변수를 바로 액세스 하는것이 문제 될 수 있다.
					// 따라서 sendReadyList는 아래와 같이 copy 후 작업 한다.
					shared_ptr<CSendReadySockets> sendReadyList = m_sendReadyList;

					// 이미 socket을 close했기 때문에 뭔가가 수신되더라도 그것을 처리하면 안된다.
					// 다른 스레드에서 핸들링을 하고 있는 상황?
					// => 고려할 필요 없음. 여기도 거기도 main lock 상태에서 메시지 핸들링하니까.
					if(sendReadyList)
						sendReadyList->Remove(socket);

					SocketToHostsMap_RemoveForAnyAddr(socket);
				}
			}
		}

		// 각 garbage socket에 대해
		for (GarbagedSockets::iterator i = m_garbagedSockets.begin(); i != m_garbagedSockets.end();)
		{
			shared_ptr<CSuperSocket> iSocket = i->GetSecond();
			assert(iSocket->StopIoRequested());

			bool delNow = false;

			// issue send or recv도 UDP socket이 is closed이면 아예 시도조차 안하므로 OK.
			// 참고: send-issue-post를 이제는 안하므로 없는 UDP socket에 대한 completion이 뒷북으로는 안온다.
			if (iSocket->StopIoAcked())
			{
				delNow = true; // 이제 더 이상 어떤 스레드에서도 액세스하지 않는다. 이 스레드를 포함해서 아무 스레드에서나 지워도 상관없다. smart ptr이므로 ok.
			}

			if (delNow)
			{
				// garbageCS를 풀지 않는다.
				// garbageCS는 superSocket의 콜백이 main lock을 안한 상태에서 오는 경우의 막기 위했던 용도뿐이므로.
				OnSocketGarbageCollected(iSocket);

				// i/o가 더 없음이 보장됨. 이제 delete를 한다.
				// 지우면서 socket close도 자동 실행된다.
				i = m_garbagedSockets.erase(i); // 여기서 바로 지울 수도 있고, 다른 스레드에서 지울 수도 있다. 어디서든 상관없다.

				// dangle pointer access 문제가 있는 객체인데 멤버 포인터 변수를 바로 액세스 하는것이 문제 될 수 있다.
				// 따라서 sendReadyList는 아래와 같이 copy 후 작업 한다.
				shared_ptr<CSendReadySockets> sendReadyList = m_sendReadyList;

				// 위에서 이미 했지만 확인사살.
				if (sendReadyList)
					sendReadyList->Remove(iSocket);

				SocketToHostsMap_RemoveForAnyAddr(iSocket);
			}
			else
			{
//  #ifdef _WIN32
//  				// 방어 코딩 취지로, 행여나 뒤늦게 overlapped i/o를 건 것이 있으면 모두 취소시켜서 non issued가 될 수 있게 유도한다.
//  				if (iSocket->m_fastSocket)
//  				{
//  					iSocket->m_fastSocket->CancelEveryIo();
// //   					if (err == 1168 && currTime - iSocket->StopIoRequested_GetTime() > 6000)
// //   					{
// //   						// issue 상태가 아닐 때 나오는 에러이다.
// //   						// 이때 flag가 'issue중'이라는 상태이면, 강제로 'not issue중'이라고 변경해버리자.
// //   						// Q: stale 문제가 있지 않을까요?
// //   						// A: cancel io를 했는데도 completion이 3초 이상 안오는 막장 상황에서는 stale 걱정이 없습니다.
// //   						AtomicCompareAndSwap32(IoState_Working, IoState_NoMoreWorkGuaranteed, &iSocket->m_recvIssued);
// //   						AtomicCompareAndSwap32(IoState_Working, IoState_NoMoreWorkGuaranteed, &iSocket->m_sendIssued);
// //
// //
// //   					}
//  				}
//  #endif
				i++;
			}
		}
	}

	// delete is safe인 garbaged host을 목록에서 제거 및 파괴를 한다.
	void CNetCoreImpl::DoGarbageCollect_Host()
	{
		CriticalSectionLock lock(GetCriticalSection(), true);

		// 더 이상 처리할 task가 없는 remote를 제거한다.
		for (CFastMap2<void*, shared_ptr<CHostBase>, int>::iterator i = m_garbagedHosts.begin(); i != m_garbagedHosts.end();)
		{
			shared_ptr<CHostBase> rc = i->GetSecond();

			bool canBeDeleted = !m_userTaskQueue.DoesHostHaveUserWorkItem(rc.get());
			if (canBeDeleted)
			{
				// 이걸 콜백하고.
				OnHostGarbageCollected(rc);

				m_candidateHosts.Remove(rc.get());
				/* Q: garbage host를 할 시점에 미리 authed hosts에서 제거되어야 하는게 정석 아닌가요?
				A: garbaged host가 아직 collect 즉 delete가 되기 전에, 아직 남아있는 user work를 처리해야 합니다.
				그런데 authed에서 이미 사라져 있으면 이를 처리할 수 없습니다. */

				// ACR 의 temp remote server 는 HostID 가 None 이므로 이렇게 처리
				if (rc->GetHostID() != HostID_None)
				{
					// 보시다시피, garbage collect가 될 때 authed hosts에서 제거된다.
					// 따라서 Get or SetHostTag에서 authed hosts만 뒤져도 안전하다.
					// 이걸 garbage collectable이 아닌데 수행하면=>위 안전이 불안전으로 바뀐다.
					m_authedHostMap.RemoveKey(rc->GetHostID());
				}

				i = m_garbagedHosts.erase(i);
			}
			else
			{
				i++;
			}
		}
	}

	// m_garbagesCS만 매번 따로 lock하기 귀찮아서
	const int CNetCoreImpl::GetGarbagedSocketsAndHostsCount_NOLOCK()
	{
		AssertIsLockedByCurrentThread();
		return m_garbagedSockets.GetCount() + m_garbagedHosts.GetCount();
	}

	void CNetCoreImpl::ClearGarbagedHostMap()
	{
		CriticalSectionLock lock(GetCriticalSection(), true);
		m_garbagedHosts.Clear();
	}

	// garbaged 상태의 Host를 un-garbage 즉 garbage 아닌 상태로 바꾼다.
	void CNetCoreImpl::UngarbageHost(const shared_ptr<CHostBase>& r)
	{
		CriticalSectionLock lock(GetCriticalSection(), true);
		m_garbagedHosts.Remove(r.get());
	}

	// 주의: 이 함수의 caller는 바로 udpSocket을 참조해제 해야 한다.
	// garbageDelayMs: 이 시간이 지나면 재사용 불가능해진다.
	void CNetCoreImpl::UdpSocketToRecycleBin(HostID hostID, const shared_ptr<CSuperSocket>& udpSocket, int garbageDelayMs)
	{
		SocketToHostsMap_RemoveForAnyAddr(udpSocket); // recycle에 들어간 것은 수신을 할 자격이 없으므로.

		udpSocket->m_dropSendAndReceive = true;
		udpSocket->m_timeToGarbage = GetPreciseCurrentTimeMs() + garbageDelayMs;
		m_recycles.Add(hostID, udpSocket);
	}

	// recyclable UDP sockets에 대한 처리
	void CNetCoreImpl::GarbageTooOldRecyclableUdpSockets()
	{
		CriticalSectionLock lock(GetCriticalSection(), true);

		// 너무 오랫동안 재사용되지 못하고 있는 peer용 UDP socket들을 파괴한다.
		int64_t currTime = GetPreciseCurrentTimeMs();

		for (CFastMap2<HostID, shared_ptr<CSuperSocket>, int>::iterator i = m_recycles.begin(); i != m_recycles.end();)
		{
			CSuperSocket* udpSocket = (CSuperSocket*)i->GetSecond().get();

			if (currTime - udpSocket->m_timeToGarbage > 0) // CNetClientImpl::DoGarbageCollect()에서는 RecyclePairReuseTimeMs보다 10초는 더 기다려야 안전빵.
			{
				GarbageSocket(i->GetSecond()); // 아래에서 참조 변수 자체를 제거하니 이 함수는 safe.
				i = m_recycles.erase(i);
			}
			else
				++i;
		}
	}

	void CNetCoreImpl::SetTag(void* value) 
	{
		m_tag = value;
	}
	void* CNetCoreImpl::GetTag() 
	{
		return m_tag;
	}

	// recycleable socket들을 모두 garbage 처리
	void CNetCoreImpl::AllClearRecycleToGarbage()
	{
		for (CFastMap2<HostID, shared_ptr<CSuperSocket>, int>::iterator i = m_recycles.begin(); i != m_recycles.end();)
		{
			GarbageSocket(i->GetSecond()); // 아래에서 참조 변수 자체를 제거하니 이 함수는 safe.
			i = m_recycles.erase(i);
		}

		m_recycles.Clear();
	}

	// Calls OnTick function object or interface class instance.
	void CNetCoreImpl::Run_OnTick()
	{
		int result = AtomicIncrement32(&m_timerCallbackParallelCount);
		if (result <= m_timerCallbackParallelMaxCount) // 동시에 실행 가능한 timer callback 수를 제한함
		{
			if (Get_HasCoreEventFunctionObjects().OnTick.m_functionObject)
				Get_HasCoreEventFunctionObjects().OnTick.m_functionObject->Run(m_timerCallbackContext);

			if (GetEventSink_NOCSLOCK() != nullptr /*&& !m_owner->m_shutdowning*/)
				GetEventSink_NOCSLOCK()->OnTick(m_timerCallbackContext);
		}
		AtomicDecrement32(&m_timerCallbackParallelCount);
	}

	void CNetCoreImpl::SetTimerCallbackParameter(uint32_t interval, int32_t maxCount, void* context)
	{
		m_timerCallbackInterval = interval;
		m_timerCallbackParallelMaxCount = maxCount;

		m_timerCallbackContext = context;

		if(m_timerCallbackInterval > 0)
		{
			if (m_timerCallbackParallelMaxCount <= 0)
				throw Exception("timerCallbackParallelMaxCount must be >0!");
		}
	}

	void CNetCoreImpl::SetTimerCallbackIntervalMs(int newVal)
	{
		m_periodicPoster_Tick->SetPostInterval(newVal);
	}


	// 로컬 이벤트를 처리한 후 해당 로컬 이벤트를 소유했던 task subject 즉 host의 '실행중'을 끈다.
	void CNetCoreImpl::UserWork_LocalEvent(CFinalUserWorkItem &UWI, const shared_ptr<CHostBase>& subject, CWorkResult* workResult)
	{
		/* LS,NS의 경우:
		OnClientJoin에서 setHostTag를 하고, OnClientLeave에서 hostTag값을 사용하는 유저는 흔하다.
		그런데, 클라가 서버와 접속을 하자 마자 끊는 경우를 고려해야 한다.

		OnClientJoin에서 setHostTag를 호출했는데, net worker thread에서 접속 해제가 감지되어서 LocalEventType_ClientLeaveAfterDispose를 enque하는 경우가 있다.
		이때 LocalEventType_ClientLeaveAfterDispose와 함께 들어간 hostTag 값은 setHostTag가 호출되기 전의 상태일 가능성이 있다.
		이러한 경우 OnClientLeave에서 얻어진 hostTag은 setHostTag 호출하기 전의 값이 들어올 수 있다.
		이렇게 되면, OnClientJoin(X)와 OnClientLeave(X)는 동시성이 없음을 보장해주는 안전함이 있더라도
		OnClientJoin(X)때 호출한 setHostTag가 OnClientLeave(X)에서는 소용없을 수 있다. 이러면 안된다.

		아래 patchwork는 그것을 해결하기 위한 방법이다.
		*/
		if (UWI.Internal().m_event->m_netClientInfo != nullptr)
			UWI.Internal().m_event->m_netClientInfo->m_hostTag = subject->m_hostTag;

		// throw exception을 하는 경우 어느 함수에서 발생한 것인지를 넣기 위함.
		// String이 아닌, char*다.
		// 어차피 const string이므로 이렇게 해도 되며,
		// 그리고 매번 실행할 때마다 여기에 무거운 string assign이 작동되면 성능 낭비.
		const PNTCHAR* functionName = _PNT("");
		if (m_allowExceptionEvent == false)
		{
			ProcessOneLocalEvent(*UWI.Internal().m_event, subject, functionName, workResult);
		}
		else
		{
			try
			{
				ProcessOneLocalEvent(*UWI.Internal().m_event, subject, functionName, workResult);
			}
			catch (std::bad_alloc &ex)
			{
				Exception ext(ex);
				ext.m_remote = UWI.Internal().m_event->m_remoteHostID;
				ext.m_userCallbackName = functionName;
				ext.m_delegateObject = GetEventSink_NOCSLOCK();
				BadAllocException(ext);
			}
			catch (Exception &ex)
			{
				ex.m_remote = UWI.Internal().m_event->m_remoteHostID;
				ex.m_userCallbackName = functionName;
				ex.m_delegateObject = GetEventSink_NOCSLOCK();

				if (Get_HasCoreEventFunctionObjects().OnException.m_functionObject)
					Get_HasCoreEventFunctionObjects().OnException.m_functionObject->Run(ex);

				if (GetEventSink_NOCSLOCK() != nullptr /*&& !m_shutdowning*/)
					GetEventSink_NOCSLOCK()->OnException(ex);
			}
			catch (std::exception &ex)
			{
				Exception ext(ex);
				ext.m_remote = UWI.Internal().m_event->m_remoteHostID;
				ext.m_userCallbackName = functionName;
				ext.m_delegateObject = GetEventSink_NOCSLOCK();

				if (Get_HasCoreEventFunctionObjects().OnException.m_functionObject)
					Get_HasCoreEventFunctionObjects().OnException.m_functionObject->Run(ext);

				if (GetEventSink_NOCSLOCK() != nullptr /*&& !m_shutdowning*/)
					GetEventSink_NOCSLOCK()->OnException(ext);
			}
#if defined (_WIN32)
			catch (_com_error &ex)
			{

				Exception ext; Exception_UpdateFromComError(ext, ex);
				ext.m_remote = UWI.Internal().m_event->m_remoteHostID;
				ext.m_userCallbackName = functionName;
				ext.m_delegateObject = GetEventSink_NOCSLOCK();

				if (Get_HasCoreEventFunctionObjects().OnException.m_functionObject)
					Get_HasCoreEventFunctionObjects().OnException.m_functionObject->Run(ext);

				if (GetEventSink_NOCSLOCK() != nullptr /*&& !m_shutdowning*/)
					GetEventSink_NOCSLOCK()->OnException(ext);

			}
#endif
#ifdef ALLOW_CATCH_UNHANDLED_EVEN_FOR_USER_ROUTINE
			catch (...)
			{

				Exception ext;
				ext.m_exceptionType = ExceptionType_Unhandled;
				ext.m_remote = UWI.m_event->m_remoteHostID;
				ext.m_userCallbackName = functionName;
				ext.m_delegateObject = GetEventSink_NOCSLOCK();

				if (Get_HasCoreEventFunctionObjects().OnException.m_functionObject)
					Get_HasCoreEventFunctionObjects().OnException.m_functionObject->Run(ext);

				if (GetEventSink_NOCSLOCK() != nullptr /*&& !m_shutdowning*/)
					GetEventSink_NOCSLOCK()->OnException(ext);

			}
#endif // ALLOW_CATCH_UNHANDLED_EVEN_FOR_USER_ROUTINE
		}
	}

	void CNetCoreImpl::UserWork_FinalReceiveRmi(CFinalUserWorkItem& UWI, const shared_ptr<CHostBase>& subject, CWorkResult* workResult)
	{
		AssertIsNotLockedByCurrentThread();

		// 		if (m_owner->IsValidHostID(UWI.Internal().m_unsafeMessage.m_remoteHostID) == false) IsValidHostID_NOLOCK caller 참고
		// 			return;
		UWI.Internal().m_unsafeMessage.m_unsafeMessage.SetSimplePacketMode(IsSimplePacketMode());

		CMessage& msgContent = UWI.Internal().m_unsafeMessage.m_unsafeMessage;
		int oldReadPointer = msgContent.GetReadOffset();

		if (!(oldReadPointer == 0))
			EnqueueHackSuspectEvent(subject, __FUNCTION__, HackType_PacketRig);

		
		//CReaderLock_NORECURSE clk(m_owner->m_callbackMon, true);

		RmiID rmiID = RmiID_None;
		if (false == msgContent.Read(rmiID))
			return;

		// ikpil.choi 2017-01-16 : 이미 처리된 stub message가 다시 처리 될 수 있는 버그 수정
		bool processed = false;
		intptr_t stubCount = m_stubList_NOCSLOCK.GetCount();
		for (intptr_t iStub = 0; false == processed && iStub < stubCount; iStub++)
		{
			IRmiStub *stub = m_stubList_NOCSLOCK[iStub];
			msgContent.SetReadOffset(oldReadPointer);
			if (m_allowExceptionEvent == false)
			{
				processed = Stub_ProcessReceiveMessage(stub, UWI.Internal().m_unsafeMessage, subject->m_hostTag, workResult);
			}
			else
			{
				// 이렇게 해줘야 막판에 task running flag를 수정할 수 있으니까 try-catch 구문이 있는거다.
				try
				{
					processed = Stub_ProcessReceiveMessage(stub, UWI.Internal().m_unsafeMessage, subject->m_hostTag, workResult);
				}
				catch (std::bad_alloc &e)
				{
					Exception ex(e);
					ex.m_remote = UWI.Internal().m_unsafeMessage.m_remoteHostID;
					ex.m_userCallbackName.Format(_PNT("RMI (ID=%d)"), rmiID);
					ex.m_delegateObject = stub;
					BadAllocException(ex);
				}
				catch (Exception& e)
				{
					e.m_remote = UWI.Internal().m_unsafeMessage.m_remoteHostID;
					e.m_userCallbackName.Format(_PNT("RMI (ID=%d)"), rmiID);
					e.m_delegateObject = stub;
					if (Get_HasCoreEventFunctionObjects().OnException.m_functionObject)
						Get_HasCoreEventFunctionObjects().OnException.m_functionObject->Run(e);
					if (GetEventSink_NOCSLOCK() != nullptr /*&& !m_shutdowning*/)
						GetEventSink_NOCSLOCK()->OnException(e);
				}
				catch (std::exception &e)
				{

					Exception ex(e);
					ex.m_remote = UWI.Internal().m_unsafeMessage.m_remoteHostID;
					ex.m_userCallbackName.Format(_PNT("RMI (ID=%d)"), rmiID);
					ex.m_delegateObject = stub;
					if (Get_HasCoreEventFunctionObjects().OnException.m_functionObject)
						Get_HasCoreEventFunctionObjects().OnException.m_functionObject->Run(ex);
					if (GetEventSink_NOCSLOCK() != nullptr /*&& !m_shutdowning*/)
						GetEventSink_NOCSLOCK()->OnException(ex);

				}
#ifdef _COMDEF_NOT_WINAPI_FAMILY_DESKTOP_APP
				catch (_com_error &e)
				{

					Exception ext; Exception_UpdateFromComError(ext, e);
					ext.m_remote = UWI.Internal().m_unsafeMessage.m_remoteHostID;
					ext.m_userCallbackName.Format(_PNT("RMI (ID=%d)"), rmiID);
					ext.m_delegateObject = stub;

					if (Get_HasCoreEventFunctionObjects().OnException.m_functionObject)
						Get_HasCoreEventFunctionObjects().OnException.m_functionObject->Run(ext);

					if (GetEventSink_NOCSLOCK() != nullptr /*&& !m_shutdowning*/)
						GetEventSink_NOCSLOCK()->OnException(ext);

				}
#endif // _COMDEF_NOT_WINAPI_FAMILY_DESKTOP_APP
#ifdef ALLOW_CATCH_UNHANDLED_EVEN_FOR_USER_ROUTINE
				catch (...)
				{
					Exception ex;
					ex.m_exceptionType = ExceptionType_Unhandled;
					ex.m_remote = UWI.Internal().m_unsafeMessage.m_remoteHostID;
					ex.m_userCallbackName.Format(_PNT("RMI (ID=%d)"), rmiID);
					ex.m_delegateObject = stub;
					if (Get_HasCoreEventFunctionObjects().OnException.m_functionObject)
						Get_HasCoreEventFunctionObjects().OnException.m_functionObject->Run(ex);
					if (m_owner->m_eventSink_NOCSLOCK != nullptr /*&& !m_owner->m_shutdowning*/)
						m_owner->m_eventSink_NOCSLOCK->OnException(ex);
				}
#endif // ALLOW_CATCH_UNHANDLED_EVEN_FOR_USER_ROUTINE
			}
		}

		// #NEED_RAII
		if (!processed)
		{
			msgContent.SetReadOffset(oldReadPointer);

			if (workResult != nullptr)
				++workResult->m_processedEventCount;

			if (Get_HasCoreEventFunctionObjects().OnNoRmiProcessed.m_functionObject)
				Get_HasCoreEventFunctionObjects().OnNoRmiProcessed.m_functionObject->Run(rmiID);

			if (GetEventSink_NOCSLOCK())
				GetEventSink_NOCSLOCK()->OnNoRmiProcessed(rmiID);

		}
	}

	void CNetCoreImpl::UserWork_FinalReceiveUserMessage(CFinalUserWorkItem& UWI, const shared_ptr<CHostBase>& subject, CWorkResult* workResult)
	{
		AssertIsNotLockedByCurrentThread();

		CMessage& msgContent = UWI.Internal().m_unsafeMessage.GetReadOnlyMessage();
		int oldReadPointer = msgContent.GetReadOffset();

		if (!(oldReadPointer == 0))
			EnqueueHackSuspectEvent(shared_ptr<CHostBase>(), __FUNCTION__, HackType_PacketRig);

		RmiContext rmiContext;
		rmiContext.m_sentFrom = UWI.Internal().m_unsafeMessage.m_remoteHostID;
		rmiContext.m_relayed = UWI.Internal().m_unsafeMessage.m_relayed;
		rmiContext.m_hostTag = subject->m_hostTag;
		rmiContext.m_encryptMode = UWI.Internal().m_unsafeMessage.m_encryptMode;
		rmiContext.m_compressMode = UWI.Internal().m_unsafeMessage.m_compressMode;

		if (m_allowExceptionEvent == false)
		{
			int payloadLength = msgContent.GetLength() - msgContent.GetReadOffset();
			uint8_t* msgPtr = msgContent.GetData() + msgContent.GetReadOffset();

			if (Get_HasCoreEventFunctionObjects().OnReceiveUserMessage.m_functionObject)
				Get_HasCoreEventFunctionObjects().OnReceiveUserMessage.m_functionObject->Run(UWI.Internal().m_unsafeMessage.m_remoteHostID, rmiContext, msgPtr, payloadLength);

			if (GetEventSink_NOCSLOCK())
				GetEventSink_NOCSLOCK()->OnReceiveUserMessage(UWI.Internal().m_unsafeMessage.m_remoteHostID, rmiContext, msgPtr, payloadLength);

			// OnReceiveUserMessage에서 Exception이 발생할 경우는 처리하지 않습니다.
			if (workResult != nullptr)
				++workResult->m_processedMessageCount;
		}
		else
		{
			try
			{
				int payloadLength = msgContent.GetLength() - msgContent.GetReadOffset();
				uint8_t* msgPtr = msgContent.GetData() + msgContent.GetReadOffset();

				if (Get_HasCoreEventFunctionObjects().OnReceiveUserMessage.m_functionObject)
					Get_HasCoreEventFunctionObjects().OnReceiveUserMessage.m_functionObject->Run(UWI.Internal().m_unsafeMessage.m_remoteHostID, rmiContext, msgPtr, payloadLength);

				if (GetEventSink_NOCSLOCK())
					GetEventSink_NOCSLOCK()->OnReceiveUserMessage(UWI.Internal().m_unsafeMessage.m_remoteHostID, rmiContext, msgPtr, payloadLength);

				// OnReceiveUserMessage에서 Exception이 발생할 경우는 처리하지 않습니다.
				if (workResult != nullptr)
					++workResult->m_processedMessageCount;
			}
			catch (std::bad_alloc &e)
			{
				Exception ex(e);
				ex.m_remote = UWI.Internal().m_unsafeMessage.m_remoteHostID;
				ex.m_userCallbackName = _PNT("OnReceiveUserMessage");
				ex.m_delegateObject = GetEventSink_NOCSLOCK();
				BadAllocException(ex);
			}
			catch (Exception& e)
			{
				e.m_remote = UWI.Internal().m_unsafeMessage.m_remoteHostID;
				e.m_userCallbackName = _PNT("OnReceiveUserMessage");
				e.m_delegateObject = GetEventSink_NOCSLOCK();
				if (Get_HasCoreEventFunctionObjects().OnException.m_functionObject)
					Get_HasCoreEventFunctionObjects().OnException.m_functionObject->Run(e);
				if (GetEventSink_NOCSLOCK())
					GetEventSink_NOCSLOCK()->OnException(e);
			}
			catch (std::exception &e)
			{
				Exception ex(e);
				ex.m_remote = UWI.Internal().m_unsafeMessage.m_remoteHostID;
				ex.m_userCallbackName = _PNT("OnReceiveUserMessage");
				ex.m_delegateObject = GetEventSink_NOCSLOCK();
				if (Get_HasCoreEventFunctionObjects().OnException.m_functionObject)
					Get_HasCoreEventFunctionObjects().OnException.m_functionObject->Run(ex);
				if (GetEventSink_NOCSLOCK())
					GetEventSink_NOCSLOCK()->OnException(ex);
			}
#ifdef _COMDEF_NOT_WINAPI_FAMILY_DESKTOP_APP
			catch (_com_error &e)
			{
				Exception ext; Exception_UpdateFromComError(ext, e);
				ext.m_remote = UWI.Internal().m_unsafeMessage.m_remoteHostID;
				ext.m_userCallbackName = _PNT("OnReceiveUserMessage");
				ext.m_delegateObject = GetEventSink_NOCSLOCK();
				if (OnException.m_functionObject)
					OnException.m_functionObject->Run(ext);
				if(GetEventSink_NOCSLOCK())
					GetEventSink_NOCSLOCK()->OnException(ext);
			}
#endif // _COMDEF_NOT_WINAPI_FAMILY_DESKTOP_APP
#ifdef ALLOW_CATCH_UNHANDLED_EVEN_FOR_USER_ROUTINE
			catch (...)
			{
				Exception ex;
				ex.m_exceptionType = ExceptionType_Unhandled;
				ex.m_remote = UWI.Internal().m_unsafeMessage.m_remoteHostID;
				ex.m_userCallbackName = _PNT("OnReceiveUserMessage");
				ex.m_delegateObject = GetEventSink_NOCSLOCK();
				if (Get_HasCoreEventFunctionObjects().OnException.m_functionObject)
					Get_HasCoreEventFunctionObjects().OnException.m_functionObject->Run(ex);
				if (GetEventSink_NOCSLOCK())
					m_owner->GetEventSink_NOCSLOCK()->OnException(ex);
			}
#endif // ALLOW_CATCH_UNHANDLED_EVEN_FOR_USER_ROUTINE
		}
	}

	void CNetCoreImpl::UserWork_FinalReceiveHla(CFinalUserWorkItem& UWI, const shared_ptr<CHostBase>& /*subject*/, CWorkResult*)
	{
		AssertIsNotLockedByCurrentThread();

		CMessage& msgContent = UWI.Internal().m_unsafeMessage.GetReadOnlyMessage();
		int oldReadPointer = msgContent.GetReadOffset();

		if (!(oldReadPointer == 0))
			EnqueueHackSuspectEvent(shared_ptr<CHostBase>(), __FUNCTION__, HackType_PacketRig);
//
// 		if (m_hlaSessionHostImpl != nullptr)
// 			m_hlaSessionHostImpl->ProcessMessage(UWI.Internal().m_unsafeMessage);
	}


//#ifdef _WIN32
// 	void CNetCoreImpl::IssueFirstRecv()
// 	{
// 		CriticalSectionLock lock(GetCriticalSection(), true);
//
// 		for (CSuperSocketArray::iterator i = m_issueFirstRecvNeededSockets.begin(); i != m_issueFirstRecvNeededSockets.end(); i++)
// 		{
// 			CSuperSocket* s = *i;
// 			s->IssueRecv();
// 		}
//
// 		// 다 처리했으므로 청소!
// 		m_issueFirstRecvNeededSockets.Clear();
// 	}

// 	void CNetCoreImpl::IssueFirstRecvNeededSocket_Set(shared_ptr<CSuperSocket> s)
// 	{
// 		CriticalSectionLock lock(GetCriticalSection(), true);
//
// 		m_issueFirstRecvNeededSockets.Add(s);
// 	}
//#endif

	void CNetCoreImpl::EnqueueHackSuspectEvent(const shared_ptr<CHostBase>& rc, const char* statement, HackType hackType)
	{
		CriticalSectionLock clk(GetCriticalSection(), true); // caller가 main lock상태일 수 있으므로
		CHECK_CRITICALSECTION_DEADLOCK(this);

		LocalEvent evt;
		evt.m_errorInfo = ErrorInfoPtr(new ErrorInfo());
		evt.m_hackType = hackType;
		evt.m_remoteHostID = rc ? rc->GetHostID() : HostID_None;
		evt.m_type = LocalEventType_HackSuspected;
		evt.m_errorInfo->m_comment = StringA2T(statement);
		EnqueLocalEvent(evt, rc);
	}

	// 모든 host들을 garbage 처분한다.
	// 처분한 것들은 m_garbagedHosts에 들어간다.
	// m_garbagedHosts까지 다 청소되는지는 CanDeleteNow()를 통해 확인하자.
	// 이 함수를 오버라이드해서 더 필요한 것에 대한 처분 기능을 추가해도 된다.
	void CNetCoreImpl::GarbageAllHosts()
	{
		// 모든 클라이언트의 연결 해제를 한다.
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		for (CFastMap2<void*, shared_ptr<CHostBase>, int>::iterator i = m_candidateHosts.begin(); i != m_candidateHosts.end(); i++)
		{
			shared_ptr<CHostBase> rc = i->GetSecond();
			GarbageHost(rc, ErrorType_DisconnectFromLocal, ErrorType_TCPConnectFailure,
				ByteArray(), _PNT("NC.GAH"), SocketErrorCode_Ok);
		}

		for (AuthedHostMap::iterator i = m_authedHostMap.begin(); i != m_authedHostMap.end(); i++)
		{
			shared_ptr<CHostBase> rc = i->GetSecond();
			GarbageHost(rc, ErrorType_DisconnectFromLocal, ErrorType_TCPConnectFailure,
				ByteArray(), _PNT("NC.GAH"), SocketErrorCode_Ok);
		}

		// NOTE: host를 garbage하더라도 authed or candidate에서는 제거 안한다.
		// 아직 남은 final user work item을 처리하려면 해당 목록에 아직 남아있어야 유저가 원하는 정보를 제공할 수 있기 때문이다.
	}

	// 이 객체를 delete해도 되는지?
	// I/O completion이 아직 진행중인 것들이 있으면 false를 리턴한다.
	// 오버라이드 가능.
	bool CNetCoreImpl::CanDeleteNow()
	{
		CriticalSectionLock clk(GetCriticalSection(), true);

		// remote host가 살아있으면 delete 안됨.
		if (!m_authedHostMap.IsEmpty())
			return false;

		// 허가 대기중인 remote host가 살아있으면 delete 안됨.
		if (!m_candidateHosts.IsEmpty())
			return false;

		// 폐기중이지만 완료가 안된 hosts들이 있으면 delete 안됨.
		if (!m_garbagedHosts.IsEmpty())
			return false;

		// 폐기중이지만 완료가 안된 socket들이 있으면 delete 안됨.
		if (!m_garbagedSockets.IsEmpty())
			return false;

		// 홀펀칭 재사용용 대기 소켓이 아직 폐기되지 않고 살아있으면 delete 안됨.
		if (!m_recycles.IsEmpty())
			return false;

		// NetCoreHeart 도 더 이상의 액세스가 없을 때 비로소 종료가 허가된다.
		shared_ptr<CReferrerHeart> acq;
		TryGetReferrerHeart(acq);
		if (acq)
			return false;

		return true;
	}

	// CanDeleteNow와 유관한 상태 정보를 출력한다.
	// 디버그용.
	void CNetCoreImpl::CanDeleteNow_DumpStatus()
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		cout << "AMP count=" << m_authedHostMap.GetCount() << endl;
		cout << "CH count=" << m_candidateHosts.GetCount() << endl;
		cout << "GH count=" << m_garbagedHosts.GetCount() << endl;
		cout << "GS count=" << m_garbagedHosts.GetCount() << endl;
		cout << "R count=" << m_recycles.GetCount() << endl;

 		int maxLoop = 0; // 출력에 너무 오래 걸려서 100개까지만 찍는다.
 		for (GarbagedSockets::iterator i = m_garbagedSockets.begin(); i != m_garbagedSockets.end(); i++)
 		{
 			maxLoop++;
 			i->GetSecond()->CanDeleteNow_DumpStatus();
			if (maxLoop > 100)
			{
				cout << "There are more to print, but we stop for saving your time.";
				break;
			}
 		}

	}

	int CNetCoreImpl::GetOverSendSuspectingThresholdInBytes(CSuperSocket* /*socket*/)
	{
		return m_settings.m_overSendSuspectingThresholdInBytes;
	}

	// rc에 대한 local event e를 추가한다.
	// 추가하면서 task subject list에도 넣으며 custom event도 추가하는 과정도 모두 한다.
	void CNetCoreImpl::EnqueLocalEvent(LocalEvent& e, const shared_ptr<CHostBase>& rc)
	{
		AssertIsLockedByCurrentThread();

		// 만약 netcore가 셧다운 중인 경우에는 이벤트를 안 쌓는다.
		if (m_netThreadPool != nullptr)  
		{
			CFinalUserWorkItem ri(e);
			TryGetReferrerHeart(ri.Internal().m_netCoreReferrerHeart);
			if(ri.Internal().m_netCoreReferrerHeart)
			m_userTaskQueue.Push(rc, ri);
		}
	}

	void CNetCoreImpl::Candidate_Remove(const shared_ptr<CHostBase>& rc)
	{
		CriticalSectionLock clk(GetCriticalSection(), true);
		CHECK_CRITICALSECTION_DEADLOCK(this);

		m_candidateHosts.Remove(rc.get());
	}

	void CNetCoreImpl::DeleteSendReadyList()
	{
		m_sendReadyList.reset();
	}


	int CNetCoreImpl::GetFinalUserWotkItemCount()
	{
		int workItemCount = 0;
		for (CFastMap2<void*, shared_ptr<CHostBase>, int>::iterator i = m_candidateHosts.begin(); i != m_candidateHosts.end(); i++)
		{
			shared_ptr<CHostBase> rc = i->GetSecond();
			workItemCount += rc->GetFinalUserWotkItemCount_STALE();
		}

		for (AuthedHostMap::iterator i = m_authedHostMap.begin(); i != m_authedHostMap.end(); i++)
		{
			shared_ptr<CHostBase> rc = i->GetSecond();
			workItemCount += rc->GetFinalUserWotkItemCount_STALE();
		}

		for (CFastMap2<void*, shared_ptr<CHostBase>, int>::iterator i = m_garbagedHosts.begin(); i != m_garbagedHosts.end(); i++)
		{
			shared_ptr<CHostBase> rc = i->GetSecond();
			workItemCount += rc->GetFinalUserWotkItemCount_STALE();
		}

		return workItemCount;
	}

	// user work item을 가진 task subject (HostBase)를 찾아,
	// 일을 수행한다.
	// 처리할게 없으면 그냥 리턴한다.
	void CNetCoreImpl::DoUserTask(const ThreadPoolProcessParam& param, CWorkResult* workResult, bool& holstered)
	{
		CUserWorkerThreadCallbackContext ctx;

		// user code 실행 중 데드락 예방
		AssertIsNotLockedByCurrentThread();

		/* Q: socket task, user task의 fairness를 위해서, 하나만 처리해야 하지 않나요?
		   A: PQCS or eventfd.write는 syscall을 유발합니다.
		      단순 핑퐁 서버의 경우 처리 댓가가 큽니다.
			  그러한 경우에서도 속도가 나오려면 do until no more를 할 수밖에 없습니다.
		*/
		while (true)
		{
			/* 예전에는 여기가 이렇게 작동했다.

			main lock을 잠깐 하고, user task queue에서 항목을 꺼낸다.
			main unlock 후 user code를 수행한다.
			main lock을 다시 하고, working flag reset을 한다.

			그런데 이 방식의 문제가 있다. main lock을 두번이나 하는 데 있다.

			따라서, 아래와 같이 user task queue에 대한 lock을 따로 가지도록 수정하였다.

			여기서는 pop을 할 때 대응하는 NetCore 객체가 비파괴 보장이 되고 있다.
			DOC-USER-TASK-QUEUE-USE-COUNT 참고하라.
			*/

			// 유저가 HolsterMoreCallbackUntilNextFrameMove를 호출하면 Queue에 처리할 작업이 있어도 일단 리턴한다. 다음 FrameMove를 호출하면 처리될 것이다.
			if (m_HolsterMoreCallbackUntilNextProcessCall_flagged)
			{
				holstered = true;
				m_HolsterMoreCallbackUntilNextProcessCall_flagged = false;
				break;
			}

			CUserTaskQueue::CPoppedTask poppedTask;
			if (!m_userTaskQueue.Pop(poppedTask))
				break;

			CFinalUserWorkItem &Item = poppedTask.m_workItem;
			shared_ptr<CHostBase> subject = poppedTask.m_taskSubject;

			AssertIsNotLockedByCurrentThread(); // 병렬병목과 데드락 예방

			INetCoreEvent * eventSink = GetEventSink_NOCSLOCK();
			if (param.m_enableUserCallback)
			{
				if (eventSink != nullptr)
					eventSink->OnUserWorkerThreadCallbackBegin(&ctx);

				if (Get_HasCoreEventFunctionObjects().OnUserWorkerThreadCallbackBegin.m_functionObject)
					Get_HasCoreEventFunctionObjects().OnUserWorkerThreadCallbackBegin.m_functionObject->Run();
			}

			if(param.m_enableUserCallback)
			{
//				span(g_mySeries, _T("User callback"));

				switch (Item.Internal().m_type)
				{
				case UWI_RMI:
					UserWork_FinalReceiveRmi(Item, subject, workResult);
					break;
				case UWI_UserMessage:
					UserWork_FinalReceiveUserMessage(Item, subject, workResult);
					break;
				case UWI_Hla:
					UserWork_FinalReceiveHla(Item, subject, workResult);
					break;

				case UWI_UserFunction:
					UserWork_FinalReceiveUserFunction(Item, subject, workResult);
					break;

				default:
					UserWork_LocalEvent(Item, subject, workResult);
					break;
				}
			}

			if (param.m_enableUserCallback)
			{
				if (eventSink != nullptr /*&& !m_owner->m_shutdowning*/)
					eventSink->OnUserWorkerThreadCallbackEnd(&ctx);

				if (Get_HasCoreEventFunctionObjects().OnUserWorkerThreadCallbackEnd.m_functionObject != nullptr /*&& !m_owner->m_shutdowning*/)
					Get_HasCoreEventFunctionObjects().OnUserWorkerThreadCallbackEnd.m_functionObject->Run();
			}

			// NOTE: poppedTask가 파괴되면서 task subject의 is-working flag가 자동 해제된다.
		}

	}

#ifdef _DEBUG
	// critical section을 제대로 쓰고 있는지 검사한다. 디버그 전용.
	// Q: 굳이 이렇게 함수 자체를 디버그 전용으로 감싸야 하나요?
	// A: 안그러면 실행파일 바이너리에 함수 이름이 떡 하니 박힙니다. 해커가 좋아하죠.
	void CNetCoreImpl::CheckCriticalsectionDeadLock(const PNTCHAR* functionName)
	{
		if (CNetConfig::CheckDeadLock)
		{
			CheckCriticalsectionDeadLock_INTERNAL(functionName);
		}
	}
#endif

	void CNetCoreImpl::AllocPreventOutOfMemory()
	{
		CriticalSectionLock(m_preventOutOfMemoryCritSec, true);

		if (m_preventOutOfMemoryDisconnectError == nullptr)
			m_preventOutOfMemoryDisconnectError = CProcHeap::Alloc(10000);
	}

	void CNetCoreImpl::FreePreventOutOfMemory()
	{
		CriticalSectionLock(m_preventOutOfMemoryCritSec, true);

		if (m_preventOutOfMemoryDisconnectError != nullptr)
			CProcHeap::Free(m_preventOutOfMemoryDisconnectError);

		m_preventOutOfMemoryDisconnectError = nullptr;
	}

	// #TCP_KEEP_ALIVE
	// socket이 존재하면 result를 업데이트한다. 존재 안하면 업데이트 안한다.
	// 업데이트는, last received time out이냐에 따라서.
	void CNetCoreImpl::UpdateSocketLastReceivedTimeOutState(
		const shared_ptr<CSuperSocket>& socket, 
		int64_t currTime,
		int timeOut,
		SocketResult* result)
	{
		if (!socket)
		{
			// result를 건들지 말아야.
			return;
		}

		if (m_settings.m_defaultTimeoutTimeMs <= 0)
		{
			if (m_settings.m_defaultTimeoutTimeMs < 0)
			{
				NTTNTRACE("Warning: weird value DTT!\n");
			}
			// 타임아웃 검사 자체를 하지 말아야. 즉 항상 not time out이어야.
			*result = NotTimeOut;
			return;
		}

		if (currTime - socket->m_lastReceivedTime >= timeOut)
		{
			*result = TimeOut;
		}
		else
			*result = NotTimeOut;
	}

	// RmiContext의 송신실패 목록에 실패한 수신대상을 항목들을 추가한다.
	// 단, '송신대상 목록을 채워라' 값이 켜져 있는 경우에만 한다.
	void CNetCoreImpl::FillSendFailListOnNeed(const SendOpt& sendContext, const HostID* sendTo, int numberOfsendTo, ErrorType sendFailReason)
	{
		if(sendContext.m_pSendWorkOutput != nullptr && sendContext.m_pSendWorkOutput->m_fillSendFailedRemotes)
		{
			for (int i = 0; i < numberOfsendTo; i++)
			{
				SendFailedRemote r;
				r.m_hostID = sendTo[i];
				r.m_reason = sendFailReason;
				sendContext.m_pSendWorkOutput->m_sendFailedRemotes.Add(r);
			}
		}
	}

	void AdjustSendOpt(SendOpt& sendOpt)
	{
		// Reliable send를 하는 경우 어떠한 일이 있어도 데이터를 씹으면 안된다.
		// 이슈 3437 때문이다.
		// 의미상으로도 reliable은 reliable이다.
		// 이게 문제가 되면, 사용자는 unreliable에서의 혼잡제어 기능에 의존하는게 옳다.
		if (sendOpt.m_reliability == MessageReliability_Reliable)
			sendOpt.m_uniqueID.m_value = 0;
	}

}
