#pragma once

#include "../include/Ptr.h"

namespace Proud
{
	template<typename T>
	inline void container_iterate_and_delete(T &cont)
	{
		for (typename T::iterator i = cont.begin();i != cont.end();i++)
		{
			if ((*i) != NULL)
				delete (*i);
		}
	}

	template<typename T>
	inline void map_iterate_and_delete(T &cont)
	{
		for (typename T::iterator i = cont.begin();i != cont.end();i++)
		{
			if (i->second != NULL)
				delete i->second;
		}
	}

	class CFastHeap;
	template<typename T>
	inline void map_iterate_and_delete_FastHeap(T &cont, CFastHeap* fastHeap)
	{
		for (typename T::iterator i = cont.begin();i != cont.end();i++)
		{
			if (i->GetSecond() != NULL)
				FastHeap_DeleteInstance(fastHeap, i->GetSecond());
		}
	}

	template<typename M, typename K, typename V>
	void insert(M &m, const K &k, const V &v)
	{
		m.insert(M::value_type(k, v));
	}


	//// (key,object pointer) map class이다.
	//// map과는 달리, 이 객체가 파괴될 때 pointer value로 물려있는 object를 delete한다.
	//// 편의를 위한 몇 가지 메서드를 제공한다.
	//template<typename K,typename V>
	//class ptrmap:public map<K,V*>
	//{
	//public:
	//	//V* find(const K& key)
	//	//{
	//	//	__super::iterator i=__super::find(key);
	//	//	if(i==end())
	//	//		return NULL;
	//	//	else
	//	//		return i->second;
	//	//}
	//	~ptrmap()
	//	{
	//		for(iterator i=begin();i!=end();i++)
	//		{
	//			if(i->second!=NULL)
	//				delete i->second;
	//		}
	//	}
	//};
	//

#if defined(_WIN32)    
	/** 편의 함수로,map에 추가한다. */
	template<typename A, typename B>
	inline typename map<A, B>::iterator map_insert(map<A, B> &m, const A& a, const B& b)
	{
		pair<typename map<A, B>::iterator, bool> x = m.insert(map<A, B>::value_type(a, b));
		return x.first;
	}


	/** ( key, object pointer ) map에서 특정 key가 가리키는 value를 직접 구한다.
	key가 발견되지 않으면 NULL을 리턴한다. */
	template<typename K, typename V>
	inline V* ptrmap_find(const map<K, V*> &m, const K &key)
	{
		typename map<K, V*>::const_iterator i = m.find(key);
		if (i == m.end())
			return NULL;
		else
			return i->second;
	}

	template<typename K, typename V>
	inline RefCount<V> smartptrmap_find(const map<K, RefCount<V> > &m, const K &key)
	{
		typename map<K, RefCount<V> >::const_iterator i = m.find(key);
		if (i == m.end())
			return RefCount<V>();
		else
			return i->second;
	}

	/** smartptrmap의 각 항목의 clone 객체를 만든다.
	value는 Clone() 메서드를 구현해야 한다. */
	template<typename K, typename V>
	inline void smartptrmap_clone(map<K, RefCount<V> > &to, map<K, RefCount<V> > &from)
	{
		to.clear();
		for (typename map<K, RefCount<V> >::iterator i = from.begin();i != from.end();i++)
		{
			RefCount<V> cloned = i->second->Clone();
			to.insert(map<K, RefCount<V> >::value_type(i->first, cloned));
		}
	}

	template<typename V>
	inline void smartptrvector_clone(vector<RefCount<V> > &to, vector<RefCount<V> > &from)
	{
		to.clear();
		for (typename vector<RefCount<V> >::iterator i = from.begin();i != from.end();i++)
		{
			RefCount<V> cloned = (*i)->Clone();
			to.push_back(cloned);
		}
	}

	// 
	// template<typename V,AllocType AllocT>
	// inline void smartptrvector_clone(vector<RefCount<V> > &to, vector<RefCount<V> > &from)
	// {
	// 	to.clear();
	// 	for (vector<RefCount<V> >::iterator i = from.begin();i != from.end();i++)
	// 	{
	// 		RefCount<V> cloned = (*i)->Clone();
	// 		to.push_back(cloned);
	// 	}
	// }


	template<typename K, typename V>
	inline void ptrmap_insert(map<K, V*> &m, const K &k, V* v)
	{
		m.insert(map<K, V*>::value_type(k, v));
	}

	// 객체에 대해 delete는 하지 않는다.
	template<typename K, typename V>
	inline void ptrmap_erase(map<K, V*> &m, const K &k)
	{
		typename map<K, V*>::iterator i = m.find(k);
		if (i != m.end())
			m.erase(i);
	}

	// 객체에 대해 delete는 하지 않는다.
	template<typename K, typename V>
	inline void ptrmap_erase_and_delete(map<K, V*> &m, const K &k)
	{
		typename map<K, V*>::iterator i = m.find(k);
		if (i != m.end())
		{
			delete i->second;
			m.erase(i);
		}
	}

	template<typename K, typename V>
	inline void FastMap_erase_and_delete(CFastMap<K, V*> &m, const K &k)
	{
		typename CFastMap<K, V*>::iterator i = m.find(k);
		if (i != m.end())
		{
			delete i->second;
			m.erase(i);
		}
	}

	template<typename M, typename K>
	bool map_erase_if_exists(M &m, const K &k)
	{
		typename M::iterator old = m.find(k);
		if (old != m.end())
		{
			m.erase(old);
			return true;
		}
		return false;
	}

	template<typename M, typename K>
	bool set_erase_if_exists(M &m, const K &k)
	{
		typename M::iterator old = m.find(k);
		if (old != m.end())
		{
			m.erase(old);
			return true;
		}
		return false;
	}

	template<typename K>
	inline bool set_exists(std::set<K> &s, K &val)
	{
		typename std::set<K>::iterator i = s.find(val);
		return i != s.end();
	}

	template<typename K, typename M>
	inline bool map_exists(M &s, K &val)
	{
		typename M::iterator i = s.find(val);
		return i != s.end();
	}
#endif // __MACH__

	// vector, list 등 단일체(unary)의 collection에 대한 루프 편의 매크로
	//#define FOREACH_UNARY(i,c) for(i._Mybase)???

// 	/** vector에 몇 가지 기능이 추가되고 오버라이드된 객체 */
// 	template<typename T>
// 	class ext_vector: public std::vector<T>
// 	{
// 	public:
// 		inline void insert(const T &item)
// 		{
// 			push_back(item);
// 		}
// 
// 		iterator find(const T &item)
// 		{
// 			return ::find(begin(), end(), item);
// 		}
// 
// 		// 배열에서, 맨 뒤의 것을 i가 가리키는 곳으로 옮기고 맨 뒤의 것을 제거한다.
// 		inline void eraseAndPullLast(iterator i)
// 		{
// 			if (size() == 1)
// 				erase(i);
// 			else
// 			{
// 				*i = back();
// 				resize(size() - 1);
// 			}
// 		}
// 	};

	/** collection이 텅 빌때까지 첫 항목에 대한 delete를 한다.
	이건 collection item이 파괴될 때 self unregister를 하는 경우에 유용하다. */
	template<typename T>
	void DeleteUntilEmpty(T &collection)
	{
		while (collection.empty() == false)
		{
			delete *(collection.begin());
		}
	}

	template<typename A,typename B>
	bool ArrayHasItem(const A& array, const B& item)
	{
		for(int i=0;i<(int)array.GetCount();i++)
		{
			if(array[i] == item)
				return true;
		}
		return false;
	}

	template<typename A,typename B>
	bool ArrayHasItem(const A* array, intptr_t arrayLength, const B& item)
	{
		for(intptr_t i=0;i<arrayLength;i++)
		{
			if(array[i] == item)
				return true;
		}
		return false;
	}

	// 맵이 항목을 갖고 있으면 true
	// 주의: value를 루프 돌면서 찾으므로 느리다. 
 	template<typename T, typename V>
 	bool ContainsValue(const T& map, const V& value)
 	{
 		for (typename T::const_iterator i = map.begin(); i != map.end(); i++)
 		{
 			if (i->GetSecond() == value)
 				return true;
 		}
 		return false;
 	}

	/* vs2003, non-vs를 위해서 있는 매크로

	사용예
	PN_FOREACH(std::list<int>, myList, i)
	{
	cout<<(*i);
	}
	*/
#define PN_FOREACH(TYPE, VAR, i) for(TYPE::iterator i=VAR.begin();i!=VAR.end();i++)

	

	}
