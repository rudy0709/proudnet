/*
ProudNet v1.7


이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의 : 저작물에 관한 위의 명시를 제거하지 마십시오.


This program is soley copyrighted by Nettention.
Any use, correction, and distribution of this program are subject to the terms and conditions of the License Agreement.
Any violated use of this program is prohibited and will be cause of immediate termination according to the License Agreement.

** WARNING : PLEASE DO NOT REMOVE THE LEGAL NOTICE ABOVE.


此程序的版权归Nettention公司所有。
与此程序的修改、使用、发布相关的事项要遵守此程序的所有权者的协议。
不遵守协议时要原则性的禁止擅自使用。
擅自使用的责任明示在与此程序所有权者的合同书里。

** 注意：不要移除关于制作物的上述明示。


このプログラムの著作権はNettentionにあります。
このプログラムの修正、使用、配布に関する事項は本プログラムの所有権者との契約に従い、
契約を遵守しない場合、原則的に無断使用を禁じます。
無断使用による責任は本プログラムの所有権者との契約書に明示されています。

** 注意：著作物に関する上記の明示を除去しないでください。

*/

#pragma once

#include "../include/Enums.h"
#include "../include/FastArray.h"
#include "../include/ErrorInfo.h"
#include "../include/CryptoAes.h"
#include "../include/CryptoFast.h"


#ifdef _MSC_VER
#pragma pack(push,8)
#endif

namespace Proud
{
#if (_MSC_VER>=1400)
#pragma managed(push, off)
#endif
	/** \addtogroup util_group
	*  @{
	*/

	class CSessionKey
	{
	public:
		CSessionKey(){}

		CCryptoAesKey m_aesKey;
		ByteArray m_aesKeyBlock; // m_aesKey에 expand되기 전의 키 블록. 서버에서 만들어 보내준 쌩 데이터다. ACR 과정에서 +1 처리를 하기 위해 별도로 보관한다.

		CCryptoFastKey m_fastKey;
		ByteArray m_fastKeyBlock;

		void Clear()
		{
			m_aesKey.Clear();
			m_fastKey.Clear();
		}

		// 갖고 있는 모든 키가 내용물이 존재하는지?
		// NOTE: NS,LS는 2개 암호키를 전부 아니면 전무 지정할 것이다. 따라서 이 함수는 잘 작동함.
		bool EveryKeyExists()
		{
			if (!m_aesKey.KeyExists())
				return false;

			if (!m_fastKey.KeyExists())
				return false;
			return true;
		}

		void CopyTo(CSessionKey &dest) const
		{
			dest.m_aesKey = m_aesKey;
			dest.m_aesKeyBlock = m_aesKeyBlock;

			dest.m_fastKey = m_fastKey;
			dest.m_fastKeyBlock = m_fastKeyBlock;
		}


	};

	/**  @} */
#if (_MSC_VER>=1400)
#pragma managed(pop)
#endif
}


#ifdef _MSC_VER
#pragma pack(pop)
#endif