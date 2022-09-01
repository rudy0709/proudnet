using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace PIDL
{
    public class Parsed_Enum
    {
        public string m_name;      // namespace 포함된 이름
        public Parsed_TypeAttr m_attr;

        public List<Parsed_EnumItem> m_items = new List<Parsed_EnumItem>();

    }

    public class Parsed_EnumItem
    {
        public string m_name;
        public string m_value=null;

        // 출력 예: aaa, 혹은 bbb=ccc,
        public string GetDefinitionStatement()
        {
            string ret = m_name;
            if (null != m_value)
                ret += " = " + m_value;

            return ret;
        }
    }

}
