/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"

#if defined(_WIN32)

#include "../include/FastList.h"
#include "../include/DbLoadedData2.h"
#include "FastList2.h"


namespace Proud
{
	CLoadedData2::CLoadedData2():
		m_Root(),
		m_propNodeMap()
	{
	}

	CLoadedData2::CLoadedData2(const CLoadedData2& from)
	{
		m_propNodeMap.Clear();
		m_removePropNodeList.Clear();

		m_propNodeMap = from.m_propNodeMap;
		m_Root = from.m_Root;

		m_removePropNodeList.AddTailList(&from.m_removePropNodeList);
	}

	CLoadedData2::~CLoadedData2()
	{
		m_propNodeMap.Clear();
		m_removePropNodeList.Clear();
	}

	CLoadedData2 & CLoadedData2::operator=(const CLoadedData2& from)
	{
		m_propNodeMap.Clear();
		m_removePropNodeList.Clear();

		m_propNodeMap = from.m_propNodeMap;
		m_Root = from.m_Root;

		m_removePropNodeList.AddTailList(&from.m_removePropNodeList);

		return *this;
	}

	void CLoadedData2::AssertThreadID( eAccessMode eMode ) const
	{
		m_RWAccessChecker.AssertThreadID(eMode);
	}

	void CLoadedData2::ClearThreadID() const
	{
		m_RWAccessChecker.ClearThreadID();
	}

	// node를 받는다.
	Proud::ErrorType CLoadedData2::MovePropNode(CLoadedData2& destForest, CPropNodePtr nodeToMove, CPropNodePtr destParentNode)
	{
		if (nodeToMove == CPropNodePtr() || destParentNode == CPropNodePtr())
			return ErrorType_ValueNotExist;

		AssertThreadID(eAccessMode_Write);
		// nodeToMove, destForest, destParentNode
		Proud::ErrorType ret = _INTERNAL_NOACCESSMODE_MovePropNode(nodeToMove, destForest, destParentNode);
		ClearThreadID();

		return ret;
	}

	Proud::ErrorType CLoadedData2::_INTERNAL_NOACCESSMODE_MovePropNode(CPropNodePtr nodeToMove, CLoadedData2& destForest, CPropNodePtr& destParentNode)
	{
		//루트를 이동시키려하면 Error발생시킨다.
		if (nodeToMove == m_Root)
			return ErrorType_Unexpected;

		// destParentNode 값 Check
		if (destParentNode == NULL)
			return ErrorType_ValueNotExist;

		// nodeToMove가 현재 forest에 존재해야 한다.
		PropNodeMap::iterator itr = m_propNodeMap.find(nodeToMove->m_UUID);
		if (itr == m_propNodeMap.end())
			return ErrorType_ValueNotExist;

		// destParentNode가 destForest에 존재해야 한다.
		itr = destForest.m_propNodeMap.find(destParentNode->m_UUID);
		if ( itr == destForest.m_propNodeMap.end() )
			return ErrorType_Unexpected;

		// nodeToMove는 현재 Tree에 속하면서 루트가 아닌데 부모가 NULL일 수 없다.
		if ( nodeToMove->m_parent==NULL )
			return ErrorType_Unexpected;

		bool isFirstNode = false;
		bool isEndNode = false;
		bool isSelfMove = false;

		// 같은 LD 내 이동
		if (m_Root == destForest.m_Root)
			isSelfMove = true;

		if (nodeToMove->m_parent->m_child == nodeToMove)
			isFirstNode = true;		// 첫번째 노드
		if (nodeToMove->m_parent->m_child->m_endSibling == nodeToMove)
			isEndNode = true;		// 마지막 노드

		// 현재 Forest에서 기존의 노드들의 연결을 끊는다.
		if (isFirstNode)
		{
			nodeToMove->m_parent->m_child = nodeToMove->m_sibling;
			if ( nodeToMove->m_sibling != NULL )
			{
				nodeToMove->m_sibling->m_endSibling = nodeToMove->m_endSibling;
			}
			//			if (nodeToMove->m_sibling != NULL)
			//				nodeToMove->m_sibling->m_endSibling = nodeToMove->m_endSibling;

		}
		else
		{
			// 해당 노드를 검색
			CPropNode* prevNode = nodeToMove->m_parent->m_child;

			// 일정이 급박하여 추후에 수정하는 것으로.... - by KJSA
			// 추후 수정이지만, 잔여 마무리를 냅두지 않는 것이 좋겠습니다. 가급적이면 다음 스플린트에서 다룰 수 있도록 트렐로 카드로 남겨주세요.
			// 엔진팀에서 사원급에서 해볼 수 있는 업무일지도.
			/*CPropNode.m_leftSibling이라는 멤버를 추가하게 되면, 이러한 루프를 삭제할 수 있을텐데요, 어때요? 너무 많이 고칠라나...
			너무 많이 고치는 부담이 생기므로, CPropNode의 트리 관계를 제어하는 함수군을 뽑아내는 것이 좋겠습니다.
			단 그 트리관계 제어 함수들은 유저에게 노출되면 안되므로 CPropForest를 friend로 둔 private 함수들로 두어야 하겠죠.
			아예 아래와 윗 부분은 nodeToMove를 parent로부터 떼어내는 루틴이므로 nodeToMove->DetachParent() extract to function을 해주면 nice!
			그렇게 하면 이 함수 뿐만 아니라 CPropNode의 트리 관계를 설정하는 다른 모든 곳도 소스가 훨씬 다루기 쉬워지게 되는 장점도 얻습니다.

			함수 모양새 예: CPropNode.AttachRightSibling,AttachLeftSibling,DetachParent,AttachFirstChild,AttachLastChild 정도면 될 듯 하네요. 타 Tree API 를 참고하면 더 좋을 듯.
			*/
			while (prevNode)
			{
				if (prevNode->m_sibling == nodeToMove)
					break;

				prevNode = prevNode->m_sibling;
			}

			prevNode->m_sibling = nodeToMove->m_sibling;

			if (isEndNode)
			{
				nodeToMove->m_parent->m_child->m_endSibling = prevNode;
			}
		}

		// 기존의 부모와 형제 연을 끊는다.
		nodeToMove->m_parent = NULL;
		nodeToMove->m_sibling = NULL;
		nodeToMove->m_endSibling = NULL;

		ErrorType ret = ErrorType_Ok;
		if ( isSelfMove )
		{
			// 현재 트리 내에서 이동하는 경우는 m_propNodeMap을 건드릴 필요가 없다.
			// nodeToMove의 하위노드들도 건드릴 필요가 없다.

			// _INTERNAL_NOACCESSMODE_InsertChild로부터 복사해온 루틴.
			if ( destParentNode->m_child != NULL )
			{
				if ( destParentNode->m_child->m_endSibling == NULL )
					destParentNode->m_child->m_sibling = nodeToMove;
				else
					destParentNode->m_child->m_endSibling->m_sibling = nodeToMove;

				destParentNode->m_child->m_endSibling = nodeToMove;
				nodeToMove->m_parent = destParentNode;
				nodeToMove->m_ownerUUID = destParentNode->UUID;
			}
			else
			{
				destParentNode->m_child = nodeToMove;
				nodeToMove->m_parent = destParentNode;
				nodeToMove->m_ownerUUID = destParentNode->UUID;
			}
		}
		else
		{
		// 이제 기존의 Forest와 연결은 다 끊었다.
		// destForest에 붙이자.
		// nodeToMove 노드와 하위노드들에 RootUUID를 변경한다.
		// RootUUID가 다를 경우만 변경한다.
			_INTERNAL_NOACCESSMODE_RecursiveUpdateRootUUID(destParentNode->m_RootUUID, nodeToMove);

		// desrtForest에 nodeToMove노드를 insert 한다.
			ret = destForest._INTERNAL_NOACCESSMODE_InsertChild(destParentNode, nodeToMove);

			//destForest의 nodeMap에 붙여 준다.
			_INTERNAL_NOACCESSMODE_RecursiveAdd(nodeToMove, &(destForest.m_propNodeMap));

			//srcForest의 nodeMap에서 삭제 한다.
			_INTERNAL_NOACCESSMODE_RecursiveRemove(nodeToMove);
		}

			return ret;
	}

	// 재귀적으로 root UUID를 다른 값으로 변경
	void CLoadedData2::_INTERNAL_NOACCESSMODE_RecursiveUpdateRootUUID(Proud::Guid& RootUUID, CPropNode* Node)
	{
		if (Node == NULL)
			return;

		Node->m_RootUUID = RootUUID;

		_INTERNAL_NOACCESSMODE_RecursiveUpdateRootUUID(RootUUID, Node->GetSibling());
		_INTERNAL_NOACCESSMODE_RecursiveUpdateRootUUID(RootUUID, Node->GetChild());
	}

	// node 및 하위 모든 노드를 제거
	void CLoadedData2::_INTERNAL_NOACCESSMODE_RecursiveRemove(CPropNode *node)
	{
		if (node == NULL)
			return;

		_INTERNAL_NOACCESSMODE_RecursiveRemove(node->m_sibling);
		_INTERNAL_NOACCESSMODE_RecursiveRemove(node->m_child);

		m_propNodeMap.Remove(node->m_UUID);
	}

	// node에 있는 모든 node들을 destMap (K,V store)에 저장
	void CLoadedData2::_INTERNAL_NOACCESSMODE_RecursiveAdd(CPropNodePtr node, PropNodeMap * destMap)
	{
		if (node == NULL)
			return;

		destMap->Add(node->m_UUID, node);

		if (node->m_sibling != NULL)
		{
			_INTERNAL_NOACCESSMODE_RecursiveAdd(_INTERNAL_NOACCESSMODE_GetNode(node->m_sibling->m_UUID), destMap);
		}

		if (node->m_child != NULL)
		{
			_INTERNAL_NOACCESSMODE_RecursiveAdd(_INTERNAL_NOACCESSMODE_GetNode(node->m_child->m_UUID), destMap);
		}
	}

	Proud::ErrorType CLoadedData2::_INTERNAL_NOACCESSMODE_CheckNode(CPropNodePtr node)
	{
		if (node == NULL)
			return ErrorType_Unexpected;

		if (Guid() == node->m_UUID)
			return ErrorType_BadSessionGuid;

		if (Guid() == node->m_RootUUID)
			return ErrorType_BadSessionGuid;

		return ErrorType_Ok;
	}

	Proud::ErrorType CLoadedData2::InsertChild( CPropNodePtr parent, CPropNodePtr newNode )
	{
		//guid발급
		if(newNode->UUID == Guid())
			newNode->m_UUID = Win32Guid::NewGuid();

		if(newNode->RootUUID == Guid())
		{
			AssertThreadID(eAccessMode_Read);

			if(GetRootNode())
				newNode->m_RootUUID = RootUUID;
			else
				newNode->m_RootUUID = newNode->m_UUID;

			ClearThreadID();
		}

		AssertThreadID(eAccessMode_Write);

		Proud::ErrorType ret;
		if ((ret = _INTERNAL_NOACCESSMODE_InsertChild(parent, newNode)) == ErrorType_Ok)
			newNode->m_dirtyFlag = true;

		ClearThreadID();

		return ret;
	}

	Proud::ErrorType CLoadedData2::_INTERNAL_NOACCESSMODE_InsertChild(CPropNodePtr parent, CPropNodePtr newNode)
	{
		ErrorType ret = _INTERNAL_NOACCESSMODE_CheckNode(newNode);
		if (ret != ErrorType_Ok)
			return ret;

		newNode->m_parent = CPropNodePtr();

		PropNodeMap::iterator itr = m_propNodeMap.find(newNode->m_UUID);

		//이미 내부에 존재 한다면.
		if (itr != m_propNodeMap.end())
		{

			return ErrorType_BadSessionGuid;
		}

		if (m_Root == NULL)
		{
			if (parent == NULL)
			{
				m_Root = newNode;
				m_Root->m_parent = CPropNodePtr();
				m_Root->m_UUID = m_Root->RootUUID;
				m_Root->m_ownerUUID = m_Root->RootUUID;
				m_Root->m_sibling = CPropNodePtr();
				m_Root->m_endSibling = CPropNodePtr();
				m_propNodeMap.Add(newNode->m_UUID, newNode);

				return ErrorType_Ok;
			}


			return ErrorType_Unexpected;
		}

		if (parent == NULL)
		{
			CPropNode* pEnd = _INTERNAL_NOACCESSMODE_GetEndSibling(m_Root);
			if (pEnd == NULL)
			{
				return ErrorType_Unexpected;
			}

			newNode->m_parent = CPropNodePtr();
			pEnd->m_sibling = newNode;
			newNode->m_UUID;
			newNode->m_ownerUUID = newNode->m_RootUUID;
		}
		else
		{
			// parent도 forest안에 있는 node인지를 검사해야한다.
			if (ErrorType_Ok != (ret = _INTERNAL_NOACCESSMODE_CheckNode(parent)))
			{

				return ret;
			}

			itr = m_propNodeMap.find(parent->m_UUID);

			if (itr == m_propNodeMap.end())
			{

				return ErrorType_BadSessionGuid;
			}

			if (parent->m_child != NULL)
			{
				if (parent->m_child->m_endSibling == NULL)
					parent->m_child->m_sibling = newNode;
				else
					parent->m_child->m_endSibling->m_sibling = newNode;

				parent->m_child->m_endSibling = newNode;
				newNode->m_parent = parent;
				newNode->m_ownerUUID = parent->UUID;
			}
			else
			{
				parent->m_child = newNode;
				newNode->m_parent = parent;
				newNode->m_ownerUUID = parent->UUID;
			}
		}

		//추가 되었으므로 맵에 넣는다.
		m_propNodeMap.Add(newNode->m_UUID, newNode);

		return ErrorType_Ok;
	}

	CPropNode* CLoadedData2::_INTERNAL_NOACCESSMODE_GetEndSibling(CPropNode* node)
	{
		if (node)
		{
			// node 가 Root 거나 Root의 sibling 이라면.
			if (node->m_parent == CPropNodePtr())
			{
				return m_Root->m_endSibling;
			}
			else
			{
				return node->m_parent->m_child->m_endSibling;
			}
		}

		return CPropNodePtr();
	}

	Proud::ErrorType CLoadedData2::RemoveNode( CPropNodePtr node , bool addremovelist)
	{
		AssertThreadID(eAccessMode_Write);
		Proud::ErrorType ret = _INTERNAL_NOACCESSMODE_RemoveNode(node, addremovelist);
		ClearThreadID();

		return ret;
	}

	Proud::ErrorType CLoadedData2::RemoveNode( Proud::Guid removeUUID, bool addremovelist )
	{
		CPropNodePtr removeNode = GetNode(removeUUID);

		if(removeNode)
		{
			AssertThreadID(eAccessMode_Write);
			Proud::ErrorType ret = _INTERNAL_NOACCESSMODE_RemoveNode(removeNode, addremovelist);
			ClearThreadID();
			return ret;
		}

		return ErrorType_LoadedDataNotFound;
	}

	Proud::ErrorType CLoadedData2::_INTERNAL_NOACCESSMODE_RemoveNode(CPropNodePtr node, bool addremovelist)
	{
		if (m_Root == NULL || m_propNodeMap.GetCount() <= 0)
			return ErrorType_Unexpected;

		Proud::ErrorType ret = _INTERNAL_NOACCESSMODE_CheckNode(node);
		if (ErrorType_Ok != ret)return ret;

		PropNodeMap::iterator itr = m_propNodeMap.find(node->m_UUID);

		if (itr == m_propNodeMap.end())
		{
			return ErrorType_Unexpected;
		}

		//부모를 돌면서 내 앞의 sibling와 연결 끊기
		CPropNode* psibling;

		//첫째로 parent가 있는지 검사.
		if (node->m_parent)
		{
			psibling = node->m_parent->m_child;

			//부모의 자식이 나라면 하위중 내가 맏형이다.
			if (psibling == node.get())
			{
				//맏이가 없어지므로 차위를 부모에게 이어준다.
				node->m_parent->m_child = node->m_sibling;

				if (node->m_sibling)
				{
					node->m_sibling->m_endSibling = node->m_endSibling;
				}

				// node의 parent 와 sibling 과의 연결을 끊어 준다.
				// 밑에 스택에 들어가기전에 sibling 과의 연결을 끊어 줘야 엄한 sibling 을 제거 하지 않는다 ..
				// child 를 여기서 끊어주면 자식들이 제거 되지 못함.
				node->m_parent = NULL;
				node->m_sibling = NULL;
				node->m_endSibling = NULL;
			}
			else
			{
				//node의 앞의 것(형)을 찾는다.
				while (psibling)
				{
					if (psibling->m_sibling == node.get())
					{
						break;
					}

					psibling = psibling->m_sibling;
				}

				//NULL이 아니라고 가정하고 테스트하자.
				//node를 빼주고 양쪽을 이어준다.
				psibling->m_sibling = node->m_sibling;

				// node 가 마지막 노드라면 앞의 것이 endSibling 이 된다.
				// 마지막이 아니라면 endSibling 이 바뀌지 않는다.
				if (_INTERNAL_NOACCESSMODE_GetEndSibling(node) == node)
				{
					psibling->m_parent->m_child->m_endSibling = psibling;
				}

				node->m_parent = NULL;
				node->m_sibling = NULL;
			}
		}
		//parent가 없다면 root에서 검사.
		else
		{
			psibling = m_Root;

			if (psibling == node.get())
			{
				m_Root = node->m_sibling;

				if (m_Root)
				{
					m_Root->m_endSibling = node->m_endSibling;
				}

				node->m_parent = NULL;
				node->m_sibling = NULL;
				node->m_endSibling = NULL;
			}
			else
			{
				//node의 앞의 것(형)을 찾는다.
				while (psibling)
				{
					if (psibling->m_sibling == node.get())
					{
						break;
					}

					psibling = psibling->m_sibling;
				}

				//NULL이 아니라고 가정하고 테스트하자.
				//node를 빼주고 양쪽을 이어준다.
				psibling->m_sibling = node->m_sibling;

				// node 가 마지막노드라면 Endsibling 을 갱신.
				// 마지막 노드가 아니라면 바뀌지 않는다.
				if (m_Root->m_endSibling == node)
				{
					m_Root->m_endSibling = psibling;
				}

				node->m_parent = NULL;
				node->m_sibling = NULL;
				node->m_endSibling = NULL;
			}
		}

		CFastList2<CPropNodePtr, int> stack;
		stack.AddTail(_INTERNAL_NOACCESSMODE_GetNode(node->m_UUID));

		while (stack.GetCount()>0)
		{
			CPropNodePtr top = stack.RemoveTail();

			if (top->m_sibling != NULL)
			{
				stack.AddTail(_INTERNAL_NOACCESSMODE_GetNode(top->m_sibling->UUID));
			}
			if (top->m_child != NULL)
			{
				stack.AddTail(_INTERNAL_NOACCESSMODE_GetNode(top->m_child->UUID));
			}

			top->m_sibling = NULL;
			top->m_child = NULL;
			top->m_parent = NULL;
			top->m_endSibling = NULL;

			m_propNodeMap.Remove(top->m_UUID);

			if (addremovelist)
				m_removePropNodeList.AddTail(top);
		}

		return ErrorType_Ok;
	}

	CPropNodePtr CLoadedData2::GetNode( const Guid& guid )
	{
		AssertThreadID(eAccessMode_Read);
		CPropNodePtr ret = _INTERNAL_NOACCESSMODE_GetNode(guid);
		ClearThreadID();
		return ret;
	}

	CPropNodePtr CLoadedData2::_INTERNAL_NOACCESSMODE_GetNode(const Guid& guid)
	{
		PropNodeMap::iterator itr = m_propNodeMap.find(guid);

		if (itr != m_propNodeMap.end())
		{
			return itr->GetSecond();
		}
		return CPropNodePtr();
	}

	CPropNodePtr CLoadedData2::GetRemoveNode( Proud::Guid removeUUID )
	{
		AssertThreadID(eAccessMode_Read);
		CPropNodePtr ret = _INTERNAL_NOACCESSMODE_GetRemoveNode(removeUUID);
		ClearThreadID();
		return ret;
	}

	Proud::CPropNodePtr CLoadedData2::_INTERNAL_NOACCESSMODE_GetRemoveNode(Proud::Guid &removeUUID)
	{
		PropNodeList::iterator itr = m_removePropNodeList.begin();

		for (; itr != m_removePropNodeList.end(); ++itr)
		{
			if ((*itr)->UUID == removeUUID)
			{
				return *itr;
			}
		}

		return CPropNodePtr();
	}

	CPropNodePtr CLoadedData2::GetRootNode()
	{
		if (m_Root == NULL)
			return CPropNodePtr();

		return _INTERNAL_NOACCESSMODE_GetNode(m_Root->UUID);
	}

	void CLoadedData2::_INTERNAL_ChangeToByteArray( ByteArray& outArray )
	{
		CMessage msg;
		msg.UseInternalBuffer();
		Change_Serialize(msg,false);
		msg.CopyTo(outArray);
	}

	void CLoadedData2::Change_Serialize( CMessage& msg,bool isReading)
	{
		if(!isReading)
		{
			AssertThreadID(eAccessMode_Read);
			_INTERNAL_NOACCESSMODE_SerializeToMessage(msg);
			ClearThreadID();
		}
		else
		{
			AssertThreadID(eAccessMode_Write);
			_INTERNAL_NOACCESSMODE_DeserializeFromMessageToChangeList(msg);
			ClearThreadID();
		}
	}

	int CLoadedData2::_INTERNAL_NOACCESSMODE_GetDirtyCount()
	{
		int nCount = 0;
		PropNodeMap::iterator itr = m_propNodeMap.begin();
		for (; itr != m_propNodeMap.end(); ++itr)
		{
			if ( itr->GetSecond()->m_dirtyFlag )
				++nCount;
		}

		return nCount;
	}

	void CLoadedData2::_INTERNAL_NOACCESSMODE_SerializeToMessage(CMessage &msg)
	{
		// dirtyout에 넣을때 parent, child 순서를 보장해야 받는곳에서 제대로 Forest를 만들수 있다.!
		CFastList2<CPropNodePtr, int> tempQueue;
		CPropNode* pRoot = m_Root;

		int ndirtycount = _INTERNAL_NOACCESSMODE_GetDirtyCount();
		msg << ndirtycount;

		//위의 방법은 문제가 있다.
		//순서 보장을 위해 이렇게 넣어야 한다.
		if (m_Root != NULL)
			tempQueue.AddTail(_INTERNAL_NOACCESSMODE_GetNode(pRoot->UUID));

		CPropNodePtr node;
		int realCount = 0;
		while (tempQueue.GetCount() > 0)
		{
			node = tempQueue.RemoveHead();

			if (node->m_sibling)
			{
				tempQueue.AddTail(_INTERNAL_NOACCESSMODE_GetNode(node->m_sibling->UUID));
			}
			if (node->m_child)
			{
				tempQueue.AddTail(_INTERNAL_NOACCESSMODE_GetNode(node->m_child->UUID));
			}

			if (node->m_dirtyFlag)
			{
				msg << *node;
				++realCount;
			}
		}

		if (ndirtycount != realCount)
		{
			assert(0);
		}

		int nRemoveCount = GetRemoveCount();
		msg << nRemoveCount;

		PropNodeList::iterator Ritr = m_removePropNodeList.begin();
		for (; Ritr != m_removePropNodeList.end(); ++Ritr)
		{
			CPropNodePtr node2 = *Ritr;

			msg << *node2;
		}
	}

	void CLoadedData2::_INTERNAL_NOACCESSMODE_DeserializeFromMessageToChangeList(CMessage &msg)
	{
		int dirtyCount = 0;
		msg >> dirtyCount;

		for (int i = 0; i < dirtyCount; ++i)
		{
			CPropNodePtr ptr = CPropNodePtr(new CPropNode(_PNT("")));
			msg >> *ptr;

			//add 인지 update인지 판단한다.
			CPropNodePtr node = _INTERNAL_NOACCESSMODE_GetNode(ptr->m_UUID);

			if (node == NULL)
			{
				//add
				CPropNodePtr parent = _INTERNAL_NOACCESSMODE_GetNode(ptr->m_ownerUUID);

				_INTERNAL_NOACCESSMODE_InsertChild(parent, ptr);
			}
			else
			{
				//update
				CProperty* node2 = node;
				CProperty* ptr2 = ptr;
				*node2 = *ptr2; // CProperty 레벨에서의 복사만 진행
			}
		}

		int removeCount = 0;
		msg >> removeCount;

		for (int i = 0; i < removeCount; ++i)
		{
			CPropNodePtr ptr = CPropNodePtr(new CPropNode(_PNT("")));

			msg >> *ptr;

			CPropNodePtr node = _INTERNAL_NOACCESSMODE_GetNode(ptr->m_UUID);

			if (node == NULL)
				//없는것을 제거 하려는 경우...
				//일단은 continue;
				continue;

			_INTERNAL_NOACCESSMODE_RemoveNode(node, true);
		}
	}

//	Proud::ErrorType CLoadedData2::_INTERNAL_NOACCESSMODE_InsertSiblingBefore(CPropNodePtr sibling, CPropNodePtr newNode)
//	{
//		ErrorType ret = _INTERNAL_NOACCESSMODE_CheckNode(newNode);
//		if (ret != ErrorType_Ok)
//			return ret;
//
//
//
//		PropNodeMap::iterator itr = m_propNodeMap.find(newNode->m_UUID);
//
//		if (itr != m_propNodeMap.end())
//		{
//
//			return ErrorType_BadSessionGuid;
//		}
//
//		if (m_Root == NULL)
//		{
//			if (sibling == NULL)
//			{
//				m_Root = newNode;
//				m_Root->m_parent = CPropNodePtr();
//				m_Root->m_ownerUUID = m_Root->m_RootUUID;
//
//				m_propNodeMap.Add(newNode->m_UUID, newNode);
//
//				return ErrorType_Ok;
//			}
//
//			return ErrorType_Unexpected;
//		}
//
//		if (sibling == NULL)
//		{
//			newNode->m_parent = CPropNodePtr();;
//			newNode->m_sibling = m_Root;
//			m_Root = newNode;
//			m_Root->m_ownerUUID = m_Root->m_RootUUID;
//		}
//		else
//		{
//			if (ErrorType_Ok != (ret = _INTERNAL_NOACCESSMODE_CheckNode(sibling)))
//			{
//				return ret;
//			}
//
//			itr = m_propNodeMap.find(sibling->m_UUID);
//
//			if (itr == m_propNodeMap.end())
//			{
//				return ErrorType_BadSessionGuid;
//			}
//
//			newNode->m_sibling = sibling;
//			newNode->m_parent = sibling->m_parent;
//
//			if (newNode->m_parent != NULL)
//			{
//				newNode->m_ownerUUID = newNode->m_parent->m_UUID;
//				newNode->m_parent->m_child = newNode;
//			}
//			else
//				newNode->m_ownerUUID = newNode->m_RootUUID;
//
//			//만일 sibling가 root라면 루트 변경
//			if (m_Root->m_UUID == sibling->m_UUID)
//			{
//				newNode->m_parent = CPropNodePtr();;
//				m_Root = newNode;
//			}
//		}
//
//		m_propNodeMap.Add(newNode->m_UUID, newNode);
//
//		return ErrorType_Ok;
//	}


	void CLoadedData2::Serialize( CMessage& msg,bool isReading )
	{
		if(isReading)
		{
			AssertThreadID(eAccessMode_Write);

			_INTERNAL_NOACCESSMODE_DeleteAll();

			int nCount = 0;
			msg >>nCount;

			CFastList2<CPropNodePtr,int> TempList;

			for(int i=0;i<nCount;++i)
			{
				CPropNodePtr prop = CPropNodePtr(new CPropNode(_PNT("")));
				msg >> *prop;

				// Root인경우
				if(prop->UUID == prop->RootUUID)
					_INTERNAL_NOACCESSMODE_InsertChild(CPropNodePtr(), prop);
				else
				{
					CPropNodePtr pFind  = _INTERNAL_NOACCESSMODE_GetNode(prop->m_ownerUUID);

					if(pFind)
						_INTERNAL_NOACCESSMODE_InsertChild(pFind,prop);
					else
						TempList.AddTail(prop);
				}
			}

			//무한루프 방지를 위한 안전장치
			nCount = (int)TempList.GetCount();
			while(TempList.GetCount() >0)
			{
				CPropNodePtr node	= TempList.RemoveHead();

				CPropNodePtr find = _INTERNAL_NOACCESSMODE_GetNode(node->m_ownerUUID);

				if(find)
				{
					_INTERNAL_NOACCESSMODE_InsertChild(find, node);
					nCount = (int)TempList.GetCount();
				}
				else
				{
					TempList.AddTail(node);
					--nCount;

					if(nCount <=0)
						break;
				}
			}

			ClearThreadID();

			if (TempList.GetCount() > 0)
			{
				stringstream ss;
				ss << "Exception from " << __FUNCTION__;
				throw Exception(ss.str().c_str());
			}
		}
		else
		{
			AssertThreadID(eAccessMode_Read);

			int nCount = (int)m_propNodeMap.GetCount();
			msg << nCount;
			PropNodeMap::const_iterator itr = m_propNodeMap.begin();

			for (; itr != m_propNodeMap.end(); ++itr)
			{
				msg << *(itr->GetSecond());
			}

			ClearThreadID();
		}
	}

//	Proud::ErrorType CLoadedData2::_INTERNAL_NOACCESSMODE_InsertSiblingAfter(CPropNodePtr sibling, CPropNodePtr newNode)
//	{
//		ErrorType ret = _INTERNAL_NOACCESSMODE_CheckNode(newNode);
//		if (ret != ErrorType_Ok)
//			return ret;
//
//		PropNodeMap::iterator itr = m_propNodeMap.find(newNode->m_UUID);
//
//		if (itr != m_propNodeMap.end())
//		{
//			return ErrorType_BadSessionGuid;
//		}
//
//		if (m_Root == NULL)
//		{
//			if (sibling == NULL)
//			{
//				m_Root = newNode;
//				m_Root->m_parent = CPropNodePtr();;
//				m_Root->m_ownerUUID = m_Root->m_RootUUID;
//
//				m_propNodeMap.Add(newNode->m_UUID, newNode);
//
//				return ErrorType_Ok;
//			}
//
//			return ErrorType_Unexpected;
//		}
//
//		if (sibling == NULL)
//		{
//			CPropNode* pEnd = _INTERNAL_NOACCESSMODE_GetEndSibling(m_Root);
//			if (pEnd == NULL)
//			{
//
//				return ErrorType_Unexpected;
//			}
//
//			newNode->m_parent = CPropNodePtr();
//			pEnd->m_sibling = newNode;
//			newNode->m_ownerUUID = newNode->m_RootUUID;
//		}
//		else
//		{
//			if (ErrorType_Ok != (ret = _INTERNAL_NOACCESSMODE_CheckNode(sibling)))
//			{
//
//				return ret;
//			}
//
//			itr = m_propNodeMap.find(sibling->m_UUID);
//
//			if (itr == m_propNodeMap.end())
//			{
//
//				return ErrorType_BadSessionGuid;
//			}
//
//			if (sibling->m_sibling != NULL)
//			{
//				CPropNode* pEnd = _INTERNAL_NOACCESSMODE_GetEndSibling(sibling);
//				if (pEnd == NULL) return ErrorType_Unexpected;
//
//				pEnd->m_sibling = newNode;
//				newNode->m_parent = pEnd->m_parent;
//			}
//			else
//			{
//				sibling->m_sibling = newNode;
//				newNode->m_parent = sibling->m_parent;
//			}
//
//			if (newNode->m_parent != NULL)
//				newNode->m_ownerUUID = newNode->m_parent->m_UUID;
//			else
//				newNode->m_ownerUUID = newNode->m_RootUUID;
//		}
//
//		m_propNodeMap.Add(newNode->m_UUID, newNode);
//
//		return ErrorType_Ok;
//	}

	void CLoadedData2::_INTERNAL_NOACCESSMODE_DeleteAll()
	{
		m_propNodeMap.Clear();
		m_removePropNodeList.Clear();
		m_Root = CPropNodePtr();
	}

	void CLoadedData2::CopyTo_NoChildren( CLoadedData2* output )
	{
		AssertThreadID(eAccessMode_Read);

		_INTERNAL_NOACCESSMODE_CopyTo(*output);

		ClearThreadID();
	}

	void CLoadedData2::_INTERNAL_NOACCESSMODE_CopyTo(CLoadedData2& to)
	{
		to.m_propNodeMap.Clear();
		to.m_removePropNodeList.Clear();
		to.m_Root = NULL;

		PropNodeMap::iterator itr = m_propNodeMap.begin();
		for (; itr != m_propNodeMap.end(); ++itr)
		{
			CPropNodePtr add = CPropNodePtr(new CPropNode(itr->GetSecond()->m_INTERNAL_TypeName));
			*add = *(itr->GetSecond());
			add->m_parent = NULL;
			add->m_child = NULL;
			add->m_sibling = NULL;
			add->m_endSibling = NULL;

			to.m_propNodeMap.Add(add->UUID, add);
		}

		//link
		itr = to.m_propNodeMap.begin();
		for (; itr != to.m_propNodeMap.end(); ++itr)
		{
			CPropNodePtr node = _INTERNAL_NOACCESSMODE_GetNode(itr->GetSecond()->UUID);

			if (node != m_Root)
				itr->GetSecond()->m_parent = to._INTERNAL_NOACCESSMODE_GetNode(itr->GetSecond()->OwnerUUID);

			if (node->Sibling)
				itr->GetSecond()->m_sibling = to._INTERNAL_NOACCESSMODE_GetNode(node->Sibling->UUID);

			if (node->Child)
				itr->GetSecond()->m_child = to._INTERNAL_NOACCESSMODE_GetNode(node->Child->UUID);

			if (node->EndSibling)
				itr->GetSecond()->m_endSibling = to._INTERNAL_NOACCESSMODE_GetNode(node->EndSibling->UUID);
		}

		Position pos = m_removePropNodeList.GetHeadPosition();
		while (pos != NULL)
		{
			CPropNodePtr &removenode = m_removePropNodeList.GetNext(pos);

			CPropNodePtr removeAdd = CPropNodePtr(new CPropNode(removenode->m_INTERNAL_TypeName));

			*removeAdd = *removenode;

			to.m_removePropNodeList.AddTail(removeAdd);
		}

		if (m_Root)
		{
			to.m_Root = to._INTERNAL_NOACCESSMODE_GetNode(m_Root->UUID);
		}
	}

	void CLoadedData2::ToByteArray( ByteArray& output )
	{
		CMessage msg;
		msg.UseInternalBuffer();
		Serialize(msg,false);
		msg.CopyTo(output);
	}

	void CLoadedData2::FromByteArray( const ByteArray& input )
	{
		CMessage msg;
		msg.UseExternalBuffer((uint8_t*)input.GetData(),(int)input.GetCount());
		msg.SetLength((int)input.GetCount());
		Serialize(msg,true);

		if(msg.GetReadOffset()!=msg.GetLength())
		{
			throw Exception("Could Not Read until the End of the Data! A bug Suspected");
		}
	}

	Guid CLoadedData2::GetRootUUID()
	{
		if(GetRootNode() == NULL)
			return Guid();

		return GetRootNode()->UUID;
	}

	CLoadedData2Ptr CLoadedData2::Clone()
	{
		CLoadedData2Ptr output(new CLoadedData2);

		CopyTo_NoChildren(output);
		output->sessionGuid=sessionGuid;

		return output;
	}

	Guid CLoadedData2::GetSessionGuid() const
	{
		return m_INTERNAL_sessionGuid;
	}

	void CLoadedData2::SetSessionGuid( Guid val )
	{
		m_INTERNAL_sessionGuid = val;
	}

	void CLoadedData2::_INTERNAL_ClearChangeNode()
	{
		AssertThreadID(eAccessMode_Write);
		_INTERNAL_NOACCESSMODE_ClearChangeNode();
		ClearThreadID();
	}

	void CLoadedData2::_INTERNAL_NOACCESSMODE_ClearChangeNode()
	{
		PropNodeMap::iterator itr = m_propNodeMap.begin();
		for (; itr != m_propNodeMap.end(); ++itr)
		{
			itr->GetSecond()->m_dirtyFlag = false;
		}

		m_removePropNodeList.Clear();
	}

	void CLoadedData2::_INTERNAL_FromByteArrayToChangeList( const ByteArray &inArray )
	{
		if(inArray.GetCount() >0)
		{
			CMessage msg;
			msg.UseExternalBuffer((uint8_t*)inArray.GetData(),(int)inArray.GetCount());
			msg.SetLength((int)inArray.GetCount());

			Change_Serialize(msg,true);

			if(msg.GetReadOffset()!=msg.GetLength())
			{
				throw Exception("Could Not Read until the End of the Data! A bug Suspected");
			}
		}
	}

	// removeNodeList를 얻는다.
	const PropNodeList* CLoadedData2::_INTERNAL_GetRemoveNodeList() const
	{
		return &m_removePropNodeList;
	}

	// 실제로 노드를 삭제하지말고 지울 노드들만 removeNodeList에 추가한다.
	Proud::ErrorType CLoadedData2::_INTERNAL_AddRemovePropNodeList( Proud::Guid removeUUID )
	{
		CPropNodePtr removeNode = GetNode(removeUUID);

		if(removeNode)
		{
			AssertThreadID(eAccessMode_Write);
			Proud::ErrorType ret = _INTERNAL_NOACCESSMODE_AddRemovePropNodeList(removeNode);
			ClearThreadID();
			return ret;
		}

		return ErrorType_LoadedDataNotFound;
	}

	void CLoadedData2::_INTERNAL_NOACCESSMODE_CopyTo_Diff(CLoadedData2& to)
	{
		//root부터 쭉 돌면서 다른것들을 판단한다.
		//root부터 하는 이유는 add를 한번에 하기위함이다.

		if (m_Root)
		{
			CFastList2<CPropNodePtr, int> tempQueue;
			tempQueue.AddTail(_INTERNAL_NOACCESSMODE_GetNode(m_Root->UUID));

			// 트리의 각 노드에 대해
			while (tempQueue.GetCount() > 0)
			{
				CPropNodePtr head = tempQueue.RemoveHead();

				if (head->m_sibling)
				{
					tempQueue.AddTail(_INTERNAL_NOACCESSMODE_GetNode(head->m_sibling->UUID));
				}
				if (head->m_child)
				{
					tempQueue.AddTail(_INTERNAL_NOACCESSMODE_GetNode(head->m_child->UUID));
				}

				// to에서도 같은 노드를 찾아
				CPropNodePtr add = to._INTERNAL_NOACCESSMODE_GetNode(head->UUID);

				if (add)
				{
					// 있으면 갱신
					if (head->m_dirtyFlag)
					{
						*add = (CProperty)(*head);
						add->m_dirtyFlag = true;
					}
				}
				else
				{
					// 없으면 새로 추가
					add = CPropNodePtr(new CPropNode(head->TypeName));
					*add = *head;

					add->m_child = NULL;
					add->m_parent = NULL;
					add->m_sibling = NULL;
					add->m_endSibling = NULL;

					if (!head->m_parent)
					{
						to._INTERNAL_NOACCESSMODE_InsertChild(CPropNodePtr(), add);
					}
					else
					{
						CPropNodePtr ownerData = to._INTERNAL_NOACCESSMODE_GetNode(head->m_ownerUUID);

						if (!ownerData)
							assert(0);

						to._INTERNAL_NOACCESSMODE_InsertChild(ownerData, add);
					}
				}
			}
		}

		// this에서 갖고 있는 '제거될 노드들' 목록을 to에 반영한다.
		Position pos = m_removePropNodeList.GetHeadPosition();
		while (pos != NULL)
		{
			CPropNodePtr &removenode = m_removePropNodeList.GetNext(pos);

			CPropNodePtr node = to._INTERNAL_NOACCESSMODE_GetNode(removenode->UUID);

			if (node)
			{
				to._INTERNAL_NOACCESSMODE_RemoveNode(node, true);
			}
		}
	}

	Proud::ErrorType CLoadedData2::_INTERNAL_NOACCESSMODE_AddRemovePropNodeList(CPropNodePtr node)
	{
		if (m_Root == NULL || m_propNodeMap.GetCount() <= 0)
			return ErrorType_Unexpected;

		Proud::ErrorType ret = _INTERNAL_NOACCESSMODE_CheckNode(node);
		if (ErrorType_Ok != ret)return ret;

		PropNodeMap::iterator itr = m_propNodeMap.find(node->m_UUID);

		if (itr == m_propNodeMap.end())
		{
			return ErrorType_Unexpected;
		}

		// 앞뒤 연결을 끊고 잇기 및 맵에서의 삭제는 removeNode에서하고 여기서는 removelist에만 추가한다.
		m_removePropNodeList.AddTail(node);

		CFastList2<CPropNodePtr, intptr_t> tempQueue;
		CPropNode* removeNode = node;
		if (removeNode->m_child)
		{
			tempQueue.AddTail(_INTERNAL_NOACCESSMODE_GetNode(removeNode->m_child->UUID));
		}

		while (tempQueue.GetCount() > 0)
		{
			CPropNodePtr head = tempQueue.RemoveHead();

			if (head->m_sibling != NULL)
			{
				tempQueue.AddTail(_INTERNAL_NOACCESSMODE_GetNode(head->m_sibling->UUID));
			}
			if (head->m_child != NULL)
			{
				tempQueue.AddTail(_INTERNAL_NOACCESSMODE_GetNode(head->m_child->UUID));
			}

			// 여기서 링크를 삭제하면 실제 데이터 삭제시 m_propNodeMap에서 삭제가 안되는 문제 발생
			//head->m_sibling = NULL;
			//head->m_child = NULL;
			//head->m_parent = NULL;

			// 실제로 삭제는 하지 않고 removelist에만 추가.
			m_removePropNodeList.AddTail(head);
		}

		return ErrorType_Ok;
	}
}

#endif // _WIN32
