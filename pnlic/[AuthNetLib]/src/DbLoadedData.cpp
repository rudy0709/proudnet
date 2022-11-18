/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#if defined(_WIN32)

#include "stdafx.h"
#include "../include/dbenums.h"


namespace Proud
{
	const PNTCHAR* TypeToString(DbmsWritePropNodePendType t)
	{
		switch(t)
		{
		case DWPNPT_None: return _PNT("None");
		case DWPNPT_AddPropNode: return _PNT("AddPropNode");
		case DWPNPT_RemovePropNode: return _PNT("RemovePropNode");
		case DWPNPT_UpdatePropNode: return _PNT("UpdatePropNode");
		case DWPNPT_UnilateralUpdatePropNode: return _PNT("UnilateralUpdatePropNode");
		case DWPNPT_RequestUpdatePropNode: return _PNT("RequestUpdatePropNode");
		case DWPNPT_SetValueIf: return _PNT("SetValueIf");
		case DWPNPT_ModifyValue: return _PNT("ModifyValue");
		case DWPNPT_UnilateralUpdatePropNodeList: return _PNT("UnilateralUpdatePropNodeList");
		case DWPNPT_RequestUpdatePropNodeList: return _PNT("RequestUpdatePropNodeList");
		case DWPNPT_MovePropNode : return _PNT("MovePropNode");
		}
		return _PNT("");
	}
}

#endif // _WIN32
