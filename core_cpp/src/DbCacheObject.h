#pragma once


#include "../include/DbLoadedData.h"
#include "../include/FastMap.h"
#include "FastMap2.h"
#include "../include/dbenums.h"
#include "../include/PropNode.h"

namespace Proud
{
	// 한개의 노드에 적용되는 PendType 구조체
    struct DbmsWritePropNodePend
    {
        DbmsWritePropNodePendType m_type;
        Guid		m_UUID;
		// 이 객체는 copy가 자주 발생한다. 따라서 smart ptr로 바꾸어야 복사 부하 절감.
		CPropNodePtr m_node;

        DbmsWritePropNodePend():
        m_type(DWPNPT_None),
            m_UUID(),
			m_node(CPropNodePtr())
        {

        }
    };

	// UpdateList를 사용시 사용되는 PendType 구조체. 
	// 즉 1개 이상의 propNode를 한꺼번에 변경할 시.
	struct DbmsWritePropNodeListPend
	{
		DbmsWritePropNodePendType m_type;

		// move node 기능에서는, 이동되는 node
		// 이 객체는 copy가 자주 발생한다. 따라서 smart ptr로 바꾸어야 복사 부하 절감.
		CPropNodePtr m_node;

		DbmsWritePropNodeListPend() :
		m_type(DWPNPT_None),
		m_node(CPropNodePtr())
		{
		}
	};

	typedef CFastArray<DbmsWritePropNodeListPend> DbmsWritePropNodeListPendArray;
    
    // Db Remote Client
    class CDbRemoteClient_S
    {
    public:
        HostID m_HostID;
		// 인증 완료 된 시간
        int64_t m_logonTime;

        CDbRemoteClient_S()
        {
            m_logonTime = 0;
        }
        bool IsLoggedOn();
    };
    
    typedef RefCount<CDbRemoteClient_S> CDbRemoteClientPtr_S;
    typedef CFastMap2<HostID,CDbRemoteClientPtr_S,int> DbRemoteClients;

	class IDbWriteDoneNotify
	{
	public:
		virtual ~IDbWriteDoneNotify(){}

	public:
		virtual void Success(HostID requester,CPropNode &modifyData,DbmsWritePropNodeListPendArray &modifyDataList) = 0;
		virtual void Failed(HostID requester,String comment,long result = 0,String source = String()) = 0;
	};

	typedef RefCount<IDbWriteDoneNotify> IDbWriteDoneNotifyPtr;
	
	class CDbWriteDoneAddNotify: public IDbWriteDoneNotify
	{
	private:
		CDbCacheServer2Impl* m_owner;
		int64_t m_tag;
		bool m_blocked;
	public:
		CDbWriteDoneAddNotify(CDbCacheServer2Impl* owner, int64_t tag, bool blocked);
		virtual ~CDbWriteDoneAddNotify();


		void Success(HostID requester,CPropNode &modifyData,DbmsWritePropNodeListPendArray &modifyDataList);
		void Failed(HostID requester,String comment,long result = 0,String source = String());
	};

	class CDbWriteDoneUpdateNotify: public IDbWriteDoneNotify
	{
	private:
		CDbCacheServer2Impl* m_owner;
		int64_t m_tag;
		bool m_blocked;
	public:
		CDbWriteDoneUpdateNotify(CDbCacheServer2Impl* owner, int64_t tag, bool blocked);
		virtual ~CDbWriteDoneUpdateNotify();

		void Success(HostID requester,CPropNode &modifyData,DbmsWritePropNodeListPendArray &modifyDataList);
		void Failed(HostID requester,String comment,long result = 0,String source = String());
	};

	class CDbWriteDoneRemoveNotify: public IDbWriteDoneNotify
	{
	private:
		CDbCacheServer2Impl* m_owner;
		int64_t m_tag;
		bool m_blocked;
	public:
		CDbWriteDoneRemoveNotify(CDbCacheServer2Impl* owner, int64_t tag, bool blocked);
		virtual ~CDbWriteDoneRemoveNotify();

		void Success(HostID requester,CPropNode &modifyData,DbmsWritePropNodeListPendArray &modifyDataList);
		void Failed(HostID requester,String comment,long result = 0,String source = String());
	};

	class CDbWriteDoneNonExclusiveAddNotify: public IDbWriteDoneNotify
	{
	private:
		CDbCacheServer2Impl* m_owner;
		int64_t m_tag;
		ByteArray m_message;
	public:
		CDbWriteDoneNonExclusiveAddNotify(CDbCacheServer2Impl* owner, int64_t tag, const ByteArray& message);
		virtual ~CDbWriteDoneNonExclusiveAddNotify();

		void Success(HostID requester,CPropNode &modifyData,DbmsWritePropNodeListPendArray &modifyDataList);
		void Failed(HostID requester,String comment,long result = 0,String source = String());
	};

	class CDbWriteDoneNonExclusiveRemoveNotify: public IDbWriteDoneNotify
	{
	private:
		CDbCacheServer2Impl* m_owner;
		int64_t m_tag;
		ByteArray m_message;
	public:
		CDbWriteDoneNonExclusiveRemoveNotify(CDbCacheServer2Impl* owner, int64_t tag, const ByteArray& message);
		virtual ~CDbWriteDoneNonExclusiveRemoveNotify();

		void Success(HostID requester,CPropNode &modifyData,DbmsWritePropNodeListPendArray &modifyDataList);
		void Failed(HostID requester,String comment,long result = 0,String source = String());
	};

	class CDbWriteDoneNonExclusiveSetValueNotify: public IDbWriteDoneNotify
	{
	private:
		CDbCacheServer2Impl* m_owner;
		ByteArray m_message;
		int64_t m_tag;
		Proud::String m_propertyName;
		Proud::CVariant m_newValue;
		int m_compareType;
		Proud::CVariant m_compareValue;
	public:
		CDbWriteDoneNonExclusiveSetValueNotify(CDbCacheServer2Impl* owner, int64_t tag, String propertyName,
			CVariant newValue, int compareType, CVariant compareValue, const ByteArray& message);
		virtual ~CDbWriteDoneNonExclusiveSetValueNotify();

		void Success(HostID requester,CPropNode &modifyData,DbmsWritePropNodeListPendArray &modifyDataList);
		void Failed(HostID requester,String comment,long result = 0,String source = String());

		String GetPropertyName(){return m_propertyName;}
		bool IsCompareSuccess(CVariant &currentValue,CVariant &changeValue);
		int Compare( CVariant& var,const CVariant& var2 );
	};

	class CDbWriteDoneNonExclusiveModifyValueNotify: public IDbWriteDoneNotify
	{
	private:
		CDbCacheServer2Impl* m_owner;
		ByteArray m_message;
		int64_t m_tag;
		Proud::String m_propertyName;
		int m_operType;
		Proud::CVariant m_value;
	public:
		CDbWriteDoneNonExclusiveModifyValueNotify(CDbCacheServer2Impl* owner, int64_t tag, String propertyName,
			int operType, CVariant value, const ByteArray &message);
		virtual ~CDbWriteDoneNonExclusiveModifyValueNotify();

		void Success(HostID requester,CPropNode &modifyData,DbmsWritePropNodeListPendArray &modifyDataList);
		void Failed(HostID requester,String comment,long result = 0,String source = String());

		String GetPropertyName(){return m_propertyName;}
		bool Operation(CVariant &currentValue,CVariant &changeValue);

		CVariant OperationMinus( CVariant& var,const CVariant& var2 );
		CVariant OperationPlus(CVariant& var,const CVariant& var2);
		CVariant OperationMultiply(CVariant& var, const CVariant& var2);
		CVariant OperationDivision(CVariant& var,const CVariant& var2);
	};

	class CDbWriteDoneUpdateListNotify: public IDbWriteDoneNotify
	{
	private:
		CDbCacheServer2Impl* m_owner;
		int64_t m_tag;
		bool m_blocked;
		bool m_transaction;
	public:
		CDbWriteDoneUpdateListNotify(CDbCacheServer2Impl* owner, int64_t tag, bool blocked, bool transaction);
		virtual ~CDbWriteDoneUpdateListNotify();

		void Success(HostID requester,CPropNode &modifyData,DbmsWritePropNodeListPendArray &modifyDataList);
		void Failed(HostID requester,String comment,long result = 0,String source = String());
	};


	// DB에 기록 대기중인 작업 항목 1개
	struct DbmsWritePend2
	{
		// 이 DBMS write를 요청한 DB cache client
		HostID m_dbClientID;
		// propNode 1개의 변경 명령
		//문경엽: MoveNode 호출 시 srcNode 전달 용도로 쓸 수 있음.
		DbmsWritePropNodePend m_changePropNode;

		// 이 DB 작업이 완료되면(성공하건 실패하건) 그때 처리해야 하는 이벤트 핸들러.
		// 가령, DB cache client에게 요청에 대한 응답 RMI를 보낸다던지.
		IDbWriteDoneNotifyPtr m_DoneNotify;

		DbmsWritePropNodeListPendArray m_changePropNodeArray;
		bool	m_transaction;

		DbmsWritePend2();
	};
}

