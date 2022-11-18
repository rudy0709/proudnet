#pragma once 

#if defined(_WIN32)

#include "../include/Event.h"
#include "../include/Enums.h"

namespace Proud
{
	/* 디비 캐시 클라에서, block function call을 하면,
	내부적으로는 async function call 후 그것의 결과가 올 때까지 기다리도록 되어 있다.
	결과가 왔을때 어느 요청에 대한 것인지 매칭해서 블러킹된 스레드가 깨어나게 하는 역할을 한다. */
	class BlockedEvent
	{
	public:
		BlockedEvent() :
			m_event(NULL),
			m_success(false),
			m_errorType(ErrorType_Ok),
			m_comment(_PNT("")),
			m_hResult(S_OK),
			m_source(_PNT(""))
		{
		}

		// 결과 응답을 기다리는 스레드는 이 event를 대기한다.
		// 결과가 오면 이 event를 set하여 awake를 유도한다.
		Proud::Event* m_event;

		// 결과 응답이 여기 저장된다.
		bool	m_success;
		ErrorType m_errorType;
		String m_comment;
		HRESULT m_hResult;
		String m_source;
	};
	typedef RefCount<BlockedEvent> BlockedEventPtr;
}

#endif // _WIN32
