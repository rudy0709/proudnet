﻿// ------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//     Runtime Version: 15.0.0.0
//  
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
// </auto-generated>
// ------------------------------------------------------------------------------
namespace PIDL
{
    using System.Linq;
    using System.Text;
    using System.Collections.Generic;
    using System;
    
    /// <summary>
    /// Class to produce the template output
    /// </summary>
    
    #line 1 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\CSCommon.tt"
    [global::System.CodeDom.Compiler.GeneratedCodeAttribute("Microsoft.VisualStudio.TextTemplating", "15.0.0.0")]
    public partial class CSCommon : CSCommonBase
    {
#line hidden
        /// <summary>
        /// Create the template output
        /// </summary>
        public virtual string TransformText()
        {
            this.Write(" \r\n");
            
            #line 1 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\AllHelper.tt"
 // 모든 언어 공통으로 사용되는 템플릿 파일

            
            #line default
            #line hidden
            this.Write("\r\n");
            this.Write("\r\n\r\n");
            this.Write("\r\n\r\n// Generated by PIDL compiler.\r\n// Do not modify this file, but modify the so" +
                    "urce .pidl file.\r\n\r\nusing System;\r\n");
            
            #line 12 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\CSCommon.tt"
 
	WriteUsings(); 
	foreach(var gi in App.g_parsed.m_globalInterfaces)
	{

            
            #line default
            #line hidden
            this.Write("namespace ");
            
            #line 17 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\CSCommon.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(gi.m_name));
            
            #line default
            #line hidden
            this.Write("\r\n{\r\n\t");
            
            #line 19 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\CSCommon.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(gi.m_accessibility));
            
            #line default
            #line hidden
            this.Write(" class Common\r\n\t{\r\n\t\t// Message ID that replies to each RMI method. \r\n");
            
            #line 22 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\CSCommon.tt"

		foreach(var mt in gi.m_methods)
		{

            
            #line default
            #line hidden
            this.Write("\t\t\tpublic const Nettention.Proud.RmiID ");
            
            #line 26 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\CSCommon.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(mt.m_name));
            
            #line default
            #line hidden
            this.Write(" = (Nettention.Proud.RmiID)");
            
            #line 26 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\CSCommon.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(mt.m_mode.m_messageID));
            
            #line default
            #line hidden
            this.Write(";\r\n");
            
            #line 27 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\CSCommon.tt"

		}

            
            #line default
            #line hidden
            this.Write("\t\t// List that has RMI ID.\r\n\t\tpublic static Nettention.Proud.RmiID[] RmiIDList = " +
                    "new Nettention.Proud.RmiID[] {\r\n");
            
            #line 32 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\CSCommon.tt"

		foreach(var mt in gi.m_methods)
		{

            
            #line default
            #line hidden
            this.Write("\t\t\t");
            
            #line 36 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\CSCommon.tt"
            this.Write(this.ToStringHelper.ToStringWithCulture(mt.m_name));
            
            #line default
            #line hidden
            this.Write(",\r\n");
            
            #line 37 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\CSCommon.tt"

		}

            
            #line default
            #line hidden
            this.Write("\t\t};\r\n\t}\r\n}\r\n");
            
            #line 43 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\CSCommon.tt"

	}

            
            #line default
            #line hidden
            this.Write("\r\n\t\t\t\t \r\n");
            return this.GenerationEnvironment.ToString();
        }
        
        #line 3 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\CSHelper.tt"
 void WriteUsings()
    {
        foreach(var ii in App.g_parsed.m_usings)
        {

        
        #line default
        #line hidden
        
        #line 7 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\CSHelper.tt"
this.Write("            \r\nusing ");

        
        #line default
        #line hidden
        
        #line 8 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\CSHelper.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(ii.m_name));

        
        #line default
        #line hidden
        
        #line 8 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\CSHelper.tt"
this.Write("; \r\n");

        
        #line default
        #line hidden
        
        #line 9 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\CSHelper.tt"
    
        }
    }

        
        #line default
        #line hidden
        
        #line 4 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\AllHelper.tt"

    // parameter definition list를 생성하는 subprogram. C++은 제외.
    void ParamDefs(Parsed_Method mt, bool startWithComma)
    {
        int cnt=0;
        foreach(var p in mt.m_params)
        {
            if(cnt>0 || startWithComma) Write(", ");
            Write(p.m_type); Write(" "); Write(p.m_name);            
            cnt++;
        }
    }

        
        #line default
        #line hidden
        
        #line 18 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\AllHelper.tt"

	// Writes like this, for example: const string RmiName_xxx = "xxx";
	// if 'hide' is true, "xxx" will be "".
    void DefRmiNames_all(string stringType, Parsed_GlobalInterface gi, bool hide)
    {

        
        #line default
        #line hidden
        
        #line 23 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\AllHelper.tt"
this.Write("// RMI name declaration.\r\n// It is the unique pointer that indicates RMI name suc" +
        "h as RMI profiler.\r\n");

        
        #line default
        #line hidden
        
        #line 26 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\AllHelper.tt"

        foreach(var mt in gi.m_methods)
        {		

        
        #line default
        #line hidden
        
        #line 30 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\AllHelper.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(stringType));

        
        #line default
        #line hidden
        
        #line 30 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\AllHelper.tt"
this.Write(" RmiName_");

        
        #line default
        #line hidden
        
        #line 30 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\AllHelper.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(mt.m_name));

        
        #line default
        #line hidden
        
        #line 30 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\AllHelper.tt"
this.Write("=\"");

        
        #line default
        #line hidden
        
        #line 30 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\AllHelper.tt"
 if(hide) Write(""); else Write(mt.m_name); 
        
        #line default
        #line hidden
        
        #line 30 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\AllHelper.tt"
this.Write("\";\r\n");

        
        #line default
        #line hidden
        
        #line 31 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\AllHelper.tt"

        }

        
        #line default
        #line hidden
        
        #line 33 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\AllHelper.tt"
this.Write("       \r\n");

        
        #line default
        #line hidden
        
        #line 34 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\AllHelper.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(stringType));

        
        #line default
        #line hidden
        
        #line 34 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\AllHelper.tt"
this.Write(" RmiName_First = ");

        
        #line default
        #line hidden
        
        #line 34 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\AllHelper.tt"
 if(hide) Write("\"\""); else Write(gi.GetFirstRmiNameOrNone("RmiName_", "", "" )); 
        
        #line default
        #line hidden
        
        #line 34 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\AllHelper.tt"
this.Write(";\r\n");

        
        #line default
        #line hidden
        
        #line 35 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\AllHelper.tt"

    }

        
        #line default
        #line hidden
        
        #line 41 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\AllHelper.tt"

    void Paramcalls(Parsed_Method mt,bool startWithComma)  
    {
        int cnt = 0;
        foreach(var p in mt.m_params)
        {
            if(cnt>0 || startWithComma) Write(", ");
            Write(p.m_name);
            cnt++;
        }         
    }        

        
        #line default
        #line hidden
        
        #line 54 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\AllHelper.tt"

    void ImportDecl(string keyword)
    {
        foreach(var ii in App.g_parsed.m_imports)
        {

        
        #line default
        #line hidden
        
        #line 60 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\AllHelper.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(keyword));

        
        #line default
        #line hidden
        
        #line 60 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\AllHelper.tt"
this.Write(" ");

        
        #line default
        #line hidden
        
        #line 60 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\AllHelper.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(ii.m_name));

        
        #line default
        #line hidden
        
        #line 60 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\AllHelper.tt"
this.Write(";\r\n");

        
        #line default
        #line hidden
        
        #line 61 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\AllHelper.tt"

        }
    }

        
        #line default
        #line hidden
        
        #line 66 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\AllHelper.tt"
   
    // e.g. package MyPackage;
    void PackageDecl()
    {
        if(App.g_parsed.m_package != null)
        {

        
        #line default
        #line hidden
        
        #line 72 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\AllHelper.tt"
this.Write("package ");

        
        #line default
        #line hidden
        
        #line 73 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\AllHelper.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(App.g_parsed.m_package.m_name));

        
        #line default
        #line hidden
        
        #line 73 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\AllHelper.tt"
this.Write(";\r\n");

        
        #line default
        #line hidden
        
        #line 74 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\AllHelper.tt"

        }
    }

        
        #line default
        #line hidden
    }
    
    #line default
    #line hidden
    #region Base class
    /// <summary>
    /// Base class for this transformation
    /// </summary>
    [global::System.CodeDom.Compiler.GeneratedCodeAttribute("Microsoft.VisualStudio.TextTemplating", "15.0.0.0")]
    public class CSCommonBase
    {
        #region Fields
        private global::System.Text.StringBuilder generationEnvironmentField;
        private global::System.CodeDom.Compiler.CompilerErrorCollection errorsField;
        private global::System.Collections.Generic.List<int> indentLengthsField;
        private string currentIndentField = "";
        private bool endsWithNewline;
        private global::System.Collections.Generic.IDictionary<string, object> sessionField;
        #endregion
        #region Properties
        /// <summary>
        /// The string builder that generation-time code is using to assemble generated output
        /// </summary>
        protected System.Text.StringBuilder GenerationEnvironment
        {
            get
            {
                if ((this.generationEnvironmentField == null))
                {
                    this.generationEnvironmentField = new global::System.Text.StringBuilder();
                }
                return this.generationEnvironmentField;
            }
            set
            {
                this.generationEnvironmentField = value;
            }
        }
        /// <summary>
        /// The error collection for the generation process
        /// </summary>
        public System.CodeDom.Compiler.CompilerErrorCollection Errors
        {
            get
            {
                if ((this.errorsField == null))
                {
                    this.errorsField = new global::System.CodeDom.Compiler.CompilerErrorCollection();
                }
                return this.errorsField;
            }
        }
        /// <summary>
        /// A list of the lengths of each indent that was added with PushIndent
        /// </summary>
        private System.Collections.Generic.List<int> indentLengths
        {
            get
            {
                if ((this.indentLengthsField == null))
                {
                    this.indentLengthsField = new global::System.Collections.Generic.List<int>();
                }
                return this.indentLengthsField;
            }
        }
        /// <summary>
        /// Gets the current indent we use when adding lines to the output
        /// </summary>
        public string CurrentIndent
        {
            get
            {
                return this.currentIndentField;
            }
        }
        /// <summary>
        /// Current transformation session
        /// </summary>
        public virtual global::System.Collections.Generic.IDictionary<string, object> Session
        {
            get
            {
                return this.sessionField;
            }
            set
            {
                this.sessionField = value;
            }
        }
        #endregion
        #region Transform-time helpers
        /// <summary>
        /// Write text directly into the generated output
        /// </summary>
        public void Write(string textToAppend)
        {
            if (string.IsNullOrEmpty(textToAppend))
            {
                return;
            }
            // If we're starting off, or if the previous text ended with a newline,
            // we have to append the current indent first.
            if (((this.GenerationEnvironment.Length == 0) 
                        || this.endsWithNewline))
            {
                this.GenerationEnvironment.Append(this.currentIndentField);
                this.endsWithNewline = false;
            }
            // Check if the current text ends with a newline
            if (textToAppend.EndsWith(global::System.Environment.NewLine, global::System.StringComparison.CurrentCulture))
            {
                this.endsWithNewline = true;
            }
            // This is an optimization. If the current indent is "", then we don't have to do any
            // of the more complex stuff further down.
            if ((this.currentIndentField.Length == 0))
            {
                this.GenerationEnvironment.Append(textToAppend);
                return;
            }
            // Everywhere there is a newline in the text, add an indent after it
            textToAppend = textToAppend.Replace(global::System.Environment.NewLine, (global::System.Environment.NewLine + this.currentIndentField));
            // If the text ends with a newline, then we should strip off the indent added at the very end
            // because the appropriate indent will be added when the next time Write() is called
            if (this.endsWithNewline)
            {
                this.GenerationEnvironment.Append(textToAppend, 0, (textToAppend.Length - this.currentIndentField.Length));
            }
            else
            {
                this.GenerationEnvironment.Append(textToAppend);
            }
        }
        /// <summary>
        /// Write text directly into the generated output
        /// </summary>
        public void WriteLine(string textToAppend)
        {
            this.Write(textToAppend);
            this.GenerationEnvironment.AppendLine();
            this.endsWithNewline = true;
        }
        /// <summary>
        /// Write formatted text directly into the generated output
        /// </summary>
        public void Write(string format, params object[] args)
        {
            this.Write(string.Format(global::System.Globalization.CultureInfo.CurrentCulture, format, args));
        }
        /// <summary>
        /// Write formatted text directly into the generated output
        /// </summary>
        public void WriteLine(string format, params object[] args)
        {
            this.WriteLine(string.Format(global::System.Globalization.CultureInfo.CurrentCulture, format, args));
        }
        /// <summary>
        /// Raise an error
        /// </summary>
        public void Error(string message)
        {
            System.CodeDom.Compiler.CompilerError error = new global::System.CodeDom.Compiler.CompilerError();
            error.ErrorText = message;
            this.Errors.Add(error);
        }
        /// <summary>
        /// Raise a warning
        /// </summary>
        public void Warning(string message)
        {
            System.CodeDom.Compiler.CompilerError error = new global::System.CodeDom.Compiler.CompilerError();
            error.ErrorText = message;
            error.IsWarning = true;
            this.Errors.Add(error);
        }
        /// <summary>
        /// Increase the indent
        /// </summary>
        public void PushIndent(string indent)
        {
            if ((indent == null))
            {
                throw new global::System.ArgumentNullException("indent");
            }
            this.currentIndentField = (this.currentIndentField + indent);
            this.indentLengths.Add(indent.Length);
        }
        /// <summary>
        /// Remove the last indent that was added with PushIndent
        /// </summary>
        public string PopIndent()
        {
            string returnValue = "";
            if ((this.indentLengths.Count > 0))
            {
                int indentLength = this.indentLengths[(this.indentLengths.Count - 1)];
                this.indentLengths.RemoveAt((this.indentLengths.Count - 1));
                if ((indentLength > 0))
                {
                    returnValue = this.currentIndentField.Substring((this.currentIndentField.Length - indentLength));
                    this.currentIndentField = this.currentIndentField.Remove((this.currentIndentField.Length - indentLength));
                }
            }
            return returnValue;
        }
        /// <summary>
        /// Remove any indentation
        /// </summary>
        public void ClearIndent()
        {
            this.indentLengths.Clear();
            this.currentIndentField = "";
        }
        #endregion
        #region ToString Helpers
        /// <summary>
        /// Utility class to produce culture-oriented representation of an object as a string.
        /// </summary>
        public class ToStringInstanceHelper
        {
            private System.IFormatProvider formatProviderField  = global::System.Globalization.CultureInfo.InvariantCulture;
            /// <summary>
            /// Gets or sets format provider to be used by ToStringWithCulture method.
            /// </summary>
            public System.IFormatProvider FormatProvider
            {
                get
                {
                    return this.formatProviderField ;
                }
                set
                {
                    if ((value != null))
                    {
                        this.formatProviderField  = value;
                    }
                }
            }
            /// <summary>
            /// This is called from the compile/run appdomain to convert objects within an expression block to a string
            /// </summary>
            public string ToStringWithCulture(object objectToConvert)
            {
                if ((objectToConvert == null))
                {
                    throw new global::System.ArgumentNullException("objectToConvert");
                }
                System.Type t = objectToConvert.GetType();
                System.Reflection.MethodInfo method = t.GetMethod("ToString", new System.Type[] {
                            typeof(System.IFormatProvider)});
                if ((method == null))
                {
                    return objectToConvert.ToString();
                }
                else
                {
                    return ((string)(method.Invoke(objectToConvert, new object[] {
                                this.formatProviderField })));
                }
            }
        }
        private ToStringInstanceHelper toStringHelperField = new ToStringInstanceHelper();
        /// <summary>
        /// Helper to produce culture-oriented representation of an object as a string
        /// </summary>
        public ToStringInstanceHelper ToStringHelper
        {
            get
            {
                return this.toStringHelperField;
            }
        }
        #endregion
    }
    #endregion
}
