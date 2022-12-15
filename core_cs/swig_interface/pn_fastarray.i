
%extend Proud::CFastArray
{
	inline void Add(T value)
	{
		assert(self);
		self->Add(value);
	}

	inline T Get(int index) throw (Proud::Exception)
	{
		assert(self);
		return (*self)[index];
	}
}

// Swig �������̽� �����Ϸ����� %ignore�� ����� Ŭ���� �� �Լ��� ������ ��󿡼� ���ܽ�ŵ�ϴ�.

%ignore Proud::CannotSetConfigErrorText;
%ignore Proud::CannotUseFastHeapForFilledCollectionErrorText;
%ignore Proud::ShowLibSignatureMismatchError();
%ignore Proud::LibSignature();
%ignore Proud::CFastArray::CFastArray(const T* data, INDEXTYPE count);
%ignore Proud::CFastArray::CFastArray(const CFastArray &src);

%ignore Proud::CFastArray::operator[];
%ignore Proud::CFastArray::InsertRange;
%ignore Proud::CFastArray::UseFastHeap;

%ignore Proud::CFastArray::GetFastHeap;
%ignore Proud::CFastArray::UsingFastAllocator;
%ignore Proud::CFastArray::SetGrowPolicy;
%ignore Proud::CFastArray::GetGrowPolicy;
%ignore Proud::CFastArray::SetMinCapacity;
%ignore Proud::CFastArray::SetCapacity;
%ignore Proud::CFastArray::SetCount;
//%ignore Proud::CFastArray::GetCount;
%ignore Proud::CFastArray::GetCapacity;
//%ignore Proud::CFastArray::Clear;
%ignore Proud::CFastArray::ClearAndKeepCapacity;
%ignore Proud::CFastArray::ElementAt;
%ignore Proud::CFastArray::GetData;
%ignore Proud::CFastArray::Insert;
%ignore Proud::CFastArray::push_back;

%ignore Proud::CFastArray::AddRange;
%ignore Proud::CFastArray::CopyRangeTo;
%ignore Proud::CFastArray::CopyTo;
%ignore Proud::CFastArray::CopyFrom;
%ignore Proud::CFastArray::RemoveRange;
//%ignore Proud::CFastArray::RemoveAt;
%ignore Proud::CFastArray::RemoveOneByValue;
%ignore Proud::CFastArray::FindByValue;
%ignore Proud::CFastArray::Contains;

%ignore Proud::CFastArray::iterator;
%ignore Proud::CFastArray::const_iterator;
%ignore Proud::CFastArray::begin;
%ignore Proud::CFastArray::end;
%ignore Proud::CFastArray::erase;
%ignore Proud::CFastArray::PopBack;
%ignore Proud::CFastArray::RemoveAndPullLast;
%ignore Proud::CFastArray::Equals;
%ignore Proud::CFastArray::clear;

%ignore Proud::CFastArray::DataBlock_Free;
%ignore Proud::CFastArray::DataBlock_Alloc;
%ignore Proud::CFastArray::DataBlock_Realloc;

%ignore Proud::CFastArray::Add; 
%ignore Proud::FastArray_In_Type; 

%include "../../include/FastArray.h"

%template(IntArray) Proud::CFastArray<int>;
%template(AddrPortArray) Proud::CFastArray<Proud::AddrPort,true,false,intptr_t>;