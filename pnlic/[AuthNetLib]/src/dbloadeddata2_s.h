#pragma once

#if defined(_WIN32)

#include "../include/DbLoadedData2.h"
#include "../include/AdoWrap.h"
#include "../include/IRMIProxy.h"
#include "../include/IRMIStub.h"
#include "DbCacheObject.h"
#include "DB_stub.h"
#include "DB_proxy.h"
#include "FastList2.h"

namespace Proud
{
	class CLoadedData2_S;
	typedef RefCount<CLoadedData2_S> CLoadedData2Ptr_S;
	typedef CFastMap2<Guid, CLoadedData2Ptr_S, int> LoadedDataList2_S;

	// 로딩 등 실패 사유를 담는 구조체
	class CFailInfo_S
	{
	public:
		// 관련된 데이터의 UUID
		Guid m_uuid;

		// 실패 사유. 
		// 사용자는 요청의 성공/실패 여부를 이 값으로 확인합니다.
		ErrorType m_reason;

		// 실패 사유에 대한 설명
		String m_comment;

		// COM에러인 경우 셋팅
		HRESULT m_hResult;

		// 기존 독점자로부터 받은 메시지
		ByteArray m_message;

		CFailInfo_S() : m_reason(ErrorType_Ok), m_hResult(0) {}
	};

	// 로딩 등 성공 결과를 담는 구조체
	class CSuccessInfo_S
	{
	public:
		// 데이터
		CLoadedData2Ptr_S m_data;

		// 기존 독점자로부터 받은 메시지
		ByteArray m_message;
	};
	typedef CFastList<CLoadedData2Ptr_S> LoadedDataList;
	typedef CFastList<CSuccessInfo_S> SuccessInfoList_S;
	typedef CFastList<CFailInfo_S> FailInfoList_S;

	// 데이터 로딩의 주체(db cache client)가 요청한 내용들
	// 데이터 로딩 요청 뿐만 아니라 언로드 요청에서도 공통으로 쓰이는 구조체
	//   ※ 독점로드 시 CLoadRequest객체의 생명주기
	//    1. 로드 요청 시 생성 (new CLoadRequest). m_waitForUnloadCount의 초기값은 0
	//    2. 로드하려는 데이터들 중 독점된 것이 전혀 없으면 바로 3번 수행, 아니면 2-1 ~ 2-3을 수행
	//      2-1. 독점된 데이터의 개수만큼 m_waitForUnloadCount값을 증가시키고 독점자들에게 Unload 요청
	//      2-2. 독점자가 Unload 또는 Deny를 하면 m_waitForUnloadCount값 감소
	//      2-1. 독점자가 Unload요청에 반응이 없으면 타임아웃 처리되면서 m_waitForUnloadCount값 감소
	//    3. m_waitForUnloadCount가 0이면 응답 후 소멸 (delete CLoadRequest)
	class CLoadRequest
	{
	public:
		// 독점 로드 요청인지 여부
		const bool m_isExclusiveRequest;

		// 요청한 호스트 
		const HostID m_requester;

		// 사용자가 입력한 태그 
		const int64_t m_tag;

		// 실패한 데이터들 정보
		FailInfoList_S m_failList;

		// 로드 성공한 목록 
		SuccessInfoList_S m_successList;

		// 기존 독점자들에게 보낼 메시지
		const ByteArray m_message;

		// 요청한 시간
		const int64_t m_requestTimeMs;

		// 독점 로드를 하려는 데이터들 중 
		// 이미 독점되어 있어서 기존 독점자에게 Unload요청을 보냈고, 그 응답을 기다리는 데이터의 개수
		int m_waitForUnloadCount;

		CLoadRequest(
			const bool& isExclusiveRequest,
			const HostID& requester,
			const int64_t& tag,
			const ByteArray& message,
			const int64_t& requestTimeMs
			)
			: m_isExclusiveRequest(isExclusiveRequest)
			, m_requester(requester)
			, m_tag(tag)
			, m_message(message)
			, m_requestTimeMs(requestTimeMs)
			, m_waitForUnloadCount(0)
		{
		}
	};
	typedef RefCount<CLoadRequest> CLoadRequestPtr;


	// 데이터 격리 요청에 대한 정보.
	// 격리 요청을 받으면 이 객체를 생성하여 저장해뒀다가
	// 해당 데이터의 백그라운드 작업이 모두 끝나면
	// 저장해둔 정보를 바탕으로 요청자에게 응답해준다.
	class CIsolateRequest
	{
	public:
		// 요청한 호스트
		HostID m_requester;

		// 요청자에게 에코할 세션 정보
		Guid m_session;

		// 격리할 대상의 Root UUID
		// IsolateDataList2_S의 키가 해당 UUID이므로 필요없다.
		// Guid m_rootUUID;

		CIsolateRequest() : m_requester(HostID_None) {}
	};
	typedef RefCount<CIsolateRequest> CIsolateRequestPtr;
	typedef CFastMap2<Guid, CIsolateRequestPtr, int> IsolateDataList2_S;


	// 서버에서 갖고 있는 loaded data tree.
	class CLoadedData2_S
	{
	public:
		enum LoadState
		{
			// 메모리에 로드되어있으나 아무도 독점중이지 않은 상태. 
			LoadState_None,

			// 메모리에 로드되었고 현재 독점 요청이 진행 중인 상태.
			// LoadState_Requested 상태인 데이터에게 또다른 독점 요청이 들어올 경우
			// 먼저 진행중이던 요청을 우선 처리하여 응답 한 후 언로드 요청을 한다.
			// NotifyLoadDataComplete을 받지도 못한 독점요청자가 NotifyDataUnloadRequested를 받는 상황을 피하기 위함.
			LoadState_Requested,

			// 독점이 완료되었고 해당 독점자는 NotifyLoadDataComplete까지 받은 상태. 
			LoadState_Exclusive
		};

		CLoadedData2 m_data;

		// 이 데이터의 로딩 진행 상태. enum LoadState의 설명 참고
		LoadState m_state;

		HostID m_INTERNAL_loaderClientHostID;
		inline HostID GetLoaderClientHostID() { return m_INTERNAL_loaderClientHostID; }

		// 휴면상태(아무도 로딩하지 않은 상태)가 된 시간.
		// 0이면 휴면 상태가 아님을 의미한다.
		int64_t m_hibernateStartTimeMs;

		// 마지막에 데이터 트리의 내용이 변경된 시간
		// 0이면 '변경된 적 없음'을 의미한다.
		int64_t m_lastModifiedTime;

		int64_t m_unloadedHibernateDuration;

		DWORD m_dbmsWritingThreadID;	// 이 스레드의 DBMS 저장을 처리하고 있는 스레드

		// DB에의 기록 대기중이 대기 작업들
		CFastList2<DbmsWritePend2, int> m_dbmsWritePendList;

		// 이미 로딩한 이 LD의 로딩 소유권을 가져가고 싶어하는 여타 db cache client들
		// 요청 들어온 순서를 보관해야 하므로 list이다.
		CFastList2<CLoadRequestPtr, int> m_unloadRequests;
		// m_unloadRequests에서 빠른 검색을 위한 인덱스
		//CFastMap2<CLoadRequestPtr, Position, int, CLoadRequestPtr::CPNElementTraits> m_unloadRequestsMap;

		CDbCacheServer2Impl* m_owner;

		CLoadedData2_S(CDbCacheServer2Impl* owner);
		~CLoadedData2_S();

		void MarkUnload();
		void MarkLoad(HostID dbCacheClientID, Guid sessionGuid, LoadState state);

		bool IsLoaded();

		static void DbmsLoad(CAdoRecordset &rs, CPropNode& data);
		static void DbmsSave(CAdoRecordset &rs, CPropNode& data, bool updatePropertiesOnly);

		CLoadedData2* GetData();

		// m_data안의 모든 지울 node를 writependlist에 추가한다.
		void AddWritePendListFromRemoveNodes(HostID dbClientID, Guid removeUUID = Guid(), IDbWriteDoneNotifyPtr doneNotify = IDbWriteDoneNotifyPtr());
	};

}

#endif // _WIN32
