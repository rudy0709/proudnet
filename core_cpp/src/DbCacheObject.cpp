/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include "../include/IRMIProxy.h"
#include "../include/IRMIStub.h"
#include "DbCacheObject.h"
#include "DbCacheServer2Impl.h"
#include "RmiContextImpl.h"


namespace Proud
{
	bool CDbRemoteClient_S::IsLoggedOn()
	{
		return m_logonTime!=0;
	}

	CDbWriteDoneAddNotify::CDbWriteDoneAddNotify(CDbCacheServer2Impl* owner, int64_t tag, bool blocked) :
		m_owner(owner),
		m_tag(tag),
		m_blocked(blocked)
	{

	}

	CDbWriteDoneAddNotify::~CDbWriteDoneAddNotify()
	{

	}

	void CDbWriteDoneAddNotify::Success(HostID requester, CPropNode &modifyData, DbmsWritePropNodeListPendArray &/*modifyDataList*/)
	{
		CriticalSectionLock lock(m_owner->m_cs, true);

		CLoadedData2Ptr_S loadData = m_owner->GetLoadedDataByRootGuid(modifyData.RootUUID);

		if ( !loadData )
			return;

		CPropNodePtr addData = CPropNodePtr(new CPropNode(modifyData.TypeName));
		*addData = modifyData;

		CPropNodePtr candidateData = loadData->m_data.GetNode(modifyData.UUID);

		if ( candidateData )
		{
			m_owner->m_s2cProxy.NotifyDbmsAccessError(requester, g_ReliableSendForPN, m_tag, ByteArray(), _PNT("Add Final Already Exist"));

			throw Exception("AccessError: Dbms Write Success but Add Final LocalMemory Already Exist");
		}

		CPropNodePtr ownerData = loadData->m_data.GetNode(modifyData.OwnerUUID);

		loadData->m_data.InsertChild(ownerData, addData);
		loadData->m_data._INTERNAL_ClearChangeNode();

		ByteArray block;
		modifyData.ToByteArray(block);

		m_owner->m_s2cProxy.NotifyAddDataSuccess(requester, g_ReliableSendForPN, m_tag, modifyData.RootUUID, block, m_blocked);
	}

	void CDbWriteDoneAddNotify::Failed(HostID requester, String comment, long result /*= 0*/, String source /*= String()*/)
	{
		m_owner->m_s2cProxy.NotifyAddDataFailed(requester, g_ReliableSendForPN, m_tag, ErrorType_Unexpected, comment, result, source, m_blocked);
	}



	CDbWriteDoneUpdateNotify::CDbWriteDoneUpdateNotify(CDbCacheServer2Impl* owner, int64_t tag, bool blocked) :
		m_owner(owner),
		m_tag(tag),
		m_blocked(blocked)
	{

	}

	CDbWriteDoneUpdateNotify::~CDbWriteDoneUpdateNotify()
	{

	}

	void CDbWriteDoneUpdateNotify::Success(HostID requester, CPropNode &modifyData, DbmsWritePropNodeListPendArray &/*modifyDataList*/)
	{
		CriticalSectionLock lock(m_owner->m_cs, true);

		CLoadedData2Ptr_S loadData = m_owner->GetLoadedDataByRootGuid(modifyData.RootUUID);

		if ( !loadData )
			return;

		CPropNodePtr updateData = loadData->m_data.GetNode(modifyData.UUID);

		if ( !updateData )
		{
			//이런 상황이면 대략 난감이다...
			m_owner->m_s2cProxy.NotifyDbmsAccessError(requester, g_ReliableSendForPN, m_tag, ByteArray(), _PNT("Update Final Not Exist"));
			throw Exception("AccessError: Dbms Write Success but Update Final LocalMemory Not Exist");
		}

		//hard가 진행되는 동안에 soft가 진행되었다면....accesserror
		if ( updateData->IsSoftWorkIssued() )
		{
			m_owner->m_s2cProxy.NotifyDbmsAccessError(requester, g_ReliableSendForPN, m_tag, ByteArray(), _PNT("Update Final but Dirty LocalMemory"));
			throw Exception("AccessError: Dbms Write Success but Update Final LocalMemory is Dirty");
		}

		//값만 바꾸자.
		*updateData = (CProperty)modifyData;
		loadData->m_data._INTERNAL_ClearChangeNode();

		ByteArray block;
		modifyData.ToByteArray(block);

		m_owner->m_s2cProxy.NotifyUpdateDataSuccess(requester, g_ReliableSendForPN, m_tag, modifyData.RootUUID, block, m_blocked);
	}

	void CDbWriteDoneUpdateNotify::Failed(HostID requester, String comment, long result /*= 0*/, String source /*= String()*/)
	{
		m_owner->m_s2cProxy.NotifyUpdateDataFailed(requester, g_ReliableSendForPN, m_tag, ErrorType_Unexpected, comment, result, source, m_blocked);
	}

	CDbWriteDoneRemoveNotify::CDbWriteDoneRemoveNotify(CDbCacheServer2Impl* owner, int64_t tag, bool blocked) :
		m_owner(owner),
		m_tag(tag),
		m_blocked(blocked)
	{

	}

	CDbWriteDoneRemoveNotify::~CDbWriteDoneRemoveNotify()
	{

	}

	void CDbWriteDoneRemoveNotify::Success(HostID requester, CPropNode &modifyData, DbmsWritePropNodeListPendArray &/*modifyDataList*/)
	{
		CriticalSectionLock lock(m_owner->m_cs, true);

		CLoadedData2Ptr_S loadData = m_owner->GetLoadedDataByRootGuid(modifyData.RootUUID);

		if ( !loadData )
			return;

		CPropNodePtr removeData = loadData->m_data.GetNode(modifyData.UUID);
		// removeData 가 없어도 성공으로 간주. by sopar
		if ( removeData )
		{
			//이런 상황이면 대략 난감이다...
			// 			m_owner->m_s2cProxy.NotifyDbmsAccessError(requester,g_ReliableSendForPN,m_waitTicket,ByteArray(),_PNT("Remove Final but LocalMemory Not Found"));
			// 			throw Exception("AccessError: Dbms Write Success but Remove Final LocalMemory Not Found");

			// 실제 로컬 메모리에서 삭제수행. 이미 DB에서 삭제되었기 때문에 removePropNodeList에 추가하지 않는다.
			loadData->m_data.RemoveNode(removeData, false);

			//change된것을 clear한다.
			loadData->m_data._INTERNAL_ClearChangeNode();
		}

		m_owner->m_s2cProxy.NotifyRemoveDataSuccess(requester, g_ReliableSendForPN, m_tag, modifyData.RootUUID, modifyData.UUID, m_blocked);
	}

	void CDbWriteDoneRemoveNotify::Failed(HostID requester, String comment, long result /*= 0*/, String source /*= String()*/)
	{
		m_owner->m_s2cProxy.NotifyRemoveDataFailed(requester, g_ReliableSendForPN, m_tag, ErrorType_Unexpected, comment, result, source, m_blocked);
	}



	CDbWriteDoneNonExclusiveAddNotify::CDbWriteDoneNonExclusiveAddNotify(CDbCacheServer2Impl* owner, int64_t tag, const ByteArray &customField) :
		m_owner(owner),
		m_tag(tag),
		m_message(customField)
	{

	}

	CDbWriteDoneNonExclusiveAddNotify::~CDbWriteDoneNonExclusiveAddNotify()
	{

	}

	// 다른 디비캐시클라에 의해 비독점적으로 node add가 성공하면
	void CDbWriteDoneNonExclusiveAddNotify::Success(HostID requester, CPropNode &modifyData, DbmsWritePropNodeListPendArray &/*modifyDataList*/)
	{
		CriticalSectionLock lock(m_owner->m_cs, true);

		CLoadedData2Ptr_S loadData = m_owner->GetLoadedDataByRootGuid(modifyData.RootUUID);

		ByteArray block;
		modifyData.ToByteArray(block);

		if ( loadData )
		{
			//있다면 로컬 업데이트.
			CPropNodePtr addData = CPropNodePtr(new CPropNode(modifyData.TypeName));
			*addData = modifyData;

			//error처리
			CPropNodePtr candidateData = loadData->m_data.GetNode(modifyData.UUID);

			if ( candidateData )
			{
				const PNTCHAR* errorText = _PNT("Non-exclusive node add failed. It already exists.");
				// _PNT("NonExclusive Add Final Already Exist"); <= 원래 이거였다. 근데 무슨 뜻이지? 일단 위에걸로 바꿈.

				m_owner->m_s2cProxy.NotifyDbmsAccessError(requester, g_ReliableSendForPN, m_tag, m_message, errorText);

				if ( loadData->IsLoaded() )
				{
					m_owner->m_s2cProxy.NotifyDbmsAccessError(loadData->GetLoaderClientHostID(), g_ReliableSendForPN, m_tag, m_message, errorText);
				}

				throw Exception("AccessError: Writing to DB successful, but there is a same node to add in cache memory already.");
				// "AccessError: Dbms Write Success but NonExclusive Add Final LocalMemory Already Exist"   <= 원래 이거였다. 근데 무슨 뜻이지? 일단 위에걸로 바꿈.
			}
			//error처리 end

			CPropNodePtr ownerData = loadData->m_data.GetNode(modifyData.OwnerUUID);

			loadData->m_data.InsertChild(ownerData, addData);
			loadData->m_data._INTERNAL_ClearChangeNode();

			if ( loadData->IsLoaded() )
			{
				// 추가된 노드의 데이터를 가져옴
				CMessage msg;
				ByteArray dataBlock;
				msg.UseInternalBuffer();

				int addDataCount = 1;

				msg << addDataCount;
				msg << *addData;

				msg.CopyTo(dataBlock);

				//누군가 추가했다고 알려야 함.
				m_owner->m_s2cProxy.NotifySomeoneAddData(loadData->GetLoaderClientHostID(), g_ReliableSendForPN, modifyData.RootUUID, dataBlock, m_message, Proud::Guid());
			}
		}

		lock.Unlock();

		m_owner->m_s2cProxy.NotifyNonExclusiveAddDataSuccess(requester, g_ReliableSendForPN, m_tag, modifyData.RootUUID, block);
	}

	void CDbWriteDoneNonExclusiveAddNotify::Failed(HostID requester, String comment, long result /*= 0*/, String source /*= String()*/)
	{
		m_owner->m_s2cProxy.NotifyNonExclusiveAddDataFailed(requester, g_ReliableSendForPN, m_tag, ErrorType_Unexpected, comment, result, source);
	}



	CDbWriteDoneNonExclusiveRemoveNotify::CDbWriteDoneNonExclusiveRemoveNotify(CDbCacheServer2Impl* owner, int64_t tag, const ByteArray &customField) :
		m_owner(owner),
		m_tag(tag),
		m_message(customField)
	{

	}

	CDbWriteDoneNonExclusiveRemoveNotify::~CDbWriteDoneNonExclusiveRemoveNotify()
	{

	}

	void CDbWriteDoneNonExclusiveRemoveNotify::Success(HostID requester, CPropNode &modifyData, DbmsWritePropNodeListPendArray &/*modifyDataList*/)
	{
		CriticalSectionLock lock(m_owner->m_cs, true);

		CLoadedData2Ptr_S loadData = m_owner->GetLoadedDataByRootGuid(modifyData.RootUUID);

		if ( loadData )
		{
			//있다면 로컬 업데이트.
			CPropNodePtr removeData = loadData->m_data.GetNode(modifyData.UUID);

			if ( !removeData )
			{
				//이런 상황이면 대략 난감이다...

				//Non-exclusive node add failed. It already exists.
				//AccessError: Writing to DB successful, but there is a same node to add in cache memory already.

				const PNTCHAR* errorText = _PNT("Non-exclusive node remove failed. It is already removed.");
				// 예전에 "NonExclusive Remove Final but LocalMemory Not Found" 이거였다. 무슨 말인지 모름. 일단 위에걸로 교체함.

				m_owner->m_s2cProxy.NotifyDbmsAccessError(requester, g_ReliableSendForPN, m_tag, m_message, errorText);

				if ( loadData->IsLoaded() )
				{
					m_owner->m_s2cProxy.NotifyDbmsAccessError(loadData->GetLoaderClientHostID(), g_ReliableSendForPN, m_tag, m_message, errorText);
				}

				throw Exception("AccessError: Writing to DB successful, but the node in cache memory had been removed.");
				// "AccessError: Dbms Write Success but NonExclusive Remove Final LocalMemory Not Found")<=예전에 이거였다. 무슨 말인지 모름. 일단 위에걸로 교체함.
			}

			// 실제 로컬 메모리에서 삭제수행. 이미 DB에서 삭제되었기 때문에 removePropNodeList에 추가하지 않는다.
			loadData->m_data.RemoveNode(removeData, false);

			//change된것을 clear한다.
			loadData->m_data._INTERNAL_ClearChangeNode();

			if ( loadData->IsLoaded() )
			{
				//누군가 추가했다고 알려야 함.
				m_owner->m_s2cProxy.NotifySomeoneRemoveData(loadData->GetLoaderClientHostID(), g_ReliableSendForPN, modifyData.RootUUID, modifyData.UUID, m_message);
			}
		}

		lock.Unlock();

		m_owner->m_s2cProxy.NotifyNonExclusiveRemoveDataSuccess(requester, g_ReliableSendForPN, m_tag, modifyData.RootUUID, modifyData.UUID);
	}

	void CDbWriteDoneNonExclusiveRemoveNotify::Failed(HostID requester, String comment, long result /*= 0*/, String source /*= String()*/)
	{
		m_owner->m_s2cProxy.NotifyNonExclusiveRemoveDataFailed(requester, g_ReliableSendForPN, m_tag, ErrorType_Unexpected, comment, result, source);
	}


	CDbWriteDoneNonExclusiveSetValueNotify::CDbWriteDoneNonExclusiveSetValueNotify(CDbCacheServer2Impl* owner, int64_t tag, String propertyName,
		CVariant newValue, int compareType, CVariant compareValue, const ByteArray &customField) :
		m_owner(owner),
		m_tag(tag),
		m_propertyName(propertyName),
		m_newValue(newValue),
		m_compareType(compareType),
		m_compareValue(compareValue),
		m_message(customField)
	{

	}
	CDbWriteDoneNonExclusiveSetValueNotify::~CDbWriteDoneNonExclusiveSetValueNotify()
	{

	}

	void CDbWriteDoneNonExclusiveSetValueNotify::Success(HostID requester, CPropNode &modifyData, DbmsWritePropNodeListPendArray &/*modifyDataList*/)
	{
		CriticalSectionLock lock(m_owner->m_cs, true);

		CLoadedData2Ptr_S loadData = m_owner->GetLoadedDataByRootGuid(modifyData.RootUUID);

		ByteArray block;
		modifyData.ToByteArray(block);

		if ( loadData )
		{
			//있다면 로컬 업데이트.
			CPropNodePtr updateData = loadData->m_data.GetNode(modifyData.UUID);

			if ( !updateData )
			{
				//이런 상황이면 대략 난감이다...
				const PNTCHAR* errorText = _PNT("Cannot update a field value of a node non-exclusively. It does not exist already.");
				// 예전에 _PNT("NonExclusive SetValue Final Not Exist"); 이거였다. 무슨 말인지 모름. 일단 위에걸로 임의 수정.
				m_owner->m_s2cProxy.NotifyDbmsAccessError(requester, g_ReliableSendForPN, m_tag, m_message, errorText);

				if ( loadData->IsLoaded() )
				{
					m_owner->m_s2cProxy.NotifyDbmsAccessError(loadData->GetLoaderClientHostID(), g_ReliableSendForPN, m_tag, m_message, errorText);
				}

				throw Exception("AccessError: DB write successful, but updating cache memory failed. The corresponding node does not exist.");
				// "AccessError: Dbms Write Success but NonExclusive SetValue Final LocalMemory Not Exist" <=예전에 이거엿다. 무슨 말인지 모름. 일단 위에걸로 임의 수정.
			}

			//hard가 진행되는 동안에 soft가 진행되었다면....accesserror
			if ( updateData->IsSoftWorkIssued() )
			{
				const PNTCHAR* errorText2 = _PNT("Both non-exclusive and exclusive access to same data went parallely. Your data may be stale.");
				// 예전에 _PNT("NonExclusive SetValue Final but Dirty LocalMemory");
				m_owner->m_s2cProxy.NotifyDbmsAccessError(requester, g_ReliableSendForPN, m_tag, m_message, errorText2);

				if ( loadData->IsLoaded() )
				{
					m_owner->m_s2cProxy.NotifyDbmsAccessError(loadData->GetLoaderClientHostID(), g_ReliableSendForPN, m_tag, m_message, errorText2);
				}

				throw Exception("AccessError: DB write successful, but your non-exclusive and exclusive to same data access went parallelly. Your data node may be stale.");
				// 예전에 throw Exception("AccessError: Dbms Write Success but NonExclusive SetValue Final LocalMemory is Dirty");
			}

			*updateData = (CProperty)modifyData;
			loadData->m_data._INTERNAL_ClearChangeNode();

			if ( loadData->IsLoaded() )
			{
				//누군가 추가했다고 알려야 함.
				m_owner->m_s2cProxy.NotifySomeoneSetValue(loadData->GetLoaderClientHostID(), g_ReliableSendForPN, modifyData.RootUUID, block, m_message);
			}
		}

		lock.Unlock();

		m_owner->m_s2cProxy.NotifyNonExclusiveSetValueSuccess(requester, g_ReliableSendForPN, m_tag, modifyData.RootUUID, block);
	}

	void CDbWriteDoneNonExclusiveSetValueNotify::Failed(HostID requester, String comment, long result /*= 0*/, String source /*= String()*/)
	{
		m_owner->m_s2cProxy.NotifyNonExclusiveSetValueFailed(requester, g_ReliableSendForPN, m_tag, ErrorType_Unexpected, comment, result, source);
	}
	bool CDbWriteDoneNonExclusiveSetValueNotify::IsCompareSuccess(CVariant &currentValue, CVariant &changeValue)
	{
		int cmdRet = Compare(currentValue, m_compareValue);

		if ( cmdRet == -2 )
		{
			return false;
		}

		bool ret = false;

		switch ( m_compareType )
		{
		case ValueCompareType_GreaterEqual:
			if ( cmdRet == 1 || cmdRet == 0 )
			{
				ret = true;
			}
			break;
		case ValueCompareType_Greater:
			if ( cmdRet == 1 )
			{
				ret = true;
			}
			break;
		case ValueCompareType_LessEqual:
			if ( cmdRet == -1 || cmdRet == 0 )
			{
				ret = true;
			}
			break;
		case ValueCompareType_Less:
			if ( cmdRet == -1 )
			{
				ret = true;
			}
			break;
		case ValueCompareType_Equal:
			if ( cmdRet == 0 )
			{
				ret = true;
			}
			break;
		case ValueCompareType_NotEqual:
			if ( cmdRet == 2 || cmdRet == -1 || cmdRet == 1 )
			{
				ret = true;
			}
			break;
		default:
			break;
		}

		if ( ret )
			changeValue = m_newValue;

		return ret;
	}

	int CDbWriteDoneNonExclusiveSetValueNotify::Compare(CVariant& var, const CVariant& var2)
	{
		//equal = 0, greater  = 1, less = -1 , notequal = 2; Error = -2;
		try
		{
			switch ( var.m_val.vt )
			{
			case VT_I1:
			case VT_I2:
			case VT_I4:
			case VT_I8:
			case VT_INT:
			{
				int64_t a = var;
				int64_t b = var2;
				if ( a>b )
					return 1;
				else if ( a == b )
					return 0;
				else
					return -1;
			}
				break;
			case VT_R4:
			{
				float a = var;
				float b = var2;
				if ( a>b )
					return 1;
				else if ( a == b )
					return 0;
				else
					return -1;
			}
				break;
			case VT_R8:
			case VT_DATE:
			{
				double a = var;
				double b = var2;
				if ( a>b )
					return 1;
				else if ( a == b )
					return 0;
				else
					return -1;
			}
				break;
			case VT_UI1:
			case VT_UI2:
			case VT_UI4:
			case VT_UI8:
			case VT_UINT:
			{
				uint64_t a = var;
				uint64_t b = var2;
				if ( a>b )
					return 1;
				else if ( a == b )
					return 0;
				else
					return -1;
			}
				break;
			case VT_BOOL:
			{
				BOOL a = var;
				BOOL b = var2;

				if ( a==b )
					return 0;
				else
					return 2;
			}
				break;
			case VT_BSTR:
			case VT_LPWSTR:
			case VT_LPSTR:
			{
				String a = var;
				String b = var2;

				if ( a == b )
					return 0;
				else
					return 2;
			}
				break;
			case VT_DECIMAL:
			{
				int a= var;
				int b = var2;
				if ( a>b )
					return 1;
				else if ( a == b )
					return 0;
				else
					return -1;
			}
				break;
			default:
				break;
			}
		}
		catch ( _com_error& )
		{
			return -2;
		}
		return -2;
	}

	CDbWriteDoneNonExclusiveModifyValueNotify::CDbWriteDoneNonExclusiveModifyValueNotify(CDbCacheServer2Impl* owner, int64_t tag,
		String propertyName, int operType, CVariant value, const ByteArray &customField) :
		m_owner(owner),
		m_tag(tag),
		m_propertyName(propertyName),
		m_operType(operType),
		m_value(value),
		m_message(customField)
	{

	}

	CDbWriteDoneNonExclusiveModifyValueNotify::~CDbWriteDoneNonExclusiveModifyValueNotify()
	{

	}

	void CDbWriteDoneNonExclusiveModifyValueNotify::Success(HostID requester, CPropNode &modifyData, DbmsWritePropNodeListPendArray &/*modifyDataList*/)
	{
		CriticalSectionLock lock(m_owner->m_cs, true);

		CLoadedData2Ptr_S loadData = m_owner->GetLoadedDataByRootGuid(modifyData.RootUUID);

		ByteArray block;
		modifyData.ToByteArray(block);

		if ( loadData )
		{
			//있다면 로컬 업데이트.
			CPropNodePtr updateData = loadData->m_data.GetNode(modifyData.UUID);


			if ( !updateData )
			{
				//이런 상황이면 대략 난감이다...
				const PNTCHAR* errorText = _PNT("Node to be non-exclusively modified does not exist in cache memory.");
				//const PNTCHAR* errorText = _PNT("NonExclusive ModifyValue Final Not Exist");
				m_owner->m_s2cProxy.NotifyDbmsAccessError(requester, g_ReliableSendForPN, m_tag, m_message, errorText);

				if ( loadData->IsLoaded() )
				{
					m_owner->m_s2cProxy.NotifyDbmsAccessError(loadData->GetLoaderClientHostID(), g_ReliableSendForPN, m_tag, m_message, errorText);
				}

				throw Exception("AccessError: DB write successful, but your non-exclusive data modification is not applied to cache memory. The corresponding node does not exist.");
				//throw Exception("AccessError: Dbms Write Success but NonExclusive ModifyValue Final LocalMemory Not Exist");
			}

			//hard가 진행되는 동안에 soft가 진행되었다면....accesserror
			if ( updateData->IsSoftWorkIssued() )
			{
				const PNTCHAR* errorText2 = _PNT("Node to be non-exclusive modified exist, but there is exclusive modification processing in parallel. Your data may become stale.");
				//const PNTCHAR* errorText2 = _PNT("NonExclusive ModifyValue Final but Dirty LocalMemory");
				m_owner->m_s2cProxy.NotifyDbmsAccessError(requester, g_ReliableSendForPN, m_tag, m_message, errorText2);

				if ( loadData->IsLoaded() )
				{
					m_owner->m_s2cProxy.NotifyDbmsAccessError(loadData->GetLoaderClientHostID(), g_ReliableSendForPN, m_tag, m_message, errorText2);
				}

				throw Exception("AccessError: DB write successful, but there is exclusive modification processing in parallel. Your data may become stale.");
				//throw Exception("AccessError: Dbms Write Success but NonExclusive ModifyValue Final LocalMemory is Dirty");
			}

			*updateData = (CProperty)modifyData;
			loadData->m_data._INTERNAL_ClearChangeNode();

			if ( loadData->IsLoaded() )
			{
				//누군가 추가했다고 알려야 함.
				m_owner->m_s2cProxy.NotifySomeoneModifyValue(loadData->GetLoaderClientHostID(), g_ReliableSendForPN, modifyData.RootUUID, block, m_message);
			}
		}

		lock.Unlock();

		m_owner->m_s2cProxy.NotifyNonExclusiveModifyValueSuccess(requester, g_ReliableSendForPN, m_tag, modifyData.RootUUID, block);
	}

	void CDbWriteDoneNonExclusiveModifyValueNotify::Failed(HostID requester, String comment, long result /*= 0*/, String source /*= String()*/)
	{
		m_owner->m_s2cProxy.NotifyNonExclusiveModifyValueFailed(requester, g_ReliableSendForPN, m_tag, ErrorType_Unexpected, comment, result, source);
	}

	bool CDbWriteDoneNonExclusiveModifyValueNotify::Operation(CVariant &currentValue, CVariant &changeValue)
	{
		switch ( m_operType )
		{
		case ValueOperType_Plus:
			changeValue = OperationPlus(currentValue, m_value);
			break;
		case ValueOperType_Minus:
			changeValue = OperationMinus(currentValue, m_value);
			break;
		case ValueOperType_Multiply:
			changeValue = OperationMultiply(currentValue, m_value);
			break;
		case ValueOperType_Division:
			changeValue = OperationDivision(currentValue, m_value);
			break;
		default:
			changeValue = CVariant(VT_NULL);
			break;
		}

		if ( changeValue.IsNull() )
		{
			return false;
		}

		return true;
	}

	Proud::CVariant CDbWriteDoneNonExclusiveModifyValueNotify::OperationPlus(CVariant& var, const CVariant& var2)
	{
		//사칙연산이 가능한 타입들만 사칙연산을 허용...
		try
		{
			switch ( var.m_val.vt )
			{
			case VT_I1:
			case VT_I2:
			case VT_I4:
			case VT_I8:
			case VT_INT:
			case VT_DECIMAL:
			{
				int64_t a = var;
				int64_t b = var2;
				a+=b;
				return CVariant(a);
			}
				break;
			case VT_R4:
			{
				float a = var;
				float b = var2;
				a+=b;
				return CVariant(a);
			}
				break;
			case VT_R8:
			case VT_DATE:
			{
				double a = var;
				double b = var2;
				a+=b;
				return CVariant(a);
			}
				break;
			case VT_UI1:
			case VT_UI2:
			case VT_UI4:
			case VT_UI8:
			case VT_UINT:
			{
				uint64_t a = var;
				uint64_t b = var2;
				a+=b;
				return CVariant(a);
			}
				break;
			default:
				break;
			}
		}
		catch ( _com_error& )
		{
			return CVariant(VT_NULL);
		}
		return CVariant(VT_NULL);
	}

	Proud::CVariant CDbWriteDoneNonExclusiveModifyValueNotify::OperationMinus(CVariant& var, const CVariant& var2)
	{
		//사칙연산이 가능한 타입들만 사칙연산을 허용...
		try
		{
			switch ( var.m_val.vt )
			{
			case VT_I1:
			case VT_I2:
			case VT_I4:
			case VT_I8:
			case VT_INT:
			case VT_DECIMAL:
			{
				int64_t a = var;
				int64_t b = var2;
				a-=b;
				return CVariant(a);
			}
				break;
			case VT_R4:
			{
				float a = var;
				float b = var2;
				a-=b;
				return CVariant(a);
			}
				break;
			case VT_R8:
			case VT_DATE:
			{
				double a = var;
				double b = var2;
				a-=b;
				return CVariant(a);
			}
				break;
			case VT_UI1:
			case VT_UI2:
			case VT_UI4:
			case VT_UI8:
			case VT_UINT:
			{
				uint64_t a = var;
				uint64_t b = var2;
				a-=b;
				return CVariant(a);
			}
				break;
			default:
				break;
			}
		}
		catch ( _com_error& )
		{
			return CVariant(VT_NULL);
		}
		return CVariant(VT_NULL);
	}

	Proud::CVariant CDbWriteDoneNonExclusiveModifyValueNotify::OperationMultiply(CVariant& var, const CVariant& var2)
	{
		//사칙연산이 가능한 타입들만 사칙연산을 허용...
		try
		{
			switch ( var.m_val.vt )
			{
			case VT_I1:
			case VT_I2:
			case VT_I4:
			case VT_I8:
			case VT_INT:
			case VT_DECIMAL:
			{
				int64_t a = var;
				int64_t b = var2;
				a*=b;
				return CVariant(a);
			}
				break;
			case VT_R4:
			{
				float a = var;
				float b = var2;
				a*=b;
				return CVariant(a);
			}
				break;
			case VT_R8:
			case VT_DATE:
			{
				double a = var;
				double b = var2;
				a*=b;
				return CVariant(a);
			}
				break;
			case VT_UI1:
			case VT_UI2:
			case VT_UI4:
			case VT_UI8:
			case VT_UINT:
			{
				uint64_t a = var;
				uint64_t b = var2;
				a*=b;
				return CVariant(a);
			}
				break;
			default:
				break;
			}
		}
		catch ( _com_error& )
		{
			return CVariant(VT_NULL);
		}
		return CVariant(VT_NULL);
	}

	Proud::CVariant CDbWriteDoneNonExclusiveModifyValueNotify::OperationDivision(CVariant& var, const CVariant& var2)
	{
		//사칙연산이 가능한 타입들만 사칙연산을 허용...
		try
		{
			switch ( var.m_val.vt )
			{
			case VT_I1:
			case VT_I2:
			case VT_I4:
			case VT_I8:
			case VT_INT:
			case VT_DECIMAL:
			{
				int64_t a = var;
				int64_t b = var2;
				if ( b == 0 )
					return CVariant(VT_NULL);
				a/=b;
				return CVariant(a);
			}
				break;
			case VT_R4:
			{
				float a = var;
				float b = var2;
				if ( b == 0.f )
					return CVariant(VT_NULL);
				a/=b;
				return CVariant(a);
			}
				break;
			case VT_R8:
			case VT_DATE:
			{
				double a = var;
				double b = var2;
				if ( b == 0.f )
					return CVariant(VT_NULL);
				a/=b;
				return CVariant(a);
			}
				break;
			case VT_UI1:
			case VT_UI2:
			case VT_UI4:
			case VT_UI8:
			case VT_UINT:
			{
				uint64_t a = var;
				uint64_t b = var2;
				if ( b == 0 )
					return CVariant(VT_NULL);
				a/=b;
				return CVariant(a);
			}
				break;
			default:
				break;
			}
		}
		catch ( _com_error& )
		{
			return CVariant(VT_NULL);
		}
		return CVariant(VT_NULL);
	}

	CDbWriteDoneUpdateListNotify::CDbWriteDoneUpdateListNotify(CDbCacheServer2Impl* owner, int64_t tag, bool blocked, bool transaction) :
		m_owner(owner),
		m_tag(tag),
		m_blocked(blocked),
		m_transaction(transaction)
	{

	}

	CDbWriteDoneUpdateListNotify::~CDbWriteDoneUpdateListNotify()
	{

	}

	void CDbWriteDoneUpdateListNotify::Success(HostID requester, CPropNode &modifyData, DbmsWritePropNodeListPendArray &modifyDataList)
	{
		CriticalSectionLock lock(m_owner->m_cs, true);

		CLoadedData2Ptr_S loadData = m_owner->GetLoadedDataByRootGuid(modifyData.UUID);

		if ( loadData )
		{
			m_owner->UpdateLocalDataList(requester, loadData, m_tag, modifyDataList);
		}

		ByteArray block;
		loadData->m_data._INTERNAL_ChangeToByteArray(block);

		loadData->m_data._INTERNAL_ClearChangeNode();
		lock.Unlock();

		m_owner->m_s2cProxy.NotifyUpdateDataListSuccess(requester, g_ReliableSendForPN, m_tag, modifyData.UUID, block, m_blocked);
	}

	void CDbWriteDoneUpdateListNotify::Failed(HostID requester, String comment, long result /*= 0*/, String source /*= String()*/)
	{
		m_owner->m_s2cProxy.NotifyUpdateDataListFailed(requester, g_ReliableSendForPN, m_tag, ErrorType_Unexpected, comment, result, source, m_blocked);

		if ( !m_transaction )
		{
			m_owner->m_s2cProxy.NotifyWarning(requester, g_ReliableSendForPN, ErrorType_Unexpected, ErrorType_InvalidPacketFormat,
				_PNT("Not all update node is successful. The loaded data may become different to DB."));
			//_PNT("Not Transaction But UpdateDataList Failed.Maybe Dbms is diffrent to LoadedData."));
		}
	}

	DbmsWritePend2::DbmsWritePend2()
	{
		m_dbClientID=HostID_None;
		m_transaction = false;
	}
}
