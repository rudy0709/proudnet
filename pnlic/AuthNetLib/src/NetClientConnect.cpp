// NetClient의 연결/연결 해제를 담당

#include "stdafx.h"

// STL header file은 xcode에서는 cpp에서만 include 가능하므로
#include <stack>

#include "NetClient.h"
#include "CollUtil.h"

namespace Proud
{
	// 연결 과정을 시작하기 전에, this와 parameter의 validation check를 한다.
	void CNetClientImpl::Connect_CheckStateAndParameters(const CNetConnectionParam &param)
	{
		/* 값의 유효성 검사는, this의 멤버에 변화를 가하기 전에 해야 합니다.
		그런데 여기까지 왔다면 이미 this의 멤버에 변화를 가한 상태입니다.
		따라서 아래 검사 후 throw를 하는 루틴은 검사를 하는 과정 즉 Connect_CheckStateAndParameters로 옮겨 주시기 바랍니다.
		같은 수정을, C#,java에도 해주세요. */

		// 파라메터 정당성 체크
		if (param.m_serverIP == _PNT("0.0.0.0")
			|| param.m_serverPort == 0
			|| param.m_serverIP == _PNT("255.255.255.255"))
		{
			throw Exception(ErrorInfo::TypeToString(ErrorType_UnknownAddrPort));
		}

		// UDP port pool에 잘못된 값이나 중복된 값이 있으면 에러 처리
		for (int i = 0; i < param.m_localUdpPortPool.GetCount(); i++)
		{
			if (param.m_localUdpPortPool[i] <= 0)
			{
				throw Exception(ErrorInfo::TypeToString(ErrorType_InvalidPortPool));
			}
		}

		m_usedUdpPorts.Clear();
		m_unusedUdpPorts.Clear();
		for (int i = 0; i < param.m_localUdpPortPool.GetCount(); i++)
		{
			if (param.m_localUdpPortPool[i] <= 0)
			{
				throw Exception(ErrorInfo::TypeToString(ErrorType_InvalidPortPool));
			}
			if (m_unusedUdpPorts.ContainsKey(param.m_localUdpPortPool[i]) == true)
			{
				throw Exception(ErrorInfo::TypeToString(ErrorType_InvalidPortPool));
			}
			m_unusedUdpPorts.Add(param.m_localUdpPortPool[i]);
		}

// 		CFastArray<int> udpPortPool = param.m_localUdpPortPool; // copy
// 		UnionDuplicates<CFastArray<int>, int, intptr_t >(udpPortPool);
// 		if (udpPortPool.GetCount() != param.m_localUdpPortPool.GetCount()) 
// 		{
// 			// 길이가 다름 => 중복된 값이 있었음
// 			throw Exception(ErrorInfo::TypeToString(ErrorType_InvalidPortPool));
// 		}
	}
}
