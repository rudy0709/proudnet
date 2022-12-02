using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace PIDL
{
    public enum ParamAttrType
    {
        None,
        In, // 기본 
        Ref,	// 파라메터 인자에 &가 붙음(C++ only)
        Value,	// 파라메터 인자에 &가 안붙음(C++ only)
        Mutable,	// 파라메터 인자에 const가 안붙음(C++ only)
        Const,	// 파라메터 인자에 const가 붙음(C++ only)
    };

    public class Parsed_Param
    {
        public Parsed_ParamAttrs m_paramAttrs = new Parsed_ParamAttrs();
        public string m_type;
        public string m_name;

    }

    public class Parsed_ParamAttr
    {
        public ParamAttrType m_type = ParamAttrType.None;

    }

    public class Parsed_ParamAttrs : List<Parsed_ParamAttr>
    {
        // 모순되는 인자가 있는지 확인한다. 있으면 오류 문구를 없으면 NULL을 리턴.
        public string GetInvalidErrorText()
        {
            if (HasType(ParamAttrType.Ref) && HasType(ParamAttrType.Value))
                return ("Only one of 'ref' and 'value' should be used.");
            if (!HasType(ParamAttrType.In))
                return ("'in' attribute must exist.");
            if (HasType(ParamAttrType.Mutable) && HasType(ParamAttrType.Const))
            {
                return ("Only one of 'mutable' and 'const' should be used.");
            }
            return null;
        }

        public bool HasType(ParamAttrType type)
        {
            foreach (var a in this)
            {
                if (a.m_type == type)
                    return true;
            }

            return false;
        }

        // 따로 선언하지 않아도 자동 추가되는 attr을 넣는다.
        public void FillDefaults()
        {
            if (!HasType(ParamAttrType.Value) && !HasType(ParamAttrType.Ref))
            {
                var a = new Parsed_ParamAttr();
                a.m_type = ParamAttrType.Ref;
                Add(a);
            }
            if (!HasType(ParamAttrType.Mutable) && !HasType(ParamAttrType.Const))
            {
                var a = new Parsed_ParamAttr();
                a.m_type = ParamAttrType.Const;
                Add(a);
            }
        }

    }

}
