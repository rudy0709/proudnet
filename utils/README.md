# 빌드 전에 체크할 사항
- `PN_BUILD_PATH` 환경변수
    - C++/C# 컴파일을 위한 도구의 경로
    - 기본값은 `C:\Program Files\Microsoft Visual Studio\2022\Professional\Msbuild\Current\Bin\MSBuild.exe` 입니다.
<br/><br/>
- `PN_SIGN_TOOL_PATH` 환경변수
    - 디지털 서명을 위한 도구의 경로
    - 기본값은 `C:\Program Files (x86)\Microsoft SDKs\ClickOnce\SignTool\signtool.exe` 입니다.
<br/><br/>
- 공용 버전파일 (2022.11.01 기준)
    - C++ : v1.8.0
        - `…\proudnet\version\PNVersion.cpp`
        - `…\proudnet\version\PNVersion.h`
    - C# : v1.8.0
        - `…\proudnet\version\PNVersion.cs`
<br/><br/>
- nuget 패키지 설치 여부
    - `PIDL 컴파일러` 프로젝트의 첫 빌드 시에는 성공하지 못하는 경우가 있습니다.<br/>
    이는 `Antlr4*`, `YamlDotNet` 패키지를 제대로 참조하지 못하기 때문일 가능성이 높습니다.<br/>
    프로젝트의 빌드를 몇번 반복하거나 Visual Studio를 재실행한 후에 빌드하면 성공하게 됩니다.
<br/>
<br/>
<br/>
# 빌드 순서

1. `CodeVirtualizer` 프로젝트
    - 다른 프로젝트에서 헤더파일을 참조할 뿐이기 때문에 빌드하지 않아도 됩니다.
<br/><br/>
2. `ImageGen` 프로젝트
    - .exe 파일을 .inl 파일로 변환하는 도구 (내부용)
    - Post-Build Event 등록
        - nuget 오픈소스 다운로드
        - Antlr4*, YamlDotNet 패키지를 설치
        - CodeVirtualizer 오픈소스 다운로드
<br/><br/>
3. `PIDL 컴파일러` 프로젝트 (PNLic 전용)
    - nuget.exe 및 Antlr4*, YamlDotNet 패키지를 참조
