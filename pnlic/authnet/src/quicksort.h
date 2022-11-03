#pragma once 

#ifndef __MACH__
#include <stack> // Xcode 사용자는, 이것을 cpp에서 include 하세요. Xcode는 STL을 header file에서의 include를 제약합니다.
#endif

namespace Proud
{
	template<typename T>
	inline void quickswap(T& left, T& right)
	{
		T temp;
		temp = left;
		left = right;
		right = temp;
	}

	/**
	\~korean
	QuickSort 정렬을 합니다. ( 예 QuickSort(array, (int)array.Count); )
	\param array 정렬할 배열
	\param hi 정렬범위 중 마지막 index +1
	\param lo 정렬범위 중 처음 배열 index

	\~english 

	\~chinese
	进行QuickSort排序。（例QuickSort(array, (int)array.Count);）
	\param array 要排序的数组。
	\param hi 排序范围中最后的index +1。
	\param lo 排序范围中第一个数组index。

	\~japanese
	\~
	*/
	template<typename T, typename INDEXTYPE>
	void QuickSort(T *array, INDEXTYPE hi, INDEXTYPE lo = 0)
	{
		while (hi > lo)
		{
			INDEXTYPE i = lo;
			INDEXTYPE j = hi;
			do
			{
				while (array[i] < array[lo] && i < j)
					i++;
				while (array[lo] < array[--j])
					;
				if (i < j)
				{
					quickswap(array[i], array[j]);
				}
			} while (i < j);
			quickswap(array[lo], array[j]);

			if (j - lo > hi - (j + 1)) {
				QuickSort(array, j - 1, lo);
				lo = j + 1;
			}
			else{
				QuickSort(array, hi, j + 1);
				hi = j - 1;
			}
		}
	}

	/** \addtogroup util_group
	*  @{
	*/

	/**
	\~korean
	QuickSort 정렬을 합니다. ( 예 QuickSort(array, MySQL, (int)array.Count); )
	\param array 정렬할 배열
	\param type 비교할 타입(타입에 따라서 비교 내용이 틀려질 경우 사용)
	\param hi 정렬범위 중 마지막 index +1
	\param lo 정렬범위 중 처음 배열 index

	\~english TODO:translate needed.

	\~chinese

	\~japanese
	\~
	*/

	template<typename T, typename INDEXTYPE, typename COMPARETYPE>
	void QuickSort(T *array, COMPARETYPE type, INDEXTYPE hi, INDEXTYPE lo = 0)
	{
		while (hi > lo)
		{
			INDEXTYPE i = lo;
			INDEXTYPE j = hi;
			do
			{
				while (type(array[i], array[lo]) < 0 && i < j)
					i++;
				while (type(array[lo], array[--j]) < 0)
					;
				if (i < j)
				{
					quickswap(array[i], array[j]);
				}
			} while (i < j);
			quickswap(array[lo], array[j]);

			if (j - lo > hi - (j + 1)) {
				QuickSort(array, type, j - 1, lo);
				lo = j + 1;
			}
			else{
				QuickSort(array, type, hi, j + 1);
				hi = j - 1;
			}
		}
	}

	/**
	\~korean
	재귀방식을 사용하지 않고 스택을 통하여 QuickSort 정렬을 합니다. ( 예 StacklessQuickSort(array, (int)array.Count); )
	\param array 정렬할 배열
	\param hi 정렬범위 중 마지막 index +1
	\param lo 정렬범위 중 처음 배열 index

	\~english TODO:translate needed.

	\~chinese
	不用再归的方式，通过stack排序QuickSort。（例StacklessQuickSort(array, (int)array.Count);）
	\param array 要排序的数组。
	\param hi 排序范围中最后的index +1。
	\param lo 排序范围中第一个数组index。

	\~japanese
	\~
	*/
	template<typename T>
	void StacklessQuickSort(T *array, int hi, int lo = 0)
	{
		if (hi - lo <= 1)
			return;

		std::stack<int> tempStack;
		T pivot;

		int pivotIndex = lo;
		int leftIndex = pivotIndex;
		int rightIndex = hi - 1;//

		tempStack.push(leftIndex);
		tempStack.push(rightIndex);

		int leftIndexOfSubSet, rightIndexOfSubset;

		while (tempStack.size() > 0)
		{
			rightIndexOfSubset = tempStack.top();
			tempStack.pop();
			leftIndexOfSubSet = tempStack.top();
			tempStack.pop();

			//피봇을 기준으로 분할
			leftIndex = leftIndexOfSubSet + 1;
			pivotIndex = leftIndexOfSubSet;
			rightIndex = rightIndexOfSubset;

			pivot = array[pivotIndex];

			if (leftIndex > rightIndex)
				continue;

			while (leftIndex < rightIndex)
			{
				//포인터를 밀면서 피봇보다 큰경우를 찾는다.
				while ((leftIndex <= rightIndex) && (array[leftIndex] <= pivot))
					leftIndex++;

				//포인터를 내리면서 피봇보다 작은 경우를 찾는다.
				while ((leftIndex <= rightIndex) && (array[rightIndex] >= pivot))
					rightIndex--;

				//작은것은 왼쪽으로 큰것은 오른쪽으로..
				if (rightIndex >= leftIndex)
				{
					std::swap(array[leftIndex], array[rightIndex]);
				}
			}

			//마지막으로 피봇인덱스가 우측 인덱스의 아래에 있고,
			//피봇인덱스의 값이 우측 인덱스의 값보다 크다면 둘을 교환한다.
			if (pivotIndex <= rightIndex && array[pivotIndex] > array[rightIndex])
			{
				std::swap(array[pivotIndex], array[rightIndex]);
			}

			//분할한..정리한곳 이외의 분할한점을 넣어준다.
			if (leftIndexOfSubSet < rightIndex)
			{
				tempStack.push(leftIndexOfSubSet);
				tempStack.push(rightIndex - 1);
			}

			if (rightIndexOfSubset > rightIndex)
			{
				tempStack.push(rightIndex + 1);
				tempStack.push(rightIndexOfSubset);
			}
		}
	}

	/* 선택적 QuickSort.
	재귀가 과도하게 일어나는 것을 막기 위해 배열의 크기에 따라 정렬 방식을 선택한다.
	\param array 배열
	\param count 배열의 크기
	\param threshold 배열의 크기가 이 크기보다 크면 스택을 이용하는 정렬을 한다. */
	template<typename T>
	void HeuristicQuickSort(T *array, int count, int threshold)
	{
		if (count > threshold)
		{
			StacklessQuickSort(array, count);
			return;
		}

		QuickSort(array, count);
	}
}

