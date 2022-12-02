using Antlr4.Runtime;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace PIDL
{
    public class PIDLException : Exception
    {
        public int line;
        public int column;
        public string comment;

        public PIDLException(IToken token, string comment)
        {
            line = token.Line;
            column = token.Column;
            this.comment = comment;
        }

        public PIDLException(int line, int column, string comment)
        {
            this.line = line;
            this.column = column;
            this.comment = comment;
        }
    }
}
