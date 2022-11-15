# 빌드 전에 체크할 사항
- `PN_BUILD_PATH` 환경변수
    - C++/C# 컴파일을 위한 도구의 경로
    - 기본값은 `C:\Program Files\Microsoft Visual Studio\2022\Professional\Msbuild\Current\Bin\MSBuild.exe` 입니다.
- `PN_SIGN_TOOL_PATH` 환경변수
    - 디지털 서명을 위한 도구의 경로
    - 기본값은 `C:\Program Files (x86)\Microsoft SDKs\ClickOnce\SignTool\signtool.exe` 입니다.
- 공용 버전파일
    - C++ : v1.8.0 (2022.11.01 기준)
        - `…\proudnet\version\PNVersion.cpp`
        - `…\proudnet\version\PNVersion.h`
    - C# : v1.8.0 (2022.11.01 기준)
        - `…\proudnet\version\PNVersion.cs`
- `PIDL 컴파일러` 프로젝트
    - 솔루션의 최초 빌드가 실패할 수 있습니다.<br/>
    이는 `<PIDL 프로젝트>`의 빌드가 실패했기 때문일 가능성이 높습니다.<br>
    `<PIDL 프로젝트>`의 빌드를 몇번 반복하면 성공하게 됩니다.
<br/>
<br/>
<br/>
# 빌드 순서
1. `PnUtils` 프로젝트
    - .exe 파일을 .inl 파일로 변환하는 도구를 생성
    - nuget 오픈소스 다운로드
    - Antlr4*, YamlDotNet 패키지를 설치
    - CodeVirtualizer 오픈소스 다운로드
2. `PIDL 컴파일러` 프로젝트 (PNLic 전용)
    - nuget.exe 및 Antlr4*, YamlDotNet 패키지를 참조
3. `CodeVirtualizer` 프로젝트 (PNLic 전용)
4. `AuthNetPidl`, `AuthNetClient`, `AutNetServer` 프로젝트 (PNLic 전용)
    - ProudNetClient, ProudNetServer 프로젝트와 같은 소스로 특정 시점에 커밋된 코드입니다.
5. `PNLicenseManager` 프로젝트
    - 다른 프로젝트에서 헤더파일을 참조할 뿐이기 때문에 빌드하지 않아도 됩니다.
6. `PNLicenseSdk` 프로젝트
    - PNLicenseManager 프로젝트의 헤더를 참조
        - Define.h / KernelObjectNames.h
    - 다른 프로젝트의 라이브러리를 참조
        - CodeVirtualizer\Lib\COFF\VirtualizerSDK64.lib
7. `PNLicense` 프로젝트
8. `PNLicAuthCommon` 프로젝트
9. `PNLicAuthClient` 프로젝트
    - PNLicenseManager 프로젝트의 헤더를 참조
        - LicenseMessage.h
        - LicenseMessage.inl
        - LicenseType.h
        - PNTypes.h
10. `PNLicAuthServer` 프로젝트
    - 라이센스 인증 서버로 빌드하지 않아도 됩니다.
    - 다른 프로젝트의 라이브러리를 참조
        - AuthNetClient.lib
        - AuthNetServer.lib
        - PNLicAuthCommon.lib
        - CodeVirtualizer\Lib\COFF\VirtualizerSDK64.lib
11. `WatermarkLib` 프로젝트
12. `WatermarkDll` 프로젝트
    - 다른 프로젝트의 라이브러리를 참조
        - AuthNetClient.lib
        - PNLicAuthClient.lib
        - PNLicense.lib
        - PNLicenseSdk.lib
        - WatermarkLib.lib
        - CodeVirtualizer\Lib\COFF\VirtualizerSDK64.lib
    - Post-Build Event 등록
        - CodeVirtualizer 오픈소스를 활용해 .exe 파일을 난독화하는 스크립트
13. `PNLicenseWarning` 프로젝트
    - 다른 프로젝트의 라이브러리를 참조
        - AuthNetClient.lib
        - PNLicenseSdk.lib
        - CodeVirtualizer\Lib\COFF\VirtualizerSDK64.lib
    - Post-Build Event 등록
        - CodeVirtualizer 오픈소스를 활용해 .exe 파일을 난독화하는 스크립트
        - .exe 파일에 디지털 서명을 추가하는 스크립트
        - .exe 파일을 숨기기 위해 .inl 파일로 변환하는 스크립트
14. `PNLicenseHidden` 프로젝트
    - PNLicenseManager 프로젝트의 헤더를 참조
        - AppExecuter.inl
        - MemoryHopper.inl
        - PNLicenseWarningImage.inl <-- PNLicenseWarning 프로젝트에서 생성하는 파일
        - RunLicenseApp.h
        - WarningAppExecuter.inl
    - 다른 프로젝트의 라이브러리를 참조
        - AuthNetClient.lib
        - PNLicenseSdk.lib
        - CodeVirtualizer\Lib\COFF\VirtualizerSDK64.lib
    - Post-Build Event 등록
        - CodeVirtualizer 오픈소스를 활용해 .exe 파일을 난독화하는 스크립트
        - .exe 파일에 디지털 서명을 추가하는 스크립트
        - .exe 파일을 숨기기 위해 .inl 파일로 변환하는 스크립트
15. `PNLicenseAuth` 프로젝트
    - 다른 프로젝트의 라이브러리를 참조
        - AuthNetClient.lib
        - PNLicAuthCommon.lib
        - PNLicAuthClient.lib
        - PNLicenseSdk.lib
        - CodeVirtualizer\Lib\COFF\VirtualizerSDK64.lib
    - Post-Build Event 등록
        - CodeVirtualizer 오픈소스를 활용해 .exe 파일을 난독화하는 스크립트
        - .exe 파일에 디지털 서명을 추가하는 스크립트
16. `PNLicenseAuthGui` 프로젝트
    - Post-Build Event 등록
        - PNLicenseAuth.exe 파일을 복사하는 스크립트
