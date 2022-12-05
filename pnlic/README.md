# 빌드 전에 체크할 사항
- `pnlic` 폴더 내의 프로젝트들은 특정 시점의 Core 용 소스코드의 스냅샷입니다.
<br/><br/>
- 윈도우 OS에서의 환경변수
	- `PN_BUILD_PATH`
		- C++/C# 컴파일을 위한 도구의 경로
		- 기본값은 `C:\Program Files\Microsoft Visual Studio\2022\Professional\Msbuild\Current\Bin\MSBuild.exe` 입니다.
	- `PN_SIGN_TOOL_PATH`
		- 디지털 서명을 위한 도구의 경로
		- 기본값은 `C:\Program Files (x86)\Microsoft SDKs\ClickOnce\SignTool\signtool.exe` 입니다.
<br/><br/>
- 리눅스 OS에서의 환경변수
	- `PN_BUILD_GCC_PATH`
		- C 컴파일을 위한 도구의 경로
		- 기본값은 `/usr/bin/gcc` 입니다.
	- `PN_BUILD_GPP_PATH`
		- C++ 컴파일을 위한 도구의 경로
		- 기본값은 `/usr/bin/g++` 입니다.
	- `CMAKE_MODULE_PATH`
		- cmake 실행파일의 경로
		- 기본값은 `/usr/local/cmake-3.25.0/bin/cmake` 입니다.
<br/><br/>
- 공용 버전파일 (v1.8.0 - 2022.11.01 기준)
	- C++
		- `…\proudnet\version\PNVersion.cpp`
		- `…\proudnet\version\PNVersion.h`
	- C#
		- `…\proudnet\version\PNVersion.cs`
<br/><br/>
- `PIDL 컴파일러` 프로젝트 (PNLic 전용)
	- Visual Studio에서 솔루션의 첫 빌드 시에는 빌드를 성공하지 못하는 경우가 있습니다. 이는 `Antlr4*`, `YamlDotNet` 패키지를 제대로 참조하지 못했기에 실패하는 것입니다.<br/>
	프로젝트의 빌드를 몇번 반복하거나 Visual Studio를 재실행한 후에 빌드하면 성공하게 됩니다.
<br/><br/>
<br/>
# 빌드 순서
`CodeVirtualizer`, `PNLicenseManager` 프로젝트는 다른 프로젝트에서 헤더파일을 참조할 뿐이기 때문에 빌드하지 않아도 됩니다.<br/>
 
1. `ImageGen` 프로젝트
	- .exe 파일을 .inl 파일로 변환하는 도구 (내부용)
	- `pnlic/` 프로젝트들 보다 `utils/` 프로젝트들과 함께 먼저 빌드되어야 합니다.
	- Post-Build Event 등록
		- nuget 오픈소스 다운로드
		- Antlr4*, YamlDotNet 패키지를 설치
		- CodeVirtualizer 오픈소스 다운로드
<br/><br/>
2. `PIDL 컴파일러` 프로젝트 (PNLic 전용)
	- nuget.exe 및 Antlr4*, YamlDotNet 패키지를 참조
<br/><br/>
3. `AuthNetPidl`, `AuthNetClient`, `AutNetServer` 프로젝트 (PNLic 전용)
	- ProudNetClient, ProudNetServer 프로젝트와 같은 소스로 특정 시점의 스냅샷
<br/><br/>
4. `PNLicAuthCommon`, `PNLicAuthClient`, `PNLicAuthServer` 프로젝트
	- `PNLicAuthServer` 프로제트는 윈도우 전용이라 리눅스에서는 빌드하지 않아도 됩니다.
<br/><br/>
5. `PNLicenseSdk`, `PNLicense` 프로젝트
<br/><br/>
6. `PNLicenseWarning` 프로젝트
	- Post-Build Event 등록
		- CodeVirtualizer 오픈소스를 활용해 .exe 파일을 난독화 (윈도우 OS에서만)
		- .exe 파일에 디지털 서명을 추가 (윈도우 OS에서만)
		- .exe 파일을 숨기기 위해 .inl 파일로 변환
<br/><br/>
7. `PNLicenseHidden` 프로젝트
	- PNLicenseManager 프로젝트의 헤더를 참조
	- Post-Build Event 등록
		- CodeVirtualizer 오픈소스를 활용해 .exe 파일을 난독화 (윈도우 OS에서만)
		- .exe 파일에 디지털 서명을 추가 (윈도우 OS에서만)
		- .exe 파일을 숨기기 위해 .inl 파일로 변환
<br/><br/>
8. `PNLicenseAuth` 프로젝트
	- Post-Build Event 등록
		- CodeVirtualizer 오픈소스를 활용해 .exe 파일을 난독화
		- .exe 파일에 디지털 서명을 추가 (윈도우 OS에서만)
<br/><br/>
9. `PNLicenseAuthGui` 프로젝트
	- Post-Build Event 등록
		- PNLicenseAuth.exe 파일을 bin/ 폴더로 복사
		- .exe 파일에 디지털 서명을 추가 (윈도우 OS에서만)
<br/><br/>
10. `WatermarkLib` 프로젝트 (윈도우 전용)
<br/><br/>
11. `WatermarkDll` 프로젝트 (윈도우 전용)
	- Post-Build Event 등록
		- CodeVirtualizer 오픈소스를 활용해 .exe 파일을 난독화
		- .exe 파일에 디지털 서명을 추가
