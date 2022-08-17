#include "stdafx.h"
#include "dbloadeddata2_S.h"
#include "DbCacheServer2Impl.h"

namespace Proud
{
	CLoadedData2_S::CLoadedData2_S(CDbCacheServer2Impl* owner)
	{
		m_INTERNAL_loaderClientHostID = HostID_None;
		m_hibernateStartTimeMs = 0;
		m_lastModifiedTime = 0;
		m_owner = owner;
		m_dbmsWritingThreadID = 0;
		m_unloadedHibernateDuration = owner->m_unloadedHibernateDuration;
		m_state = LoadState_None;
	}

	void CLoadedData2_S::DbmsLoad(CAdoRecordset &rs, CPropNode& data)
	{
		data.Clear();

		// 필드 값을 로딩하면서 채우되, 특별 이름을 가진 field는 별도로 저장한다.
		for (int i = 0; i < rs->Fields->Count; i++)
		{
			CVariant fieldValue = rs.FieldValues[i];
			String fieldName = (const PNTCHAR*)(_bstr_t)rs.FieldNames[i];
			fieldName.MakeUpper();
			if (fieldName == _PNT("UUID"))
			{
				data.m_UUID = fieldValue;
			}
			else if (fieldName == _PNT("OWNERUUID"))
			{
				data.m_ownerUUID = fieldValue;
			}
			else if (fieldName == _PNT("ROOTUUID"))
			{
				data.m_RootUUID = fieldValue;
			}
			else
				data.Add(fieldName, fieldValue);
		}
	}

	/* Recordset 객체에 필드 값을 채운다.
	data에 있는 것이 rs에 들어간다.
	updatePropertiesOnly=true이면, 3가지 필수 필드는 건드리지 않는다.
	대부분의 경우 이것이 갱신될 필요가 없으며 이미 같은값이더라도 문제가 된다.
	참고 http://ndn.nettention.com/general/support/view/?siidx=2200 */ 
	void CLoadedData2_S::DbmsSave(CAdoRecordset &rs, CPropNode& data, bool updatePropertiesOnly)
	{
		CProperty::MapType::iterator i;
		for ( i = data.m_map.begin(); i != data.m_map.end(); ++i )
		{
			const String& fieldName = i->GetFirst();
			rs.FieldValues[fieldName] = i->GetSecond();
		}
		if ( !updatePropertiesOnly )
		{
			rs.FieldValues[L"UUID"] = data.m_UUID;
			rs.FieldValues[L"OWNERUUID"] = data.m_ownerUUID;
			rs.FieldValues[L"ROOTUUID"] = data.m_RootUUID;
		}
	}

	// 이 LD가 디비 캐시 클라에 의해 로딩된 상태라고 표식을 남긴다.
	void CLoadedData2_S::MarkLoad(HostID dbCacheClientID, const Guid sessionGuid, LoadState state)
	{
		assert(dbCacheClientID != HostID_None);
		m_INTERNAL_loaderClientHostID = dbCacheClientID;
		m_data.sessionGuid = sessionGuid;
		m_hibernateStartTimeMs = 0;
		m_state = state; // may be LoadState_Exclusive or LoadState_Requeted.
	}

	// 휴면 상태로 만든다.
	void CLoadedData2_S::MarkUnload()
	{
		m_INTERNAL_loaderClientHostID = HostID_None;
		m_data.sessionGuid = Guid();
		m_hibernateStartTimeMs = GetPreciseCurrentTimeMs();
		m_state = LoadState_None;
	}

	bool CLoadedData2_S::IsLoaded()
	{
		return GetLoaderClientHostID() != HostID_None;
	}

	CLoadedData2_S::~CLoadedData2_S()
	{

	}

	CLoadedData2* CLoadedData2_S::GetData()
	{
		return &m_data;
	}

	void CLoadedData2_S::AddWritePendListFromRemoveNodes(HostID dbClientID, Guid removeUUID, IDbWriteDoneNotifyPtr doneNotify)
	{
		// remove되는 Node 뿐만 아니라 하위노드들도 전부 삭제해야한다.
		const PropNodeList* removeNodeList = m_data._INTERNAL_GetRemoveNodeList();

		int count = 0;

		CPropNodePtr removeNode = m_data.GetNode(removeUUID);

		// removeUUID가 지정되어 있으면 it는 remove node list에서 removeUUID가 가리키는 노드를 가리킴.
		// 미지정시 it는 remove node list의 처음을 가리킴.
		PropNodeList::const_iterator it = removeNodeList->begin();

		if (removeUUID != Guid())
		{
			for (; it != removeNodeList->end(); ++it)
			{
				CPropNodePtr node = (*it);

				if (removeNode == node)
					break;
			}
		}

		for (; it != removeNodeList->end(); ++it, ++count)
		{
			const CPropNodePtr &node = (*it);
			if (node != NULL)
			{
				//pendlist를 붙이도록 하자.
				DbmsWritePend2 newPend;
				newPend.m_dbClientID = dbClientID;
				newPend.m_changePropNode.m_type = DWPNPT_RemovePropNode;
				newPend.m_changePropNode.m_node = node->CloneNoChildren();
				newPend.m_changePropNode.m_UUID = node->GetUUID();

				// donNotify를 해야하고, 첫번째 노드라면
				// 첫번째 노드에서 doneNotify를 해야하는 이유는 실제 로컬 메모리를 지울때
				// removedata를 최초 한번만 해야하기때문임. (최초 한번할때 모든 data를 삭제및 재링크한다.)
				if (doneNotify != NULL && count == 0)
					newPend.m_DoneNotify = doneNotify;

				//작성한 리스트를 토대로 pendlist를 작성.
				m_dbmsWritePendList.AddTail(newPend);
			}
		}
	}


}

