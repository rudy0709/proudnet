#include "stdafx.h"
#include "../include/PNPath.h"
#include "IncludeFileSystem.h"

namespace Proud
{
	Proud::String Path::GetExtension(const Proud::String& fileName)
	{
		// 리눅스 static lib에서 boost를 의존하는 경우 문제가 된다.
		// 리눅스에서 c++17에서나 가능한 std::filesystem을 쓸 수 있게 되면 그때 여기 자체구현을 대체하도록 하자.

		int whereIsDot = fileName.FindFromLast(_PNT('.'));
		if (whereIsDot >= 0)
		{
			return fileName.c_str() + whereIsDot;
		}
		else
			return _PNT(".");
	}

	String Path::Combine(const String& directory, const String& fileName)
	{
		String ret = directory;
		if (fileName.GetLength() > 0
			&& fileName[fileName.GetLength() - 1] != PN_PATH_SEPARATOR_TCHAR  )
		{
			ret += PN_PATH_SEPARATOR_TSTRING;
		}
		ret += fileName;
		return ret;
	}

	Proud::String Path::RemoveExtension(const Proud::String& fileName)
	{
		// 리눅스 static lib에서 boost를 의존하는 경우 문제가 된다.
		// 리눅스에서 c++17에서나 가능한 std::filesystem을 쓸 수 있게 되면 그때 여기 자체구현을 대체하도록 하자.

		int whereIsDot = fileName.FindFromLast(_PNT('.'));
		if (whereIsDot >= 0)
		{
			return fileName.Left(whereIsDot);
		}
		else
			return fileName;
	}

	String Path::GetFileName(const PNTCHAR* fileName)
	{

// 가급적이면 자체 구현을 선호한다. VS2013 이하에서는 안 작동하는데 혹시 모르니까.

// #ifdef USE_STD_FILESYSTEM
// 		return StringA2T(fs::path(StringT2A(fileName).GetString()).filename().string());
// #else
// 		throw Exception("Not supported. Use newer compiler.");
// #endif
		
		// 백슬래시나 슬래시를 발견하고 그 앞을 다 지워버린다.
 		// 없으면 그냥 리턴.
 		String fileName2(fileName);
 
 		int index = fileName2.FindFromLast('/');
 		if (index >= 0)
 			return fileName2.GetString() + index + 1;
 
 		index = fileName2.FindFromLast('\\');
 		if (index >= 0)
 			return fileName2.GetString() + index + 1;
 
 		return fileName2;
	}

	String Path::ChangeExtension(const String& fileName, const PNTCHAR* fileExtensionWithDot)
	{
		return RemoveExtension(fileName) + fileExtensionWithDot;
	}

	String Path::AppendFileNameWithoutExt(const String& fileName, const String& postfix)
	{
		auto right = GetExtension(fileName);
		auto left = RemoveExtension(fileName);
		return left + postfix + right;
	}

	String Path::GetDirectoryName(const PNTCHAR* fileName)
	{
		String fileName2(fileName);
		int lastSeparator = fileName2.FindFromLast(PN_PATH_SEPARATOR_TCHAR);
		if (lastSeparator >= 0) //찾았으면
		{
			return fileName2.Left(lastSeparator);
		}
		else
			return _PNT("");

		// 리눅스 static lib에서 boost를 의존하는 경우 문제가 된다.
		// 리눅스에서 c++17에서나 가능한 std::filesystem을 쓸 수 있게 되면 그때 여기 자체구현을 대체하도록 하자.

		// #ifdef USE_STD_FILESYSTEM
// 		fs::path p(fileName);
// 		return (p.parent_path().c_str());
// #else
// 		throw Exception("Not supported. Use newer compiler.");
// #endif
	}

}