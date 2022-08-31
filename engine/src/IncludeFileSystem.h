#pragma once

/*
// 리눅스 static lib에서 boost를 의존하는 경우 문제가 된다.
// 리눅스에서 c++17에서나 가능한 std::filesystem을 쓸 수 있게 되면 그때 여기 자체구현을 대체하도록 하자.

*****************************

#if (_MSC_VER >= 1900)  // std filesystem은 VS2013부터 지원합니다. VS2010 이하에서는 이것에 의존하는 함수들을 다 preprocessor if로 막아버려도 됩니다. => VS2015부터 지원하기로 함. VS2013이 실제로 안되는 경우가 있는 듯.
#include <filesystem>
namespace fs = std::experimental::filesystem::v1;
typedef std::error_code u_error_code;

#define USE_STD_FILESYSTEM
#endif

// 리눅스에서는 c++17 미만의 컴파일러를 지원하자. 그래서 boost를 쓴다.
// 그러나 boost는 프로그램 파일 용량이 커진다. 따라서 모바일과 콘솔은 빼버리자. 어차피 filesystem 관련 기능을 거기서 쓸 일은 없다. 주로 서버에서 쓴다.
#if (__cplusplus > 199711L) && !defined(__ANDROID__) && !defined(__MACH__) && !defined(__ORBIS__)
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
typedef boost::system::error_code u_error_code;

constexpr bool g_std_filesystem_enabled = true;

#define USE_STD_FILESYSTEM

#endif

*/
