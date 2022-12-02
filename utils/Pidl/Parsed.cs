using Antlr4.Runtime;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace PIDL
{
    public class Parsed_Root
    {
        // 사용자가 정의한 RMI 함수 그룹
        public List<Parsed_GlobalInterface> m_globalInterfaces = new List<Parsed_GlobalInterface>();

        public Parsed_Usings m_imports = new Parsed_Usings();
        public Parsed_Usings m_usings = new Parsed_Usings();
        public Parsed_Usings m_includes = new Parsed_Usings();
        public Parsed_PackageDef m_package; // java나 actionscript에서 파일당 1회 선언되는 package name

        public List<Parsed_Class> m_classes = new List<Parsed_Class>();
        public List<Parsed_Enum> m_enums = new List<Parsed_Enum>();

        // 파싱 결과물을 출력 파일 만들기를 하기 쉽게 정리해준다.
        public void RefineParsed()
        {
            foreach (var itf in m_globalInterfaces)
            {
                Parsed_GlobalInterface gi = itf;
                // 출력하고자 하는 언어에 대응하는 marshaler를 대입해준다.
                foreach (var mar in gi.m_marshalers)
                {
                    if (LangUtil.GetLangEnum(mar.m_langName) == App.g_lang)
                    {
                        gi.m_marshaler = mar.m_name;
                        break;
                    }
                }

                // 사용자 타입 마샬러가 지정되어있지 않다면 default marshaler를 지정한다.
                if (gi.m_marshaler == null)
                {
                    gi.m_marshaler = App.GetLangDefaultMarshaler(App.g_lang);
                }

                // accessibility가 미지정이면 하나 지정한다.
                if (gi.m_accessibility == null)
                    gi.m_accessibility = "internal";
            }
        }
    }

    public class Parsed_Usings : List<Parsed_Using>
    {
        // 출력 언어와 import 구문 결과가 매치되면 목록에 추가.
        public void AddOnLangMatch(Parsed_Using imp)
        {
            Lang impLang = LangUtil.GetLangEnum(imp.m_langName);
            if (impLang != Lang.Undefined && impLang == App.g_lang)
                Add(imp);
        }
    }

    public class Parsed_Using
    {
        public string m_langName; // 어떤 언어 출력에 한해 들어갈 import 구문인가? (예: as or cs)
        public string m_name; // import 구문에 들어갈 package name
    }

    public class Parsed_PackageDef
    {
        public string m_name;
    }

    public class Parsed_GlobalInterface
    {
        public string m_name; // global xxx {} 에서 xxx 부분
        public string m_firstMessageID; // 이 global interface의 first message ID
        public string m_accessibility; // 기본적으로는 internal 이지만 C#이나 여타 언어에서는 외부에 어셈블리 심볼이 노출되지 않게 하려고 다른 것을 선언할 수도 있어야 하므로.

        public List<Parsed_Marshaler> m_marshalers = new List<Parsed_Marshaler>();
        public string m_marshaler;  // 선택된 marshaler
        public int m_currentRmiIDSerial; // 현재 들어가야할 Rmi ID 

        public List<Parsed_Method> m_methods = new List<Parsed_Method>();

        public void ChainElements()
        {
            // RMI ID를 세팅한다.
            int currentRmiIDSerial = 1;
            foreach (var method in m_methods)
            {
                if (method.m_mode.m_messageID == null)
                {
                    string currentRmiID = m_firstMessageID + "+" + currentRmiIDSerial.ToString();
                    method.m_mode.m_messageID = currentRmiID;
                    currentRmiIDSerial++;
                }
            }
        }

        // 입력 예: "MyPrefix_", "_PNT(", ")"
        // method가 있으면 => 리턴값 예: MyPrefix_FirstRmiName 
        // 없으면 => 리턴값 예: _PNT("")
        public string GetFirstRmiNameOrNone(string prefix, string stringLiteralPrefix, string stringLiteralPostfix)
        {
            if(m_methods.Count == 0)
            {
                return stringLiteralPrefix + "\"\""+stringLiteralPostfix;
            }
            else
                return prefix + m_methods[0].m_name;
        }

        // xx_xxx 형태로 만든다.
        public string GetUnderbarizedName() {
            return m_name.Replace(".", "_").Replace("::","_");
        }

        public string GetCppNestedNamespaceDefinition()
        {
            return App.GetCppNestedNamespaceDefinition(m_name, false/*여기서는 class name이 없이 순수 namespace 이름만 갖고 있어서*/);
        }
        // 위 함수의 반대 역할. 즉 } } 같은걸 만든다.
        public string GetCppNestedNamespaceDefinitionEnd()
        {
            return App.GetCppNestedNamespaceDefinitionEnd(m_name, false/*여기서는 class name이 없이 순수 namespace 이름만 갖고 있어서*/);
        }
    }

    // global keyword 이전에 [] 구문 안에 선언되는 attribute. c#의 그것과 같은 문법이다.
    // 이것이 CGlobalInterface 객체에 0개 이상이 Apply()를 통해 반영된다.
    public class Parsed_GlobalInterfaceAttr
    {
        // 이 global interface의 marshaler class name
        // c# or as 등 marshaler class가 따로 설정되어야 하는 언어를 위함. 
        public Parsed_Marshaler m_marshaler;

        // public,private,internal or etc.
        public string m_accessibility = null;

        public void Apply(Parsed_GlobalInterface tgt)
        {
            if (m_marshaler != null)
                tgt.m_marshalers.Add(m_marshaler);

            // accessibility를 파서로부터 얻어서 여기에 넣는다.
            // attribute에 중복 선언된 경우 파싱 에러를 던진다.
            if (tgt.m_accessibility != null && m_accessibility != null)
            {
                throw new Exception("Duplicate attribute 'accessibility' is found!");
            }

            tgt.m_accessibility = m_accessibility;
        }

    }


    public class Parsed_Method
    {
        public string m_name;
        public List<Parsed_Param> m_params = new List<Parsed_Param>();
        public Parsed_MethodMode m_mode = new Parsed_MethodMode(); // 파싱 결과에서 mode가 전혀 없더라도 이것의 messageID 멤버가 액세스되므로 꼭 생성한다.
    }

    public class Parsed_MethodMode
    {
        /* 각 RMI 함수마다 고유 message ID를 지정할 수 있다. variable이건 int이건. 
        ""이면 지정한 적 없으므로 global에서 지정한 일련 번호가 들어감을 의미 */
        public string m_messageID;

        // [id=12345] 등, method 별 추가된 attribute에 대한 처리를 한다.
        public void ProcessAttribute(IToken start, string name, string value)
        {
            if (name.ToLower() == "id")
            {
                m_messageID = value;
            }
            else
            {
                throw new PIDLException(start, "Invalid Method Attribute. It must be id=xxxx or something.");
            }
        }
    }

    public class Parsed_Marshaler
    {
        public string m_langName; // 어떤 언어에 대해서만 작용하는 marshaler인가
        public string m_name; // marshaler 클래스 이름

    }
}
