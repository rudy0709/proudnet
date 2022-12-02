using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace PIDL
{
    partial class App
    {
        // 기본적으로 PIDL에서 정의한 타입이 그대로 출력 언어별 타입으로 쓰이지만, 예외를 두려고 하면 이게 사용된다.
        // key1: 언어
        // key2: PIDL 파일에 정의된 타입
        // value: 언어별 실제 타입
        static readonly Dictionary<Lang, Dictionary<string, string>> g_specializedTypeToActualType = new Dictionary<Lang, Dictionary<string, string>>
        {
            {
                Lang.CPP, new Dictionary<string, string>
                {
                    { "const string", "PNTCHAR*" },
                    { "string", "Proud::String" },
                    { "int32", "int32_t" },
                    { "uint32", "uint32_t" },
                    { "int64", "int64_t" },
                    { "uint64", "uint64_t" },
                    { "const int32", "int32_t" },
                    { "const uint32", "uint32_t" },
                    { "const int64", "int64_t" },
                    { "const uint64", "uint64_t" },
                }
            },
            {
                Lang.CS, new Dictionary<string, string>
                {
                    { "int32", "int" },
                    { "uint32", "uint" },
                    { "int64", "long" },
                    { "uint64", "ulong" },
                    { "const int32", "int" },
                    { "const uint32", "uint" },
                    { "const int64", "long" },
                    { "const uint64", "ulong" },
                }
            }
        };

        internal static object GetValueStatement(bool isStringLiteral, string valueAsString)
        {
            if (true == isStringLiteral)
            {
                if (App.g_lang == Lang.CPP)
                {
                    return "_PNT(" + valueAsString + ")";
                }
            }

            return valueAsString;
        }

        // 출력 언어별 변수 타입을 리턴한다. PIDL rename 키워드를 지원하는게 정석이긴 하지만, class, enum 기능은 일단은 내부 기능인지라 이렇게 두자.
        // TODO: 나중에 사용자가 쓰게 할거면, default type에 대해는 언어별 지원을 하게 애당초 만들어도 나쁘지 않을 뜻... rename 키워드와 별개로. 혹은 rename을 특정 PIDL에 정의하고 그것을 다른 PIDL이 공유하게 한다던지...
        internal static string GetVariableTypeByLanguage(bool isConst, string type)
        {
            string type2 = type;
            if (isConst)
            {
                type2 = "const " + type;    // 임시로 const 붙인걸 사용
            }
            if (true == g_specializedTypeToActualType.TryGetValue(App.g_lang, out var value1) && value1.TryGetValue(type2, out var value2))
            {
                return value2;
            }
            return type;    // 임시로 const 붙인건 빼고 줘야
        }

        static string[] tokens = { ".", "::" };

        // C++에서는 namespace xx::xx { 대신 namespace xx { namespace xx {  이렇게 해줘야 하위호환이 된다.
        // 이러한 형태로 만들어준다.
        public static string GetCppNestedNamespaceDefinition(string fullTypeName, bool withClassName)
        {
            var names = fullTypeName.Split(tokens, StringSplitOptions.RemoveEmptyEntries);

            string ret = "";
            int nameCount = names.Length;
            if (withClassName)
                nameCount--;
            int indent = 0;
            foreach (var name in names)
            {
                if (nameCount <= 0)
                    break;
                // C# 6.0 보간 문자열 표현식으로 변경
                for (int i = 0; i < indent; i++)
                    ret += " ";
                ret += $"namespace {name} {{";
                ret += Environment.NewLine;
                nameCount--;
                indent += 4;
            }
            return ret;
        }
        // 위 함수의 반대 역할. 즉 } } 같은걸 만든다.
        public static string GetCppNestedNamespaceDefinitionEnd(string fullTypeName, bool withClassName)
        {
            var names = fullTypeName.Split(tokens, StringSplitOptions.RemoveEmptyEntries);

            int nameCount = names.Length;
            if (withClassName)
                nameCount--;
            string ret = "";
            int indent = 4 * (nameCount-1);
            foreach (var name in names)
            {
                if (nameCount <= 0)
                    break;
                for (int i = 0; i < indent; i++)
                    ret += " ";
                ret += "}";
                ret += Environment.NewLine;

                nameCount--;
                indent -= 4;
            }
            return ret;
        }

        // 입력 예: Nettention.Proud.Type1
        // 출력: Type1
        public static string GetLeafName(string fullTypeName)
        {
            var names = fullTypeName.Split(tokens, StringSplitOptions.RemoveEmptyEntries);

            return names.Last();
        }

        // 입력 예: Nettention.Proud.Type1
        // 출력: Nettention.Proud
        public static string GetNamespacePart(string fullTypeName)
        {
            var leaf = GetLeafName(fullTypeName);
            var v1 = fullTypeName.Substring(0, fullTypeName.Length - leaf.Length);

            // 뒤에 ::나 .가 붙은 상태니까 이건 빼버리자
            foreach (var token in tokens)
            {
                if (true == v1.EndsWith(token))
                    v1 = v1.Substring(0, v1.Length - token.Length);
            }
            return v1;
        }

        // PIDL에 class 정의는 현재 넷텐션 내부 전용이다. C#,C++여러 언어에서 동일한 상수를 사용하기 위해 만드는 기능이다.
        // 따라서 개발자의 실수를 예방하기 위해 static 변수 즉 전역변수는 상수로서 두어야 하며, 이에 따라 const 키워드를 필수로 붙이게 한다.
        internal static void AssureStaticConstVariable(Parsed_Variable ret)
        {
            if(true==ret.m_attr.m_isStatic && false==ret.m_attr.m_isConst)
            {
                throw new Exception($"Variable `{ret.m_name}` have to have `const` attribute. This policy goes on until `class` and `enum` statement is in public.");
            }
        }
    }
}
