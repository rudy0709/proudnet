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
    
    #line 1 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
    [global::System.CodeDom.Compiler.GeneratedCodeAttribute("Microsoft.VisualStudio.TextTemplating", "15.0.0.0")]
    public partial class UCProxy : UCProxyBase
    {
#line hidden
        /// <summary>
        /// Create the template output
        /// </summary>
        public virtual string TransformText()
        {
            
            #line 1 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\AllHelper.tt"
 // 모든 언어 공통으로 사용되는 템플릿 파일

            
            #line default
            #line hidden
            this.Write("\r\n");
            this.Write("\r\n\r\n");
            this.Write("\r\n\r\n// Generated by PIDL compiler.\r\n// Do not modify this file, but modify the so" +
                    "urce .pidl file.\r\n\r\n");
            
            #line 11 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"

	ImportDecl("dependson");

    foreach(var gi in App.g_parsed.m_globalInterfaces)
    {
        ProxyClassDef(gi);
    }

            
            #line default
            #line hidden
            this.Write("\r\n");
            return this.GenerationEnvironment.ToString();
        }
        
        #line 20 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"

    void ProxyClassDef(Parsed_GlobalInterface gi)
    {

        
        #line default
        #line hidden
        
        #line 23 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
this.Write("class ");

        
        #line default
        #line hidden
        
        #line 24 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(App.m_ProxyUCFileNameWithoutExtension));

        
        #line default
        #line hidden
        
        #line 24 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
this.Write(" extends IRmiProxy\r\n");

        
        #line default
        #line hidden
        
        #line 25 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
 
        foreach(var ii in App.g_parsed.m_imports)
        {

        
        #line default
        #line hidden
        
        #line 28 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
this.Write("    dependson(");

        
        #line default
        #line hidden
        
        #line 29 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(ii.m_name));

        
        #line default
        #line hidden
        
        #line 29 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
this.Write(")\r\n");

        
        #line default
        #line hidden
        
        #line 30 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"

        }
        DefRmiNames(App.m_ProxyUCFileNameWithoutExtension, gi);
        DefRmiIDList(App.m_ProxyUCFileNameWithoutExtension, gi);

        foreach(var mt in gi.m_methods)
        {
            ProxyMethodDef(gi, mt);
        }

        DefInitEvent(App.m_ProxyUCFileNameWithoutExtension, gi);

        
        #line default
        #line hidden
        
        #line 41 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
this.Write("\r\ndefaultproperties\r\n{\r\n\t// defaultproperties\r\n}\r\n\r\n");

        
        #line default
        #line hidden
        
        #line 48 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"

    }

        
        #line default
        #line hidden
        
        #line 52 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
 
    void ProxyMethodDef(Parsed_GlobalInterface gi, Parsed_Method mt) 
    {

        
        #line default
        #line hidden
        
        #line 55 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
this.Write("\t\r\n\tfinal function bool ");

        
        #line default
        #line hidden
        
        #line 57 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
MultiMethodDecl(mt); 
        
        #line default
        #line hidden
        
        #line 57 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
this.Write("\t{\r\n\t\t");

        
        #line default
        #line hidden
        
        #line 59 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
 SerializeParams(gi,mt); 
        
        #line default
        #line hidden
        
        #line 59 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
this.Write("\t\t\r\n\t\treturn RmiSend(remotes, context, __msg, RmiName_");

        
        #line default
        #line hidden
        
        #line 61 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(App.m_ProxyUCFileNameWithoutExtension));

        
        #line default
        #line hidden
        
        #line 61 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
this.Write("_");

        
        #line default
        #line hidden
        
        #line 61 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(mt.m_name));

        
        #line default
        #line hidden
        
        #line 61 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
this.Write(", Rmi_");

        
        #line default
        #line hidden
        
        #line 61 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(App.m_ProxyUCFileNameWithoutExtension));

        
        #line default
        #line hidden
        
        #line 61 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
this.Write("_");

        
        #line default
        #line hidden
        
        #line 61 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(mt.m_name));

        
        #line default
        #line hidden
        
        #line 61 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
this.Write(");\r\n\t}\r\n");

        
        #line default
        #line hidden
        
        #line 63 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"

    }

        
        #line default
        #line hidden
        
        #line 67 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"

    void SerializeParams(Parsed_GlobalInterface gi, Parsed_Method mt)
    {

        
        #line default
        #line hidden
        
        #line 70 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
this.Write("\tlocal int __msgid;\r\n\tlocal Message __msg;\r\n\t\r\n\t__msg = class\'Message\'.static.Cre" +
        "ate();\r\n\t__msgid = Rmi_");

        
        #line default
        #line hidden
        
        #line 75 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(App.m_ProxyUCFileNameWithoutExtension));

        
        #line default
        #line hidden
        
        #line 75 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
this.Write("_");

        
        #line default
        #line hidden
        
        #line 75 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(mt.m_name));

        
        #line default
        #line hidden
        
        #line 75 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
this.Write(";\r\n\r\n\t__msg.Write_RmiID(__msgid);\r\n\t\r\n");

        
        #line default
        #line hidden
        
        #line 79 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
 
        foreach(var p in mt.m_params)
        {

        
        #line default
        #line hidden
        
        #line 82 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
this.Write("\tclass\'");

        
        #line default
        #line hidden
        
        #line 83 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(gi.m_marshaler));

        
        #line default
        #line hidden
        
        #line 83 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
this.Write("\'.static.Write_");

        
        #line default
        #line hidden
        
        #line 83 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(p.m_type));

        
        #line default
        #line hidden
        
        #line 83 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
this.Write("(__msg,");

        
        #line default
        #line hidden
        
        #line 83 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(p.m_name));

        
        #line default
        #line hidden
        
        #line 83 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
this.Write(");\r\n");

        
        #line default
        #line hidden
        
        #line 84 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"

        }
    }

        
        #line default
        #line hidden
        
        #line 89 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
 
    void MultiMethodDecl(Parsed_Method mt)  
    {

        
        #line default
        #line hidden
        
        #line 92 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
this.Write("\t");

        
        #line default
        #line hidden
        
        #line 93 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(mt.m_name));

        
        #line default
        #line hidden
        
        #line 93 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
this.Write("( array<int> remotes, RmiContext context");

        
        #line default
        #line hidden
        
        #line 93 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
ParamDefs(mt,true);
        
        #line default
        #line hidden
        
        #line 93 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"
this.Write(")\r\n");

        
        #line default
        #line hidden
        
        #line 94 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCProxy.tt"

    }

        
        #line default
        #line hidden
        
        #line 3 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCHelper.tt"

    void DefRmiIDList(string prefix,Parsed_GlobalInterface gi)
    {
        foreach(var mt in gi.m_methods)
        {

        
        #line default
        #line hidden
        
        #line 8 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCHelper.tt"
this.Write("var int Rmi_");

        
        #line default
        #line hidden
        
        #line 9 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCHelper.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(prefix));

        
        #line default
        #line hidden
        
        #line 9 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCHelper.tt"
this.Write("_");

        
        #line default
        #line hidden
        
        #line 9 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCHelper.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(mt.m_name));

        
        #line default
        #line hidden
        
        #line 9 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCHelper.tt"
this.Write(";\r\n");

        
        #line default
        #line hidden
        
        #line 10 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCHelper.tt"

        }
    }

        
        #line default
        #line hidden
        
        #line 15 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCHelper.tt"

    void DefInitEvent(string prefix, Parsed_GlobalInterface gi)
    {

        
        #line default
        #line hidden
        
        #line 18 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCHelper.tt"
this.Write("event InitRmiIDList()\r\n{\r\n");

        
        #line default
        #line hidden
        
        #line 21 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCHelper.tt"

        foreach(var mt in gi.m_methods)
        {

        
        #line default
        #line hidden
        
        #line 24 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCHelper.tt"
this.Write("    Rmi_");

        
        #line default
        #line hidden
        
        #line 25 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCHelper.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(prefix));

        
        #line default
        #line hidden
        
        #line 25 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCHelper.tt"
this.Write("_");

        
        #line default
        #line hidden
        
        #line 25 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCHelper.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(mt.m_name));

        
        #line default
        #line hidden
        
        #line 25 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCHelper.tt"
this.Write(" = (");

        
        #line default
        #line hidden
        
        #line 25 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCHelper.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(mt.m_mode.m_messageID));

        
        #line default
        #line hidden
        
        #line 25 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCHelper.tt"
this.Write(");\r\n    m_rmiIDList.AddItem( ");

        
        #line default
        #line hidden
        
        #line 26 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCHelper.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(mt.m_mode.m_messageID));

        
        #line default
        #line hidden
        
        #line 26 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCHelper.tt"
this.Write("));\r\n");

        
        #line default
        #line hidden
        
        #line 27 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCHelper.tt"

        }

        
        #line default
        #line hidden
        
        #line 29 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCHelper.tt"
this.Write("}\r\n");

        
        #line default
        #line hidden
        
        #line 31 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCHelper.tt"

    }

        
        #line default
        #line hidden
        
        #line 36 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCHelper.tt"
 
    void DefRmiNames(string prefix,Parsed_GlobalInterface gi) 
	{
        foreach(var mt in gi.m_methods)
        {

        
        #line default
        #line hidden
        
        #line 41 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCHelper.tt"
this.Write("const RmiName_");

        
        #line default
        #line hidden
        
        #line 42 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCHelper.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(prefix));

        
        #line default
        #line hidden
        
        #line 42 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCHelper.tt"
this.Write("_");

        
        #line default
        #line hidden
        
        #line 42 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCHelper.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(mt.m_name));

        
        #line default
        #line hidden
        
        #line 42 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCHelper.tt"
this.Write(" = \"");

        
        #line default
        #line hidden
        
        #line 42 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCHelper.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(mt.m_name));

        
        #line default
        #line hidden
        
        #line 42 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCHelper.tt"
this.Write("\";\r\n");

        
        #line default
        #line hidden
        
        #line 43 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCHelper.tt"

        }

        
        #line default
        #line hidden
        
        #line 45 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCHelper.tt"
this.Write("const RmiName_");

        
        #line default
        #line hidden
        
        #line 46 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCHelper.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(prefix));

        
        #line default
        #line hidden
        
        #line 46 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCHelper.tt"
this.Write("_First = \"");

        
        #line default
        #line hidden
        
        #line 46 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCHelper.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(gi.GetFirstRmiNameOrNone("","","")));

        
        #line default
        #line hidden
        
        #line 46 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCHelper.tt"
this.Write("\";\r\n");

        
        #line default
        #line hidden
        
        #line 47 "D:\imays-2\works\engine\dev004-01\R2\ProudNet\UtilSrc\PIDL\PIDL\OutputTemplate\UCHelper.tt"
 
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
    public class UCProxyBase
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
