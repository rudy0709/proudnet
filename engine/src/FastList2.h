/*
ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.

ProudNet

This program is soley copyrighted by Nettention.
Any use, correction, and distribution of this program are subject to the terms and conditions of the License Agreement.
Any violated use of this program is prohibited and will be cause of immediate termination according to the License Agreement.

** WARNING : PLEASE DO NOT REMOVE THE LEGAL NOTICE ABOVE.

*/

#pragma once

#include <assert.h>
#include "../include/Ptr.h"
#include "../include/BasicTypes.h"
#include "../include/Singleton.h"
#include "../include/Enums.h"

namespace Proud
{
	extern const char* CannotUseFastHeapForFilledCollectionErrorText;

	// Proud.CFastList와 유사하지만 훨씬 속도가 빠름
	// 내부 기술 유출 방지를 위해 src 폴더에 따로 있음.
	// 내부적으로 memory pool을 사용함
	template < typename E, typename INDEXTYPE, typename ETraits = CPNElementTraits< E > >
	class CFastList2
	{
	public:
		typedef typename ETraits::INARGTYPE INARGTYPE;
	private:
		class CNode : public __Position
		{
		public:
			inline CNode()
			{
			}
			inline CNode(INARGTYPE element)
				:m_element(element)
			{
			}
			inline ~CNode() throw()
			{
			}

		public:
			CNode* m_pNext;	// pooled 상태에서는 재활용 가능한 다음 node를 가리킴
			CNode* m_pPrev; // pooled 상태에서는 무효
			E m_element;

		private:
			CNode(const CNode&) throw();
		};

	public:
		CFastList2() throw()
		{
			m_nElements = 0;
			m_pHead = NULL;
			m_pTail = NULL;

			m_freeList = NULL;
		}

#if defined(_MSC_VER)
		__declspec(property(get = GetCount)) INDEXTYPE Count;
#endif

		inline INDEXTYPE GetCount() const throw()
		{
			return(m_nElements);
		}

		/// \~korean \return 아무런 데이터가 없으면 true, 존재하면 false를 리턴 \~english \~
		inline bool IsEmpty() const throw()
		{
			return(m_nElements == 0);
		}

		/// \~korean \return list의 첫번째 data를 리턴합니다. \~english \~
		inline E& GetHead()
		{
			assert(m_pHead != NULL);
			return(m_pHead->m_element);
		}

		/// \~korean \return list의 첫번째 data를 const 변수로 리턴합니다. \~english \~
		inline const E& GetHead() const
		{
			assert(m_pHead != NULL);
			return(m_pHead->m_element);
		}

		inline const void GetHead(E* output) const
		{
			assert(m_pHead != NULL);
			*output = m_pHead->m_element;
		}

		/// \~korean \return list의 마지막 Data를 리턴합니다. \~english \~
		inline E& GetTail()
		{
			assert(m_pTail != NULL);
			return(m_pTail->m_element);
		}

		/// \~korean \return list의 마지막 Data를 const 변수로 리턴합니다. \~english \~
		inline const E& GetTail() const
		{
			assert(m_pTail != NULL);
			return(m_pTail->m_element);
		}

		/// \~korean list 의 첫번째 Data를 list내에서 제거하고 리턴해 줍니다. \return list에서 제거된 data \~english \~
		inline E RemoveHead()
		{
			assert(m_pHead != NULL);

			CNode* pNode = m_pHead;

			// 사본 뜸
			E element(pNode->m_element);

			m_pHead = pNode->m_pNext;

			if (m_pHead != NULL)
			{
				m_pHead->m_pPrev = NULL;
			}
			else
			{
				m_pTail = NULL;
			}

			NodeToPool(pNode);

			return(element);
		}

		// 맨 앞의 것을 꺼낸다. by reference이므로 속도 유리.
		inline void RemoveHead(E& output)
		{
			assert(m_pHead != NULL);

			CNode* pNode = m_pHead;

			// 이동시킨다. E가 shared_ptr을 가질 때 속도 유리.
			output = MOVE_OR_COPY(pNode->m_element);

			m_pHead = pNode->m_pNext;

			if (m_pHead != NULL)
			{
				m_pHead->m_pPrev = NULL;
			}
			else
			{
				m_pTail = NULL;
			}

			NodeToPool(pNode);
		}

		/// \~korean list 의 마지막 Data를 list내에서 제거하고 리턴해 줍니다. \return list에서 제거된 data \~english \~
		inline E RemoveTail()
		{
			assert(m_pTail != NULL);

			CNode* pNode = m_pTail;

			// 사본 뜸
			E element(pNode->m_element);

			m_pTail = pNode->m_pPrev;

			if (m_pTail != NULL)
			{
				m_pTail->m_pNext = NULL;
			}
			else
			{
				m_pHead = NULL;
			}
			NodeToPool(pNode);

			return(element);
		}

		/// \~korean list의 첫번째 data를 제거합니다. \~english \~
		inline void RemoveHeadNoReturn() throw()
		{
			assert(m_pHead != NULL);

			CNode* pNode = m_pHead;

			m_pHead = pNode->m_pNext;

			if (m_pHead != NULL)
			{
				m_pHead->m_pPrev = NULL;
			}
			else
			{
				m_pTail = NULL;
			}
			NodeToPool(pNode);
		}

		/// \~korean list의 마지막 data를 제거합니다. \~english \~
		inline void RemoveTailNoReturn() throw()
		{
			assert(m_pTail != NULL);

			CNode* pNode = m_pTail;

			m_pTail = pNode->m_pPrev;
			if (m_pTail != NULL)
			{
				m_pTail->m_pNext = NULL;
			}
			else
			{
				m_pHead = NULL;
			}
			NodeToPool(pNode);
		}

		/// \~korean Data가 비어있는 해더를 추가한다. \return 새로 추가된 노드의 Position \~english \~
		inline Proud::Position AddHead()
		{
			CNode* pNode = NewOrRecycleNode(NULL, m_pHead);
			if (m_pHead != NULL)
			{
				m_pHead->m_pPrev = pNode;
			}
			else
			{
				m_pTail = pNode;
			}
			m_pHead = pNode;

			return(Proud::Position(pNode));
		}

		/**
		\~korean 헤더를 추가합니다.
		\param element 새로운 헤더에 넣을 Data
		\return 새로 추가된 노드의 Position
		\~english
		*/
		inline Proud::Position AddHead(INARGTYPE element)
		{
			CNode* pNode;

			pNode = NewOrRecycleNode(element, NULL, m_pHead);

			if (m_pHead != NULL)
			{
				m_pHead->m_pPrev = pNode;
			}
			else
			{
				m_pTail = pNode;
			}
			m_pHead = pNode;

			return(Proud::Position(pNode));
		}

		/**
		\~korean
		헤더에 새로운 list를 추가합니다.
		\param plNew 헤더에 추가할 list
		\~english
		\~
		*/
		inline void AddHeadList(const CFastList2* plNew)
		{
			assert(plNew != NULL);

			Proud::Position pos = plNew->GetTailPosition();
			while (pos != NULL)
			{
				INARGTYPE element = plNew->GetPrev(pos);
				AddHead(element);
			}
		}

		/**
		\~korean
		list의 마지막에 빈 노드를 추가한다.
		\return 새로 추가된 노드의 Position
		\~english
		\~
		*/
		inline Proud::Position AddTail()
		{
			CNode* pNode = NewOrRecycleNode(m_pTail, NULL);
			if (m_pTail != NULL)
			{
				m_pTail->m_pNext = pNode;
			}
			else
			{
				m_pHead = pNode;
			}
			m_pTail = pNode;

			return(Proud::Position(pNode));
		}

		/**
		\~korean
		list의 마지막에 node를 추가
		\param element 마지막 노드에 삽입을 Data
		\return 새로 추가된 노드의 Position
		\~english
		\~
		*/
		inline Proud::Position AddTail(INARGTYPE element)
		{
			CNode* pNode;

			pNode = NewOrRecycleNode(element, m_pTail, NULL);

			if (m_pTail != NULL)
			{
				m_pTail->m_pNext = pNode;
			}
			else
			{
				m_pHead = pNode;
			}
			m_pTail = pNode;

			return(Proud::Position(pNode));
		}

		/**
		\~korean
		List의 마지막노드에 새로운 List를 추가
		\param plNew 추가할 List
		\~english
		\~
		*/
		inline void AddTailList(const CFastList2* plNew)
		{
			assert(plNew != NULL);

			Proud::Position pos = plNew->GetHeadPosition();
			while (pos != NULL)
			{
				INARGTYPE element = plNew->GetNext(pos);
				AddTail(element);
			}
		}

		/// \~korean 모든 데이터를 제거. Clear()와 같음. \~english \~
		inline void RemoveAll() throw()
		{
			while (m_nElements > 0)
			{
				CNode* pKill = m_pHead;
				assert(pKill != NULL);

				m_pHead = m_pHead->m_pNext;
				NodeToPool(pKill);
			}

			assert(m_nElements == 0);
			m_pHead = NULL;
			m_pTail = NULL;
		}

		/// \~korean 모든 데이터를 제거. RemoveAll()같음. \~english \~
		inline void Clear() { RemoveAll(); }
		inline void ClearAndKeepCapacity() { Clear(); }

		/// \~korean \return 헤더의 Position을 리턴 \~english \~
		inline Proud::Position GetHeadPosition() const throw()
		{
			return(Proud::Position(m_pHead));
		}

		/// \~korean \return list의 마지막 노드의 Position을 리턴
		inline Proud::Position GetTailPosition() const throw()
		{
			return(Proud::Position(m_pTail));
		}

		/**
		\~korean
		인자로 넣은 Position의 다음 node Position을 얻는다.
		\param pos 현재 Position
		\return 다음 노드의 Data를 리턴
		\~english
		\~
		*/
		inline E& GetNext(Proud::Position& pos)
		{
			CNode* pNode;

			assert(pos != NULL);
			pNode = (CNode*)pos;
			pos = Proud::Position(pNode->m_pNext);

			return(pNode->m_element);
		}

		/**
		\~korean
		인자로 넣은 Position의 다음 node Position을 얻는다.
		\param pos 현재 Position
		\return 다음 노드의 Data를 const변수로 리턴
		\~english
		\~
		*/
		inline const E& GetNext(Proud::Position& pos) const
		{
			CNode* pNode;

			assert(pos != NULL);
			pNode = (CNode*)pos;
			pos = Proud::Position(pNode->m_pNext);

			return(pNode->m_element);
		}

		/**
		\~korean
		인자로 넣은 Position의 전 node Position을 얻는다.
		\param pos 현재 Position
		\return 전 노드의 Data
		\~english
		\~
		*/
		inline E& GetPrev(Proud::Position& pos)
		{
			CNode* pNode;

			assert(pos != NULL);
			pNode = (CNode*)pos;
			pos = Proud::Position(pNode->m_pPrev);

			return(pNode->m_element);
		}

		/**
		\~korean
		인자로 넣은 Position의 전 node Position을 얻는다.
		\param pos 현재 Position
		\return 전 노드의 Data를 const 변수로 리턴한다.
		\~english
		\~
		*/
		inline const E& GetPrev(Proud::Position& pos) const throw()
		{
			CNode* pNode;

			assert(pos != NULL);
			pNode = (CNode*)pos;
			pos = Proud::Position(pNode->m_pPrev);

			return(pNode->m_element);
		}

		/**
		\~korean
		현재 노드의 Data 를 얻는다.
		\param pos 현재 노드의 Position
		\return 현재 노드의 Data
		\~english
		\~
		*/
		inline E& GetAt(Proud::Position pos)
		{
			assert(pos != NULL);
			CNode* pNode = (CNode*)pos;
			return(pNode->m_element);
		}

		/**
		\~korean
		현재 노드의 Data 를 const 변수로 얻는다.
		\param pos 현재 노드의 Position
		\return 현재 노드의 Data의 const 변수 리턴
		\~english
		\~
		*/
		inline const E& GetAt(Proud::Position pos) const
		{
			assert(pos != NULL);
			CNode* pNode = (CNode*)pos;
			return(pNode->m_element);
		}

		/**
		\~korean
		현재 Position이 가리키는 노드의 Data를 세팅한다.
		\param pos 현재 노드의 Position
		\param element 세팅할 data
		\~english
		\~
		*/
		inline void SetAt(Proud::Position pos, INARGTYPE element)
		{
			assert(pos != NULL);
			CNode* pNode = (CNode*)pos;
			pNode->m_element = element;
		}

		/**
		\~korean
		현재 Position이 가르키는 node 를 제거하고 다음 Position을 리턴해 준다.
		\param pos 제거하고자 하는 node의 Position
		\return 제거된 Position의 다음 Position
		\~english
		\~
		*/
		inline Proud::Position RemoveAt(Proud::Position pos) throw()
		{
			Proud::Position newPos = pos;

			GetNext(newPos);  // 다음 위치를 미리 얻어둔다.
			assert(pos != NULL);

			CNode* pOldNode = (CNode*)pos;

			// remove pOldNode from list
			if (pOldNode == m_pHead)
			{
				m_pHead = pOldNode->m_pNext;
			}
			else
			{
#if defined(_WIN32)
				//assert( AtlIsValidAddress( pOldNode->m_pPrev, sizeof(CNode) ));
#endif
				pOldNode->m_pPrev->m_pNext = pOldNode->m_pNext;
			}
			if (pOldNode == m_pTail)
			{
				m_pTail = pOldNode->m_pPrev;
			}
			else
			{
#if defined(_WIN32)
				//assert( AtlIsValidAddress( pOldNode->m_pNext, sizeof(CNode) ));
#endif
				pOldNode->m_pNext->m_pPrev = pOldNode->m_pPrev;
			}
			NodeToPool(pOldNode);

			return newPos;
		}

		/**
		\~korean
		현재 Position이 가리키는 node의 앞에 새로운 node를 추가한다.
		\param pos 현재 node를 가리키는 Position
		\param element 추가할 새 노드의 Data
		\~english
		\~
		*/
		inline Proud::Position InsertBefore(Proud::Position pos, INARGTYPE element)
		{
#if defined(_WIN32)
			//ATLASSERT_VALID(this);
#endif
			if (pos == NULL)
				return AddHead(element); // insert before nothing -> head of the list

			// Insert it before position
			CNode* pOldNode = (CNode*)pos;
			CNode* pNewNode = NewOrRecycleNode(element, pOldNode->m_pPrev, pOldNode);

			if (pOldNode->m_pPrev != NULL)
			{
				assert(pOldNode->m_pPrev);
				pOldNode->m_pPrev->m_pNext = pNewNode;
			}
			else
			{
				assert(pOldNode == m_pHead);
				m_pHead = pNewNode;
			}
			pOldNode->m_pPrev = pNewNode;

			return(Proud::Position(pNewNode));
		}

		/**
		\~korean
		현재 Position이 가리키는 node의 뒤에 새로운 node를 추가한다.
		\param pos 현재 node를 가리키는 Position
		\param element 추가할 새 노드의 Data
		\~english
		\~
		*/
		inline Proud::Position InsertAfter(Proud::Position pos, INARGTYPE element)
		{
			assert(this != nullptr);

			if (pos == NULL)
				return AddTail(element); // insert after nothing -> tail of the list

			// Insert it after position
			CNode* pOldNode = (CNode*)pos;
			CNode* pNewNode = NewOrRecycleNode(element, pOldNode, pOldNode->m_pNext);

			if (pOldNode->m_pNext != NULL)
			{
				assert(AtlIsValidAddress(pOldNode->m_pNext, sizeof(CNode)));
				pOldNode->m_pNext->m_pPrev = pNewNode;
			}
			else
			{
				assert(pOldNode == m_pTail);
				m_pTail = pNewNode;
			}
			pOldNode->m_pNext = pNewNode;

			return(Proud::Position(pNewNode));
		}

		/**
		\~korean
		Data로 node를 찾는다.
		\param element 찾을 node의 data
		\param posStartAfter 이 Position이후부터 비교하여 찾는다.
		\~english
		\~
		*/
		inline Proud::Position Find(INARGTYPE element, Proud::Position posStartAfter = NULL) const throw()
		{
			assert(this != nullptr);

			CNode* pNode = (CNode*)posStartAfter;
			if (pNode == NULL)
			{
				pNode = m_pHead;  // start at head
			}
			else
			{
#ifdef _WIN32
				assert(AtlIsValidAddress(pNode, sizeof(CNode)));
#endif
				pNode = pNode->m_pNext;  // start after the one specified
			}

			for (; pNode != NULL; pNode = pNode->m_pNext)
			{
				if (ETraits::CompareElements(pNode->m_element, element))
					return(Proud::Position(pNode));
			}

			return(NULL);
		}

		/**
		\~korean
		헤더로 부터 iElement갯수 만큼의 다음 node의 Position을 리턴
		\param iElement 다음으로 넘어갈 노드의 갯수
		\return node의 Position
		\~english
		\~
		*/
		inline Proud::Position FindIndex(INDEXTYPE iElement) const throw()
		{
			assert(this != nullptr);

			if (iElement >= m_nElements)
				return NULL;  // went too far

			if (m_pHead == NULL)
				return NULL;

			CNode* pNode = m_pHead;
			for (INDEXTYPE iSearch = 0; iSearch < iElement; iSearch++)
			{
				pNode = pNode->m_pNext;
			}

			return(Proud::Position(pNode));
		}

		/**
		\~korean
		현재 Position이 가리키는 node를 list의 처음으로 보낸다.
		\param pos 현재 node의 Position
		\~english
		\~
		*/
		inline void MoveToHead(Proud::Position pos)
		{
			assert(pos != NULL);

			CNode* pNode = static_cast< CNode* >(pos);

			if (pNode == m_pHead)
			{
				// Already at the head
				return;
			}

			if (pNode->m_pNext == NULL)
			{
				assert(pNode == m_pTail);
				m_pTail = pNode->m_pPrev;
			}
			else
			{
				pNode->m_pNext->m_pPrev = pNode->m_pPrev;
			}

			assert(pNode->m_pPrev != NULL);  // This node can't be the head, since we already checked that case
			pNode->m_pPrev->m_pNext = pNode->m_pNext;

			m_pHead->m_pPrev = pNode;
			pNode->m_pNext = m_pHead;
			pNode->m_pPrev = NULL;
			m_pHead = pNode;
		}

		/**
		\~korean
		현재 Position이 가리키는 node를 list의 마지막으로 보낸다.
		\param pos 현재 node의 Position
		\~english
		\~
		*/
		inline void MoveToTail(Proud::Position pos)
		{
			assert(pos != NULL);
			CNode* pNode = static_cast< CNode* >(pos);

			if (pNode == m_pTail)
			{
				// Already at the tail
				return;
			}

			if (pNode->m_pPrev == NULL)
			{
				assert(pNode == m_pHead);
				m_pHead = pNode->m_pNext;
			}
			else
			{
				pNode->m_pPrev->m_pNext = pNode->m_pNext;
			}

			pNode->m_pNext->m_pPrev = pNode->m_pPrev;

			m_pTail->m_pNext = pNode;
			pNode->m_pPrev = m_pTail;
			pNode->m_pNext = NULL;
			m_pTail = pNode;
		}

		// itemPos가 가리키는 항목을 dest의 맨 끝으로 옮긴다.
		// 내부적으로 move semantics를 쓰므로 shared_ptr을 쓸 경우 속도 이익.
		void MoveToOtherListTail(Proud::Position itemPos, CFastList2& dest)
		{
			E& c1 = GetAt(itemPos);
			Position p2 = dest.AddTail(); // 빈 항목
			E& c2 = dest.GetAt(p2);
			c2 = MOVE_OR_COPY(c1); // move 연산. 이제 c1은 비어있다.
			RemoveAt(itemPos);
		}

		/**
		\~korean
		두 노드의 위치를 바꾼다. Elenemt를 직접 Swap해주게되면 큰 낭비가 있을 수 있음으로 두 노드에 해당 포인터 들을 바꾸어준다.
		\param pos1 바꿀 첫번째 node의 Position
		\param pos1 바꿀 두번째 node의 Position
		\~english
		\~
		*/
		void SwapElements(Proud::Position pos1, Proud::Position pos2) throw()
		{
			assert(pos1 != NULL);
			assert(pos2 != NULL);

			if (pos1 == pos2)
			{
				// Nothing to do
				return;
			}

			CNode* pNode1 = static_cast< CNode* >(pos1);
			CNode* pNode2 = static_cast< CNode* >(pos2);
			if (pNode2->m_pNext == pNode1)
			{
				// Swap pNode2 and pNode1 so that the next case works
				CNode* pNodeTemp = pNode1;
				pNode1 = pNode2;
				pNode2 = pNodeTemp;
			}
			if (pNode1->m_pNext == pNode2)
			{
				// Node1 and Node2 are adjacent
				pNode2->m_pPrev = pNode1->m_pPrev;
				if (pNode1->m_pPrev != NULL)
				{
					pNode1->m_pPrev->m_pNext = pNode2;
				}
				else
				{
					assert(m_pHead == pNode1);
					m_pHead = pNode2;
				}
				pNode1->m_pNext = pNode2->m_pNext;
				if (pNode2->m_pNext != NULL)
				{
					pNode2->m_pNext->m_pPrev = pNode1;
				}
				else
				{
					assert(m_pTail == pNode2);
					m_pTail = pNode1;
				}
				pNode2->m_pNext = pNode1;
				pNode1->m_pPrev = pNode2;
			}
			else
			{
				// The two nodes are not adjacent
				CNode* pNodeTemp;

				pNodeTemp = pNode1->m_pPrev;
				pNode1->m_pPrev = pNode2->m_pPrev;
				pNode2->m_pPrev = pNodeTemp;

				pNodeTemp = pNode1->m_pNext;
				pNode1->m_pNext = pNode2->m_pNext;
				pNode2->m_pNext = pNodeTemp;

				if (pNode1->m_pNext != NULL)
				{
					pNode1->m_pNext->m_pPrev = pNode1;
				}
				else
				{
					assert(m_pTail == pNode2);
					m_pTail = pNode1;
				}
				if (pNode1->m_pPrev != NULL)
				{
					pNode1->m_pPrev->m_pNext = pNode1;
				}
				else
				{
					assert(m_pHead == pNode2);
					m_pHead = pNode1;
				}
				if (pNode2->m_pNext != NULL)
				{
					pNode2->m_pNext->m_pPrev = pNode2;
				}
				else
				{
					assert(m_pTail == pNode1);
					m_pTail = pNode2;
				}
				if (pNode2->m_pPrev != NULL)
				{
					pNode2->m_pPrev->m_pNext = pNode2;
				}
				else
				{
					assert(m_pHead == pNode1);
					m_pHead = pNode2;
				}
			}
		}

#ifdef _DEBUG
		void AssertValid() const
		{
			if (IsEmpty())
			{
				// empty list
				assert(m_pHead == NULL);
				assert(m_pTail == NULL);
			}
			else
			{
				// 메모리를 긁는지 시험한다.
				CNode* node = m_pHead;
				while (node != NULL)
				{
					node = node->m_pNext;
				}
				// non-empty list
				//		assert(AtlIsValidAddress(m_pHead, sizeof(CNode)));
				//		assert(AtlIsValidAddress(m_pTail, sizeof(CNode)));
			}
		}

#endif  // _DEBUG

		// Implementation
	private:
		CNode* m_pHead;
		CNode* m_pTail;
		INDEXTYPE m_nElements; // 몇개를 갖고 있나

		/* object pool.
		단, CNode.m_element는 pooling되지 않음.
		이 안에 있을때는 CNode.m_pNext만 유효함. */
		CNode* m_freeList;
	private:
		inline CNode* NewOrRecycleNode(CNode* pPrev, CNode* pNext)
		{
			CNode* ret;
			// 재활용 가능한게 있으면 그걸 꺼내고 element의 ctor를 호출한다.
			if (m_freeList != NULL)
			{
				ret = m_freeList;
				m_freeList = m_freeList->m_pNext;

			}
			else
			{
				ret = (CNode*)CProcHeap::Alloc(sizeof(CNode));
				if (ret == NULL)
					throw std::bad_alloc();
			}

			// 이제 ctor를 콜한다.
			CallConstructor<CNode>(ret);

			// 나머지 변수 채우기
			ret->m_pPrev = pPrev;
			ret->m_pNext = pNext;

			m_nElements++;

			return ret;
		}

		inline CNode* NewOrRecycleNode(INARGTYPE element, CNode* pPrev, CNode* pNext)
		{
			CNode* ret;
			// 재활용 가능한게 있으면 그걸 꺼내고 element의 ctor를 호출한다.
			if (m_freeList != NULL)
			{
				ret = m_freeList;
				m_freeList = m_freeList->m_pNext;
			}
			else
			{
				ret = (CNode*)CProcHeap::Alloc(sizeof(CNode));
				if (ret == NULL)
					throw std::bad_alloc();
			}

			// 이제 ctor를 콜한다.
			CallConstructor<CNode>(ret, element);

			// 나머지 변수 채우기
			ret->m_pPrev = pPrev;
			ret->m_pNext = pNext;

			m_nElements++;

			return ret;
		}

		inline void NodeToPool(CNode* pNode) throw()
		{
			// 일단 element의 파괴자를 콜 한다.
			CallDestructor<CNode>(pNode);

			// 이제 node의 pool로 옮긴다.
			pNode->m_pNext = m_freeList;
			m_freeList = pNode;

			m_nElements--;
		}

	public:
		inline ~CFastList2() throw()
		{
			RemoveAll();
			assert(m_nElements == 0);

			// node pool도 청소한다.
			CNode* r = m_freeList;
			while (r != NULL)
			{
				CNode* del = r;
				r = r->m_pNext;
				CProcHeap::Free(del);
			}
		}

	private:
		// 이렇게 덩치 큰 객체는 묵시적 복사 즐
		CFastList2(const CFastList2&) throw();
		CFastList2& operator=(const CFastList2&) throw();

	public:

		class value_type
		{
		public:
			Proud::Position m_pos;
			CFastList2* m_owner;
		};

		class iterator;
		friend class iterator;

#ifndef PROUDNET_NO_CONST_ITERATOR
		class const_iterator;
		friend class const_iterator;

		/**
		\~korean
		STL의 const_iterator와 같은 역할을 한다.
		\~english
		Performs a role as const_iterator of STL
		\~
		*/
		class const_iterator :public value_type
		{
		public:
			inline const_iterator() {}
			inline const_iterator(const iterator& src) :value_type(src) {}

			inline const_iterator& operator++()
			{
				// preincrement
				value_type::m_owner->GetNext(value_type::m_pos);
				return (*this);
			}

			inline const_iterator operator++(int)
			{
				// postincrement
				const_iterator _Tmp = *this;
				++*this;
				return (_Tmp);
			}

			inline bool operator==(const const_iterator& a) const
			{
				return value_type::m_pos == a.m_pos && value_type::m_owner == a.m_owner;
			}

			inline bool operator!=(const const_iterator& a) const
			{
				return !(value_type::m_pos == a.m_pos && value_type::m_owner == a.m_owner);
			}

			inline const E& operator*() const
			{
				return value_type::m_owner->GetAt(value_type::m_pos);
			}
		};
#endif
		/**
		\~korean
		STL의 iterator와 같은 역할을 한다.
		\~english
		Performs a role as iterator of STL
		\~
		*/
		class iterator :public value_type
		{
		public:
			inline iterator() {}
			inline iterator(const const_iterator& src) :value_type(src) {}

			inline bool operator==(const iterator& a) const
			{
				return value_type::m_pos == a.m_pos && value_type::m_owner == a.m_owner;
			}

			inline bool operator!=(const iterator& a) const
			{
				return !(value_type::m_pos == a.m_pos && value_type::m_owner == a.m_owner);
			}

			inline iterator& operator++()
			{
				// preincrement
				value_type::m_owner->GetNext(value_type::m_pos);
				return (*this);
			}

			inline iterator operator++(int)
			{
				// postincrement
				iterator _Tmp = *this;
				++*this;
				return (_Tmp);
			}

			inline E& operator*() const
			{
				return value_type::m_owner->GetAt(value_type::m_pos);
			}
		};

		/**
		\~korean
		STL의 동명 메서드와 같은 역할을 한다.
		\~english
		Performs a role as same name method of STL
		\~
		*/
		inline iterator begin()
		{
			iterator ret;
			ret.m_pos = GetHeadPosition();
			ret.m_owner = this;

			return ret;
		}

		/**
		\~korean
		STL의 동명 메서드와 같은 역할을 한다.
		\~english
		Performs a role as same name method of STL
		\~
		*/
		inline iterator end()
		{
			iterator ret;
			ret.m_pos = NULL;
			ret.m_owner = this;

			return ret;
		}

#ifndef PROUDNET_NO_CONST_ITERATOR
		/**
		\~korean
		STL의 동명 메서드와 같은 역할을 한다.
		\~english
		Performs a role as same name method of STL
		\~
		*/
		inline const_iterator begin() const
		{
			const_iterator ret;
			ret.m_pos = GetHeadPosition();
			ret.m_owner = (CFastList2*)this;

			return ret;
		}

		/**
		\~korean
		STL의 동명 메서드와 같은 역할을 한다.
		\~english
		Performs a role as same name method of STL
		\~
		*/
		inline const_iterator end() const
		{
			const_iterator ret;
			ret.m_pos = NULL;
			ret.m_owner = (CFastList2*)this;

			return ret;
		}
#endif

		/**
		\~korean
		STL의 동명 메서드와 같은 역할을 한다.
		\~english
		Performs a role as same name method of STL
		\~
		*/
		inline iterator erase(iterator iter)
		{
			if (iter.m_owner != this)
			{
				ThrowInvalidArgumentException();
			}
			iterator ret = iter;
			++ret;
			RemoveAt(iter.m_pos);

			return ret;
		}

		/**
		\~korean
		현재 CFastList2와 다른 CFastList2의 내용이 같은지 비교한다. Data를 직접 비교하여 확인한다..
		\param rhs 비교할 CFastList2
		\return 같으면 true 다른면 false 리턴
		\~english
		\~
		*/
		inline bool Equals(const CFastList2& rhs) const
		{
			if (rhs.GetCount() != GetCount())
				return false;

			Proud::Position i1 = GetHeadPosition();
			Proud::Position i2 = rhs.GetHeadPosition();
			while (i1 != NULL && i2 != NULL)
			{
				if (GetAt(i1) != rhs.GetAt(i2))
					return false;
				GetNext(i1);
				rhs.GetNext(i2);
			}

			return true;
		}

		// called by object pool classes.
		void OnDrop()
		{
			Clear(); // list는 비워지지만 list를 내포하던 memory block들은 this의 free list에 모인다. 그리고 추후 재사용된다.
		}
		void OnRecycle() {}
	};
}
