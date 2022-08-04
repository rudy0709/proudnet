using System;
using System.IO;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Antlr4.Runtime;
using System.Reflection;

[assembly: Obfuscation(Feature = "embed Antlr4.Runtime.dll", Exclude = false)]

namespace PIDL
{
    partial class Program
    {
        enum Ahead
        {
            None,
            OutDir,
            ImplExt,
            IncDir,
        }

        static int Main(string[] args)
        {
            if (args.Length == 0)
            {
                Console.Write("ProudNet Remote Method Invocation Interface Definition Language Compiler\n");
                Console.Write("Version " + PNVersion.g_versionText + "\n");
                Console.Write("(c) Nettetion Inc. All rights reserved.\n");
                Console.Write("Check out http://help.nettention.com => PIDL compiler for details.\n");
                return 1;
            }

            // 불법복제 방지 루틴을 구현하게 되면 여기다 구현하자.

            // 실행 아규먼트를 분석한다.
            try
            {
                Ahead ahead = Ahead.None;
                List<string> inputFileFullPathList = new List<string>();

                string errorTypeFileFullPath = null; // ErrorType.yaml의 전체 파일 이름

                string outDir = null;
                string incDir = null;
                App.g_lang = Lang.CPP;

                for (int c = 0; c < args.Length; c++)
                {
                    string v = args[c];

                    switch (ahead)
                    {
                        default:
                            if (v.ToLower() == "-impl-ext")
                            {
                                ahead = Ahead.ImplExt;
                            }
                            if (v.ToLower() == "-outdir")
                            {
                                // 출력 디렉터리를 판독
                                ahead = Ahead.OutDir;
                            }
                            else if (v.ToLower() == "-incdir")
                            {
                                ahead = Ahead.IncDir;
                            }
                            else if (v.Length > 0 && v[0] == '-')
                            {
                                // 어떤 언어로 코드 생성을 할 것인지를 판독
                                Lang lang;
                                lang = LangUtil.GetLangEnum(v.Substring(1, v.Length - 1));

                                if (lang != Lang.Undefined)
                                {
                                    App.g_lang = lang;
                                }
                            }
                            else
                            {
                                string[] fileFullPathList = Util.GetFiles(v);
                                foreach (var fileFullPath in fileFullPathList)
                                {
                                    if (Path.GetExtension(fileFullPath).Equals(".pidl", StringComparison.OrdinalIgnoreCase))
                                    {
                                        inputFileFullPathList.Add(fileFullPath);
                                    }

                                    // ErrorType.yaml을 PIDL을 다루는건 사용자에게 비노출 기능이다. 따로 출력하지 말자.
                                    if (Path.GetFileName(fileFullPath).Equals("errortype.yaml", StringComparison.OrdinalIgnoreCase))
                                    {
                                        errorTypeFileFullPath = fileFullPath;
                                    }
                                }
                            }
                            break;
                        case Ahead.OutDir:
                            outDir = Util.GetDirectoryName(v);
                            ahead = Ahead.None;
                            break;
                        case Ahead.ImplExt:
                            App.m_implExt = v;
                            ahead = Ahead.None;
                            break;
                        case Ahead.IncDir:
                            incDir = Util.GetDirectoryName(v);
                            ahead = Ahead.None;
                            break;
                    }
                }

                // ErrorType.yaml을 처리하는건 사용자에게는 공개되지 않는 기능이다.
                // 뭐 하나 할 때마다 툴을 또 만드는건 귀찮은(중복코드 및 유지보수 등의 일을 생각하면 그냥 귀찮다...는 쓸모없는 표현일까?) 일이다.
                // 그래서 PIDL 툴에서 한다.
                if(errorTypeFileFullPath != null)
                {
                    if (ProcessErrorTypeFile(errorTypeFileFullPath, incDir, outDir) != 0)
                        return 1;
                    return 0;
                }

                if (inputFileFullPathList.Count <= 0)
                {
                    Console.Write("Cannot find PIDL file(s). Check out the command line!\n");
                    return 1;
                }

                foreach (var fileName in inputFileFullPathList)
                {
                    if (ProcessPidl(fileName, outDir) != 0)
                        return 1;
                }
            }
            catch (Exception e)
            {
                Console.WriteLine(e.ToString());
                return 1;
            }
            return 0;
        }

        // 주요 역할은 여기서 한다.
        static int ProcessPidl(string inputFileFullPath, string outDir)
        {
            string fileName = null;
            PidlSpecLexer lexer = null;
            StreamReader ifileStream = null;
            try
            {
                // 사용자가 outdir 미지정이면=>입력파일의 폴더가 출력 폴더가 됨
                string outDirP;

                if (outDir != null)
                {
                    // 출력 파일이 저장될 디렉터리를 만든다. 없으면.
                    if (!Directory.Exists(outDir))
                        Directory.CreateDirectory(outDir);
                    outDirP = outDir;
                }
                else
                {
                    outDirP = Path.GetDirectoryName(inputFileFullPath);
                }

                // 출력 파일 이름들을 구한다.
                string tempExtName;
                string cppExt = ".cpp";
                if (App.m_implExt != null)
                    cppExt = App.m_implExt;

                Util.ModifyFileName(inputFileFullPath, ("_common"), (".h"), outDirP, out App.m_CommonHFilePathName, out App.m_CommonHFileName, out tempExtName);
                Util.ModifyFileName(inputFileFullPath, ("_common"), cppExt, outDirP, out App.m_CommonCPPFilePathName, out App.m_CommonCPPFileName, out tempExtName);
                Util.ModifyFileName(inputFileFullPath, ("_common"), (".cs"), outDirP, out App.m_CommonCSFilePathName, out App.m_CommonCSFileName, out tempExtName);

                Util.ModifyFileName(inputFileFullPath, ("_common"), (".java"), outDirP, out App.m_CommonJavaFilePathName, out App.m_CommonJavaFileName, out App.m_CommonJavaFileNameWithoutExtension);
                Util.ModifyFileName(inputFileFullPath, ("_proxy"), cppExt, outDirP, out App.m_ProxyCPPFilePathName, out App.m_ProxyCPPFileName, out App.m_ProxyCPPFileNameWithoutExtension);
                Util.ModifyFileName(inputFileFullPath, ("_proxy"), (".h"), outDirP, out App.m_ProxyHFilePathName, out App.m_ProxyHFileName, out App.m_ProxyHFileNameWithoutExtension);
                Util.ModifyFileName(inputFileFullPath, ("_proxy"), (".cs"), outDirP, out App.m_ProxyCSFilePathName, out App.m_ProxyCSFileName, out tempExtName);

                Util.ModifyFileName(inputFileFullPath, ("_proxy"), (".java"), outDirP, out App.m_ProxyJavaFilePathName, out App.m_ProxyJavaFileName, out App.m_ProxyJavaFileNameWithoutExtension);
                Util.ModifyFileName(inputFileFullPath, ("_proxy"), (".uc"), outDirP, out App.m_ProxyUCFilePathName, out App.m_ProxyUCFileName, out App.m_ProxyUCFileNameWithoutExtension);
                Util.ModifyFileName(inputFileFullPath, ("_stub"), cppExt, outDirP, out App.m_StubCPPFilePathName, out App.m_StubCPPFileName, out App.m_StubCPPFileNameWithoutExtension);
                Util.ModifyFileName(inputFileFullPath, ("_stub"), (".h"), outDirP, out App.m_StubHFilePathName, out App.m_StubHFileName, out App.m_StubHFileNameWithoutExtension);
                Util.ModifyFileName(inputFileFullPath, ("_stub"), (".cs"), outDirP, out App.m_StubCSFilePathName, out App.m_StubCSFileName, out tempExtName);

                Util.ModifyFileName(inputFileFullPath, ("_stub"), (".java"), outDirP, out App.m_StubJavaFilePathName, out App.m_StubJavaFileName, out App.m_StubJavaFileNameWithoutExtension);
                Util.ModifyFileName(inputFileFullPath, ("_stub"), (".java"), outDirP, out App.m_StubNDKJavaFilePathName, out App.m_StubNDKJavaFileName, out App.m_StubNDKJavaFileNameWithoutExtension);
                Util.ModifyFileName(inputFileFullPath, ("_stub"), (".uc"), outDirP, out App.m_StubUCFilePathName, out App.m_StubUCFileName, out App.m_StubUCFileNameWithoutExtension);
                Util.ModifyFileName(inputFileFullPath, ("_stubER"), (".h"), outDirP, out App.m_StubERHFilePathName, out App.m_StubERHFileName, out tempExtName);

                // 컴파일을 하기 전에 생성될 파일들을 먼저 지운다. 이미 있으면.
                App.DeleteOutputFiles();

                // 입력 파일을 로딩한다. 인코딩 상관없이.
                if (!File.Exists(inputFileFullPath))
                    throw new Exception("File " + inputFileFullPath + "not found!");

                // 파싱 시작!
                ifileStream = new StreamReader(inputFileFullPath);
                fileName = Path.GetFileName(inputFileFullPath);
                var input = new AntlrInputStream(ifileStream.ReadToEnd());
                lexer = new PidlSpecLexer(input);
                //lexer.GrammarFileName = fileName;
                var tokens = new CommonTokenStream(lexer);
                var parser = new PidlSpecParser(tokens);
                // parse error를 throw exception하게 한다. 이게 없으면 listener에서 처리해야 함.
                parser.ErrorHandler = new BailErrorStrategy();
                //parser.GrammarFileName = fileName;
                //parser.strateg(new ParseErrorListener());
                var cp = parser.compilationUnit();

                App.g_parsed = cp.ret;
                App.g_parsed.RefineParsed();

                /* TODO: 나중에 이 부분이 채워지도록 만들자.
                        ENCODE_START
                        {
                            // 라이센스 정보를 넣는다.
                            cp->m_licenseInfo = RefCount<CLicenseInfo>(new CLicenseInfo);

                            if(g_name[0]!= 0 || g_company[0] !=0)
                            {
                                cp->m_licenseInfo->m_name = CW2A(g_name, CP_UTF8);
                                int nameLength = cp->m_licenseInfo->m_name.length();

                                vector<uint8_t> nameBuffer;
                                nameBuffer.resize(nameLength);
                                memcpy(&nameBuffer[0], cp->m_licenseInfo->m_name.c_str(),nameLength);

                                EncryptBinaryWaterMark(&nameBuffer[0], nameLength);


                                cp->m_licenseInfo->m_company = CW2A(g_company, CP_UTF8);
                                int companyLength = cp->m_licenseInfo->m_company.length();

                                vector<uint8_t> companyBuffer;
                                companyBuffer.resize(companyLength);
                                memcpy(&companyBuffer[0], cp->m_licenseInfo->m_company.c_str(),companyLength);

                                EncryptBinaryWaterMark(&companyBuffer[0], companyLength);


                                cp->m_licenseInfo->m_customdata = CW2A(g_customdata, CP_UTF8);
                                int customdataLength = cp->m_licenseInfo->m_customdata.length();

                                vector<uint8_t> customdataBuffer;
                                customdataBuffer.resize(customdataLength);
                                memcpy(&customdataBuffer[0], cp->m_licenseInfo->m_customdata.c_str(),customdataLength);

                                EncryptBinaryWaterMark(&customdataBuffer[0], customdataLength);

                                int i;
                                // 배열에 값 모두를 따로 입력한다.
                                for(i=0; i < nameLength; ++i)
                                {
                                    char buffer[100] = {0,};
                                    sprintf(buffer, "%02x", nameBuffer[i]);
                                    string str(buffer);
                                    cp->m_licenseInfo->m_nameFragments.push_back(str);
                                }

                                for(i=0; i < companyLength; ++i)
                                {
                                    char buffer[100] = {0,};
                                    sprintf(buffer, "%02x", companyBuffer[i]);
                                    string str(buffer);
                                    cp->m_licenseInfo->m_companyFragments.push_back(str);
                                }

                                for(i=0; i < customdataLength; ++i)
                                {
                                    char buffer[100] = {0, };
                                    sprintf(buffer, "%02x", customdataBuffer[i]);
                                    string str(buffer);
                                    cp->m_licenseInfo->m_customdataFragments.push_back(str);
                                }
                            }
                        }
                        ENCODE_END
                         */

                App.CreateOutputFiles();

            }
            catch (Antlr4.Runtime.Misc.ParseCanceledException e)
            {
                RecognitionException e2 = (RecognitionException)e.InnerException;
                string token = e2.OffendingToken.Text;
                int line = e2.OffendingToken.Line;
                int column = e2.OffendingToken.Column;
                Console.WriteLine($"{inputFileFullPath}({line},{column + 1}): error: Syntax near {token}.");
            }
            catch (RecognitionException e)
            {
                //Console.WriteLine(e.Message);
                Console.WriteLine(inputFileFullPath + ": " + e.Message);
                App.DeleteOutputFiles();
                return 1;
            }
            catch (PidlException e)
            {
                int line = e.line;
                int column = e.column;
                Console.WriteLine($"{inputFileFullPath}({line},{column + 1}): {e.comment}");
            }
            catch (Exception e)
            {
                Console.WriteLine(e.Message);
                App.DeleteOutputFiles();
                return 1;
            }
            finally
            {
                if (ifileStream!=null)
                    ifileStream.Close();
            }

            return 0;
        }

    }

}
