// #pragma once
// 
// namespace Proud
// {
// 	// ProudNet client DLL에서 singleton을 관리한다. 따라서 이것은 사용자에게 노출될 수 없다.
// 	//
// 	// 로컬 변수로 T를 만들되 obj-pool을 이용해서 heap access를 최소화한다.
// 	// 로컬 변수로만 이 객체의 인스턴스를 만들 것.
// 	// 배열 객체 등이 효과적. resize cost가 시간이 지나면서 사라지니까.
// 	// 이 클래스를 쓰면 알아서 ShrinkOnNeed도 자동으로 호출되므로 당신에게 좋을거임.
// 	// 주의: 배열 객체를 풀링하는 것이라면 내용물이 남아있으므로 쓰기 전에 가령 ClearAndKeepCapacity를 호출한다던지 해야. 
// 	// 주의: NetCore보다 나중에 파괴되는 객체에 이것을 쓰지 말것. NetCore보다 나중에 파괴됨이 보장되긴 하지만.
// 	template<typename T>
// 	class ClientDllPooledObjectAsLocalVar
// 	{
// 		T* m_obj;
// 	public:
// 		inline ClientDllPooledObjectAsLocalVar()
// 		{
// 			// 이 로컬 변수 생성/파괴는 성능에 민감하고,
// 			// NC or NS 안에서만 사용되므로, GetUnsafeRef를 쓴다.
// 			m_obj = ClientDllClassObjectPool<T>::NewOrRecycle();
// 		}
// 		inline ~ClientDllPooledObjectAsLocalVar()
// 		{
// 			ClientDllClassObjectPool<T>::Drop(m_obj);
// 		}
// 
// 
// 		// 실제 객체를 얻는다.
// 		inline T& Get()
// 		{
// 			return *m_obj;
// 		}
// 	};
// 
// }
// 
// // ProudNet client DLL이 존재하는한 이것을 관리하는 singleton이 유지된다.
// // 
// // 로컬 변수로서 object pooling을 할 때 사용하는 매크로. 로컬 변수 선언시 쓴다.
// // 가급적이면 이 매크로를 쓰자. 두번째 줄에서 &를 빠뜨리는 사고도 있었으니까.
// //
// // Q: 그냥 각 메인 객체가 object pool을 가지면 되지, 굳이 이렇게 singleton을 두나요? 
// // A: 메인A에서 생성하고 메인B에서 버리는 객체가 잦으면, A와 B가 각각 object pool을 가지면 안됩니다. 그런데 A부터 Z까지 있다고 칩시다.
// // 프로그램 덩치가 커지게 되면 pool은 가급적 단일화되어야 합니다. 그래서 singleton을 둡니다.
// // 물론 singleton은 dll unload시 전역변수의 파괴 순서를 지키기 위한 추가 코딩 등 번거롭긴 합니다. 그럼에도 불구하고요.
// #define PNCLIENT_POOLED_LOCAL_VAR(type, name)					\
// 	ClientDllPooledObjectAsLocalVar<type> name##_LV;			\
// 	type& name = name##_LV.Get();
// 
// // BiasManagedPointer.AllocTombstone, FreeTombstone을 구현한다.
// // 중복코드 방지차.
// // NOTE: object pooling과 smart pointer化를 한번에 하는 유틸리티 클래스는 사용자에게 노출시킬 필요가 없으므로 이렇게 template specialization을 한다.
// #define BiasManagedPointer_IMPLEMENT_TOMBSTONE(__Type, __ClearOnDrop) \
// 	template<> \
// 	PROUD_API BiasManagedPointer<__Type, __ClearOnDrop>::Tombstone* BiasManagedPointer<__Type, __ClearOnDrop>::AllocTombstone() \
// 	{ \
// 		typedef  BiasManagedPointer<__Type, __ClearOnDrop>::Tombstone TombstoneType; \
// 		Tombstone* ret = CClassObjectPoolEx<TombstoneType, FavoritePooledObjectType_Tombstone_##__Type>::GetUnsafeRef().NewOrRecycle(); \
// 		return ret; \
// 	} \
// 	template<> \
// 	PROUD_API void BiasManagedPointer<__Type, __ClearOnDrop>::FreeTombstone(Tombstone* tombstone) \
// 	{ \
// 		typedef  BiasManagedPointer<__Type, __ClearOnDrop>::Tombstone TombstoneType; \
// 		CClassObjectPoolEx<TombstoneType, FavoritePooledObjectType_Tombstone_##__Type>::GetUnsafeRef().Drop(tombstone); \
// 	}
// 
// #include "PooledObjects.inl"
// 
// 
// 
