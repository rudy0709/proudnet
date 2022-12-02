#include "NativeConvert.h"
#include "NativeType.h"

void NativeToGuid_Delete(void* guid)
{
	if (guid != NULL)
	{
		delete static_cast<Proud::Guid*>(guid);
	}
}

void NativeToByteArray_Delete(void* byteArray)
{
	if (byteArray != NULL)
	{
		delete static_cast<Proud::ByteArray*>(byteArray);
	}
}

void* NativeToIntArray_New()
{
	return new Proud::CFastArray<int>();
}

void NativeToIntArray_Delete(void* intArray)
{
	if (intArray != NULL)
	{
		delete static_cast<Proud::CFastArray<int>*>(intArray);
	}
}

//////////////////////////////////////////////////////////////////////////

// byte[16] guidData를 복사받은 new Proud::Guid object의 포인터를 리턴한다.
void* GuidToNative(void* guidData, int dataLength)
{
	if (dataLength <= 0)
	{
		return NULL;
	}

	assert(sizeof(Proud::PNGUID) == dataLength);

	Proud::PNGUID uuid;
	memcpy(&uuid, guidData, dataLength);

	Proud::Guid* guid = new Proud::Guid(uuid);

	return guid;
}

void CopyGuidToNative(void* guidData, int dataLength, void* nativeGuid)
{
	if (dataLength <= 0)
	{
		return;
	}

	assert(nativeGuid);
	assert(sizeof(Proud::PNGUID) == dataLength);

	Proud::Guid* guid = static_cast<Proud::Guid*>(nativeGuid);

	Proud::PNGUID uuid;
	memcpy(&uuid, guidData, dataLength);

	*guid = Proud::Guid(uuid);
}

void* ByteArrayToNative(void* data, int dataLength)
{
	if (dataLength <= 0)
	{
		return NULL;
	}

	Proud::ByteArray* byteArray = new Proud::ByteArray();

	byteArray->SetCount(dataLength);
	memcpy(byteArray->GetData(), data, dataLength);

	return byteArray;
}

void CopyByteArrayToNative(void* data, int dataLength, void* nativeData)
{
	if (dataLength <= 0)
	{
		return;
	}

	assert(nativeData);

	Proud::ByteArray* byteArray = static_cast<Proud::ByteArray*>(nativeData);
	byteArray->SetCount(dataLength);
	memcpy(byteArray->GetData(), data, dataLength);
}

void IntArray_Add(void* obj, int data)
{
	assert(obj);

	Proud::CFastArray<int>* intArray = static_cast<Proud::CFastArray<int>*>(obj);

	intArray->Add(data);
}

int ByteArray_GetCount(void* obj)
{
	assert(obj);

	Proud::ByteArray* byteArray = static_cast<Proud::ByteArray*>(obj);
	
	return byteArray->GetCount();
}

void ByteArray_Copy(void* dst, void* obj)
{
	assert(obj);
	assert(dst);

	Proud::ByteArray* src = static_cast<Proud::ByteArray*>(obj);

	memcpy(dst, src->GetData(), src->GetCount());
}

void ByteArray_CopyFromData(void* dst, void* src, int length)
{
	assert(dst);
	assert(src);
	assert(0 < length);

	memcpy(dst, src, length);
}

std::string ByteToString(void* obj)
{
	assert(obj);
	std::string msg(static_cast<std::string::pointer>(obj));
	return msg;
}
