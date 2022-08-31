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

#include "LowFragMemArray.h"
#include "../include/FakeClrBase.h"
#include "quicksort.h"

/* Collection관련 유틸리티 모듈
*/
namespace Proud
{

	template<typename T, typename INDEXTYPE>
	inline INDEXTYPE UnionDuplicates(T* obj, INDEXTYPE objSize)
	{
		// 대부분의 경우 size is zero이므로 이런 검사를 하는 것이 성능상 이익.
		if (0 == objSize)
		{
			return 0;
		}

		// 정렬. 이러면 중복 값들은 배열 안에서 서로 인접하게 된다.
		HeuristicQuickSort(obj, objSize, (INDEXTYPE)100);
		//QuickSort(obj.GetData(), objSize);
		//std::sort(obj.begin(), obj.end());

		// 일단 정렬을 하게 되면 중복값은 서로 인접합니다.
		// 이러면 굳이 MALLOCA를 안쓰고도 배열 안에서 중복값을 쉽게 제거할 수 있습니다.
		// 0부터 시작하는 커서를 하나 두고 배열을 끝까지 뒤지면서 커서가 가리키는 것과 다른 값이
		// 발견되면 커서 바로 우측에 그 다른 값을 옮기고 커서를 한칸 옮겨주면 되죠.
		// 이런 식으로 끝까지 돌면 커서가 가리키는 곳 바로 앞 까지가 union된 배열의 크기가 됩니다.
		// 이러한 식으로 아래 소스를 수정 바랍니다. 즉 MALLOCA를 아예 쓸 일이 없게 만듭시다.
		// 어차피 이거 꼭 해야 함. MALLOCA를 안쓰는 방식으로 모든 소스를 고칠 것이므로.

		// 중복 항목을 옮기기
		INDEXTYPE c = 1;
		for (INDEXTYPE i = 1; i < objSize; i++)
		{
			if (obj[i] != obj[c - 1])
			{
				if (c != i)
				{
					obj[c++] = MOVE_OR_COPY(obj[i]);	// T가 shared_ptr을 가진 타입인 경우 이렇게 해주어야 속도가 빠르다.
				}
				else
				{
					c++;
				}
			}
		}

		// 중복처리 하고 남은 잔여에 대해서 의도적 클리어를 한다.
		// plain type이면 이것은 아무것도 안하겠지만 생성자나 파괴자를 가진 타입은 이렇게 해주어야 한다.
		for (INDEXTYPE i = c; i < objSize; i++)
		{
			obj[i] = MOVE_OR_COPY(T());
		}

		return c;
	}

	template<typename T_ARRAY, typename T, typename INDEXTYPE>
	inline void UnionDuplicates(T_ARRAY& obj)
	{
		INDEXTYPE c = UnionDuplicates<T, INDEXTYPE>(obj.GetData(), obj.GetCount());

		// 원본에 엎어쓰기
		obj.SetCount(c);
	}

	// 배열은 이미 정렬되어있어야함에 주의할것. 찾으면 true
	// 코드 수정
	template<typename T, typename INDEXTYPE>
	bool BinarySearch(const T* array, INDEXTYPE num, const T& key)
	{
		INDEXTYPE upper, lower, mid;
		lower = 0;
		upper = num-1;

		while (lower <= upper)
		{
			mid = (lower + upper) / 2;

			if (array[mid] > key)
			{
				upper = mid - 1;
			}
			else if (array[mid] < key)
			{
				lower = mid + 1;
			}
			else
			{
				return true;
			}
		}

		return false;
	}
}
