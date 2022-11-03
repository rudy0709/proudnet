using System;
using System.IO;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace PIDL
{
    public partial class App
    {
        public static Lang g_lang;

        // T4에서 사용되는 값들
        static public string m_ProxyCPPFileName, m_ProxyHFileName, m_ProxyCSFileName, m_StubCPPFileName, m_StubHFileName, m_StubCSFileName, m_StubERHFileName;
        static public string m_CommonHFileName, m_CommonCPPFileName, m_CommonCSFileName;
        static public string m_ProxyCPPFilePathName, m_ProxyHFilePathName, m_ProxyCSFilePathName, m_StubCPPFilePathName, m_StubHFilePathName, m_StubCSFilePathName, m_StubERHFilePathName;
        static public string m_ComponentCPPDeclFilePathName, m_ComponentCPPImplFilePathName;
        static public string m_CommonHFilePathName, m_CommonCPPFilePathName, m_CommonCSFilePathName;
        static public string m_ProxyCPPFileNameWithoutExtension, m_ProxyHFileNameWithoutExtension, m_StubCPPFileNameWithoutExtension, m_StubHFileNameWithoutExtension;
        static public string m_CommonJavaFileName, m_CommonJavaFilePathName, m_CommonJavaFileNameWithoutExtension;
        static public string m_ProxyJavaFileName, m_ProxyJavaFilePathName, m_ProxyJavaFileNameWithoutExtension;
        static public string m_StubJavaFileName, m_StubJavaFilePathName, m_StubJavaFileNameWithoutExtension, m_StubNDKJavaFileName, m_StubNDKJavaFilePathName, m_StubNDKJavaFileNameWithoutExtension;
        static public string m_ProxyUCFileName, m_StubUCFileName;
        static public string m_ProxyUCFilePathName, m_StubUCFilePathName;
        static public string m_ProxyUCFileNameWithoutExtension, m_StubUCFileNameWithoutExtension;

        // .cpp대신 사용할 확장자. .으로 시작함.
        static public string m_implExt=null;

        static void DeleteFile(string fn)
        {
            if (File.Exists(fn) && !Directory.Exists(fn))
                File.Delete(fn);
        }

        static public void DeleteOutputFiles()
        {
            if (g_lang == Lang.CPP)
            {
                DeleteFile(m_ProxyCPPFilePathName);
                DeleteFile(m_ProxyHFilePathName);
                DeleteFile(m_StubCPPFilePathName);
                DeleteFile(m_StubHFilePathName);
                DeleteFile(m_CommonCPPFilePathName);
                DeleteFile(m_CommonHFilePathName);
                DeleteFile(m_ComponentCPPDeclFilePathName);
                DeleteFile(m_ComponentCPPImplFilePathName);
            }
            if (g_lang == Lang.CS)
            {
                DeleteFile(m_ProxyCSFilePathName);
                DeleteFile(m_StubCSFilePathName);
                DeleteFile(m_CommonCSFilePathName);
            }
            if (g_lang == Lang.JAVA)
            {
                DeleteFile(m_ProxyJavaFilePathName);
                DeleteFile(m_StubJavaFilePathName);
                DeleteFile(m_CommonJavaFilePathName);
            }
            if (g_lang == Lang.UC)
            {
                DeleteFile(m_ProxyUCFilePathName);
                DeleteFile(m_StubUCFilePathName);
            }
        }

        public static Parsed_Root g_parsed;

        public static void CreateOutputFiles()
        {
            switch (g_lang)
            {
                case Lang.CPP:
                    CreateOutputFile(m_CommonHFilePathName, new CppCommonH().TransformText());
                    CreateOutputFile(m_CommonCPPFilePathName, new CppCommonCpp().TransformText());
                    CreateOutputFile(m_ProxyHFilePathName, new CppProxyH().TransformText());
                    CreateOutputFile(m_ProxyCPPFilePathName, new CppProxyCpp().TransformText());
                    CreateOutputFile(m_StubHFilePathName, new CppStubH().TransformText());
                    CreateOutputFile(m_StubCPPFilePathName, new CppStubCpp().TransformText());
                    if(App.g_parsed.m_components.Count>0)
                    {
                        CreateOutputFile(m_ComponentCPPDeclFilePathName, new CppComponentDecl().TransformText());
                        CreateOutputFile(m_ComponentCPPImplFilePathName, new CppComponentImpl().TransformText());
                    }
                    break;
                case Lang.CS:
                    CreateOutputFile(m_CommonCSFilePathName, new CSCommon().TransformText());
                    CreateOutputFile(m_ProxyCSFilePathName, new CSProxy().TransformText());
                    CreateOutputFile(m_StubCSFilePathName, new CSStub().TransformText());
                    break;
                case Lang.JAVA:
                    CreateOutputFile(m_CommonJavaFilePathName, new JavaCommon().TransformText());
                    CreateOutputFile(m_ProxyJavaFilePathName, new JavaProxy().TransformText());
                    CreateOutputFile(m_StubJavaFilePathName, new JavaStub().TransformText());
                    break;
                case Lang.UC:
                    CreateOutputFile(m_ProxyUCFilePathName, new UCProxy().TransformText());
                    CreateOutputFile(m_StubUCFilePathName, new UCStub().TransformText());
                    break;
                default:
                    throw new Exception("Cannot generate files! Lang is undefined!");
            }
        }

        static void CreateOutputFile(string filePathName, string content)
        {
            var encoding = Encoding.UTF8;
            if (Path.GetExtension(filePathName).ToLower() == ".uc")
                encoding = Encoding.Default;
            File.WriteAllText(filePathName, content, encoding);
        }

        // C#에서 (::) 으로 넣은 인자를 (.)으로 바꾸기 위함.
        public static string GetMemberSpecificationSymbol()
        {
            if (g_lang == Lang.CPP)
                return "::";
            else
                return ".";
        }

        static public string GetLangDefaultMarshaler(Lang lang)
        {
            switch (lang)
            {
                case Lang.CS:
                    return "Nettention.Proud.Marshaler";
                case Lang.JAVA:
                    return "com.nettention.proud.Marshaler";
                case Lang.UC:
                    return "Marshaler";
                default:
                    return "";
            }
        }

        // 입력 파일에서 alias 구문이 있으면 그 선언을 흡수&쟁여둔다.
        static public void AddAlias(string langName, string varType1, string varType2)
        {
            Lang langIndex = LangUtil.GetLangEnum(langName);

            if (langIndex < 0)
            {
                throw new Exception("Wrong language name! Must be cpp,cs,uc,...!");
            }

            if (!g_aliases.ContainsKey(langIndex))
            {
                g_aliases[langIndex] = new Dictionary<string, string>();
            }
            g_aliases[langIndex][varType1] = varType2;
        }

        // 입력 파일에서 alias 구문 정의가 있었을 경우 조건이 맞으면 이름을 바꿔서 리턴한다.
        static public string ApplyAlias(string varType)
        {
            Dictionary<string, string> map2;
            if (g_aliases.TryGetValue(g_lang, out map2))
            {
                string ret;
                if (map2.TryGetValue(varType, out ret))
                    return ret;
            }
            return varType;// 안바뀐 값 그대로
        }

        // alias map.
        // 1st key: language enum
        // 2nd key: alias source
        // value: alias target
        static Dictionary<Lang, Dictionary<string, string>> g_aliases = new Dictionary<Lang, Dictionary<string, string>>();

        public static void ParseWarn(string text)
        {
            Console.WriteLine(text);
        }

        static public LicenseInfo g_licenseInfo = new LicenseInfo();

        // ErrorType.yaml에서 읽은거.
        // T4에서 액세스한다.
        public static ErrorTypeList g_errorTypeList; 

        // 소스코드에 삽입 가능한 문자열 형태로 바꾼다. 예: john "the" man ==> john \"the\"man
        public static string GetStringForSourceCode(string text)
        {
            return text.Replace("\"", "\\\"");
        }

        // 파일명 자체를 변수 이름으로 쓸 수 있게 바꾼다.
        // 불법복제 예방차 워터마크를 박을 때 사용된다.
        public static string GetVariablizedName(string fileName)
        {
            StringBuilder sb = new StringBuilder();
            sb.Append("_");
            sb.Append(fileName);
            sb.Replace(".", "_");
            sb.Replace("\\", "_");
            sb.Replace("/", "_");

            return sb.ToString();
        }

        // 출력 예: 'abc' => `65,66,67,`
        static public string GenerateWatermarkByteArrayDefinition(string text)
        {
            int count = 0;
            StringBuilder ret = new StringBuilder();
            if(text !=null)
            {
                foreach (char x in text)
                {
                    String t = String.Format("{0}, ", x);
                    ret.Append(t);

                    if (count > 7)
                    {
                        count = 0;
                        ret.Append('\n');
                    }
                }
            }

            return ret.ToString();
        }

        // Component 안에 멤버 함수들이 어떤 dllexport 매크로를 써야 하는지가 여기 정해진다.
        static public string m_dllExportMacroName = "";

        // dllexport에 사용되는 매크로 이름이 들어간다.
        public static void SetExportDef(string macroName)
        {
            m_dllExportMacroName = macroName;
        }

    }
}
