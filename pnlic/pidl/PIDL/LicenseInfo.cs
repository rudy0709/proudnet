using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace PIDL
{
    public class LicenseInfo
    {
       public string m_name;
       public List<string> m_nameFragments = new List<string>();
       public string m_company;
       public List<string> m_companyFragments = new List<string>();
       public string m_customdata;
       public List<string> m_customdataFragments = new List<string>();
    }
}
