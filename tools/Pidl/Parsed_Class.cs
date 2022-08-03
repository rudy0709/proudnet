using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace PIDL
{
    public class Parsed_Class
    {
        public string m_name;   // namespace가 포함된 이름
        public Parsed_TypeAttr m_attr;

        public List<Parsed_Variable> m_variables = new List<Parsed_Variable>();

    }

    public enum Parsed_TypeAttrEnum
    {
        None,
        Private,
        Internal,
        Public,
    }

    // enum or class의 속성
    public class Parsed_TypeAttr
    {
        public const string privateOrPublicErrorText = "`private` or `public` must be set only once.";

        public Parsed_TypeAttrEnum m_enum = Parsed_TypeAttrEnum.None;

        public void Set(Parsed_TypeAttrEnum e)
        {
            if(m_enum != Parsed_TypeAttrEnum.None)
                throw new Exception(privateOrPublicErrorText);
            m_enum = e;
        }

        public string GetAttrStatement()
        {
            switch (m_enum)
            {
                case Parsed_TypeAttrEnum.Public:
                    return "public ";
                case Parsed_TypeAttrEnum.Internal:
                    return "internal ";
                default:
                    return "private ";
            }
        }

    }


    public class Parsed_Variable
    {
        public string m_type;
        public string m_name;
        public Parsed_Value m_value = null;

        public Parsed_VariableAttr m_attr = null;

        // 출력 예: const int x=1;
        public string GetCppDefinitionStatement(string className)
        {
            // _PNT 붙이는거도 만들자. App.g_lang 따져서.

            string ret = "";
            //if (true == m_attr.m_isStatic)            // C++ impl부분에서는 이거 안 붙인다.
            //{
            //    ret += "static ";
            //}
            if (true == m_attr.m_isConst)
            {
                ret += "const ";
            }
            string actualType = App.GetVariableTypeByLanguage(m_attr.m_isConst , m_type);

            if(string.IsNullOrEmpty(className))
                ret += $"{actualType} {m_name}";
            else
                ret += $"{actualType} {className}::{m_name}";
            if (null != m_value)
            {
                ret += $" = {App.GetValueStatement(m_value.m_isStringLiteral, m_value.m_text)}";
            }

            ret += ";";

            return ret;
        }

        public string GetCSDefinitionStatement()
        {
            // _PNT 붙이는거도 만들자. App.g_lang 따져서.

            string ret = "";
            ret += m_attr.GetAttrStatement();

            if (true == m_attr.m_isStatic && false == m_attr.m_isConst)
            {
                ret += "static ";
            }
            if (true == m_attr.m_isConst)
            {
                ret += "const ";
            }
            string actualType = App.GetVariableTypeByLanguage(m_attr.m_isConst, m_type);

            ret += $"{actualType} {m_name}";
            if (null != m_value)
            {
                ret += $" = {App.GetValueStatement(m_value.m_isStringLiteral, m_value.m_text)}";
            }

            ret += ";";

            return ret;
        }

        // 출력 예: static const PNTCHAR* xxx;
        public string GetCppDeclarationStatement()
        {
            string ret = "";
            ret += m_attr.GetAttrStatement() + ":  ";

            if (true == m_attr.m_isStatic)
            {
                ret += "static ";
            }
            if (true == m_attr.m_isConst)
            {
                ret += "const ";
            }

            string actualType = App.GetVariableTypeByLanguage(m_attr.m_isConst, m_type);

            ret += $"{actualType} {m_name}";

            ret += ";";

            return ret;
        }
    }

    public class Parsed_VariableAttr: Parsed_TypeAttr
    {
        // const 키워드가 선언됐나
        public bool m_isConst = false;

        // static 키워드가 선언됐나
        public bool m_isStatic = false;
    }

    public class Parsed_Value
    {
        public string m_text = null;
        public bool m_isStringLiteral = false;   // true이면 codegen시 따옴표를 붙여야.
    }

}
