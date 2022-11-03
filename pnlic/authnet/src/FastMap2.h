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

#include "../include/BasicTypes.h"
#include "../include/sysutil.h"
#include "../include/FastArray.h"
#include "../include/Enums.h"

//#pragma pack(push,8)

#ifdef _MSC_VER
#pragma warning(disable:4290)
#endif

namespace Proud
{
	/** \addtogroup util_group
	*  @{
	*/

	/* CFastMap과 사용법이 같으나 속도가 훨씬 빠름.
	내부적으로 node 객체들을 obj-pool하여 heap 접근 횟수를 줄임.
	기술 유출 방지를 위해 src 폴더에 존재하는 클래스. 유저에게는 비노출. */
	template < typename K, typename V, typename INDEXTYPE, 
		typename KTraits = CPNElementTraits< K >, typename VTraits = CPNElementTraits< V > >
	class CFastMap2
	{
	public:
#ifndef PROUDNET_NO_CONST_ITERATOR
		class const_iterator;
		friend class const_iterator;
#endif
		typedef typename KTraits::INARGTYPE KINARGTYPE;
		typedef typename KTraits::OUTARGTYPE KOUTARGTYPE;
		typedef typename VTraits::INARGTYPE VINARGTYPE;
		typedef typename VTraits::OUTARGTYPE VOUTARGTYPE;
		typedef typename CFastMap2<K,V,INDEXTYPE,KTraits,VTraits>::const_iterator ConstIterType;
	public:
		class CPair : public __Position
		{
		protected:
			inline CPair(KINARGTYPE key)
				: m_key( key ) 
			{
			}
		private:
			inline const CPair& operator =(const CPair&);

		public:
			K m_key;
			V m_value;
		};

	private:
		class CNode :
			public CPair
		{
		public:
			inline CNode( KINARGTYPE key ) 
				: CPair( key )
			{
			}
			inline ~CNode()
			{
			}
		private:
			inline const CNode& operator =(const CNode&);

		public:

			// bin내의 마지막 node인 경우 next는 다른 bin의 head node를 가리킨다.  next=NULL인 경우 이 node는 last bin의 last node이다.
			// prev는 next의 반대 역할을 한다. prev = NULL인 경우, 이 node는 head bin의 head node이다
			// next는 이 객체가 obj-pooled인 상태일 때 다음 free node를 가리킨다.
			CNode *m_pNext, *m_pPrev;

			// 이 node의 hashed key 값. rehash를 할 때 과도한 hash 함수 사용을 절약하기 위함.
			uint32_t m_nHash;

			// 이 노드가 몇 번째 bin에 들어가 있는가.
			// rehash를 할 때마다 바뀜. 그 외에는 안 바뀜.
			uint32_t m_nBin;
		};
	private:
		bool m_enableSlowConsistCheck;
		
		// hash table. 배열의 N번째 항목은 hash % bin size가 N인 개체들의 linked list head이다.
		// 할당/해제시 ProudNet DLL시 런타임 오류를 피하기 위해 CFirstHeap을 쓴다.
		CNode** m_ppBins;
		
		// 각 bin의 head node는 다른 bin의 tail node이다. 이 변수는 head bin, 즉 다른 bin의 tail node의 다음이 아닌
		// 노드이다. 즉 iteration의 최초 대상임을 의미한다.
		CNode* m_pHeadBinHead;
		
		// 가장 마지막에 들어있는 항목
		CNode* m_pTailBinTail;
		
		// 이 map에 있는 총 항목의 갯수
		INDEXTYPE m_nElements;
		
		// hash table 크기
		uint32_t m_nBins;
		
		float m_fOptimalLoad;
		float m_fLoThreshold;
		float m_fHiThreshold;
		INDEXTYPE m_nHiRehashThreshold;
		INDEXTYPE m_nLoRehashThreshold;
		uint32_t m_nLockCount;
		
		// obj-pool. 잦은 할당/해제를 절약하기 위함
		CNode* m_freeList;
	private:
		void InitVars( uint32_t nBins, float fOptimalLoad, float fLoThreshold, float fHiThreshold )
		{
			m_ppBins= NULL;
			m_pHeadBinHead=NULL;
			m_pTailBinTail = NULL;
			m_nBins=nBins ;
			m_nElements= 0 ;
			m_nLockCount=0;  // Start unlocked
			m_fOptimalLoad= fOptimalLoad ;
			m_fLoThreshold= fLoThreshold ;
			m_fHiThreshold= fHiThreshold ;
			m_nHiRehashThreshold= UINT_MAX ;
			m_nLoRehashThreshold= 0 ;
			m_freeList = NULL;
			
			SetOptimalLoad( fOptimalLoad, fLoThreshold, fHiThreshold, false );
		}

	public:
		/** 
		\~korean
		생성자입니다. 
		
		\param nBins 기본 해시 테이블의 크기입니다. 솟수로 설정해야 제 성능을 냅니다. 상세한 내용은 \ref hash_map 에 있습니다.
		\param fOptimalLoad 최적 부하 비율입니다. 상세한 내용은 \ref hash_map_load 에 있습니다.
		\param fLoThreshold 최소 부하 비율입니다. 상세한 내용은 \ref hash_map_load 에 있습니다.
		\param fHiThreshold 최대 부하 비율입니다. 상세한 내용은 \ref hash_map_load 에 있습니다.
		\~english
		This is constructor 
		
		\param nBins = The size of base hash table. Performs ok when set with prime number. Please refer \ref hash_map for further detail.
		\param fOptimalLoad = Optimized load proportion. Please refer \ref hash_map for further detail.
		\param fLoThreshold = Minimum load proportion. Please refer \ref hash_map for further detail.
		\param fHiThreshold = Maximum load proportion. Please refer \ref hash_map for further detail.
		\~
		*/
		CFastMap2( uint32_t nBins = 17, float fOptimalLoad = 0.75f, float fLoThreshold = 0.25f, float fHiThreshold = 2.25f)
		{
			m_enableSlowConsistCheck = false;

			assert( nBins > 0 );

			InitVars(nBins, fOptimalLoad, fLoThreshold, fHiThreshold);
		}

#if defined(_MSC_VER)        
		/// \~korean 이미 갖고 있는 항목의 갯수를 구한다. \~english Gets number of item that already owned \~
		__declspec(property(get = GetCount)) INDEXTYPE Count;
#endif
		
		inline INDEXTYPE GetCount() const throw()
		{
			return( m_nElements );
		}

		/// \~korean 이미 갖고 있는 항목의 갯수를 구한다. \~english Gets number of item that already owned \~
		inline INDEXTYPE size() const 
		{
			return GetCount();
		}

		/// \~korean 텅빈 상태인가? \~english Is it empty? \~
		inline bool IsEmpty() const throw()
		{
			return( m_nElements == 0 );
		}

		/** 
		\~korean
		key에 대응하는 value를 얻는다. 
		\param key [in] 찾을 키
		\param value [out] 찾은 키에 대응하는 값이 저장될 곳
		\return key를 찾았으면 true를 리턴한다.
		\~english
		Gets value correspnds to key 
		\param key [in] = key to find
		\param value [out] = a space where the value corresponds to the key found to be stored
		\return = returns true if key is found. 
		\~
		 */
		bool Lookup(KINARGTYPE key, VOUTARGTYPE value) const
		{
			// 빈 것은 빨리 찾아버리자.
			if (m_nElements == 0)
				return false;

			uint32_t iBin;
			uint32_t nHash;
			CNode* pNode;

			pNode = GetNode(key, iBin, nHash);
			if (pNode == NULL)
			{
				return false;
			}

			value = pNode->m_value;

			return true;
		}

		/** 
		\~korean
		key에 대응하는 value를 찾되, CPair 객체를 리턴한다.
		\~english
		Finds value corrsponds to key but returns CPair object
		\~ 
		*/
		const CPair* Lookup(KINARGTYPE key) const throw()
		{
			// 빈 것은 빨리 찾아버리자.
			if (m_nElements == 0)
				return NULL;

			uint32_t iBin;
			uint32_t nHash;
			CNode* pNode;

			pNode = GetNode(key, iBin, nHash);

			return pNode;
		}

		/** 
		\~korean
		key에 대응하는 value를 찾되, CPair 객체를 리턴한다.
		\~english
		Finds value corrspond﻿s to key but returns CPair object
		\~
		*/
		CPair* Lookup( KINARGTYPE key ) throw()
		{
			// 빈 것은 빨리 찾아버리자.
			if(m_nElements == 0)
				return NULL;

			uint32_t iBin;
			uint32_t nHash;
			CNode* pNode;

			pNode = GetNode( key, iBin, nHash );

			return( pNode );
		}
		
		/** 
		\~korean
		key에 대응하는 value 값을 찾는다. 없을 경우 새 entry를 내부적으로 만든다
		\return 이미 있거나 새로 추가된 entry의 reference를 리턴한다.
		\~english
		Finds value corrsponds to key. If not existed then internally creates a new entry.
		\return = returns reference of entry that is eiter alread existing or newly added
		\~
		*/
		V& operator[]( KINARGTYPE key ) throw()
		{
			CNode* pNode;
			uint32_t iBin;
			uint32_t nHash;

			pNode = GetNode( key, /*out */ iBin, /*out */ nHash);
			if ( pNode == NULL )
			{
				pNode = CreateNode( key, iBin, nHash );
			}

			return( pNode->m_value );
		}    
			  
		/** 
		\~korean
		key,value pair를 새로 추가한다. 이미 있으면 오버라이트한다. 
		\param key 추가할 키값
		\param value 추가할 값 객체
		\return 추가를 한 후 추가 위치를 가리키는 포인터. Position이 베이스 클래스이므로 바로 Position을 리턴값으로 간주해도 된다.
		\~english
		Newly addes key and value pair. If already exist then overwrites. 
		\param key = key value to be added
		\param value = value object to be added
		\return = pointer that points additional location after adding. Since Position is a base class, it is possible to regards Position as return value.
		\~
		*/
		CNode* SetAt( KINARGTYPE key, VINARGTYPE value )
		{
			CNode* pNode;
			uint32_t iBin;
			uint32_t nHash;

			pNode = GetNode( key, iBin, nHash );
			if ( pNode == NULL )
			{
				pNode = CreateNode( key, iBin, nHash );
				pNode->m_value = value;
			}
			else
			{
				pNode->m_value = value;
			}

			return pNode;/*( Position( pNode ) ); */
		}

		/** 
		\~korean
		일전에 얻은 Position 객체가 가리키는 곳의 value를 새로 넣는다.
		\param pos value를 넣을 node의 Position
		\param value node에 넣을 data
		\~english
		Newly enters the value of location where previously acquired Position object points.
		\~
		*/
		void SetValueAt( Proud::Position pos, VINARGTYPE value )
		{
			if( pos == NULL )
			{
				assert(0);
			}
			else
			{
				CNode* pNode = static_cast< CNode* >( pos );
				pNode->m_value = value;
			}
		}

		/** 
		\~korean
		\copydoc Remove
		\~english
		\copydoc Remove
		\~
		*/
		bool RemoveKey( KINARGTYPE key,bool rehashOnNeed=false ) throw()
		{
			CNode* pNode;
			uint32_t iBin;
			uint32_t nHash;

			pNode = GetNode( key, iBin, nHash );
			if ( pNode == NULL )
			{
				return( false );
			}

			RemoveNode( pNode,rehashOnNeed );

			return( true );
		}

		/// \~korean 완전히 비운다 \~english Completely empty it \~
		inline void Clear()
		{
			RemoveAll();
		}

		/** 
		\~korean
		key가 가리키는 항목을 찾아 제거한다.
		\param key 제거할 키
		\param rehashOnNeed true인 경우, hash table이 충분히 작아진 경우 hash table을 재조정한다. 
		이때 사용중이던 iterator나 Position 가 있을 경우 이는 무효화됨을 주의해야 한다. 
		\return 찾아서 제거를 했으면 true를 리턴한다. 못찾았으면 false다.
		\~english
		Finds the clause pointed by key then removes it.
		\param key = key to be removed
		\param rehashOnNeed = if ture then hash table is re-modified when hash table became small enough 
					  Must pay attention to the fact that if there exists either iterator and/or Position during the process, it/they will be nullified.
		\return = returns if found and removed. False if failed to find.
		\~
		*/
		inline bool Remove( KINARGTYPE key,bool rehashOnNeed=false ) 
		{
			return RemoveKey(key,rehashOnNeed);
		}

		/// \copydoc Clear
		void RemoveAll()
		{
			DisableAutoRehash();

			AssertConsist();

			// 모든 node들을 제거한다.
			// map이 모든 node들은 bin 종류 막론하고 모두 연결되어 있다.
			CNode* pNode;
			int removeCount = 0;
			for (pNode = m_pHeadBinHead; pNode != NULL; )
			{
				CNode* pNext = pNode->m_pNext; // save
				assert(m_nElements > 0);
				FreeNode(pNode,false);
				pNode = pNext; // restore
				removeCount++;
			}

			assert(m_nElements == 0);

			CProcHeap::Free(m_ppBins);

			m_ppBins = NULL;

			m_nElements = 0;

			m_pHeadBinHead = NULL;

			m_pTailBinTail = NULL;

			AssertConsist();

			if ( !IsLocked() )
			{
				if (!InitHashTable(PickSize(m_nElements), false))
					throw std::bad_alloc();
			}

			AssertConsist();

			EnableAutoRehash();
		}

		/** 
		\~korean
		Position이 가리키는 곳의 key-value pair를 제거한다.
		\param 일전에 얻은 Position 값. 이 값은 유효한 값이어야 한다!
		\param rehashOnNeed true일 때, hash table이 충분히 작아진 경우 hash table을 재조정한다. 
		이때 사용중이던 iterator나 Position 가 있을 경우 이는 무효화됨을 주의해야 한다. 
		\~english
		Removes key-value pair of where pointed by Position
		\param = Position value acquired before. This value must be valid!
		\param rehashOnNeed = if ture then hash table is re-modified when hash table became small enough
		Must pay attention to the fact that if there exists either iterator and/or Position during the process, it/they will be nullified.
		\~ 
		*/
		void RemoveAtPos( Proud::Position pos ,bool rehashOnNeed=false) throw()
		{
			assert( pos != NULL );

			CNode* pNode = static_cast< CNode* >( pos );

			RemoveNode( pNode ,rehashOnNeed);
		}

		/** 
		\~korean
		보유하고 있는 항목 중 맨 앞의 것을 얻는다. 주로 iteration을 위해서 쓰인다.
		GetNext 등을 써서 다음 항목을 iteration해 나갈 수 있다.
		\return 맨앞 항목의 Position
		\~english
		Gets the foremost thing among possessed clauses. Mainly used for iteration.
		It is possible to perform iteration to next clause by using GetNext or others.
		\~
		*/
		inline Proud::Position GetStartPosition() const throw()
		{
			if ( IsEmpty() )
			{
				return( NULL );
			}

			return Proud::Position(m_pHeadBinHead);			
		}

		/** 
		\~korean
		보유하고 있는 항목 중 맨 뒤의 것을 얻는다.주로 reverse_iteration을 위해서 쓰인다.
		GetPrev 등을 써서 다음 항목을 iteration해 나갈 수있다.
		\return 맨 마지막 항목의 Position
		\~english
		Gets the very last thing among possessed clauses. Mainly used for reverse_iteration.
		It is possible to perform iteration to next clause by using GetPrev or others.
		\~
		*/
		inline Proud::Position GetEndPosition() const throw()
		{
			if( IsEmpty() )
			{
				return ( NULL );
			}

			return Proud::Position(m_pTailBinTail);
		}


		/** 
		\~korean
		다음 항목을 얻는다.
		\param pos [in&out] 다음 항목의 Position값
		\param key [out] 다음항목의 key
		\param value [out] 다음 항목의 값
		\~english
		Gets following clause
		\~
		*/
		void GetNextAssoc( Proud::Position& pos, KOUTARGTYPE key, VOUTARGTYPE value ) const
		{
			CNode* pNode;
			CNode* pNext;

			assert( m_ppBins != NULL );
			assert( pos != NULL );

			pNode = (CNode*)pos;
			pNext = pNode != NULL? pNode->m_pNext : NULL; //FindNextNode( pNode );

			pos = Proud::Position( pNext );
			key = pNode->m_key;
			value = pNode->m_value;
		}

		/** 
		\~korean
		다음 항목을 얻는다.
		\param pos 가리키고 있는 node의 다음 Position을 얻어온다.
		\return 다음 node의 const CPair값을 리턴
		\~english
		Gets following clause
		\~
		*/
		const CPair* GetNext( Proud::Position& pos ) const throw()
		{
			CNode* pNode;
			CNode* pNext;

			assert( m_ppBins != NULL );
			assert( pos != NULL );

			pNode = (CNode*)pos;
			pNext = pNode != NULL? pNode->m_pNext : NULL; //FindNextNode( pNode );

			pos = Proud::Position( pNext );

			return( pNode );
		}

		/** 
		\~korean
		다음 항목을 얻는다.
		\param pos pos 가리키고 있는 node의 다음 Position을 얻어온다.
		\return 다음 node의 \ref CPair값을 리턴
		\~english
		Gets following clause
		\~
		*/
		CPair* GetNext( Proud::Position& pos ) throw()
		{
			assert( m_ppBins != NULL );
			assert( pos != NULL );

			CNode* pNode = static_cast< CNode* >( pos );
			CNode* pNext = pNode != NULL? pNode->m_pNext : NULL; //FindNextNode( pNode );

			pos = Proud::Position( pNext );

			return( pNode );
		}

		/** 
		\~korean
		이전 항목을 얻는다. 
		\param pos pos가 가르키고 있는 node의 이전 Position을 얻어온다.
		\return 이전노드의 \ref CPair값
		\~english
		Gets previous clause
		\~
		*/
		CPair* GetPrev( Proud::Position& pos ) throw()
		{
			assert( m_ppBins != NULL );
			assert( pos != NULL );

			CNode* pNode = static_cast< CNode* >( pos );
			CNode* pNext = FindPrevNode( pNode );

			pos = Proud::Position( pNext );

			return( pNode );
		}

		/** 
		\~korean
		다음 항목을 얻는다.
		\param pos 이 pos 가리키고 있는 node의 다음 Position을 얻어온다.
		\return 다음 node의 key값
		\~english
		Gets following clause
		\~
		*/
		const K& GetNextKey( Proud::Position& pos ) const
		{
			CNode* pNode;
			CNode* pNext;

			assert( m_ppBins != NULL );
			assert( pos != NULL );

			pNode = (CNode*)pos;
			pNext = pNode != NULL? pNode->m_pNext : NULL; //FindNextNode( pNode );

			pos = Proud::Position( pNext );

			return( pNode->m_key );
		}

		/** 
		\~korean
		다음 항목을 얻는다.
		\param pos 이 pos 가리키고 있는 node의 다음 Position을 얻어온다.
		\return 다음 node의 const value값
		\~english
		Gets following clause
		\~
		*/
		const V& GetNextValue( Proud::Position& pos ) const
		{
			CNode* pNode;
			CNode* pNext;

			assert( m_ppBins != NULL );
			assert( pos != NULL );

			pNode = (CNode*)pos;
			pNext = pNode != NULL? pNode->m_pNext : NULL; //FindNextNode( pNode );

			pos = Proud::Position( pNext );

			return( pNode->m_value );
		}

		/** 
		\~korean
		다음 항목을 얻는다.
		\param pos 이 pos 가리키고 있는 node의 다음 Position을 얻어온다.
		\return 다음 node의 value값
		\~english
		Gets following clause
		\~
		*/
		V& GetNextValue( Proud::Position& pos )
		{
			CNode* pNode;
			CNode* pNext;

			assert( m_ppBins != NULL );
			assert( pos != NULL );

			pNode = (CNode*)pos;
			pNext = pNode != NULL? pNode->m_pNext : NULL; //FindNextNode( pNode );

			pos = Proud::Position( pNext );

			return( pNode->m_value );
		}

		/** 
		\~korean
		Position이 가리키고 있는 곳의 key와 value를 얻는다.
		\param pos 노드를 가리키는 Position
		\param key 해당노드의 key을 얻어옴
		\param value 해당 노드의 data를 얻어옴
		\~english
		Gets key and value of where Position points
		\~
		*/
		inline void GetAt( Proud::Position pos, KOUTARGTYPE key, VOUTARGTYPE value ) const
		{
			assert( pos != NULL );

			CNode* pNode = static_cast< CNode* >( pos );

			key = pNode->m_key;
			value = pNode->m_value;
		}

		/** 
		\~korean
		해당 index에 있는 곳의 key와 value를 얻는다.
		\param index 첫번째 정보로 부터 이 index만큼 다음노드로 이동한다.
		\return 찾은 노드의 CPair
		\~english
		Gets key and value of where index locates
		\~
		*/
		inline CPair* GetPairByIndex(int index) throw()
		{
			assert( index < (int)GetCount() );

			CNode* node = m_pHeadBinHead;

			for (int i = 0; i < index; ++i)
				node = node->m_pNext;

			return( static_cast< CPair* >( node ) );
		}      

		/** 
		\~korean
		해당 index에 있는 곳의 key와 value를 얻는다.
		\param index 첫번째 정보로 부터 이 index만큼 다음노드로 이동한다.
		\return 찾은 노드의 const CPair
		\~english
		Gets key and value of where index locates
		\~
		*/
		inline const CPair* GetPairByIndex(int index) const throw()
		{
			assert( index < (int)GetCount() );

			CNode* node = m_pHeadBinHead;

			for (int i = 0; i < index; ++i)
				node = node->m_pNext;

			return( static_cast< CPair* >( node ) );
		}      

		/** 
		\~korean
		Position이 가리키고 있는 곳의 key와 value를 얻는다.
		\param pos Position정보
		\return Position에 해당하는 CPair
		\~english
		Gets key and value of where Position points
		\~
		*/
		inline CPair* GetAt( Proud::Position pos ) throw()
		{
			assert( pos != NULL );

			return( static_cast< CPair* >( pos ) );
		}

		/** 
		\~korean
		Position이 가리키고 있는 곳의 key와 value를 얻는다.
		\param pos Position정보
		\return Position에 해당하는 const CPair
		\~english
		Gets key and value of where Position points
		\~
		 */
		inline const CPair* GetAt( Proud::Position pos ) const throw()
		{
			assert( pos != NULL );

			return( static_cast< const CPair* >( pos ) );
		}

		/** 
		\~korean
		Position이 가리키고 있는 곳의 key와 value를 얻는다.
		\param pos node를 가리키는 Position
		\return Position이 가리키는 node의 key
		\~english
		Gets key and value of where Position points
		\~
		*/
		inline const K& GetKeyAt( Proud::Position pos ) const
		{
			assert( pos != NULL );

			CNode* pNode = (CNode*)pos;

			return( pNode->m_key );
		}

		/** 
		\~korean
		Position이 가리키고 있는 곳의 key와 value를 얻는다.
		\param pos node를 가리키는 Position
		\return Position이 가리키는 node의 const data
		\~english
		Gets key and value of where Position points
		\~
		*/
		inline const V& GetValueAt( Proud::Position pos ) const
		{
			assert( pos != NULL );

			CNode* pNode = (CNode*)pos;

			return( pNode->m_value );
		}

		/** 
		\~korean
		Position이 가리키고 있는 곳의 key와 value를 얻는다.
		\param pos node를 가리키는 Position
		\return Position이 가리키는 node의 data
		\~english
		Gets key and value of where Position points
		\~
		*/
		inline V& GetValueAt( Proud::Position pos )
		{
			assert( pos != NULL );

			CNode* pNode = (CNode*)pos;

			return( pNode->m_value );
		}
				
		/// \~korean hash 테이블의 크기를 얻습니다. \~english \~
		uint32_t GetHashTableSize() const throw()
		{
			return( m_nBins );
		}

		/**
		\~korean 
		hash 테이블을 초기화 합니다.
		- 노드 생성시 자동으로 호출됩니다.
		\param nBins 헤쉬 사이즈
		\param bAllocNow 헤쉬 메모리 생성 여부 설정
		\return hash 초기화에 성공하면 true, 실패하면 false 리턴
		\~english
		\~
		*/
		bool InitHashTable( uint32_t nBins, bool bAllocNow = true )
		{
			assert( m_nElements == 0 );
			assert( nBins > 0 );

			if ( m_ppBins != NULL )
			{
				CProcHeap::Free(m_ppBins);
				m_ppBins = NULL;
			}

			if ( bAllocNow )
			{
				m_ppBins = (CNode**)CProcHeap::Alloc(sizeof(CNode*) * nBins);
				if ( m_ppBins == NULL )
				{
					return false;
				}
				memset( m_ppBins, 0, sizeof( CNode* )*nBins );
			}
			m_nBins = nBins;

			UpdateRehashThresholds();

			return true;
		}

		/// \~korean 자동으로 Rehash를 합니다. \~english \~
		void EnableAutoRehash() throw()
		{
			assert( m_nLockCount > 0 );
			m_nLockCount--;
		}

		/// \~korean 자동으로 Rehash를 하지 않습니다. \~english \~
		void DisableAutoRehash() throw()
		{
			m_nLockCount++;
		}

		/**
		\~korean
		hash 테이블을 다시 생성한다.
		\param nBins hash테이블의 크기
		\~english
		\~
		*/
		void Rehash( uint32_t nBins = 0 )
		{
			CNode** ppBins = NULL;

			if ( nBins == 0 )
			{
				nBins = PickSize( m_nElements );
			}

			if ( nBins == m_nBins )
			{
				return;
			}

			// NTTNTRACE(atlTraceMap, 2, "Rehash: %u bins\n", nBins );

			if ( m_ppBins == NULL )
			{
				// Just set the new number of bins
				if (!InitHashTable(nBins, false))
					throw std::bad_alloc();

				return;
			}

			// 새 hash table을 준비한다. 여기에 구 hash table이 옮겨질 것이다.
			ppBins = (CNode**)CProcHeap::Alloc(sizeof(CNode*) * nBins);
			if (ppBins == NULL)
				throw std::bad_alloc();

			memset( ppBins, 0, nBins*sizeof( CNode* ) );

			// 각 항목의 hash 값을 다시 얻어서 구 테이블로부터 새 테이블로 옮긴다. 이때 linked list 관계도 재설정한다.
			CNode* pHeadBinHead = NULL;
			CNode* pTailBinTali = NULL;
			int reAddCount = 0;
			for(CNode* pNode = m_pHeadBinHead; pNode != NULL; )
			{
				uint32_t iDestBin;

				CNode* pOldNext = pNode->m_pNext;

				iDestBin = pNode->m_nHash % nBins; // 새로 hash 값을 얻는다. 그리고 head로서 넣도록 한다.
				pNode->m_nBin = iDestBin;

				CNode* pOldBinHead = ppBins[iDestBin];

				// 완전 최초의 추가인 경우
				if(reAddCount == 0)
				{
					assert(pHeadBinHead == NULL);
					assert(pOldBinHead == NULL);
					pHeadBinHead = pNode;
					pTailBinTali = pNode;
					pNode->m_pPrev = NULL;
					pNode->m_pNext = NULL;

					ppBins[iDestBin] = pNode;

					reAddCount++; // 마무리
					//AssertConsist();
				}
				else
				{
					//AssertConsist();

					if(pOldBinHead != NULL)
					{
						// 들어있는 bin에 추가하는 경우 bin head로서 추가한다.
						//AssertConsist();

						if(pOldBinHead->m_pPrev != NULL)
						{
							pOldBinHead->m_pPrev->m_pNext = pNode;
						}
						else
						{
							// head bin이다. head bin ptr도 교체 필수
							pHeadBinHead = pNode; 
						}

						pNode->m_pPrev = pOldBinHead->m_pPrev; // pOldBinHead->m_pPrev = NULL OK
						pNode->m_pNext = pOldBinHead;
						pOldBinHead->m_pPrev = pNode;
						assert(pOldBinHead == ppBins[iDestBin]);
						ppBins[iDestBin] = pNode;

						reAddCount++; // 마무리

						//AssertConsist();
					}
					else
					{
						// 빈 bin에 추가하는 경우, 최초의 linked bin이 되게 rewire한다.
						//AssertConsist();

						CNode* pOldHeadBin = pHeadBinHead;

						pNode->m_pPrev = NULL;
						pNode->m_pNext = pOldHeadBin;
						if(pNode->m_pNext != NULL)
							pNode->m_pNext->m_pPrev = pNode;
						pHeadBinHead = pNode;
						ppBins[iDestBin] = pNode;

						reAddCount++; // 마무리

						//AssertConsist();
					}

				}

				/*pNode->m_pNext = ppBins[iDestBin];
				pNode->m_pPrev = NULL;
				if(ppBins[iDestBin] != NULL)
					ppBins[iDestBin]->m_pPrev = pNode;

				ppBins[iDestBin] = pNode;
				pHeadBinHead = pNode; */

				pNode = pOldNext;
				//reAddCount++;
			}

			assert(reAddCount == m_nElements);

			// 구 hash table을 제거하고 신 hash table로 교체한다.
			CProcHeap::Free(m_ppBins);
			m_ppBins = ppBins;
			m_nBins = nBins;
			m_pHeadBinHead = pHeadBinHead;
			m_pTailBinTail = pTailBinTali;

			AssertConsist();

			UpdateRehashThresholds();

#ifdef ENABLE_REHASH_COUNT
			InterlockedIncrement(&FastMap_RehashCount);
#endif // ENABLE_REHASH_COUNT
		}

		/** 
		\~korean
		맵의 최적 부하를 재설정합니다. 맵의 최적 부하에 대해서는 \ref hash_map_load 에 있습니다.

		\param fOptimalLoad 최적 부하 비율입니다.
		\param fLoThreshold 부하 비율의 최소 한계입니다.
		\param fHiThreshold 부하 비율의 최대 한계입니다.
		\param bRehashNow true이면 해시테이블 재설정이 즉시 시행됩니다.
		\~english
		Re-sets the optimal load to map. Please refer \ref hash_map_load.

		\param fOptimalLoad = Optimal load proportion
		\param fLoThreshold = Minimum limit of load proportion
		\param fHiThreshold = Maximum limit of load proportion
		\param bRehashNow = hash table re-setting proccess runs immediately if true.
		\~
		*/
		void SetOptimalLoad( float fOptimalLoad, float fLoThreshold, float fHiThreshold,
			bool bRehashNow = false )
		{
			assert( fOptimalLoad > 0 );
			assert( (fLoThreshold >= 0) && (fLoThreshold < fOptimalLoad) );
			assert( fHiThreshold > fOptimalLoad );

			m_fOptimalLoad = fOptimalLoad;
			m_fLoThreshold = fLoThreshold;
			m_fHiThreshold = fHiThreshold;

			UpdateRehashThresholds();

			if ( bRehashNow && ((m_nElements > m_nHiRehashThreshold) ||
				(m_nElements < m_nLoRehashThreshold)) )
			{
				Rehash( PickSize( m_nElements ) );
			}
		}

		/** 
		\~korean
		lookup 최적 성능, 웬만해서는 rehash를 최대한 안하고, 그 대신 메모리를 많이 사용하는 설정.
		증감폭이 워낙 큰데다 rehash cost가 큰 경우에 유용하다.
		\param rehashNow 설정하면서 Rehash를 할것인지를 선택한다. true이면 Rehash를 함
		\~english
		lookup best performance, This setting use more memory rather than doing rehash.
		It is useful when rehash cost is expensive.
		\~
		*/
		void SetOptimalLoad_BestLookup(bool rehashNow = false)
		{
			SetOptimalLoad(0.1f,0.0000001f,2.1f,rehashNow); 
		}


		/// \~korean 각 bin을 뒤져서, 최악의 bin, 즉 가장 많은 item을 가진 bin의 item 갯수를 리턴한다. \~english \~
		INDEXTYPE GetWorstBinItemCount()
		{
			INDEXTYPE worstValue = 0;

			CNode* prevNode = NULL;
			INDEXTYPE itemCountInCurrentBin = 0;
			for(CNode* pNode = m_pHeadBinHead; pNode!=NULL;pNode=pNode->m_pNext)
			{
				if(prevNode != NULL && prevNode->m_nBin == pNode->m_nBin)
				{
					itemCountInCurrentBin++;
				}
				else
					itemCountInCurrentBin = 1;
				worstValue = PNMAX(worstValue, itemCountInCurrentBin);
			}

			return worstValue;
		}


// #ifdef _DEBUG
// 		void AssertValid() const
// 		{
// 			assert( m_nBins > 0 );
// 			// non-empty map should have hash table
// 			assert( IsEmpty() || (m_ppBins != NULL) );
// 		}
// #endif  // _DEBUG

		void EnableSlowConsistCheck()
		{
			m_enableSlowConsistCheck = true;
		}

		/** 
		\~korean
		map 객체의 key들을 모은 배열을 만들어 제공합니다.
		\~english
		Provide array that contain key of map objects
		\~
		 */
		void KeysToArray(CFastArray<K> &output) const
		{
			output.Clear();
			output.SetCount(GetCount());
			int c = 0;
			
			for(ConstIterType i=CFastMap2<K,V,INDEXTYPE,KTraits,VTraits>::begin();i!=CFastMap2<K,V,INDEXTYPE,KTraits,VTraits>::end();i++)
			{
				output[c] = i.first;
				c++;
			}
		}

		/** 
		\~korean
		map 객체의 value들을 모은 배열을 만들어 제공합니다.
		\~english
		Provide array that contain value of map objects
		\~
		 */
		void ValuesToArray(CFastArray<V> &output) const
		{
			output.Clear();
			output.SetCount(GetCount());
			int c = 0;
			for(ConstIterType i=CFastMap2<K,V,INDEXTYPE,KTraits,VTraits>::begin();i!=CFastMap2<K,V,INDEXTYPE,KTraits,VTraits>::end();i++)
			{
				output[c] = i.second;
				c++;
			}
		}

		/** 
		\~korean
		내부 상태가 깨진 것이 없는지 확인한다.
		- 크기가 크더라도 inline으로 해둬야 빈 함수일때 noop가 된다. (regardless of compilers)
		\~english
		Must check if there exitst any cracked internal status
		- Must set this as inline though its size is big in order to make noop when function is empty. (regardless of compilers)
		\~
		 */
		inline void AssertConsist() const
		{
			// UnitTester에서 릴리즈 빌드에서도 작동해야 하므로
			if(m_enableSlowConsistCheck)
			{
				int count=0;

				if( ! (IsEmpty() || (m_ppBins != NULL) ) )
				{
					assert(0);
					throw Exception("CFastMap2 consistency error #0!");
				}

				for(CNode* pNode = m_pHeadBinHead; pNode!=NULL;pNode=pNode->m_pNext)
				{
					if(pNode->m_pNext)
					{
						if(pNode->m_pNext->m_pPrev != pNode)
						{
							assert(0);
							throw Exception("CFastMap2 consistency error #1!");
						}
					}
					if(pNode==m_pHeadBinHead)
					{
						if(pNode->m_pPrev)
						{
							assert(0);
							throw Exception("CFastMap2 consistency error #2!");
						}
					}
					if(pNode==m_pTailBinTail)
					{
						if(pNode->m_pNext)
						{
							assert(0);
							throw Exception("CFastMap2 consistency error #3!");
						}
					}
					count++;
				}

				if(count != (int)m_nElements)
				{
					for(CNode* pNode = m_pHeadBinHead; pNode!=NULL;pNode=pNode->m_pNext)
					{
						NTTNTRACE("Node at Bin #%d\n",pNode->m_nBin);
					}

					assert(0);
					throw Exception("CFastMap2 consistency error #4!");
				}
			}
		}


		// Implementation
	private:
		inline bool IsLocked() const throw()
		{
			return( m_nLockCount != 0 );
		}

		// map에 들어있는 총 항목 갯수에 근거해서 가장 적절한 hash table의 크기값을 얻는다.
		uint32_t PickSize( INDEXTYPE nElements ) const throw()
		{
			// List of primes such that s_anPrimes[i] is the smallest prime greater than 2^(5+i/3)
			static const uint32_t s_anPrimes[] =
			{
				17, 23, 29, 37, 41, 53, 67, 83, 103, 131, 163, 211, 257, 331, 409, 521, 647, 821,
				1031, 1291, 1627, 2053, 2591, 3251, 4099, 5167, 6521, 8209, 10331,
				13007, 16411, 20663, 26017, 32771, 41299, 52021, 65537, 82571, 104033,
				131101, 165161, 208067, 262147, 330287, 416147, 524309, 660563,
				832291, 1048583, 1321139, 1664543, 2097169, 2642257, 3329023, 4194319,
				5284493, 6658049, 8388617, 10568993, 13316089, UINT_MAX
			};

			INDEXTYPE nBins = (INDEXTYPE)(nElements / m_fOptimalLoad);
			uint32_t nBinsEstimate = uint32_t(  UINT_MAX < (uintptr_t)nBins ? UINT_MAX : nBins );
			
			// Find the smallest prime greater than our estimate
			int iPrime = 0;
			while ( nBinsEstimate > s_anPrimes[iPrime] )
			{
				iPrime++;
			}

			if ( s_anPrimes[iPrime] == UINT_MAX )
			{
				return( nBinsEstimate );
			}
			else
			{
				return( s_anPrimes[iPrime] );
			}
		}

		// iBin이 가리키는 bin에 key 값을 가지는 새 node를 추가하도록 한다. 이때 hash 값은 nHash이다.
		CNode* NewNode( KINARGTYPE key, uint32_t iBin, uint32_t nHash )
		{
			//CNode* pNewNode = new CNode( key, nHash );
			CNode* pNewNode;

			// obj-pool에서 꺼낼 것이 있으면 꺼내고 그렇지 않으면 새로 할당한다.
			if(m_freeList != NULL)
			{
				pNewNode = m_freeList;
				m_freeList = m_freeList->m_pNext;
			}
			else
			{
				pNewNode = (CNode*) CProcHeap::Alloc(sizeof(CNode));
				if (pNewNode == NULL)
					throw std::bad_alloc();
			}

			CallConstructor<CNode>(pNewNode, key);
			pNewNode->m_nHash = nHash;
			pNewNode->m_nBin = iBin;

			CNode* pOldBinHead = m_ppBins[iBin];

			AssertConsist();

			// 완전 최초의 추가인 경우
			if(m_nElements == 0)
			{
				assert(m_pHeadBinHead == NULL);
				assert(pOldBinHead == NULL);
				m_pHeadBinHead = pNewNode;
				m_pTailBinTail = pNewNode;
				pNewNode->m_pPrev = NULL;
				pNewNode->m_pNext = NULL;
				
				m_ppBins[iBin] = pNewNode;

				m_nElements++; // 마무리

				AssertConsist();
			}
			else
			{
				//앞쪽 헤드로 붙이므로 tail에는 변화가 없다.
				AssertConsist();

				if(pOldBinHead != NULL)
				{
					// 들어있는 bin에 추가하는 경우 bin head로서 추가한다.
					AssertConsist();

					if(pOldBinHead->m_pPrev != NULL)
					{
						pOldBinHead->m_pPrev->m_pNext = pNewNode;
					}
					else
					{
						// head bin이다. head bin ptr도 교체 필수
						m_pHeadBinHead = pNewNode; 
					}

					pNewNode->m_pPrev = pOldBinHead->m_pPrev; // pOldBinHead->m_pPrev = NULL OK
					pNewNode->m_pNext = pOldBinHead;
					pOldBinHead->m_pPrev = pNewNode;
					assert(pOldBinHead == m_ppBins[iBin]);
					m_ppBins[iBin] = pNewNode;

					m_nElements++; // 마무리

					AssertConsist();
				}
				else
				{
					// 빈 bin에 추가하는 경우, 최초의 linked bin이 되게 rewire한다.
					AssertConsist();

					CNode* pOldHeadBin = m_pHeadBinHead;

					pNewNode->m_pPrev = NULL;
					pNewNode->m_pNext = pOldHeadBin;
					if(pNewNode->m_pNext != NULL)
						pNewNode->m_pNext->m_pPrev = pNewNode;
					m_pHeadBinHead = pNewNode;
					m_ppBins[iBin] = pNewNode;

					m_nElements++; // 마무리

					AssertConsist();
				}

			}

			AssertConsist();

			// 갯수가 지나치게 많아졌으면 rehash한다.
			if ( (m_nElements > m_nHiRehashThreshold) && !IsLocked() )
			{
				Rehash( PickSize( m_nElements ) );
			}

			AssertConsist();

			return( pNewNode );
		}

		// node 제거의 마지막 단계
		void FreeNode( CNode* pNode, bool rehashOnNeed )
		{
			assert( pNode != NULL );
			CallDestructor<CNode>(pNode);

			// free list에 넣는다.
			pNode->m_pNext = m_freeList;
			m_freeList = pNode;

			assert( m_nElements > 0 );
			m_nElements--;

			if ( rehashOnNeed && (m_nElements < m_nLoRehashThreshold) && !IsLocked() )
			{
				Rehash( PickSize( m_nElements ) );
			} 
		}

		/* 
		\~korean
		특정 key를 가진 node를 찾되 부수 정보도 같이 찾는다. 내부 함수
		\param iBin [out] node가 속한 bin의 index
		\param nHash [out] node의 hashed value
		\param pPrev [out] node의 prev node
		\~english
		Finds the node with certain key but also searches attached info. An internal function.
		\param iBin [out] = index of bin that node is possessed
		\param nHash [out] = hashed value of node
		\param pPrev [out] = prev node of node
		\~
		*/
		CNode* GetNode( const KINARGTYPE key, uint32_t& iBin, uint32_t& nHash ) const throw()
		{
			// hash한 값을 얻고, 적절한 bin index를 얻는다.
			nHash = KTraits::Hash( key );
			iBin = nHash % m_nBins;

			// 아직 bin이 없으면 즐
			if ( m_ppBins == NULL )
			{
				return( NULL );
			}

			// bin에서 key와 같은 항목을 순회하며 찾는다.
			for ( CNode* pNode = m_ppBins[iBin]; 
				(pNode != NULL && iBin == pNode->m_nBin); 
				pNode = pNode->m_pNext )
			{
				if ( KTraits::CompareElements( pNode->m_key, key ) )
				{
					return( pNode );
				}
			}

			return( NULL );
		}

		CNode* CreateNode( KINARGTYPE key, uint32_t iBin, uint32_t nHash ) /*throw(Exception)*/
		{
			CNode* pNode;

			if ( m_ppBins == NULL )
			{
				if (!InitHashTable(m_nBins))
					throw std::bad_alloc();
			}

			AssertConsist();

			pNode = NewNode( key, iBin, nHash );

			AssertConsist();

			return( pNode );
		}

		void RemoveNode( CNode* pNode,bool rehashOnNeed=false ) /*throw(Exception)*/
		{
			assert( pNode != NULL );
			uint32_t iBin = pNode->m_nBin;

			// 제거하려는 node가 유일한 개체인 경우
			if(m_nElements == 1)
			{
				m_ppBins[iBin] = NULL;
				m_pHeadBinHead = NULL;
				m_pTailBinTail = NULL;
			}
			else 
			{
				// 이 노드가 bin의 마지막 node인 경우 bin entry도 수정한다.
				if(IsBinUniqueNode(pNode))
					m_ppBins[iBin] = NULL;
				else if(IsBinHeadNode(pNode, iBin))
					m_ppBins[iBin] = pNode->m_pNext;

				// 이 노드가 head bin의 head node이면 head bin도 변경한다.
				if(pNode == m_pHeadBinHead)
				{
					m_pHeadBinHead = pNode->m_pNext;
					pNode->m_pPrev = NULL;	
				}

				// 이 노드가 전체 bin의 마지막 노드라면 노드 변경..
				if(pNode == m_pTailBinTail)
				{
					m_pTailBinTail = pNode->m_pPrev;
					pNode->m_pNext = NULL; 
				}

				// 이 노드의 앞뒤를 서로 연결한다.
				if(pNode->m_pPrev!=NULL)
					pNode->m_pPrev->m_pNext = pNode->m_pNext;
				if(pNode->m_pNext!=NULL)
					pNode->m_pNext->m_pPrev = pNode->m_pPrev;
			}

			// 마지막 단계
			FreeNode( pNode ,rehashOnNeed);

			AssertConsist();
		}

		inline bool IsBinUniqueNode(CNode* pNode) const
		{
			if (pNode->m_pPrev != NULL && (pNode->m_pPrev->m_nBin) == pNode->m_nBin)
				return false;

			if (pNode->m_pNext != NULL && (pNode->m_pNext->m_nBin) == pNode->m_nBin)
				return false;
			
			return true;
		}

		inline bool IsBinHeadNode(CNode* pNode,int iBin) const
		{
			return pNode == m_ppBins[iBin];
		}

		inline CNode* FindNextNode( CNode* pNode ) const throw()
		{
			if (pNode == NULL)
			{
				assert(false);
				return NULL;
			}

			return pNode->m_pNext;
		}

		inline CNode* FindPrevNode( CNode* pNode ) const throw()
		{
			if (pNode == NULL)
			{
				assert(false);
				return NULL;
			}

			return pNode->m_pPrev;
		}

		void UpdateRehashThresholds() throw()
		{
			m_nHiRehashThreshold = INDEXTYPE( m_fHiThreshold * m_nBins );
			m_nLoRehashThreshold = INDEXTYPE( m_fLoThreshold * m_nBins );
			if ( m_nLoRehashThreshold < 17 )
			{
				m_nLoRehashThreshold = 0;
			}
		}

		public:
			~CFastMap2() 
			{
				RemoveAll();
				
				// obj-pool도 청소
				CNode* r = m_freeList;
				while( r != NULL)
				{
					CNode* del = r;
					r = r->m_pNext;
					CProcHeap::Free(del);
				}
			}


		/** 
		\~korean
		CFastMap2은 복사 가능한 객체이다.
		\~english
		CFastMap2 is an object can be copied.
		\~
		 */
		CFastMap2( const CFastMap2& a) 
		{
			m_enableSlowConsistCheck = false;

			InitVars(a.m_nBins,a.m_fOptimalLoad,a.m_fLoThreshold,a.m_fHiThreshold);

			RemoveAll();

			for(iterator i=a.begin();i!=a.end();i++)
			{
				Add(i->GetFirst(), i->GetSecond());
			}
		}

		/** 
		\~korean
		CFastMap2은 복사 가능한 객체이다.
		\~english
		CFastMap2 is an object can be copied.
		\~
		 */
		CFastMap2& operator=(const CFastMap2& a)
		{
			RemoveAll();

			for (iterator i = a.begin(); i != a.end(); i++)
			{
				Add(i->GetFirst(), i->GetSecond());
			}

			return *this;
		}

		/** 
		\~korean
		CFastMap2은 비정렬 container인지라 비교 연산이 std.map보다 느립니다.
		\~english
		CFastMap2 is non-array container so it shows slower comparison operation than std.map
		\~
		 */
#if defined(_WIN32)
		bool Equals(const CFastMap2& a) const
		{
			if (a.GetCount() != GetCount())
				return false;

			std::map<K, V> t2, a2;
			for (const_iterator i = begin(); i != end(); i++)
			{
				t2.insert(std::map<K, V>::value_type(i.GetFirst(), i.GetSecond()));
			}

			for (const_iterator i = a.begin(); i != a.end(); i++)
			{
				a2.insert(std::map<K, V>::value_type(i.GetFirst(), i.GetSecond()));
			}
			return t2 == a2;
		}
#endif

	public:
		/** 
		\~korean
		key 집합을 모아서 준다.
		\~english
		Collects and gives key
		\~
		 */
		void CopyKeysTo(CFastArray<K> &dest)
		{
			dest.SetCount(size());
			int c = 0;
			for (iterator i = begin(); i != end(); i++)
			{
				dest[c++] = i->GetFirst();
			}
		}

		/** 
		\~korean
		key가 있는지 확인한다. 
		\return 키가 있으면 true
		\~english
		Checks if there is key 
		\return If there is key then true
		\~
		 */
		inline bool ContainsKey(const K& key)
		{
			return find(key) != end();
		}

		/** 
		\~korean
		value가 있는지 확인한다.
		\~english
		Checks if there is value
		\~
		 */
		bool ContainsValue(const V& val)
		{
			for (iterator i = begin();i!= end();i++)
			{
				if(val == i->GetSecond())
					return true;
			}
			return false;
		}

		/** 
		\~korean
		key에 대응하는 value가 있으면 true를 리턴한다.
		\~english
		Returns true if there exist value corresponds to key
		\~
		 */
		inline bool TryGetValue(KINARGTYPE key, VOUTARGTYPE value)
		{
			return Lookup(key, value);
		}
		
		/** 
		\~korean
		새 항목을 추가한다.
		\param key 추가할 항목의 key
		\param value 추가할 항목의 value
		\return 성공적으로 들어갔으면 true, 아니면 false를 리턴한다.
		\~english
		Adds a new clause
		\param key = key of the clause to be added
		\param value = value of the clause to be added
		\return = true if successfully entered, otherwise, returns false.
		\~
		 */
		inline bool Add(KINARGTYPE key, VINARGTYPE value)
		{
			// 중복 코딩이지만 워낙 자주 호출되기에 optimize off then code profile을 할 때 
			// 더 잘 보이게 한다.
			CNode* pNode;
			uint32_t iBin;
			uint32_t nHash;

			pNode = GetNode(key, iBin, nHash);
			if (pNode != NULL)
			{
				// SetAt과 달리, 여기서는 기존 값을 교체하지 않고,그냥 포기한다.
				return false;
			}

			pNode = CreateNode(key, iBin, nHash);
			pNode->m_value = value;

			return true;
		}

		class value_type
		{
		public:
			Proud::Position m_pos;
			CFastMap2* m_owner;

#if defined(_MSC_VER)
			__declspec(property(get=GetFirst)) const K& first;
			__declspec(property(get=GetSecond,put=SetSecond)) V& second;
#endif

			inline const K& GetFirst() const { return m_owner->GetKeyAt(m_pos); }
			inline V& GetSecond() const { return m_owner->GetValueAt(m_pos); }
			inline void SetSecond(const V& val) const { return m_owner->SetValueAt(m_pos,val); }
		};

		class iterator;
		friend class iterator;

		class reverse_iterator;
		friend class reverse_iterator;

#ifndef PROUDNET_NO_CONST_ITERATOR

		/** 
		\~korean
		STL의 const_iterator와 같은 역할을 한다.
		\~english
		Performs a role as const_iterator of STL 
		\~
		 */
		class const_iterator:public value_type
		{
		public:
			inline const_iterator() {}
			inline const_iterator(const iterator& src):value_type(src) {}

			const_iterator& operator++()
			{
				// preincrement
				value_type::m_owner->GetNext(value_type::m_pos);
				return (*this);
			}

			const_iterator operator++(int)
			{
				// postincrement
				const_iterator _Tmp = *this;
				++*this;
				return (_Tmp);
			}

			inline bool operator==(const const_iterator& a) const 
			{
				return value_type::m_pos==a.m_pos && value_type::m_owner==a.m_owner;
			}

			inline bool operator!=(const const_iterator& a) const 
			{
				return !(value_type::m_pos==a.m_pos && value_type::m_owner==a.m_owner);
			}

// 			inline const K& first() const
// 			{
// 				return m_owner->GetKeyAt(m_pos);
// 			}
// 
// 			inline const V& second() const
// 			{
// 				return m_owner->GetValueAt(m_pos);
// 			}

			inline const value_type& operator*() const 
			{
				return *this;
			}

			inline const value_type* operator->() const 
			{
				return this;
			}
		};

		class const_reverse_iterator;
		friend class const_reverse_iterator;

		/** 
		\~korean
		STL의 iterator과 같은 역할을 한다.
		\~english
		Performs a role as iterator of STL
		\~
		 */
		class const_reverse_iterator:public value_type
		{
		public:
			inline const_reverse_iterator(){}
			inline const_reverse_iterator(const reverse_iterator& src):value_type(src){}
			
			const_reverse_iterator& operator++()
			{
				// preincrement
				value_type::m_owner->GetPrev(value_type::m_pos);
				return (*this);
			}

			const_reverse_iterator operator++(int)
			{
				// postincrement
				const_reverse_iterator _Tmp = *this;
				++*this;
				return (_Tmp);
			}

			inline bool operator==(const const_reverse_iterator& a) const 
			{
				return value_type::m_pos==a.m_pos && value_type::m_owner==a.m_owner;
			}

			inline bool operator!=(const const_reverse_iterator& a) const 
			{
				return !(value_type::m_pos==a.m_pos && value_type::m_owner==a.m_owner);
			}

			inline const value_type& operator*() const 
			{
				return *this;
			}

			inline const value_type* operator->() const 
			{
				return this;
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
		class iterator:public value_type
		{
		public:
			inline iterator() {}
			inline iterator(const const_iterator& src):value_type(src) {}

			/** 
			\~korean
			STL iterator와 같은 역할을 하게 해주는 연산자 함수
			\~english
			Operator Function that lets this perform a role as STL iterator
			\~
			 */
			inline bool operator==(const iterator& a) const 
			{
				return value_type::m_pos==a.m_pos && value_type::m_owner==a.m_owner;
			}

			/** 
			\~korean
			STL iterator와 같은 역할을 하게 해주는 연산자 함수
			\~english
			Operator Function that lets this perform a role as STL iterator
			\~
			 */
			inline bool operator!=(const iterator& a) const 
			{
				return !(value_type::m_pos==a.m_pos && value_type::m_owner==a.m_owner);
			}

			/** 
			\~korean
			STL iterator와 같은 역할을 하게 해주는 연산자 함수
			\~english
			Operator Function that lets this perform a role as STL iterator
			\~
			 */
			inline iterator& operator++()
			{	
				// preincrement
				value_type::m_owner->GetNext(value_type::m_pos);
				return (*this);
			}

			/** 
			\~korean
			STL iterator와 같은 역할을 하게 해주는 연산자 함수
			\~english
			Operator Function that lets this perform a role as STL iterator
			\~
			 */
			inline iterator operator++(int)
			{	
				// postincrement
				iterator _Tmp = *this;
				++*this;
				return (_Tmp);
			}

			/** 
			\~korean
			STL iterator와 같은 역할을 하게 해주는 연산자 함수
			\~english
			Operator Function that lets this perform a role as STL iterator
			\~
			 */
			inline iterator& operator--()
			{	
				// preincrement
				value_type::m_owner->GetPrev(value_type::m_pos);
				return (*this);
			}

			/** 
			\~korean
			STL iterator와 같은 역할을 하게 해주는 연산자 함수
			\~english
			Operator Function that lets this perform a role as STL iterator
			\~
			 */
			inline iterator operator--(int)
			{	
				// predecrement
				--(*this);
				return (*this);
			}

			/** 
			\~korean
			STL iterator와 같은 역할을 하게 해주는 연산자 함수
			\~english
			Operator Function that lets this perform a role as STL iterator
			\~
			 */
			inline const value_type& operator*() const 
			{
				return *this;
			}

			/** 
			\~korean
			STL iterator와 같은 역할을 하게 해주는 연산자 함수
			\~english
			Operator Function that lets this perform a role as STL iterator
			\~
			 */
			inline const value_type* operator->() const 
			{
				return this;
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
			ret.m_pos = GetStartPosition();
			ret.m_owner=this;

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
			ret.m_pos=NULL;
			ret.m_owner=this;

			return ret;
		}

		inline value_type front() 
		{
			if(GetCount()==0)
				ThrowInvalidArgumentException();

			return *begin();
		}

		/** 
		\~korean
		STL의 iterator와 같은 역할을 한다.
		\~english
		Performs a role as iterator of STL
		\~ */
		class reverse_iterator:public value_type
		{
		public:
			inline reverse_iterator() {}
			inline reverse_iterator(const const_reverse_iterator& src):value_type(src) {}

			/** 
			\~korean
			STL reverse_iterator와 같은 역할을 하게 해주는 연산자 함수
			\~english
			Operator Function that lets this perform a role as STL reverse_iterator
			\~
			 */
			inline bool operator==(const reverse_iterator& a) const 
			{
				return value_type::m_pos==a.m_pos && value_type::m_owner==a.m_owner;
			}

			/** 
			\~korean
			STL reverse_iterator와 같은 역할을 하게 해주는 연산자 함수
			\~english
			Operator Function that lets this perform a role as STL reverse_iterator
			\~
			 */
			inline bool operator!=(const reverse_iterator& a) const 
			{
				return !(value_type::m_pos==a.m_pos && value_type::m_owner==a.m_owner);
			}

			/** 
			\~korean
			STL reverse_iterator와 같은 역할을 하게 해주는 연산자 함수
			\~english
			Operator Function that lets this perform a role as STL reverse_iterator
			\~
			 */
			inline reverse_iterator& operator++()
			{	
				// preincrement
				value_type::m_owner->GetPrev(value_type::m_pos);
				return (*this);
			}

			/** 
			\~korean
			STL reverse_iterator와 같은 역할을 하게 해주는 연산자 함수
			\~english
			Operator Function that lets this perform a role as STL reverse_iterator
			\~
			 */
			inline reverse_iterator operator++(int)
			{	
				// postincrement
				reverse_iterator _Tmp = *this;
				++*this;
				return (_Tmp);
			}

			/** 
			\~korean
			STL reverse_iterator와 같은 역할을 하게 해주는 연산자 함수
			\~english
			Operator Function that lets this perform a role as STL reverse_iterator
			\~
			 */
			inline reverse_iterator& operator--()
			{	
				// preincrement
				value_type::m_owner->GetNext(value_type::m_pos);
				return (*this);
			}

			/** 
			\~korean
			STL reverse_iterator와 같은 역할을 하게 해주는 연산자 함수
			\~english
			Operator Function that lets this perform a role as STL reverse_iterator
			\~
			 */
			inline reverse_iterator operator--(int)
			{	
				// predecrement
				--(*this);
				return (*this);
			}

			/** 
			\~korean
			STL reverse_iterator와 같은 역할을 하게 해주는 연산자 함수
			\~english
			Operator Function that lets this perform a role as STL reverse_iterator
			\~
			 */
			inline const value_type& operator*() const 
			{
				return *this;
			}

			/** 
			\~korean
			STL reverse_iterator와 같은 역할을 하게 해주는 연산자 함수
			\~english
			Operator Function that lets this perform a role as STL reverse_iterator
			\~
			 */
			inline const value_type* operator->() const 
			{
				return this;
			}
		};

		/** 
		\~korean
		STL의 동명 메서드와 같은 역할을 한다.
		\~english
		Performs a role as same name method of STL
		\~ 
		*/
		inline reverse_iterator rbegin() 
		{
			reverse_iterator ret;
			ret.m_pos = GetEndPosition();
			ret.m_owner=this;

			return ret;
		}

		/** 
		\~korean
		STL의 동명 메서드와 같은 역할을 한다.
		\~english
		Performs a role as same name method of STL
		\~
		 */
		inline reverse_iterator rend()
		{
			reverse_iterator ret;
			ret.m_pos=NULL;
			ret.m_owner=this;

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
			ret.m_pos = GetStartPosition();
			ret.m_owner = (CFastMap2*)this;

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
			ret.m_pos=NULL;
			ret.m_owner=(CFastMap2*)this;

			return ret;
		}

		inline const_reverse_iterator rbegin() const
		{
			const_reverse_iterator ret;
			ret->m_pos = GetEndPosition();
			ret->m_owner = (CFastMap2*)this;

			return ret;
		}

		inline const_reverse_iterator rend() const
		{
			const_reverse_iterator ret;
			ret->m_pos = NULL;
			ret->m_owner = (CFastMap2*)this;

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
		reverse_iterator erase(reverse_iterator iter)
		{
			if(iter.m_owner!=this)
			{
				ThrowInvalidArgumentException();
			}
			reverse_iterator ret = iter;
			ret++;
			RemoveAtPos(iter.m_pos);

			return ret;
		}

		/** 
		\~korean
		STL의 동명 메서드와 같은 역할을 한다.
		\~english
		Performs a role as same name method of STL
		\~
		 */
		iterator erase(iterator iter)
		{
			if(iter.m_owner!=this)
			{
				ThrowInvalidArgumentException();
			}
			iterator ret = iter;
			ret++;
			RemoveAtPos(iter.m_pos);

			return ret;
		}
		/** 
		\~korean
		STL의 동명 메서드와 같은 역할을 한다.
		\~english
		Performs a role as same name method of STL
		\~
		 */
		iterator find(const K& key) 
		{
			iterator ret;
			ret.m_owner=this;
			uint32_t iBin, nHash;
			CNode* node = GetNode(key, iBin, nHash);
			if(node!=NULL)
			{
				ret.m_pos=Proud::Position(node);
			}
			else
			{
				ret.m_pos=NULL;
			}

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
		const_iterator find(const K& key) const
		{
			const_iterator ret;
			ret.m_owner=(CFastMap2*)this;
			uint32_t iBin,nHash;
			CNode* node = GetNode(key,iBin,nHash);
			if(node!=NULL)
			{
				ret.m_pos=Proud::Position(node);
			}
			else
			{
				ret.m_pos=NULL;
			}

			return ret;
		}

#endif
// 		/** 내부 버퍼로 CFastHeap을 사용한다.  이 객체를 쓴다는 것 자체가 잦은 heap 억세스를 줄이는 것도 목표. 따라서 이 함수는 무의미.
// 		\param heap CFastHeap포인터
// 		*/
// 		void UseFastHeap(CFastHeap* heap)
// 		{
// 			if(heap == NULL)
// 			{
// 				ThrowInvalidArgumentException();
// 			}
// 			if(GetCount() > 0)
// 				throw Exception(CannotUseFastHeapForFilledCollectionErrorText);
// 		}
		//inline CFastHeap* GetRefFastHeap() { return m_refHeap; }
	};

#ifdef ENABLE_REHASH_COUNT
	extern PROUDNET_VOLATILE_ALIGN int32_t FastMap_RehashCount;
#endif // ENABLE_REHASH_COUNT

	/**  @} */
}

//#pragma pack(pop)