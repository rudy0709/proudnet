using Antlr4.Runtime;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace PIDL
{
    public class PidlException : Exception
    {
        public int line;
        public int column;
        public string comment;

        public PidlException(IToken token, string comment)
        {
            line = token.Line;
            column = token.Column;
            this.comment = comment;
        }

        public PidlException(int line, int column, string comment)
        {
            this.line = line;
            this.column = column;
            this.comment = comment;
        }
    }
}
