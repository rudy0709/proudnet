/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.

blocked function call을 구현한 모듈.
내부적으로는 요청-응답형 함수를 이용하여 스레드 대기-깸 방식으로 작동한다.

*/

#if defined(_WIN32)

#include "stdafx.h"
#include "../include/variant-marshaler.h"
#include "DbCacheClient2Impl.h"
#include "dbconfig.h"
#include "RmiContextImpl.h"

namespace Proud
{
	bool CDbCacheClient2Impl::BlockedAddData(Guid rootUUID, Guid ownerUUID, CPropNodePtr addData, uint32_t timeOutTime /*= 30000*/, ErrorInfoPtr outError /*= ErrorInfoPtr()*/)
	{
		CriticalSectionLock lock(m_cs, true);

		//UserWorkerthread에서 호출되면 안됨.
		if (IsCurrentThreadUserWorker())
		{
			outError->m_errorType = ErrorType_Unexpected;
			outError->m_comment = _PNT("BlockedAddData failed! Call is bug in UserWorkerThread!");
			return false;
		}

		CLoadedData2* loadedData = GetOrgLoadedDataByRootUUID(rootUUID);
		if (loadedData == NULL)
		{
			outError->m_errorType = ErrorType_BadSessionGuid;
			outError->m_comment = _PNT("BlockedAddData failed! NodeData that confronts RootUUID cannot be found.");
			return false;
		}

		CPropNode* parentNode = loadedData->GetNode(ownerUUID);

		if (parentNode == NULL)
		{
			outError->m_errorType = ErrorType_BadSessionGuid;
			outError->m_comment = _PNT("BlockedAddData failed! NodeData that confronts OwnerUUID cannot be found.");
			return false;
		}

		addData->m_UUID = Win32Guid::NewGuid();

		addData->m_RootUUID = rootUUID;

		addData->m_ownerUUID = ownerUUID;

		//blockEvent 할당.
		BlockedEventPtr blockEvent(new BlockedEvent);

		blockEvent->m_event = m_eventPool.NewOrRecycle();

		intptr_t tag = (intptr_t)blockEvent->m_event->m_event;

		ByteArray addDataBlock;

		addData->ToByteArray(addDataBlock);

		m_activeEvents.Add(tag, blockEvent);


		// blocked에 true를 보내서 사용자가 호출한 RequestAddData와 구분한다.
		m_c2sProxy.RequestAddData(HostID_Server, g_ReliableSendForPN, rootUUID, ownerUUID, addDataBlock, (int64_t)tag, true);

		// 사용자가 응답 줄 때까지 대기. 잠금 안하고 있어야.
		lock.Unlock();
		bool isNotTimeout = blockEvent->m_event->WaitOne(timeOutTime);

		lock.Lock();

		m_activeEvents.TryGetValue(tag, blockEvent);

		m_activeEvents.RemoveKey(tag);

		if (isNotTimeout == false)
		{
			// 기다렸는데 응답이 안왔음.
			m_eventPool.Drop(blockEvent->m_event);

			if (outError != NULL)
			{
				outError->m_errorType = ErrorType_TimeOut;
				outError->m_comment = _PNT("time out");
			}

			return false;
		}

		if (!blockEvent->m_success && outError != NULL)
		{
			// 결과는 왔는데 실패라고.
			outError->m_errorType = blockEvent->m_errorType;
			outError->m_comment = blockEvent->m_comment;
			outError->m_hResult = blockEvent->m_hResult;
			outError->m_source = blockEvent->m_source;
		}

		m_eventPool.Drop(blockEvent->m_event);

		return blockEvent->m_success;
	}

	bool CDbCacheClient2Impl::BlockedUpdateData(CPropNodePtr updateData, uint32_t timeOutTime /*= 30000*/, ErrorInfoPtr outError /*= ErrorInfoPtr()*/)
	{
		CriticalSectionLock lock(m_cs, true);

		//UserWorkerthread에서 호출되면 안됨.
		if (IsCurrentThreadUserWorker())
		{
			outError->m_errorType = ErrorType_Unexpected;
			outError->m_comment = _PNT("BlockUpdateData failed! Call is bug in UserWorkerThread!");
			return false;
		}

		CLoadedData2* oldOwnerData = GetOrgLoadedDataByRootUUID(updateData->RootUUID);
		if (oldOwnerData == NULL)
		{
			outError->m_errorType = ErrorType_BadSessionGuid;
			outError->m_comment = _PNT("BlockUpdateData failed! NodeData that confronts RootUUID cannot be found.");
			return false;
		}

		CPropNode* oldData = oldOwnerData->GetNode(updateData->UUID);
		if (oldData == NULL)
		{
			outError->m_errorType = ErrorType_BadSessionGuid;
			outError->m_comment = _PNT("BlockUpdateData failed! NodeData that confronts UUID cannot be found.");
			return false;
		}

		//blockEvent 할당.
		BlockedEventPtr blockEvent(new BlockedEvent);

		blockEvent->m_event = m_eventPool.NewOrRecycle();

		intptr_t tag = (intptr_t)blockEvent->m_event->m_event;

		// 서버에 요청한다.
		ByteArray block;

		updateData->ToByteArray(block);

		m_activeEvents.Add(tag, blockEvent);

		Guid rootUUID = oldOwnerData->RootUUID;

		// blocked에 true를 보내서 사용자가 호출한 RequestUpdateData와 구분한다.
		m_c2sProxy.RequestUpdateData(HostID_Server, g_ReliableSendForPN, rootUUID, block, tag, true);

		// 사용자가 응답 줄 때까지 대기. 잠금 안하고 있어야.
		lock.Unlock();
		bool isNotTimeout = blockEvent->m_event->WaitOne(timeOutTime);

		lock.Lock();

		m_activeEvents.TryGetValue(tag, blockEvent);

		m_activeEvents.RemoveKey(tag);

		if (isNotTimeout == false)
		{
			// 기다렸는데 응답이 안왔음.
			m_eventPool.Drop(blockEvent->m_event);

			if (outError)
			{
				outError->m_errorType = ErrorType_TimeOut;
				outError->m_comment = _PNT("timeOut");
			}

			return false;
		}

		if (!blockEvent->m_success && outError)
		{
			// 결과는 왔는데 실패라고.
			outError->m_errorType = blockEvent->m_errorType;
			outError->m_comment = blockEvent->m_comment;
			outError->m_hResult = blockEvent->m_hResult;
			outError->m_source = blockEvent->m_source;
		}

		m_eventPool.Drop(blockEvent->m_event);

		return blockEvent->m_success;
	}

	bool CDbCacheClient2Impl::BlockedRecursiveUpdateData(CLoadedData2Ptr loadedData, bool transactional /*= true*/, uint32_t timeOutTime /*= 30000*/, ErrorInfoPtr outError /*= ErrorInfoPtr()*/)
	{
		CriticalSectionLock lock(m_cs, true);

		if (IsCurrentThreadUserWorker())
		{
			outError->m_errorType = ErrorType_Unexpected;
			outError->m_comment = _PNT("BlockedRecursiveUpdateData failed! Call is bug in UserWorkerThread!");
			return false;
		}

		CLoadedData2* oldOwnerData = GetOrgLoadedDataByRootUUID(loadedData->RootUUID);
		if (oldOwnerData == NULL)
		{
			outError->m_errorType = ErrorType_BadSessionGuid;
			outError->m_comment = _PNT("BlockedRecursiveUpdateData failed! NodeData that confronts RootUUID cannot be found.");
			return false;
		}

		//blockEvent 할당.
		BlockedEventPtr blockEvent(new BlockedEvent);

		blockEvent->m_event = m_eventPool.NewOrRecycle();

		intptr_t tag = (intptr_t)blockEvent->m_event->m_event;

		// 서버에 요청한다.
		ByteArray userblock;

		//olddata를 호출하도록 변경 이유: 외부에서 들어온데이터는 신뢰 할수 없기때문이다.
		loadedData->_INTERNAL_ChangeToByteArray(userblock);

		//보낸후에 dirtyflag와 삭제리스트를 삭제한다.
		loadedData->_INTERNAL_ClearChangeNode();

		m_activeEvents.Add(tag, blockEvent);

		Guid rootUUID = oldOwnerData->RootUUID;


		// blocked에 true를 보내서 사용자가 호출한 RequestRecursiveUpdateData와 구분한다.
		m_c2sProxy.RequestUpdateDataList(HostID_Server, g_ReliableSendForPN, rootUUID, userblock, tag, transactional, true);

		// 사용자가 응답 줄 때까지 대기. 잠금 안하고 있어야.
		lock.Unlock();
		bool isNotTimeout = blockEvent->m_event->WaitOne(timeOutTime);

		lock.Lock();

		m_activeEvents.TryGetValue(tag, blockEvent);

		m_activeEvents.RemoveKey(tag);

		if (isNotTimeout == false)
		{
			// 기다렸는데 응답이 안왔음.
			m_eventPool.Drop(blockEvent->m_event);

			if (outError)
			{
				outError->m_errorType = ErrorType_TimeOut;
				outError->m_comment = _PNT("timeOut");
			}

			return false;
		}

		if (!blockEvent->m_success && outError)
		{
			// 결과는 왔는데 실패라고.
			outError->m_errorType = blockEvent->m_errorType;
			outError->m_comment = blockEvent->m_comment;
			outError->m_hResult = blockEvent->m_hResult;
			outError->m_source = blockEvent->m_source;
		}

		m_eventPool.Drop(blockEvent->m_event);

		return blockEvent->m_success;
	}
	bool CDbCacheClient2Impl::BlockedRemoveData(Guid rootUUID, Guid removeUUID, uint32_t timeOutTime /*= 30000*/, ErrorInfoPtr outError /*= ErrorInfoPtr()*/)
	{
		CriticalSectionLock lock(m_cs, true);

		//UserWorkerthread에서 호출되면 안됨.
		if (IsCurrentThreadUserWorker())
		{
			outError->m_errorType = ErrorType_Unexpected;
			outError->m_comment = _PNT("BlockedRemoveData failed! Call is bug in UserWorkerThread!");
			return false;
		}

		CLoadedData2* data = GetOrgLoadedDataByRootUUID(rootUUID);
		if (data == NULL)
		{
			outError->m_errorType = ErrorType_BadSessionGuid;
			outError->m_comment = _PNT("BlockedRemoveData failed! NodeData that confronts RootUUID cannot be found.");
			return false;
		}

		CPropNode* removeData = data->GetNode(removeUUID);
		if (!removeData)
		{
			outError->m_errorType = ErrorType_BadSessionGuid;
			outError->m_comment = _PNT("BlockedRemoveData failed! NodeData that confronts UUID cannot be found.");
			return false;
		}

		BlockedEventPtr blockEvent(new BlockedEvent);

		//blockEvent 할당.
		blockEvent->m_event = m_eventPool.NewOrRecycle();

		intptr_t tag = (intptr_t)blockEvent->m_event->m_event;

		m_activeEvents.Add(tag, blockEvent);

		// blocked에 true를 보내서 사용자가 호출한 RequestRemoveData와 구분한다.
		m_c2sProxy.RequestRemoveData(HostID_Server, g_ReliableSendForPN, rootUUID, removeUUID, tag, true);

		// 사용자가 응답 줄 때까지 대기. 잠금 안하고 있어야.
		lock.Unlock();
		bool isNotTimeout = blockEvent->m_event->WaitOne(timeOutTime);

		lock.Lock();

		// wait를 했다. 성공했건 실패했건.
		m_activeEvents.TryGetValue(tag, blockEvent);
		m_activeEvents.RemoveKey(tag);

		if (isNotTimeout == false)
		{
			// 기다렸는데 응답이 안왔음.
			m_eventPool.Drop(blockEvent->m_event);

			if (outError != NULL)
			{
				outError->m_errorType = ErrorType_TimeOut;
				outError->m_comment = _PNT("timeOut");
			}

			return false;
		}

		if (!blockEvent->m_success && outError != NULL)
		{
			// 결과는 왔는데 실패라고.
			outError->m_errorType = blockEvent->m_errorType;
			outError->m_comment = blockEvent->m_comment;
			outError->m_hResult = blockEvent->m_hResult;
			outError->m_source = blockEvent->m_source;
		}

		// 나머지 처리
		m_eventPool.Drop(blockEvent->m_event);

		return blockEvent->m_success;
	}

}

#endif // _WIN32
