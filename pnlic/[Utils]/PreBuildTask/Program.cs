using System;

namespace PreBuildTask
{
    class Program
    {
        static void Main(string[] args)
        {
            Console.WriteLine("#### 이 프로젝트의 빌드 결과물(.exe)은 무시 하세요. ####");
            Console.WriteLine("#### 오픈소스 및 nuget 패키지를 미리 설치하기 위한 프로젝트입니다. ####");
            Console.WriteLine("  - nuget 오픈소스 다운로드");
            Console.WriteLine("  - Antlr4 / Antlr4.CodeGenerator / Antlr4.Runtime / YamlDotNet 패키지 설치");
            Console.WriteLine("  - CodeVirtualizer 오픈소스 다운로드");
        }
    }
}
