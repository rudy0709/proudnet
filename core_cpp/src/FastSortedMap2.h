#pragma once

#include <map>
#include <cstddef>
#include "../include/ListNode.h"
#include "ThreadUnsafeAllocator.h"

namespace Proud
{

	/* 성능상 장단점 및 구현 내용에 대해서는, 위키 'FastSortedMap2 클래스' 참고.
	언젠가는 std::map을 의존하지 말고, 자체구현으로 바꾸자.
	INDEX: int or int64_t
	*/
	template<typename KEY, typename VALUE, typename INDEX>
	class CFastSortedMap2
	{
	public:
		class iterator;
		class const_iterator;
		class reverse_iterator;

		friend class iterator;
		friend class const_iterator;
		friend class reverse_iterator;
	private:
		class ValueNode;

		// 바퀴 재발명 대신에 C++ 표준 sorted map을 사용한다.
		// 이것 때문에 msvc 버전호환이 안되므로 유저에게 노출하지 못한다.
		typedef std::map < KEY, ValueNode, std::less<KEY>, ThreadUnsafeAllocator<std::pair<const typename KEY, ValueNode> > > StdMap;
		StdMap m_stdMap;

		// std::map은 red-black tree + tune variation으로 구현되기 마련이다.
		// 따라서 iterator++은 O(1)~O(n)의 시간복잡도를 가진다.
		// 게임개발 특성상 iteration이 잦은 상황에서는 이는 불리하다.
		// 따라서 tree node간에 linked list를 만들어 두자. 그러면 iteration을 O(1)에 끝내게 된다.		
		class ValueNode :public CListNode<ValueNode>
		{
		public:
			// 사용자가 넣은 key-value 중 value
			VALUE m_value;

			// 자가보유라는 괴상한 형태지만, 아무튼 map의 value가 node를 직접 가리키는 std::map::iterator를 가진다. std::map을 가져다쓰면서 std::map의 느린 iteration문제를 해결한다.
			typename StdMap::iterator m_stdIter;

			// 이렇게 해놔야 컴파일러가 value copy를 최소화한다.
			ValueNode(const VALUE& newVal) : m_value(newVal) {}
		};

		// iterate를 할 때는 이것을 갖고 한다.
		typename ValueNode::CListOwner m_nodeList;


	public:

		// 사용자는 iterator로서 주로 이것을 씁니다.
		class iterator
		{
			// iterator 내부에는 std::map::iterator를 쥔다. linked list는 std::map 안의 value에 있으므로 iterator가 linked list 자체관련해서 뭘 가질건 없다.
			typename StdMap::iterator m_stdIter;
			CFastSortedMap2* m_pOwner;
			friend typename CFastSortedMap2;
		public:

			iterator() :m_pOwner(nullptr) {}

			// linked list setup을 하는 메인 함수다. insert 함수에 의해 호출된다. 
			// 생성자가 일을 많이 하면 나쁘다는 코딩규칙에는 위배되지만, 이렇게 해야 사용자가 입력하는 key-value 중 value의 assign 연산을 최소화한다. 자세한건 콜러(add, find 함수 등) 참고.
			// stdIter: insert 함수에 의해 지금 막 추가된 key를 가리킨다.
			// pOwner: 소유자
			// addSuccess: 기존 키가 없어서 새 키를 잘 넣었으면 true
			iterator(const typename StdMap::iterator& stdIter, CFastSortedMap2* pOwner)
			{
				m_pOwner = pOwner;
				m_stdIter = stdIter;
			}

			/** STL iterator의 ++ 연산자와 같은 역할을 합니다. 전위증감연산자입니다. */
			iterator& operator++()
			{
				assert(m_stdIter != m_pOwner->m_stdMap.end());

				// std::map의 ++는 O(n)이다. stdIter의 것을 ++하는게 아니고, linked list에 있는 것으로 다음으로 넘어간다.
				auto nextNode = m_stdIter->second.GetNext();
				if (nextNode == nullptr)
				{
					// 끝에 도달했다. m_stdIter 도 변경한다.
					m_stdIter = m_pOwner->m_stdMap.end();
				}
				else
				{
					// nextNode가 가진 iterator로 바꾼다.
					m_stdIter = nextNode->m_stdIter;
				}
				return *this;
			}

			/** STL iterator의 -- 연산자와 같은 역할을 합니다. 전위증감연산자입니다. */
			iterator& operator--()
			{
				assert(m_stdIter != m_pOwner->m_stdMap.begin());

				if (m_stdIter == m_pOwner->m_stdMap.end())
				{
					// 유효한 값을 가리키는 iter가 아니므로, linked list를 액세스하지 말자.
					// 대신 last node를 그냥 찾아서 배치시키자.
					auto lastNode = m_pOwner->m_nodeList.GetLast();
					assert(lastNode != nullptr); // 텅빈 map이었다면 애당초 여기까지 안온다.
					m_stdIter = lastNode->m_stdIter;
				}
				else
				{
					// std::map의 ++는 O(n)이다. stdIter의 것을 ++하는게 아니고, linked list에 있는 것으로 다음으로 넘어간다.
					auto prevNode = m_stdIter->second.GetPrev();
					if (prevNode == nullptr)
					{
						// 맨앞에 도달했다. m_stdIter 도 변경한다.
						m_stdIter = m_pOwner->m_stdMap.begin();
					}
					else
					{
						// nextNode가 가진 iterator로 바꾼다.
						m_stdIter = prevNode->m_stdIter;
					}
				}
				return *this;
			}

			/** STL iterator의 ++ 연산자와 같은 역할을 합니다. 후위증감연산자입니다. */
			iterator operator++(int)
			{
				iterator ret = *this;
				++* this;
				return ret;
			}
			/** STL iterator의 -- 연산자와 같은 역할을 합니다. 후위증감연산자입니다. */
			iterator operator--(int)
			{
				iterator ret = *this;
				--* this;
				return ret;
			}


			bool operator==(const iterator& rhs) const
			{
				return rhs.m_pOwner == m_pOwner && rhs.m_stdIter == m_stdIter;
			}

			bool operator!=(const iterator& rhs) const
			{
				return rhs.m_pOwner != m_pOwner || rhs.m_stdIter != m_stdIter;
			}

			const KEY& GetFirst() const
			{
				return m_stdIter->first;
			}

			VALUE& GetSecond() const
			{
				return m_stdIter->second.m_value;
			}

			// 이것이 있어야 for(auto a:map) 구문을 쓸 수 있다.
			inline const iterator& operator*() const
			{
				return *this;
			}

			inline const iterator* operator->() const
			{
				return this;
			}

		};

		// 읽기 전용에 대해서는 const_iterator를 쓰세요. STL의 그것과 똑같습니다.
		class const_iterator
		{
			typename StdMap::const_iterator m_stdIter;
			const CFastSortedMap2* m_pOwner;
		public:

			const_iterator() :m_pOwner(nullptr) {}

			const_iterator(const typename StdMap::const_iterator& stdIter, const CFastSortedMap2* pOwner)
			{
				m_stdIter = stdIter;
				m_pOwner = pOwner;
			}

			/** STL iterator의 ++ 연산자와 같은 역할을 합니다. 전위증감연산자입니다. */
			const_iterator& operator++()
			{
				assert(m_stdIter != m_pOwner->m_stdMap.end());

				// std::map의 ++는 O(n)이다. stdIter의 것을 ++하는게 아니고, linked list에 있는 것으로 다음으로 넘어간다.
				auto nextNode = m_stdIter->second.GetNext();
				if (nextNode == nullptr)
				{
					// 끝에 도달했다. m_stdIter 도 변경한다.
					m_stdIter = m_pOwner->m_stdMap.end();
				}
				else
				{
					// nextNode가 가진 const_iterator로 바꾼다.
					m_stdIter = nextNode->m_stdIter;
				}
				return *this;
			}

			/** STL iterator의 -- 연산자와 같은 역할을 합니다. 전위증감연산자입니다. */
			const_iterator& operator--()
			{
				assert(m_stdIter != m_pOwner->begin());

				// std::map의 ++는 O(n)이다. stdIter의 것을 ++하는게 아니고, linked list에 있는 것으로 다음으로 넘어간다.
				auto prevNode = m_stdIter->second.GetPrev();
				if (prevNode == nullptr)
				{
					// 맨앞에 도달했다. m_stdIter 도 변경한다.
					m_stdIter = m_pOwner->begin();
				}
				else
				{
					// nextNode가 가진 const_iterator로 바꾼다.
					m_stdIter = prevNode->m_stdIter;
				}
				return *this;
			}

			/** STL iterator의 ++ 연산자와 같은 역할을 합니다. 후위증감연산자입니다. */
			const_iterator operator++(int)
			{
				const_iterator ret = *this;
				++* this;
				return ret;
			}
			/** STL iterator의 -- 연산자와 같은 역할을 합니다. 후위증감연산자입니다. */
			const_iterator operator--(int)
			{
				const_iterator ret = *this;
				--* this;
				return ret;
			}


			bool operator==(const const_iterator& rhs) const
			{
				return rhs.m_pOwner == m_pOwner && rhs.m_stdIter == m_stdIter;
			}

			bool operator!=(const const_iterator& rhs) const
			{
				return rhs.m_pOwner != m_pOwner || rhs.m_stdIter != m_stdIter;
			}

			/** std.map.iterator.first와 같은 역할을 합니다. */
			const KEY& GetFirst() const
			{
				return m_stdIter->first;
			}

			/** std.map.iterator.second와 같은 역할을 합니다. */
			const VALUE& GetSecond() const
			{
				return m_stdIter->second.m_value;
			}

			// 이것이 있어야 for(auto a:map) 구문을 쓸 수 있다.
			inline const const_iterator& operator*() const
			{
				return *this;
			}

			inline const const_iterator* operator->() const
			{
				return this;
			}

		};

		// reverse_iterator는 위 iterator와 달리 O(1)이 아니다. std::map과 성능이 똑같다.
		class reverse_iterator
		{
			typename StdMap::reverse_iterator m_stdIter;
		public:

			reverse_iterator()
			{
			}

			reverse_iterator(const typename StdMap::reverse_iterator& rhs) :m_stdIter(rhs)
			{
			}

			reverse_iterator& operator++()
			{
				m_stdIter++;
				return *this;
			}

			reverse_iterator& operator--()
			{
				m_stdIter--;
				return *this;
			}

			reverse_iterator operator++(int)
			{
				reverse_iterator ret = *this;
				++* this;
				return ret;
			}

			reverse_iterator operator--(int)
			{
				reverse_iterator ret = *this;
				--* this;
				return ret;
			}

			bool operator==(const reverse_iterator& rhs) const
			{
				return rhs.m_stdIter == m_stdIter;
			}

			bool operator!=(const reverse_iterator& rhs) const
			{
				return rhs.m_stdIter != m_stdIter;
			}

			const KEY& GetFirst() const
			{
				return m_stdIter->first;
			}

			const VALUE GetSecond() const
			{
				return m_stdIter->second.m_value;
			}

			// 이것이 있어야 for(auto a:map) 구문을 쓸 수 있다.
			inline const reverse_iterator& operator*() const
			{
				return *this;
			}

			inline const reverse_iterator* operator->() const
			{
				return this;
			}

		};

		// #TODO-FastSortedMap const_reverse_iterator는 당장 쓰지 않아서 만들지 않았다. 나중에 필요하면 만들자.

		// #TODO-FastSortedMap std.map의 모든 함수를 여기다 만들지는 않았다. 필요시 그때그때마다 여러분들이 추가해주세요.

		/** \copydoc Proud::CFastMap::Add  */
		bool Add(const KEY& key, const VALUE& value)
		{
			return insert(key, value).second;
		}

		/** \copydoc Proud::CFastMap::Remove */
		bool Remove(const KEY& key)
		{
			// ~CListNode에서 알아서 linked list 정리를 한다. 따라서 여기서는 달리 할 것이 없다.
			return m_stdMap.erase(key);
		}
		bool RemoveKey(const KEY& key)
		{
			return Remove(key);
		}

		///** \copydoc Proud::CFastMap::Remove */
		//bool erase(const KEY& key)
		//{
		//	return Remove(key);
		//}

		/** \copydoc Proud::CFastMap::Remove */
		iterator erase(const iterator& iter)
		{
			assert(iter.m_pOwner == this);
			// ~CListNode에서 알아서 linked list 정리를 한다. 따라서 여기서는 달리 할 것이 없다.

			return iterator(m_stdMap.erase(iter.m_stdIter), this);
		}

		//reverse_iterator erase(reverse_iterator iter)
		//{
		//	return m_stdMap.erase(iter.m_stdIter);
		//}


		/** \copydoc Proud.CFastMap.KeysToArray */
		void KeysToArray(CFastArray<KEY>& output) const
		{
			output.Clear();
			output.SetCount(GetCount());
			int c = 0;

			for (auto i = begin(); i != end(); i++)
			{
				output[c] = i.GetFirst();
				c++;
			}
		}

		template<typename T>
		void KeysToArray(T* outputArray, int arraySize) const
		{
			assert(arraySize == GetCount());

			int c = 0;
			for (auto i = begin(); i != end(); i++)
			{
				outputArray[c] = (T)i.GetFirst();
				c++;
			}
		}

		/** \copydoc Proud.CFastMap.ValuesToArray */
		void ValuesToArray(CFastArray<VALUE>& output) const
		{
			output.Clear();
			output.SetCount(GetCount());

			int c = 0;
			for (auto i = begin(); i != end(); i++)
			{
				output[c] = i.GetSecond();
				c++;
			}
		}

		/** \copydoc Proud.CFastMap.size */
		INDEX size() const { return (INDEX)m_stdMap.size(); }

		/** \copydoc Proud.CFastMap.size */
		INDEX GetCount() const { return (INDEX)m_stdMap.size(); }


		/** std::map::insert처럼 처리한다. pair를 리턴한다.
		pair.first는 추가된 것을 가리키는 iterator이고
		second는 추가 성공시 true, 이미 key가 있어서 추가를 안했으면 false이다. */
		std::pair<iterator, bool> insert(const KEY& key, const VALUE& value)
		{
			// 이미 key가 있는 경우 newItem.second is false이고, map에는 아무 변화도 안 일어난다.
			auto stdMapInsertResult = m_stdMap.insert(StdMap::value_type(key, ValueNode(value)));

			auto& stdIter = stdMapInsertResult.first;
			if (stdMapInsertResult.second == true)	 // if newly added
			{
				stdIter->second.m_stdIter = stdIter;		// ValueNode가 first를 가리키는 iterator를 멤버변수로 가지는데, 그걸 업데이트한다. 괴상하긴 하지만 이걸 여기저기서 사용한다. 아 글쎄, 궁극적인 방법은 우리가 자체적으로 sorted map을 구현하는 것이라니까요.

				// 인접한 key에 대한 linked list를 구축
				assert(stdIter->second.GetPrev() == nullptr);
				if (stdIter == m_stdMap.begin()) // 추가한 키가 맨 앞이면
				{
					// linked list에서도 맨 앞의 것을 가리키게 바꾸자.
					m_nodeList.Insert(nullptr, &stdIter->second);
				}
				else
				{
					// 바로 앞 키와 바로 뒤 키 사이에 끼워넣자.
					auto stdPrevIter = stdIter;
					stdPrevIter--;
					m_nodeList.Insert(&stdPrevIter->second, &stdIter->second);
				}
			}
			return std::pair<iterator, bool>(iterator(stdIter, this), stdMapInsertResult.second);
		}


		/** \copydoc Proud.CFastMap.operator[] */
		inline VALUE& operator[](const KEY& key)
		{
			auto stdIter = m_stdMap.find(key);
			if (stdIter == m_stdMap.end())
			{
				// 새 키를 추가하고 추가한 키의 value의 주소를 리턴한다.
				auto iter = insert(key, VALUE());
				return iter.first.GetSecond();
			}
			else
			{
				// 기존 키의 주소를 리턴한다.
				return stdIter->second.m_value;
			}
		}

		/** \copydoc Proud.CFastMap.TryGetValue */
		bool TryGetValue(const KEY& key, VALUE& output) const
		{
			auto i = m_stdMap.find(key);
			if (i == m_stdMap.end())
				return false;

			output = i->second.m_value;
			return true;
		}

		// Value가 RefCount나 shared_ptr 등과 같은 smart_pointer일 때,
		// Key에 해당하는 Value의 Raw Pointer를 바로 가져오는 함수이다.
		template<typename RAWPTR>
		inline bool TryGetRawPtrValue(const KEY& key, RAWPTR& rawPtrOfValue) const
		{
			auto i = m_stdMap.find(key);
			if (i == m_stdMap.end())
				return false;

			rawPtrOfValue = i->second.m_value.get();
			return true;
		}

		inline iterator find(const KEY& key)
		{
			auto i = m_stdMap.find(key);
			return iterator(i, this);
		}

		/** \copydoc Proud.CFastMap.ContainsKey */
		bool ContainsKey(const KEY& key) const
		{
			auto i = m_stdMap.find(key);
			return (i != m_stdMap.end());
		}

		/** \copydoc Proud.CFastMap.IsEmpty */
		inline bool IsEmpty() const
		{
			return m_stdMap.empty();
		}

		/** \copydoc Proud.CFastMap.begin */
		inline iterator begin()
		{
			return iterator(m_stdMap.begin(), this);
		}

		/** \copydoc Proud.CFastMap.begin */
		inline const_iterator begin() const
		{
			return const_iterator(m_stdMap.begin(), this);
		}

		/** \copydoc Proud.CFastMap.Clear */
		void Clear()
		{
			clear();
		}

		/** \copydoc Proud.CFastMap.Clear */
		void clear()
		{
			m_stdMap.clear();
			assert(m_nodeList.GetFirst() == nullptr);
			assert(m_nodeList.GetLast() == nullptr);
		}

		// 버그 감시용. 릴리즈빌드에서 두는 이유는 PNTest가 릴리즈빌드로도 이걸 해야 하니까.
		// 실패시 에러문자열이 출력되고 false를 리턴한다.
		// (사용자에게 노출할 필요는 없는 함수이다.)
		bool CheckConsistency(String& outErrorText) const
		{
			auto i = m_stdMap.begin();
			auto n = m_nodeList.GetFirst();
			while (true)
			{
				if (i == m_stdMap.end())
				{
					if (n != nullptr)
					{
						outErrorText = (_PNT("linked list보다 std.map의 길이가 더 깁니다."));
						return false;
					}
					return true;
				}
				if (n == nullptr)
				{
					if (i != m_stdMap.end())
					{
						outErrorText = (_PNT("linked list보다 std.map의 길이가 더 짧습니다."));
						return false;
					}
					return true;
				}
				if (i->first != n->m_stdIter->first)
				{
					outErrorText = (_PNT("linked list와 std.map의 내용이 서로 다릅니다."));
					return false;
				}
				i++;
				n = n->GetNext();
			}
			return true;
		}

		const_iterator lower_bound(const KEY& key) const
		{
			return const_iterator(m_stdMap.lower_bound(key), this);
		}

		iterator lower_bound(const KEY& key)
		{
			return iterator(m_stdMap.lower_bound(key), this);
		}

		const_iterator upper_bound(const KEY& key) const
		{
			return const_iterator(m_stdMap.upper_bound(key), this);
		}

		iterator upper_bound(const KEY& key)
		{
			return iterator(m_stdMap.upper_bound(key), this);
		}


		/** \copydoc Proud.CFastMap.end */
		inline iterator end()
		{
			return iterator(m_stdMap.end(), this);
		}

		/** \copydoc Proud.CFastMap.end */
		inline const_iterator end() const
		{
			return const_iterator(m_stdMap.end(), this);
		}

		// end()와 달리, end 리턴값의 바로 앞 것을 가리키는 iterator를 리턴한다.
		inline iterator back()
		{
			assert(IsEmpty() == false);
			iterator ret = this->end();
			ret--;
			return ret;
		}

		/** \copydoc Proud.CFastMap.rbegin */
		inline reverse_iterator rbegin()
		{
			return reverse_iterator(m_stdMap.rbegin());
		}

		/** \copydoc Proud.CFastMap.rend */
		inline reverse_iterator rend()
		{
			return reverse_iterator(m_stdMap.rend());
		}
	};
}