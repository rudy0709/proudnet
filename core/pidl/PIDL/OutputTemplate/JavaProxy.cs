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
    
    #line 1 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
    [global::System.CodeDom.Compiler.GeneratedCodeAttribute("Microsoft.VisualStudio.TextTemplating", "15.0.0.0")]
    public partial class JavaProxy : JavaProxyBase
    {
#line hidden
        /// <summary>
        /// Create the template output
        /// </summary>
        public virtual string TransformText()
        {
            this.Write(" \r\n");
            
            #line 1 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\AllHelper.ttinclude"
 // 모든 언어 공통으로 사용되는 템플릿 파일

            
            #line default
            #line hidden
            this.Write("\r\n");
            this.Write("\r\n\r\n");
            this.Write("\r\n\r\n// Generated by PIDL compiler.\r\n// Do not modify this file, but modify the so" +
                    "urce .pidl file.\r\n\r\n");
            
            #line 11 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"

    PackageDecl();

            
            #line default
            #line hidden
            this.Write("import com.nettention.proud.*;\r\n");
            
            #line 15 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"

    ImportDecl("import");

    foreach(var gi in App.g_parsed.m_globalInterfaces)
    {
        ProxyClassDef(gi);
    }

            
            #line default
            #line hidden
            this.Write("\r\n");
            return this.GenerationEnvironment.ToString();
        }
        
        #line 24 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
 
    void ProxyClassDef(Parsed_GlobalInterface gi) 
    {

        
        #line default
        #line hidden
        
        #line 27 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
this.Write("\tpublic class ");

        
        #line default
        #line hidden
        
        #line 28 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(App.m_ProxyJavaFileNameWithoutExtension));

        
        #line default
        #line hidden
        
        #line 28 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
this.Write(" extends com.nettention.proud.RmiProxy\r\n\t{\r\n");

        
        #line default
        #line hidden
        
        #line 30 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"

        foreach(var mt in gi.m_methods)
        {
            ProxyMethodDef(gi, mt);
        }
        DefRmiNames_all("final String", gi, true);

        
        #line default
        #line hidden
        
        #line 36 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
this.Write("\t\tpublic int[] getRmiIDList() { return ");

        
        #line default
        #line hidden
        
        #line 37 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(App.m_ProxyJavaFileNameWithoutExtension));

        
        #line default
        #line hidden
        
        #line 37 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
this.Write(".RmiIDList; }\r\n\t\tpublic int getRmiIDListCount() { return ");

        
        #line default
        #line hidden
        
        #line 38 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(App.m_ProxyJavaFileNameWithoutExtension));

        
        #line default
        #line hidden
        
        #line 38 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
this.Write(".RmiIDList.length; }\r\n\t}\r\n");

        
        #line default
        #line hidden
        
        #line 40 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"

    }

        
        #line default
        #line hidden
        
        #line 44 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"

    void ProxyMethodDef(Parsed_GlobalInterface gi, Parsed_Method mt)
    {

        
        #line default
        #line hidden
        
        #line 47 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
this.Write("\tpublic boolean ");

        
        #line default
        #line hidden
        
        #line 48 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
 MethodDecl(mt); 
        
        #line default
        #line hidden
        
        #line 48 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
this.Write("\t{\r\n\t\t");

        
        #line default
        #line hidden
        
        #line 50 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
 SerializeParams(gi,mt);
        
        #line default
        #line hidden
        
        #line 50 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
this.Write("\t\t\r\n\t\tint[] __list = new int[1];\r\n\t\t__list[0] = remote;\r\n\t\t\r\n\t\treturn rmiSend(__l" +
        "ist,rmiContext,__msg,\r\n\t\t\tRmiName_");

        
        #line default
        #line hidden
        
        #line 56 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(mt.m_name));

        
        #line default
        #line hidden
        
        #line 56 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
this.Write(", ");

        
        #line default
        #line hidden
        
        #line 56 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(App.m_CommonJavaFileNameWithoutExtension));

        
        #line default
        #line hidden
        
        #line 56 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
this.Write(".");

        
        #line default
        #line hidden
        
        #line 56 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(mt.m_name));

        
        #line default
        #line hidden
        
        #line 56 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
this.Write(");\r\n\t}\r\n\r\n\tpublic boolean ");

        
        #line default
        #line hidden
        
        #line 59 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
 MultiMethodDecl(mt); 
        
        #line default
        #line hidden
        
        #line 59 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
this.Write("\t{\r\n\t\t");

        
        #line default
        #line hidden
        
        #line 61 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
 SerializeParams(gi,mt);
        
        #line default
        #line hidden
        
        #line 61 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
this.Write("\t\t\r\n\t\treturn rmiSend(remotes,rmiContext,__msg,\r\n\t\t\tRmiName_");

        
        #line default
        #line hidden
        
        #line 64 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(mt.m_name));

        
        #line default
        #line hidden
        
        #line 64 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
this.Write(", ");

        
        #line default
        #line hidden
        
        #line 64 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(App.m_CommonJavaFileNameWithoutExtension));

        
        #line default
        #line hidden
        
        #line 64 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
this.Write(".");

        
        #line default
        #line hidden
        
        #line 64 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(mt.m_name));

        
        #line default
        #line hidden
        
        #line 64 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
this.Write(");\r\n\t}\r\n");

        
        #line default
        #line hidden
        
        #line 66 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"

    }

        
        #line default
        #line hidden
        
        #line 70 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
 
    void SerializeParams(Parsed_GlobalInterface gi, Parsed_Method mt)
    {
 
        
        #line default
        #line hidden
        
        #line 73 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
this.Write("    com.nettention.proud.Message __msg=new com.nettention.proud.Message();\r\n\t\r\n\t_" +
        "_msg.setSimplePacketMode(core.isSimplePacketMode());\r\n\r\n\tint __msgid = ");

        
        #line default
        #line hidden
        
        #line 78 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(App.m_CommonJavaFileNameWithoutExtension));

        
        #line default
        #line hidden
        
        #line 78 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
this.Write(".");

        
        #line default
        #line hidden
        
        #line 78 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(mt.m_name));

        
        #line default
        #line hidden
        
        #line 78 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
this.Write(";\r\n\t__msg.writeRmiID(__msgid);\r\n\t\r\n");

        
        #line default
        #line hidden
        
        #line 81 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"

        foreach(var p in mt.m_params)
        {

        
        #line default
        #line hidden
        
        #line 84 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
this.Write("\t");

        
        #line default
        #line hidden
        
        #line 85 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(gi.m_marshaler));

        
        #line default
        #line hidden
        
        #line 85 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
this.Write(".write(__msg,");

        
        #line default
        #line hidden
        
        #line 85 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(p.m_name));

        
        #line default
        #line hidden
        
        #line 85 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"
this.Write(");\r\n");

        
        #line default
        #line hidden
        
        #line 86 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaProxy.tt"

        }
    }

        
        #line default
        #line hidden
        
        #line 3 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaHelper.ttinclude"
 
    // method declaration statement을 생성하는 subprogram.
    void MethodDecl(Parsed_Method mt)   
    {

        
        #line default
        #line hidden
        
        #line 8 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaHelper.ttinclude"
this.Write(this.ToStringHelper.ToStringWithCulture(mt.m_name));

        
        #line default
        #line hidden
        
        #line 8 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaHelper.ttinclude"
this.Write("(int remote,com.nettention.proud.RmiContext rmiContext");

        
        #line default
        #line hidden
        
        #line 8 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaHelper.ttinclude"
 ParamDefs(mt,true);
        
        #line default
        #line hidden
        
        #line 8 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaHelper.ttinclude"
this.Write(")\r\n");

        
        #line default
        #line hidden
        
        #line 9 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaHelper.ttinclude"

    }

        
        #line default
        #line hidden
        
        #line 13 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaHelper.ttinclude"
 
    // method declaration statement을 생성하는 subprogram.
    void MultiMethodDecl(Parsed_Method mt)   
    {

        
        #line default
        #line hidden
        
        #line 18 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaHelper.ttinclude"
this.Write(this.ToStringHelper.ToStringWithCulture(mt.m_name));

        
        #line default
        #line hidden
        
        #line 18 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaHelper.ttinclude"
this.Write("(int[] remotes,com.nettention.proud.RmiContext rmiContext");

        
        #line default
        #line hidden
        
        #line 18 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaHelper.ttinclude"
 ParamDefs(mt,true);
        
        #line default
        #line hidden
        
        #line 18 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaHelper.ttinclude"
this.Write(")\r\n");

        
        #line default
        #line hidden
        
        #line 19 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\JavaHelper.ttinclude"

    }

        
        #line default
        #line hidden
        
        #line 4 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\AllHelper.ttinclude"

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
        
        #line 18 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\AllHelper.ttinclude"

	// Writes like this, for example: const string RmiName_xxx = "xxx";
	// if 'hide' is true, "xxx" will be "".
    void DefRmiNames_all(string stringType, Parsed_GlobalInterface gi, bool hide)
    {

        
        #line default
        #line hidden
        
        #line 23 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\AllHelper.ttinclude"
this.Write("// RMI name declaration.\r\n// It is the unique pointer that indicates RMI name suc" +
        "h as RMI profiler.\r\n");

        
        #line default
        #line hidden
        
        #line 26 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\AllHelper.ttinclude"

        foreach(var mt in gi.m_methods)
        {		

        
        #line default
        #line hidden
        
        #line 30 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\AllHelper.ttinclude"
this.Write(this.ToStringHelper.ToStringWithCulture(stringType));

        
        #line default
        #line hidden
        
        #line 30 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\AllHelper.ttinclude"
this.Write(" RmiName_");

        
        #line default
        #line hidden
        
        #line 30 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\AllHelper.ttinclude"
this.Write(this.ToStringHelper.ToStringWithCulture(mt.m_name));

        
        #line default
        #line hidden
        
        #line 30 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\AllHelper.ttinclude"
this.Write("=\"");

        
        #line default
        #line hidden
        
        #line 30 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\AllHelper.ttinclude"
 if(hide) Write(""); else Write(mt.m_name); 
        
        #line default
        #line hidden
        
        #line 30 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\AllHelper.ttinclude"
this.Write("\";\r\n");

        
        #line default
        #line hidden
        
        #line 31 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\AllHelper.ttinclude"

        }

        
        #line default
        #line hidden
        
        #line 33 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\AllHelper.ttinclude"
this.Write("       \r\n");

        
        #line default
        #line hidden
        
        #line 34 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\AllHelper.ttinclude"
this.Write(this.ToStringHelper.ToStringWithCulture(stringType));

        
        #line default
        #line hidden
        
        #line 34 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\AllHelper.ttinclude"
this.Write(" RmiName_First = ");

        
        #line default
        #line hidden
        
        #line 34 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\AllHelper.ttinclude"
 if(hide) Write("\"\""); else Write(gi.GetFirstRmiNameOrNone("RmiName_", "", "" )); 
        
        #line default
        #line hidden
        
        #line 34 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\AllHelper.ttinclude"
this.Write(";\r\n");

        
        #line default
        #line hidden
        
        #line 35 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\AllHelper.ttinclude"

    }

        
        #line default
        #line hidden
        
        #line 41 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\AllHelper.ttinclude"

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
        
        #line 54 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\AllHelper.ttinclude"

    void ImportDecl(string keyword)
    {
        foreach(var ii in App.g_parsed.m_imports)
        {

        
        #line default
        #line hidden
        
        #line 60 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\AllHelper.ttinclude"
this.Write(this.ToStringHelper.ToStringWithCulture(keyword));

        
        #line default
        #line hidden
        
        #line 60 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\AllHelper.ttinclude"
this.Write(" ");

        
        #line default
        #line hidden
        
        #line 60 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\AllHelper.ttinclude"
this.Write(this.ToStringHelper.ToStringWithCulture(ii.m_name));

        
        #line default
        #line hidden
        
        #line 60 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\AllHelper.ttinclude"
this.Write(";\r\n");

        
        #line default
        #line hidden
        
        #line 61 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\AllHelper.ttinclude"

        }
    }

        
        #line default
        #line hidden
        
        #line 66 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\AllHelper.ttinclude"
   
    // e.g. package MyPackage;
    void PackageDecl()
    {
        if(App.g_parsed.m_package != null)
        {

        
        #line default
        #line hidden
        
        #line 72 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\AllHelper.ttinclude"
this.Write("package ");

        
        #line default
        #line hidden
        
        #line 73 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\AllHelper.ttinclude"
this.Write(this.ToStringHelper.ToStringWithCulture(App.g_parsed.m_package.m_name));

        
        #line default
        #line hidden
        
        #line 73 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\AllHelper.ttinclude"
this.Write(";\r\n");

        
        #line default
        #line hidden
        
        #line 74 "D:\imays-2\works\engine\dev024\R2\ProudNet\UtilSrc\PIDL_temp\PIDL\OutputTemplate\AllHelper.ttinclude"

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
    public class JavaProxyBase
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
