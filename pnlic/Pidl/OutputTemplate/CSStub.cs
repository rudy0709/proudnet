﻿// ------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//     Runtime Version: 17.0.0.0
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
    
    #line 1 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
    [global::System.CodeDom.Compiler.GeneratedCodeAttribute("Microsoft.VisualStudio.TextTemplating", "17.0.0.0")]
    public partial class CSStub : CSStubBase
    {
#line hidden
        /// <summary>
        /// Create the template output
        /// </summary>
        public virtual string TransformText()
        {
            
            #line 1 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\AllHelper.tt"
 // 모든 언어 공통으로 사용되는 템플릿 파일

            
            #line default
            #line hidden
            this.Write("\r\n");
            this.Write("\r\n\r\n");
            this.Write("\r\n\r\n// Generated by PIDL compiler.\r\n// Do not modify this file, but modify the so" +
                    "urce .pidl file.\r\n\r\nusing System;\r\nusing System.Net;\t     \r\n\r\n");
            
            #line 14 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
  
    WriteUsings();
    foreach(var gi in App.g_parsed.m_globalInterfaces)
    {
        StubClassDef(gi);        
    }

            
            #line default
            #line hidden
            this.Write("\r\n");
            return this.GenerationEnvironment.ToString();
        }
        
        #line 22 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
 
    void StubClassDef(Parsed_GlobalInterface gi)
    {

        
        #line default
        #line hidden
        
        #line 25 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write("namespace ");

        
        #line default
        #line hidden
        
        #line 26 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(gi.m_name));

        
        #line default
        #line hidden
        
        #line 26 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write("\r\n{\r\n\t");

        
        #line default
        #line hidden
        
        #line 28 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(gi.m_accessibility));

        
        #line default
        #line hidden
        
        #line 28 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(@" class Stub:Nettention.Proud.RmiStub
	{
public AfterRmiInvocationDelegate AfterRmiInvocation = delegate(Nettention.Proud.AfterRmiSummary summary) {};
public BeforeRmiInvocationDelegate BeforeRmiInvocation = delegate(Nettention.Proud.BeforeRmiSummary summary) {};

");

        
        #line default
        #line hidden
        
        #line 33 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"

        foreach(var mt in gi.m_methods)
        {

        
        #line default
        #line hidden
        
        #line 36 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write("\t\tpublic delegate bool ");

        
        #line default
        #line hidden
        
        #line 37 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
 DelegateDecl(mt); 
        
        #line default
        #line hidden
        
        #line 37 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write("\t\tpublic ");

        
        #line default
        #line hidden
        
        #line 38 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(mt.m_name));

        
        #line default
        #line hidden
        
        #line 38 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write("Delegate ");

        
        #line default
        #line hidden
        
        #line 38 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(mt.m_name));

        
        #line default
        #line hidden
        
        #line 38 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(" = delegate(Nettention.Proud.HostID remote,Nettention.Proud.RmiContext rmiContext" +
        "");

        
        #line default
        #line hidden
        
        #line 38 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
ParamDefs(mt,true);
        
        #line default
        #line hidden
        
        #line 38 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(")\r\n\t\t{ \r\n\t\t\treturn false;\r\n\t\t};\r\n");

        
        #line default
        #line hidden
        
        #line 42 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
            
        }	
        ProcessReceivedMessage(gi);
		Write("#if USE_RMI_NAME_STRING\n");
        DefRmiNames_all("public const string", gi, false);
		Write("#else\n");
        DefRmiNames_all("public const string", gi, true);
		Write("#endif\n");

        
        #line default
        #line hidden
        
        #line 50 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write("\t\tpublic override Nettention.Proud.RmiID[] GetRmiIDList { get{return Common.RmiID" +
        "List;} }\r\n\t\t\r\n\t}\r\n}\r\n");

        
        #line default
        #line hidden
        
        #line 55 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"

    }

        
        #line default
        #line hidden
        
        #line 59 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
 void DelegateDecl(Parsed_Method mt)  //////////////////////////// method declaration statement을 생성하는 subprogram. 
    {

        
        #line default
        #line hidden
        
        #line 62 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(mt.m_name));

        
        #line default
        #line hidden
        
        #line 62 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write("Delegate(Nettention.Proud.HostID remote,Nettention.Proud.RmiContext rmiContext");

        
        #line default
        #line hidden
        
        #line 62 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
ParamDefs(mt,true);
        
        #line default
        #line hidden
        
        #line 62 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(");  \r\n");

        
        #line default
        #line hidden
        
        #line 63 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"

    }

        
        #line default
        #line hidden
        
        #line 67 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
 
    void ProcessReceivedMessage(Parsed_GlobalInterface gi) 
    {

        
        #line default
        #line hidden
        
        #line 70 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(@"	public override bool ProcessReceivedMessage(Nettention.Proud.ReceivedMessage pa, Object hostTag) 
	{
		Nettention.Proud.HostID remote=pa.RemoteHostID;
		if(remote==Nettention.Proud.HostID.HostID_None)
		{
			ShowUnknownHostIDWarning(remote);
		}

		Nettention.Proud.Message __msg=pa.ReadOnlyMessage;
		int orgReadOffset = __msg.ReadOffset;
        Nettention.Proud.RmiID __rmiID = Nettention.Proud.RmiID.RmiID_None;
        if (!__msg.Read( out __rmiID))
            goto __fail;
					
		switch(__rmiID)
		{
");

        
        #line default
        #line hidden
        
        #line 87 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"

        foreach(var mt in gi.m_methods)
        {
            DoCasePerMethod(mt); 
        }

        
        #line default
        #line hidden
        
        #line 92 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write("\t\tdefault:\r\n\t\t\t goto __fail;\r\n\t\t}\r\n\t\treturn true;\r\n__fail:\r\n\t  {\r\n\t\t\t__msg.ReadOf" +
        "fset = orgReadOffset;\r\n\t\t\treturn false;\r\n\t  }\r\n\t}\r\n");

        
        #line default
        #line hidden
        
        #line 103 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"

    foreach(var mt in gi.m_methods)
    {

        
        #line default
        #line hidden
        
        #line 106 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write("    void ProcessReceivedMessage_");

        
        #line default
        #line hidden
        
        #line 107 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(mt.m_name));

        
        #line default
        #line hidden
        
        #line 107 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(@"(Nettention.Proud.Message __msg, Nettention.Proud.ReceivedMessage pa, Object hostTag, Nettention.Proud.HostID remote)
    {
        Nettention.Proud.RmiContext ctx = new Nettention.Proud.RmiContext();
        ctx.sentFrom=pa.RemoteHostID;
        ctx.relayed=pa.IsRelayed;
        ctx.hostTag=hostTag;
        ctx.encryptMode = pa.EncryptMode;
        ctx.compressMode = pa.CompressMode;

        ");

        
        #line default
        #line hidden
        
        #line 116 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
 DeserializeParams("__msg",gi,mt); 
        
        #line default
        #line hidden
        
        #line 116 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write("        if(enableNotifyCallFromStub==true)\r\n        {\r\n        string parameterSt" +
        "ring = \"\";\r\n        ");

        
        #line default
        #line hidden
        
        #line 120 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
 MakeParameterText("parameterString", mt); 
        
        #line default
        #line hidden
        
        #line 120 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write("        NotifyCallFromStub(Common.");

        
        #line default
        #line hidden
        
        #line 121 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(mt.m_name));

        
        #line default
        #line hidden
        
        #line 121 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(", RmiName_");

        
        #line default
        #line hidden
        
        #line 121 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(mt.m_name));

        
        #line default
        #line hidden
        
        #line 121 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(",parameterString);\r\n        }\r\n\r\n        if(enableStubProfiling)\r\n        {\r\n    " +
        "    Nettention.Proud.BeforeRmiSummary summary = new Nettention.Proud.BeforeRmiSu" +
        "mmary();\r\n        summary.rmiID = Common.");

        
        #line default
        #line hidden
        
        #line 127 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(mt.m_name));

        
        #line default
        #line hidden
        
        #line 127 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(";\r\n        summary.rmiName = RmiName_");

        
        #line default
        #line hidden
        
        #line 128 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(mt.m_name));

        
        #line default
        #line hidden
        
        #line 128 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(";\r\n        summary.hostID = remote;\r\n        summary.hostTag = hostTag;\r\n        " +
        "BeforeRmiInvocation(summary);\r\n        }\r\n\r\n        long t0 = Nettention.Proud.P" +
        "reciseCurrentTime.GetTimeMs();\r\n\r\n        // Call this method.\r\n        bool __r" +
        "et =");

        
        #line default
        #line hidden
        
        #line 137 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(mt.m_name));

        
        #line default
        #line hidden
        
        #line 137 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(" (remote,ctx ");

        
        #line default
        #line hidden
        
        #line 137 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
Paramcalls(mt,true);
        
        #line default
        #line hidden
        
        #line 137 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(" );\r\n\r\n        if(__ret==false)\r\n        {\r\n        // Error: RMI function that a" +
        " user did not create has been called. \r\n        core.ShowNotImplementedRmiWarnin" +
        "g(RmiName_");

        
        #line default
        #line hidden
        
        #line 142 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(mt.m_name));

        
        #line default
        #line hidden
        
        #line 142 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(");\r\n        }\r\n\r\n        if(enableStubProfiling)\r\n        {\r\n        Nettention.P" +
        "roud.AfterRmiSummary summary = new Nettention.Proud.AfterRmiSummary();\r\n        " +
        "summary.rmiID = Common.");

        
        #line default
        #line hidden
        
        #line 148 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(mt.m_name));

        
        #line default
        #line hidden
        
        #line 148 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(";\r\n        summary.rmiName = RmiName_");

        
        #line default
        #line hidden
        
        #line 149 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(mt.m_name));

        
        #line default
        #line hidden
        
        #line 149 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(";\r\n        summary.hostID = remote;\r\n        summary.hostTag = hostTag;\r\n        " +
        "summary.elapsedTime = Nettention.Proud.PreciseCurrentTime.GetTimeMs()-t0;\r\n     " +
        "   AfterRmiInvocation(summary);\r\n        }\r\n    }\r\n");

        
        #line default
        #line hidden
        
        #line 156 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"

    }
}

        
        #line default
        #line hidden
        
        #line 162 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
 
    void DoCasePerMethod(Parsed_Method mt) 
    { 

        
        #line default
        #line hidden
        
        #line 165 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write("        case Common.");

        
        #line default
        #line hidden
        
        #line 166 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(mt.m_name));

        
        #line default
        #line hidden
        
        #line 166 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(":\r\n            ProcessReceivedMessage_");

        
        #line default
        #line hidden
        
        #line 167 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(mt.m_name));

        
        #line default
        #line hidden
        
        #line 167 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write("(__msg, pa, hostTag, remote);\r\n            break;\r\n");

        
        #line default
        #line hidden
        
        #line 169 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"

    }

        
        #line default
        #line hidden
        
        #line 173 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"

    void DeserializeParams(string msg, Parsed_GlobalInterface gi, Parsed_Method mt)
    {
        foreach(Parsed_Param p in mt.m_params)
        {

        
        #line default
        #line hidden
        
        #line 179 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(p.m_type));

        
        #line default
        #line hidden
        
        #line 179 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(" ");

        
        #line default
        #line hidden
        
        #line 179 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(p.m_name));

        
        #line default
        #line hidden
        
        #line 179 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write("; ");

        
        #line default
        #line hidden
        
        #line 179 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(gi.m_marshaler));

        
        #line default
        #line hidden
        
        #line 179 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(".Read(");

        
        #line default
        #line hidden
        
        #line 179 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(msg));

        
        #line default
        #line hidden
        
        #line 179 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(",out ");

        
        #line default
        #line hidden
        
        #line 179 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(p.m_name));

        
        #line default
        #line hidden
        
        #line 179 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(");\t\r\n");

        
        #line default
        #line hidden
        
        #line 180 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
 
        }

        
        #line default
        #line hidden
        
        #line 182 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write("core.PostCheckReadMessage(");

        
        #line default
        #line hidden
        
        #line 183 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(msg));

        
        #line default
        #line hidden
        
        #line 183 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(", RmiName_");

        
        #line default
        #line hidden
        
        #line 183 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(mt.m_name));

        
        #line default
        #line hidden
        
        #line 183 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(");\r\n");

        
        #line default
        #line hidden
        
        #line 184 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"

    }

        
        #line default
        #line hidden
        
        #line 188 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"

    void MakeParameterText(string parameterString, Parsed_Method mt)
    {
        foreach(var p in mt.m_params)
        {

        
        #line default
        #line hidden
        
        #line 193 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write("parameterString+=");

        
        #line default
        #line hidden
        
        #line 194 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(p.m_name));

        
        #line default
        #line hidden
        
        #line 194 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"
this.Write(".ToString()+\",\";\r\n");

        
        #line default
        #line hidden
        
        #line 195 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSStub.tt"

        }
    }

        
        #line default
        #line hidden
        
        #line 3 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSHelper.tt"
 void WriteUsings()
    {
        foreach(var ii in App.g_parsed.m_usings)
        {

        
        #line default
        #line hidden
        
        #line 7 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSHelper.tt"
this.Write("            \r\nusing ");

        
        #line default
        #line hidden
        
        #line 8 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSHelper.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(ii.m_name));

        
        #line default
        #line hidden
        
        #line 8 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSHelper.tt"
this.Write("; \r\n");

        
        #line default
        #line hidden
        
        #line 9 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\CSHelper.tt"
    
        }
    }

        
        #line default
        #line hidden
        
        #line 4 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\AllHelper.tt"

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
        
        #line 18 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\AllHelper.tt"

	// Writes like this, for example: const string RmiName_xxx = "xxx";
	// if 'hide' is true, "xxx" will be "".
    void DefRmiNames_all(string stringType, Parsed_GlobalInterface gi, bool hide)
    {

        
        #line default
        #line hidden
        
        #line 23 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\AllHelper.tt"
this.Write("// RMI name declaration.\r\n// It is the unique pointer that indicates RMI name suc" +
        "h as RMI profiler.\r\n");

        
        #line default
        #line hidden
        
        #line 26 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\AllHelper.tt"

        foreach(var mt in gi.m_methods)
        {		

        
        #line default
        #line hidden
        
        #line 30 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\AllHelper.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(stringType));

        
        #line default
        #line hidden
        
        #line 30 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\AllHelper.tt"
this.Write(" RmiName_");

        
        #line default
        #line hidden
        
        #line 30 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\AllHelper.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(mt.m_name));

        
        #line default
        #line hidden
        
        #line 30 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\AllHelper.tt"
this.Write("=\"");

        
        #line default
        #line hidden
        
        #line 30 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\AllHelper.tt"
 if(hide) Write(""); else Write(mt.m_name); 
        
        #line default
        #line hidden
        
        #line 30 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\AllHelper.tt"
this.Write("\";\r\n");

        
        #line default
        #line hidden
        
        #line 31 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\AllHelper.tt"

        }

        
        #line default
        #line hidden
        
        #line 33 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\AllHelper.tt"
this.Write("       \r\n");

        
        #line default
        #line hidden
        
        #line 34 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\AllHelper.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(stringType));

        
        #line default
        #line hidden
        
        #line 34 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\AllHelper.tt"
this.Write(" RmiName_First = ");

        
        #line default
        #line hidden
        
        #line 34 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\AllHelper.tt"
 if(hide) Write("\"\""); else Write(gi.GetFirstRmiNameOrNone("RmiName_", "", "" )); 
        
        #line default
        #line hidden
        
        #line 34 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\AllHelper.tt"
this.Write(";\r\n");

        
        #line default
        #line hidden
        
        #line 35 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\AllHelper.tt"

    }

        
        #line default
        #line hidden
        
        #line 41 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\AllHelper.tt"

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
        
        #line 54 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\AllHelper.tt"

    void ImportDecl(string keyword)
    {
        foreach(var ii in App.g_parsed.m_imports)
        {

        
        #line default
        #line hidden
        
        #line 60 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\AllHelper.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(keyword));

        
        #line default
        #line hidden
        
        #line 60 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\AllHelper.tt"
this.Write(" ");

        
        #line default
        #line hidden
        
        #line 60 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\AllHelper.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(ii.m_name));

        
        #line default
        #line hidden
        
        #line 60 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\AllHelper.tt"
this.Write(";\r\n");

        
        #line default
        #line hidden
        
        #line 61 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\AllHelper.tt"

        }
    }

        
        #line default
        #line hidden
        
        #line 66 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\AllHelper.tt"
   
    // e.g. package MyPackage;
    void PackageDecl()
    {
        if(App.g_parsed.m_package != null)
        {

        
        #line default
        #line hidden
        
        #line 72 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\AllHelper.tt"
this.Write("package ");

        
        #line default
        #line hidden
        
        #line 73 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\AllHelper.tt"
this.Write(this.ToStringHelper.ToStringWithCulture(App.g_parsed.m_package.m_name));

        
        #line default
        #line hidden
        
        #line 73 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\AllHelper.tt"
this.Write(";\r\n");

        
        #line default
        #line hidden
        
        #line 74 "D:\Projects\github\proudnet\pnlic\Pidl\OutputTemplate\AllHelper.tt"

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
    [global::System.CodeDom.Compiler.GeneratedCodeAttribute("Microsoft.VisualStudio.TextTemplating", "17.0.0.0")]
    public class CSStubBase
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
