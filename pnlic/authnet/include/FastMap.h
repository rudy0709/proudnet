/*
ProudNet HERE_SHALL_BE_EDITED_BY_BUILD_HELPER


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

#include <assert.h>
#include <map>
#include "FastHeap.h"
#include "BasicTypes.h"
#include "Singleton.h"
#include "LookasideAllocator.h"
#include "FastArray.h"
#include "Enums.h"
#include "sysutil.h"
#include "Exception.h"

//#pragma pack(push,8)

#if defined(_MSC_VER)
#pragma warning(disable:4290)
#endif

namespace Proud
{
	/** \addtogroup util_group
	*  @{
	*/

	/** 
	\~korean
	(Key,Value) pair의 hash 알고리즘 기반 map class입니다. 상세한 내용은 \ref hash_map 에 있습니다.
	- CAtlMap과 사용법이 동일합니다. 그러면서도 STL.map의 iterator 및 일부 메서드와 동일하게 사용할 수 있습니다.
	게다가 .Net framework의 Dictionary class와 같은 형식으로도 쓸 수 있습니다.
	- 이 클래스의 iterator는 STL.map의 iterator보다 성능이 훨씬 빠른 성능을 냅니다.

	\param K 콜렉션의 키 타입
	\param V 콜렉션의 값 타입
	\param KTraits 콜렉션의 키 타입을 다루는 특성을 정의한 클래스
	\param VTraits 콜렉션의 값 타입을 다루는 특성을 정의한 클래스
	\param AllocT AllocType 값 중 하나

	\~english
	An hash algorithm base map class of (Key,Value) pair. Please refer \ref hash_map for further detail. 
	- Has very same usage as CAtlMap while still can be used same as iterator of STL.map and some methods.
	  Plus, can be used as same format as Dictionary class of .NET framework.
	- The iterator of this class performs much faster that the iterator of STL.map.

	\param K key type of collection
	\param V value tpe of collection
	\param KTraits class that defines the characteristics that handle key type of collection
	\param VTraits class that defines the characteristics that handle value type of collection

	\~chinese 
	是(Key,Value) pair的hash算法基础的map class。详细内容在\ref hash_map%。
	- 与 CAtlMap%使用方法一致。而且能与 STL.map%的iterator及一些方法相同使用。加上能与.Net framework 的Dictionary class用于相同形式。
	- 此类的iterator比 STL.map%的iterator显示出更快的性能。

	\param K collection key类型。
	\param V collection值类型。
	\param KTraits 定义了处理collection的key类型的特性的类。
	\param VTraits 定义了处理collection的值类型的特性的类。
	\param AllocT AllocType值中之一。

	\~japanese
	\~

	example code of traits class

	\code

	class Foo
	{
		public:
		string a;
		int b;
	};

	class FooTraits
	{
	public:
		// returns hash value of element
		uint32_t Hash(const Foo& v)
		{
			// regularily, variables are XORed for hashing values.
			return Hash(v.a) ^ Hash(v.b);
		}

		// true if two values are the same
		inline static bool CompareElements(const Foo& e1, const Foo& e2)
		{
			return e1.a == e2.a && e1.b == e2.b;
		}

		// negative value if left is smaller.
		// zero if the same.
		// positive value if left is larger.
		inline static int CompareElementsOrdered(const Foo& e1, const Foo& e2)
		{
			if (e1.a < e2.a)
				return -1;
			if (e1.a > e2.a)
				return 1;
			else
			{
				return e1.b - e2.b;
			}
		}
	};

	CFastMap<Foo, int, FooTraits> fooToIntMap;
	
	\endcode

	*/
	template < typename K, typename V, typename KTraits = CPNElementTraits< K >, typename VTraits = CPNElementTraits< V > >
	class CFastMap
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
		typedef typename CFastMap<K,V>::const_iterator ConstIterType;
	public:
		class CPair : public __Position
		{
		protected:
			inline CPair(KINARGTYPE key) :
				 m_key( key )
				 {
				 }
		private:
			inline CPair& operator =(const CPair&);

		public:
			K m_key;
			V m_value;

		};

	private:
		class CNode :
			public CPair
		{
		public:
			inline CNode( KINARGTYPE key ) :
				CPair( key )
			{
				m_pNext = NULL;
				m_pPrev = NULL;
			}
			inline ~CNode() 
			{

			}

		private:
			inline const CNode& operator =(const CNode&);

		public:
			// bin내의 마지막 node인 경우 next는 다른 bin의 head node를 가리킨다.  next=NULL인 경우 이 node는 last bin의 last node이다.
			// prev는 next의 반대 역할을 한다. prev = NULL인 경우, 이 node는 head bin의 head node이다
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
		intptr_t m_nElements;
		
		// hash table 크기
		uint32_t m_nBins;
		
		float m_fOptimalLoad;
		float m_fLoThreshold;
		float m_fHiThreshold;
		intptr_t m_nHiRehashThreshold;
		intptr_t m_nLoRehashThreshold;
		uint32_t m_nLockCount;
		
		CFastHeap* m_refHeap;
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
		
		\param nBins The size of base hash table. Performs ok when set with prime number. Please refer \ref hash_map for further detail.
		\param fOptimalLoad Optimized load proportion. Please refer \ref hash_map for further detail.
		\param fLoThreshold Minimum load proportion. Please refer \ref hash_map for further detail.
		\param fHiThreshold Maximum load proportion. Please refer \ref hash_map for further detail.

		\~chinese
		是生成者。

		\param nBins 是基本hash table的大小。设置为素数才能发挥它的性能。详细内容在\ref hash_map%。
		\param fOptimalLoad 最佳负荷比率。详细内容在\ref hash_map%。
		\param fLoThreshold 最小负荷比率。详细内容在\ref hash_map%。
		\param fHiThreshold 最大负荷比率。详细内容在\ref hash_map%。

		\~japanese
		\~
		*/
		CFastMap( uint32_t nBins = 17, float fOptimalLoad = 0.75f, float fLoThreshold = 0.25f, float fHiThreshold = 2.25f)
		{
			m_enableSlowConsistCheck = false;
			m_refHeap = NULL;

			assert( nBins > 0 );

			InitVars(nBins, fOptimalLoad, fLoThreshold, fHiThreshold);
		}

#if defined(_MSC_VER)        
		/**
		\~korean
		이미 갖고 있는 항목의 갯수를 구한다.

		\~english
		Gets number of item that already owned

		\~chinese
		求已拥有项目的数量。

		\~japanese
		\~
		*/
		__declspec(property(get = GetCount)) intptr_t Count;
#endif
		
		inline intptr_t GetCount() const throw()
		{
			return( m_nElements );
		}

		/**
		\~korean
		이미 갖고 있는 항목의 갯수를 구한다.
		
		\~english
		Gets number of item that already owned
		
		\~chinese
		求已拥有项目的数量。

		\~japanese
		\~
		*/
		inline intptr_t size() const 
		{
			return GetCount();
		}

		/**
		\~korean
		텅빈 상태인가?
		
		\~english
		Is it empty?

		\~chinese
		是否为空？

		\~japanese
		\~
		*/
		inline bool IsEmpty() const throw()
		{
			return( m_nElements == 0 );
		}

		/** 
		\~korean
		key에 대응하는 value를 얻는다. 
		\param [in] key 찾을 키
		\param [out] value 찾은 키에 대응하는 값이 저장될 곳
		\return key를 찾았으면 true를 리턴한다.

		\~english
		Gets value correspnds to key 
		\param [in] key key to find
		\param [out] value a space where the value corresponds to the key found to be stored
		\return returns true if key is found. 

		\~chinese
		获得对应key的value。
		\param [in] key 要找的key。
		\param [out] value 对象已找的key的值要被储存的位置。
		\return 找到key的话返回true。

		\~japanese
		\~
		 */
		bool Lookup( KINARGTYPE key, VOUTARGTYPE value ) const
		{
			// 빈 것은 빨리 찾아버리자.
			if(m_nElements == 0)
				return false;

			uint32_t iBin;
			uint32_t nHash;
			CNode* pNode;

			pNode = GetNode( key, iBin, nHash );
			if ( pNode == NULL )
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

		\~chinese
		找对应key的value，返回 CPair%对象。

		\~japanese
		\~ 
		*/
		const CPair* Lookup( KINARGTYPE key ) const throw()
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
		key에 대응하는 value를 찾되, CPair 객체를 리턴한다.

		\~english
		Finds value corrsponds to key but returns CPair object.

		\~chinese
		找对应key的value，返回 CPair%对象。

		\~japanese
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

		\return returns reference of entry that is eiter alread existing or newly added

		\~chinese
		找对应key的value值。没有的话在内部创建新的entry。

		\return 返回已有或者新添加的entry的reference。

		\~japanese
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
		\param key key value to be added
		\param value value object to be added
		\return pointer that points additional location after adding. Since Position is a base class, it is possible to regards Position as return value.

		\~chinese
		新添加key,value pair。已有的话就覆盖。 
		\param key 要添加的key值。
		\param value 要添加的值对象。
		\return 添加后指向添加位置的指针。因为Position是默认类，可以直接把Position看作是返回值。

		\~japanese
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
				//try
				//{
					pNode->m_value = value;
				//}
				//catch(...)
				//{
				//    RemoveAtPos( Proud::Position( pNode ) );
				//    throw;
				//}
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

		\~english TODO:translate needed.
		Newly enters the value of location where previously acquired Position object points.
		\param pos
		\param value

		\~chinese
		新填入事先获得的Position对象指向的位置的value。
		\param pos 要放入value的node的Position。
		\param value 要往node放入的data。

		\~japanese
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
		\copydoc 제거

		\~english
		\copydoc Remove

		\~chinese
		\copydoc 删除

		\~japanese
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

		/**
		\~korean
		완전히 비운다
		
		\~english
		Completely empty it
		
		\~chinese
		完全腾空。

		\~japanese
		\~
		*/
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
		\param key key to be removed
		\param rehashOnNeed if ture then hash table is re-modified when hash table became small enough 
		Must pay attention to the fact that if there exists either iterator and/or Position during the process, it/they will be nullified.
		\return returns if found and removed. False if failed to find.

		\~chinese
		找到key指向的项目并删除。
		\param key 要删除的key。
		\param rehashOnNeed 是true的话，在hash table充分变小时的情况重新调整hash table。
		要注意这时候如有使用中的iterator 나 Position，注意这便会无效。
		\return 找到并删除了的话返回true。没有找到的话是false。

		\~japanese
		\~
		*/
		inline bool Remove( KINARGTYPE key,bool rehashOnNeed=false ) 
		{
			return RemoveKey(key,rehashOnNeed);
		}

		/**
		\~korean

		\~english

		\~chinese

		\~japanese

		\~
		\copydoc Clear
		*/
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
				if(!InitHashTable( PickSize( m_nElements ), false ))
					ThrowBadAllocException();
			}

			AssertConsist();

			EnableAutoRehash();
		}

		/** 
		\~korean
		Position이 가리키는 곳의 key-value pair를 제거한다.
		\param pos 일전에 얻은 Position 값. 이 값은 유효한 값이어야 한다!
		\param rehashOnNeed true일 때, hash table이 충분히 작아진 경우 hash table을 재조정한다. 
		이때 사용중이던 iterator나 Position 가 있을 경우 이는 무효화됨을 주의해야 한다. 

		\~english
		Removes key-value pair of where pointed by Position
		\param pos Position value acquired before. This value must be valid!
		\param rehashOnNeed if ture then hash table is re-modified when hash table became small enough
		Must pay attention to the fact that if there exists either iterator and/or Position during the process, it/they will be nullified.

		\~chinese
		删除Position指向的地方的key-value pair。
		\param pos 事先获得的Position值。此值得是有效的值！
		\param rehashOnNeed true时，如hash table充分变小，则要重新调整hash table。
		要注意，这时在使用的iterator或Position，注意这会变得无效。

		\~japanese
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
		\return position of the foremost clause

		\~chinese
		获得所拥有的项目中最前面的项目。主要为了iteration所用。
		在使用GetNext等后，才能进行iteration之后的项目。
		\return 最前面项目的Position。

		\~japanese
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
		\return position of the last clause

		\~chinese
		获得拥有的项目中最后面的项目。主要为了reverse_iteration所用。
		在使用GetPrev等后，才能进行iteration之后的项目。
		\return 最后项目的Position。


		\~japanese
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
		\param [in,out] pos 다음 항목의 Position값
		\param [out] key 다음항목의 key
		\param [out] value 다음 항목의 값

		\~english TODO:translate needed.
		Gets following clause
		\param [in,out] pos
		\param [out] key
		\param [out] value

		\~chinese
		获得之后项目。
		\param [in,out] pos 之后项目的Position值。
		\param [out] key 之后项目的key。
		\param [out] value 之后项目的值。

		\~japanese
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

		\~english TODO:translate needed.
		Gets following clause

		\~chinese
		获得之后的项目。
		\param pos 获得所指的node的之后Position。
		\return 返回之后node的const CPair%值。

		\~japanese
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
		\return 다음 node의 \ref CPair 값을 리턴

		\~english TODO:translate needed.
		Gets following clause

		\~chinese
		获得之后的项目。
		\param pos pos获得所指的node之后的Position。
		\return 返回之后node的\ref CPair%值 。

		\~japanese
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
		\return 이전노드의 \ref CPair 값

		\~english TODO:translate needed.
		Gets previous clause

		\~chinese
		获得之前项目。
		\param pos 获得pos所指的node的之前Position。
		\return 之前node的\ref CPair%值 。

		\~japanese
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

		\~english TODO:translate needed.
		Gets following clause

		\~chinese
		获得之后项目。
		\param pos 获得这个pos所指的node之后的Position。
		\return 之后node的key值。

		\~japanese
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

		\~english TODO:translate needed.
		Gets following clause

		\~chinese
		获得之后项目。
		\param pos 获得这个pos所指的node之后的Position。
		\return 之后node的const value值。

		\~japanese
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

		\~english TODO:translate needed.
		Gets following clause

		\~chinese
		获得之后的项目。
		\param pos 获得这个pos所指的node之后的Position。
		\return 之后node的value值。

		\~japanese
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

		\~english TODO:translate needed.
		Gets key and value of where Position points

		\~chinese
		获得Position所指的位置的key和value。
		\param pos 指向node的Position。
		\param key 获得相关node的key。
		\param value 获得相关node的data。

		\~japanese
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

		\~english TODO:translate needed.
		Gets key and value of where index locates

		\~chinese
		获得在相关index位置的key和value。
		\param index 从第一个信息开始移动至相当于此index，之后移动到下一个node。
		\return 找到的node的 CPair%。

		\~japanese
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

		\~english TODO:translate needed.
		Gets key and value of where index locates

		\~chinese
		获得在相关index位置的key和value。
		\param index 从第一个信息开始移动至相当于此index，之后移动到下一个node。
		\return 找到的node的const CPair%。

		\~japanese
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

		\~english TODO:translate needed.
		Gets key and value of where Position points

		\~chinese
		获得Position指向的位置的key和value。
		\param pos Position信息。
		\return 相当于Position的 CPair%。

		\~japanese
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

		\~english TODO:translate needed.
		Gets key and value of where Position points

		\~chinese
		获得Position指向的位置的key和value。
		\param pos Position信息。
		\return 相当于Position的const CPair%。

		\~japanese
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

		\~english TODO:translate needed.
		Gets key and value of where Position points

		\~chinese
		获得Position指向的位置的key和value。
		\param pos 指向node的Position。
		\return Position指向的node的key。

		\~japanese
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

		\~english TODO:translate needed.
		Gets key and value of where Position points

		\~chinese
		获得Position指向的位置的key和value。
		\param pos 指向node的Position。
		\return Position指向的node的cons data。

		\~japanese
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

		\~english TODO:translate needed.
		Gets key and value of where Position points

		\~chinese
		获得Position指向的位置的key和value。
		\param pos 指向node的Position。
		\return Position指向的node的data。

		\~japanese
		\~
		*/
		inline V& GetValueAt( Proud::Position pos )
		{
			assert( pos != NULL );

			CNode* pNode = (CNode*)pos;

			return( pNode->m_value );
		}
				
		/**
		\~korean
		hash 테이블의 크기를 얻습니다.

		\~english TODO:translate needed.


		\~chinese
		获取hash table的大小。

		\~japanese
		\~
		*/
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

		\~english TODO:translate needed.


		\~chinese
		初始化hash table。
		- 生成node的时候会自动被呼叫。
		\param nBins hash的大小。
		\param bAllocNow 设置hash内存生成与否。
		\return hash初始化成功的话true，失败的话返回false。

		\~japanese
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

		/**
		\~korean
		자동으로 Rehash를 합니다.

		\~english TODO:translate needed.

		\~chinese
		自动Rehash。

		\~japanese
		\~
		*/
		void EnableAutoRehash() throw()
		{
			assert( m_nLockCount > 0 );
			m_nLockCount--;
		}

		/**
		\~korean
		자동으로 Rehash를 하지 않습니다.

		\~english TODO:translate needed.

		\~chinese
		不自动Rehash。

		\~japanese
		\~
		*/
		void DisableAutoRehash() throw()
		{
			m_nLockCount++;
		}

		/**
		\~korean
		hash 테이블을 다시 생성한다.
		\param nBins hash테이블의 크기

		\~english TODO:translate needed.


		\~chinese
		重新生成hash table。
		\param nBins hash table的大小。

		\~japanese
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
				if(!InitHashTable( nBins, false ))
					ThrowBadAllocException();

				return;
			}

			// 새 hash table을 준비한다. 여기에 구 hash table이 옮겨질 것이다.
			ppBins = (CNode**)CProcHeap::Alloc(sizeof(CNode*) * nBins);
			if (ppBins == NULL)
				ThrowBadAllocException();

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

		\param fOptimalLoad Optimal load proportion
		\param fLoThreshold Minimum limit of load proportion
		\param fHiThreshold Maximum limit of load proportion
		\param bRehashNow hash table re-setting proccess runs immediately if true.

		\~chinese
		重新设置map的最佳负荷。对map的最佳负荷的内容在\ref hash_map_load%。
		\param fOptimalLoad 最佳负荷的比率。
		\param fLoThreshold 最佳负荷比率的最小界限。
		\param fHiThreshold 最佳负荷比率的最大界限。
		\param bRehashNow true的话将立即执行hash table的重新设置将立即执行。

		\~japanese
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

		\~english TODO:translate needed.
		lookup best performance, This setting use more memory rather than doing rehash.
		It is useful when rehash cost is expensive.

		\~chinese
		Lookup 最佳性能，尽可能不要最大化rehash，使用很多内存的设置。
		增减幅度本来就大，rehash cost 大的时候会很有用。

		\param rehashNow 根据设置选择是否要Rehash。True 的话会Rehash。

		\~japanese
		\~
		*/
		void SetOptimalLoad_BestLookup(bool rehashNow = false)
		{
			SetOptimalLoad(0.1f,0.0000001f,2.1f,rehashNow); 
		}


		/**
		\~korean 
		각 bin을 뒤져서, 최악의 bin, 즉 가장 많은 item을 가진 bin의 item 갯수를 리턴한다.

		\~english TODO:translate needed.

		\~chinese
		找各个bin，返回最坏的bin，即拥有最多item的bin的item个数。

		\~japanese
		\~
		*/
		intptr_t GetWorstBinItemCount()
		{
			intptr_t worstValue = 0;

			CNode* prevNode = NULL;
			intptr_t itemCountInCurrentBin = 0;
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

		\~chinese
		创建并提供收集map对象key的数组。

		\~japanese
		\~
		 */
		void KeysToArray(CFastArray<K> &output) const
		{
			output.Clear();
			output.SetCount(GetCount());
			int c = 0;
			
			for(ConstIterType i=CFastMap<K,V>::begin();i!=CFastMap<K,V>::end();i++)
			{
				output[c] = i.GetFirst();
				c++;
			}
		}

		/** 
		\~korean
		map 객체의 value들을 모은 배열을 만들어 제공합니다.

		\~english
		Provide array that contain value of map objects

		\~chinese
		创建并提供收集map对象value的数组。

		\~japanese
		\~
		 */
		void ValuesToArray(CFastArray<V> &output) const
		{
			output.Clear();
			output.SetCount(GetCount());
			int c = 0;
			for(ConstIterType i=CFastMap<K,V>::begin();i!=CFastMap<K,V>::end();i++)
			{
				output[c] = i.GetSecond();
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

		\~chinese
		确认是否有内部状态不良的。
		- 即使容量大也要inline才能在空函数的时候成为noop。(regardless of compilers)

		\~japanese
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
					ThrowException("CFastMap consistency error #0!");
				}

				for(CNode* pNode = m_pHeadBinHead; pNode!=NULL;pNode=pNode->m_pNext)
				{
					if(pNode->m_pNext)
					{
						if(pNode->m_pNext->m_pPrev != pNode)
						{
							assert(0);
							ThrowException("CFastMap consistency error #1!");
						}
					}
					if(pNode==m_pHeadBinHead)
					{
						if(pNode->m_pPrev)
						{
							assert(0);
							ThrowException("CFastMap consistency error #2!");
						}
					}
					if(pNode==m_pTailBinTail)
					{
						if(pNode->m_pNext)
						{
							assert(0);
							ThrowException("CFastMap consistency error #3!");
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
					ThrowException("CFastMap consistency error #4!");
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
		uint32_t PickSize( intptr_t nElements ) const throw()
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

			intptr_t nBins = (intptr_t)(nElements / m_fOptimalLoad);
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

			if(m_refHeap)
				pNewNode = (CNode*) m_refHeap->Alloc(sizeof(CNode));
			else
				pNewNode = (CNode*) CProcHeap::Alloc(sizeof(CNode));

			if(pNewNode == NULL)
				ThrowBadAllocException();


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
			if(m_refHeap)
				m_refHeap->Free(pNode);
			else
				CProcHeap::Free(pNode);

			//delete pNode;

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
		\param [out] iBin node가 속한 bin의 index
		\param [out] nHash node의 hashed value
		\param [out] pPrev node의 prev node

		\~english
		Finds the node with certain key but also searches attached info. An internal function.
		\param [out] iBin index of bin that node is possessed
		\param [out] nHash hashed value of node
		\param [out] pPrev prev node of node


		\~chinese
		找到拥有特定key的node，同时也要找到附加信息。内部函数。
		\param [out] iBin node所属的bin的index。
		\param [out] nHash node的hashed value。
		\param [out] pPrev node的prey node。

		\~japanese
		\~
		*/
		CNode* GetNode( KINARGTYPE key, uint32_t& iBin, uint32_t& nHash ) const throw()
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
				if(!InitHashTable( m_nBins ))
					ThrowBadAllocException();
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
			m_nHiRehashThreshold = intptr_t( m_fHiThreshold * m_nBins );
			m_nLoRehashThreshold = intptr_t( m_fLoThreshold * m_nBins );
			if ( m_nLoRehashThreshold < 17 )
			{
				m_nLoRehashThreshold = 0;
			}
		}

		public:
		~CFastMap() 
		{
#if !defined(_WIN32)
		//try
		//{
				RemoveAll();
		//}
		//    catch(...)
		//{
		//        assert(false);
		//    }
#else
			//_ATLTRY
			//try
			//{
				RemoveAll();
			//}
			//_ATLCATCHALL()
			//catch(...)
			//{
			//	assert(false);
			//}
#endif
		}


		/** 
		\~korean
		CFastMap 은 복사 가능한 객체이다.

		\~english
		CFastMap is an object can be copied.


		\~chinese
		CHashMap 是可以复制的对象。

		\~japanese
		\~
		 */
		CFastMap( const CFastMap& a) 
		{
			m_refHeap = a.m_refHeap;
			m_enableSlowConsistCheck = false;

			InitVars(a.m_nBins,a.m_fOptimalLoad,a.m_fLoThreshold,a.m_fHiThreshold);

			RemoveAll();

			for(iterator i=a.begin();i!=a.end();i++)
			{
				Add(i->GetFirst(),i->GetSecond());
			}
		}

		/** 
		\~korean
		CFastMap 은 복사 가능한 객체이다.

		\~english
		CFastMap is an object can be copied.


		\~chinese
		CHashMap 是可以复制的对象。

		\~japanese
		\~
		 */
		CFastMap& operator=( const CFastMap& a)
		{
			RemoveAll();

			m_refHeap = a.m_refHeap;
			for(iterator i=a.begin();i!=a.end();i++)
			{
				Add(i->GetFirst(),i->GetSecond());
			}

			return *this;
		}

		/** 
		\~korean
		CFastMap 은 비정렬 container인지라 비교 연산이 std.map 보다 느립니다.

		\~english
		CFastMap is non-array container so it shows slower comparison operation than std.map

		\~chinese
		因为ChashMap是非列队container，比较运算比 std.map%缓慢。

		\~japanese
		\~
		 */
		bool Equals( const CFastMap& a) const
		{
			if(a.GetCount() != GetCount())
				return false;

			std::map<K,V> t2,a2;
			for(const_iterator i=begin();i!=end();i++)
			{
				t2.insert(typename std::map<K, V>::value_type(i.GetFirst(), i.GetSecond()));
			}

			for(const_iterator i=a.begin();i!=a.end();i++)
			{
				a2.insert(typename std::map<K, V>::value_type(i.GetFirst(), i.GetSecond()));
			}
			return t2==a2;
		}

	public:
		/** 
		\~korean
		key 집합을 모아서 준다.

		\~english
		Collects and gives key

		\~chinese
		收集key集合并给予。

		\~japanese
		\~
		 */
		void CopyKeysTo(CFastArray<K> &dest)
		{
			dest.SetCount(size());
			int c = 0;
			for (iterator i = begin();i != end();i++)
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

		\~chinese
		确认是否有key。
		\retur 有key的话true。

		\~japanese
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

		\~chinese
		确认是否有value。

		\~japanese
		\~
		 */
		bool ContainsValue(const V& val)
		{
			for (iterator i = begin();i!= end();i++)
			{
				if (val == i->GetSecond())
					return true;
			}
			return false;
		}

		/** 
		\~korean
		key에 대응하는 value가 있으면 true를 리턴한다.

		\~english
		Returns true if there exist value corresponds to key

		\~chinese
		有对应key的value的话返回true。

		\~japanese
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
		\param key key of the clause to be added
		\param value value of the clause to be added
		\return true if successfully entered, otherwise, returns false.

		\~chinese
		添加新项目。
		\param key 要添加的项目的key。
		\param value 要添加的项目的value。
		\return 成功进入的话true，否则返回false。

		\~japanese
		\~
		 */
		inline bool Add(KINARGTYPE key, VINARGTYPE value)
		{
			if(ContainsKey(key))
				return false;
			
			(*this)[key] = value;
			return true;
		}

		class value_type
		{
		public:
			Proud::Position m_pos;
			CFastMap* m_owner;

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

		\~chinese
		起着与STL的const_iterator一样的作用。

		\~japanese
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

		\~chinese
		起着与STL的iterator一样的作用。

		\~japanese
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

		\~chinese
		起着与STL的iterator一样的作用。

		\~japanese
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

			\~chinese
			使得起着与STL iterator一样作用的运算者函数。

			\~japanese
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

			\~chinese
			使得起着与STL iterator一样作用的运算者函数。

			\~japanese
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

			\~chinese
			使得起着与STL iterator一样作用的运算者函数。

			\~japanese
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

			\~chinese
			使得起着与STL iterator一样作用的运算者函数。

			\~japanese
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

			\~chinese
			使得起着与STL iterator一样作用的运算者函数。

			\~japanese
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

			\~chinese
			使得起着与STL iterator一样作用的运算者函数。

			\~japanese
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

			\~chinese
			使得起着与STL iterator一样作用的运算者函数。

			\~japanese
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

			\~chinese
			使得起着与STL iterator一样作用的运算者函数。

			\~japanese
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

		\~chinese
		起着与STL的同名方法相同的作用。

		\~japanese
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

		\~chinese
		起着与STL的同名方法相同的作用。

		\~japanese
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

		\~chinese
		起着与STL的同名方法相同的作用。

		\~japanese
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

			\~chinese
			使得起着与STL reverse_iterator 相同作用的运算者函数。

			\~japanese
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

			\~chinese
			使得起着与STL reverse_iterator 相同作用的运算者函数。

			\~japanese
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

			\~chinese
			使得起着与STL reverse_iterator 相同作用的运算者函数。

			\~japanese
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

			\~chinese
			使得起着与STL reverse_iterator 相同作用的运算者函数。

			\~japanese
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

			\~chinese
			使得起着与STL reverse_iterator 相同作用的运算者函数。

			\~japanese
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

			\~chinese
			使得起着与STL reverse_iterator 相同作用的运算者函数。

			\~japanese
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

			\~chinese
			使得起着与STL reverse_iterator 相同作用的运算者函数。

			\~japanese
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

			\~chinese
			使得起着与STL reverse_iterator 相同作用的运算者函数。

			\~japanese
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

		\~chinese
		起着与STL同名方法相同的的作用。

		\~japanese
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

		\~chinese
		起着与STL同名方法相同的的作用。

		\~japanese
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

		\~chinese
		起着与STL同名方法相同的的作用。

		\~japanese
		\~ 
		*/
		inline const_iterator begin() const
		{
			const_iterator ret;
			ret.m_pos = GetStartPosition();
			ret.m_owner = (CFastMap*)this;

			return ret;
		}

		/** 
		\~korean
		STL의 동명 메서드와 같은 역할을 한다.

		\~english
		Performs a role as same name method of STL

		\~chinese
		起着与STL同名方法相同的的作用。

		\~japanese
		\~
		 */
		inline const_iterator end() const
		{
			const_iterator ret;
			ret.m_pos=NULL;
			ret.m_owner=(CFastMap*)this;

			return ret;
		}

		inline const_reverse_iterator rbegin() const
		{
			const_reverse_iterator ret;
			ret->m_pos = GetEndPosition();
			ret->m_owner = (CFastMap*)this;

			return ret;
		}

		inline const_reverse_iterator rend() const
		{
			const_reverse_iterator ret;
			ret->m_pos = NULL;
			ret->m_owner = (CFastMap*)this;

			return ret;
		}
#endif


		/** 
		\~korean
		STL의 동명 메서드와 같은 역할을 한다.

		\~english
		Performs a role as same name method of STL

		\~chinese
		起着与STL同名方法相同的的作用。

		\~japanese
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

		\~chinese
		起着与STL同名方法相同的的作用。

		\~japanese
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

		\~chinese
		起着与STL同名方法相同的的作用。

		\~japanese
		\~
		 */
		iterator find(const K& key)
		{
			iterator ret;
			ret.m_owner=this;
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

#ifndef PROUDNET_NO_CONST_ITERATOR
		/** 
		\~korean
		STL의 동명 메서드와 같은 역할을 한다.

		\~english
		Performs a role as same name method of STL

		\~chinese
		起着与STL同名方法相同的的作用。

		\~japanese
		\~
		 */
		const_iterator find(const K& key) const
		{
			const_iterator ret;
			ret.m_owner=(CFastMap*)this;
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
		/** 
		\~korean
		내부 버퍼로 CFastHeap 을 사용한다.
		\param heap CFastHeap 포인터

		\~english TODO:translate needed.

		\~chinese
		使用 CFastHeap%为内部buffer。
		\param heap CFastHeap%指针。

		\~japanese
		*/
		void UseFastHeap(CFastHeap* heap)
		{
			if(heap == NULL)
			{
				ThrowInvalidArgumentException();
			}
			if(GetCount() > 0)
				ThrowException(CannotUseFastHeapForFilledCollectionErrorText);
			m_refHeap = heap;
		}

		inline CFastHeap* GetRefFastHeap() { return m_refHeap; }
	};

#ifdef ENABLE_REHASH_COUNT
	extern PROUDNET_VOLATILE_ALIGN int32_t FastMap_RehashCount;
#endif // ENABLE_REHASH_COUNT

	/**  @} */
}

//#pragma pack(pop)