using Microsoft.VisualBasic;
using System;
using System.Diagnostics;
using System.IO;

namespace PnUtils
{
    class Program
    {
        static void Main(string[] args)
        {
            if (args.Length < 2 || args[0] != "--bin2inl")
            {
                PrintUsage();
                return;
            }

            string sourceFilePath = args[1];
            string targetFilePath = (args.Length == 3) ? args[2] : "";
            if (sourceFilePath == targetFilePath)
            {
                PrintUsage();
                return;
            }

            if (targetFilePath == "")
            {
                string path = Path.GetDirectoryName(sourceFilePath);
                string fullFileName = Path.GetFileNameWithoutExtension(sourceFilePath);
                targetFilePath = path + "\\" + fullFileName + "Image.inl";
            }

            WriteImageInlFile(sourceFilePath, targetFilePath);
        }

        public static void WriteImageInlFile(string sourceFilePath, string targetFilePath)
        {
            try
            {
                // .exe 파일을 읽습니다.
                byte[] bytes;
                bytes = File.ReadAllBytes(sourceFilePath);

                // .ini 파일에 바이너로 형식으로 저장합니다.
                string inlFileNameWithoutExt = Path.GetFileNameWithoutExtension(targetFilePath);
                StreamWriter sw = File.CreateText(targetFilePath);

                sw.WriteLine(@"#pragma once");
                sw.WriteLine(@"");
                sw.WriteLine(@"const static unsigned char g_" + inlFileNameWithoutExt + "[] = {");

                for (int i = 0; i < bytes.Length; i++)
                {
                    sw.Write((int)bytes[i]);

                    if (i != bytes.Length - 1)
                        sw.Write(@",");

                    // 50개를 쓰고 줄바꿈합니다.
                    if (i % 50 == 0)
                        sw.WriteLine(@"");
                }

                sw.WriteLine("\n};");
                sw.Close();
            }
            catch (Exception e)
            {
                Console.WriteLine(e.Message);
            }
        }

        public static void PrintUsage()
        {
            Console.Write(".exe to .inl Converter\n");
            Console.Write("PN Version " + PIDL.PNVersion.g_versionText + "\n");
            Console.Write("(c) Nettetion Inc. All rights reserved.\n");
            Console.Write("PnUtils.exe --bin2inl <source-file-path> <target-file-path>\n");
        }
    }
}
