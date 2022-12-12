//#include "stdafx.h"

namespace Proud
{
	// [ToDo] 이 곳에는 프라우드넷 버전을 입력해야 합니다. (HERE_SHALL_BE_EDITED_BY_BUILD_HELPER)
	// 소스 저장소를 github으로 옮긴 이후로는 버전을 major.minor.patch 형식으로 관리합니다. (perforce에서의 마지막 버전은 1.7.56164 입니다.)
	const char* g_versionText = "1.7.0";

	// [ToDo] 이 곳에는 프라우드넷 버전의 해시값을 입력해야 합니다. (HERE_2_SHALL_BE_EDITED_BY_BUILD_HELPER)
	// ../core/include/BasicTypes.h 파일의 "PROUDNET_H_LIB_SIGNATURE" 매크로에 해시값을 반영해야 합니다.
	// 	- #define PROUDNET_H_LIB_SIGNATURE VER_3233192701_DEBUG
	// 	- #define PROUDNET_H_LIB_SIGNATURE VER_3233192701_RELEASE
	//
	// 해시값은 g_versionText 값을 이용하여 C#의 Object.GetHashCode() 메소드를 호출하면 얻을 수 있습니다.
	// 온라인 C# 컴파일러 사이트(https://www.programiz.com/csharp-programming/online-compiler/)에서 아래 코드를 실행합니다.
	// public static void Main(string[] args)
	// {
	// 		string temp = "1.7.0";
	// 		Console.WriteLine((UInt32) temp.GetHashCode()); // 3233192701
	// }
}
