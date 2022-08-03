using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace PIDL
{
    public enum Lang
    {
        Undefined = -1,
        //CPP = 1,
        CS = 2, // C#
        //CLR,
        CPP, // C++
        //AS,
        JAVA,
        UC // Unreal Script (이제는 안 씀)
    };

    public class LangUtil
    {
        static public Lang GetLangEnum(string langName)
        {
            foreach (Lang l in Enum.GetValues(typeof(Lang)))
            {
                if (l.ToString().ToLower() == langName.ToLower())
                    return l;
            }
            return Lang.Undefined;
        }
    }
}
