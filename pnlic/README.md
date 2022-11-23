# 빌드 전에 체크할 사항
- `PN_BUILD_PATH` 환경변수
    - C++/C# 컴파일을 위한 도구의 경로
    - 기본값은 `C:\Program Files\Microsoft Visual Studio\2022\Professional\Msbuild\Current\Bin\MSBuild.exe` 입니다.
- `PN_SIGN_TOOL_PATH` 환경변수
    - 디지털 서명을 위한 도구의 경로
    - 기본값은 `C:\Program Files (x86)\Microsoft SDKs\ClickOnce\SignTool\signtool.exe` 입니다.
- 공용 버전파일 (2022.11.01 기준)
    - C++ : v1.8.0
        - `…\proudnet\version\PNVersion.cpp`
        - `…\proudnet\version\PNVersion.h`
    - C# : v1.8.0
        - `…\proudnet\version\PNVersion.cs`
- `PIDL 컴파일러` 프로젝트
    - 솔루션의 최초 빌드가 실패할 수 있습니다.<br/>
    이는 `PIDL 컴파일러` 프로젝트의 빌드가 실패했기 때문일 가능성이 높습니다.<br/>
    `PIDL 컴파일러` 프로젝트의 빌드를 몇번 반복하면 성공하게 됩니다.
<br/>
<br/>
<br/>
# 빌드 순서
`CodeVirtualizer`, `PNLicenseManager` 프로젝트는 다른 프로젝트에서 헤더파일을 참조할 뿐이기 때문에 빌드하지 않아도 됩니다.<br/>
 
1. `PnImageGen` 프로젝트
    - .exe 파일을 .inl 파일로 변환하는 도구 (내부용)
    - Post-Build Event 등록
        - nuget 오픈소스 다운로드
        - Antlr4*, YamlDotNet 패키지를 설치
        - CodeVirtualizer 오픈소스 다운로드
2. `PIDL 컴파일러` 프로젝트 (PNLic 전용)
    - nuget.exe 및 Antlr4*, YamlDotNet 패키지를 참조
3. `AuthNetPidl`, `AuthNetClient`, `AutNetServer` 프로젝트 (PNLic 전용)
    - ProudNetClient, ProudNetServer 프로젝트와 같은 소스로 특정 시점에 커밋된 코드
4. `PNLicAuthCommon` 프로젝트
5. `PNLicAuthClient` 프로젝트
    - PNLicenseManager 프로젝트의 헤더를 참조
6. `PNLicAuthServer` 프로젝트
    - 라이센스 인증 서버로 빌드하지 않아도 됩니다.
7. `PNLicenseSdk` 프로젝트
    - PNLicenseManager 프로젝트의 헤더를 참조
8. `PNLicense` 프로젝트
9. `PNLicenseWarning` 프로젝트
    - Post-Build Event 등록
        - CodeVirtualizer 오픈소스를 활용해 .exe 파일을 난독화하는 스크립트
        - .exe 파일에 디지털 서명을 추가하는 스크립트
        - .exe 파일을 숨기기 위해 .inl 파일로 변환하는 스크립트
10. `PNLicenseHidden` 프로젝트
    - PNLicenseManager 프로젝트의 헤더를 참조
    - Post-Build Event 등록
        - CodeVirtualizer 오픈소스를 활용해 .exe 파일을 난독화하는 스크립트
        - .exe 파일에 디지털 서명을 추가하는 스크립트
        - .exe 파일을 숨기기 위해 .inl 파일로 변환하는 스크립트
11. `PNLicenseAuth` 프로젝트
    - Post-Build Event 등록
        - CodeVirtualizer 오픈소스를 활용해 .exe 파일을 난독화하는 스크립트
        - .exe 파일에 디지털 서명을 추가하는 스크립트
12. `PNLicenseAuthGui` 프로젝트
    - Post-Build Event 등록
        - PNLicenseAuth.exe 파일을 복사하는 스크립트
        - .exe 파일에 디지털 서명을 추가하는 스크립트 (//222-추가)
13. `WatermarkLib` 프로젝트
14. `WatermarkDll` 프로젝트
    - Post-Build Event 등록
        - CodeVirtualizer 오픈소스를 활용해 .exe 파일을 난독화하는 스크립트
        - .exe 파일에 디지털 서명을 추가하는 스크립트 (//222-추가)
