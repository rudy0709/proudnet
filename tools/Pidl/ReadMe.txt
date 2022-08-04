.g 파일과 .tt 파일을 작업하려면, 사전에 설치해야 하는 애드온이 있습니다.

VS tools and addons 메뉴 들어가서

ANTLR tools => .g 파일 수정시 반영됨
DevArt T4 tools => .tt 파일 수정이 편리함

-- 2022.08.04 --
* ANTLR, DevArt T4 툴 모두 VS 2022를 지원하지 않습니다.
* NuGet 관련 오류가 발생한다면 4개의 모듈을 삭제 후에 재설치하는 것을 추천합니다.
  - Antlr4 v4.6.6 (최신 버전으로 설치 가능, 아래 2개의 모듈과 함께 설치됨)
  - Antlr4.CodeGenerator v4.6.6
  - Antlr4.Runtime v4.6.6
  - YamlDotNet v12.0.0 (최신 버전으로 설치 가능)
* OutputTemplate/ 폴더에 C# 파일은 git으로 관리하지 않습니다.
  .tt 파일의 컨텍스트 메뉴에서 "Run Custom Tool" 메뉴를 실행하면 C# 파일을 생성시킬 수 있습니다.
