using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using YamlDotNet.Serialization;
using YamlDotNet.Serialization.NamingConventions;

namespace PIDL
{
    [System.Reflection.ObfuscationAttribute(Feature ="renaming", ApplyToMembers = true)]
    public class ErrorTypeList
    {
        public List<ErrorTypeItem> items { get; set; }
    }

    [System.Reflection.ObfuscationAttribute(Feature = "renaming", ApplyToMembers = true)]
    public class ErrorTypeItem
    {
        public int ID { get; set; }
        public string Name { get; set; }
        public string Kor { get; set; }
        public string Eng { get; set; }
        public string Chn { get; set; }
        public string Jpn { get; set; }
    }

    partial class Program
    {
        // ErrorType.yaml을 처리한다.
        // 내부용이고, ErrorType.yaml 소스파일의 상대경로에서 일을 해야 하므로, 추가 파라메터없이 가자.
        private static int ProcessErrorTypeFile(string errorTypeFileFullPath)
        {
            string cppSourceDir = Path.GetDirectoryName(errorTypeFileFullPath);

            // yaml 읽기
            var fileContents = File.ReadAllText(errorTypeFileFullPath);
            var input = new StringReader(fileContents);
            var deserializer = new Deserializer();
            var errorTypeList = deserializer.Deserialize<ErrorTypeList>(input);

            App.g_errorTypeList = errorTypeList;

            // 읽은 것을 토대로 파일 출력을 한다.
            var cppHText = new ErrorTypeH().TransformText();

            Console.WriteLine("Generating ErrorType.h...");
            File.WriteAllText(Path.Combine(cppSourceDir, @"..\include\ErrorType.h".Replace('\\', Path.DirectorySeparatorChar)), cppHText, Encoding.UTF8);

            Console.WriteLine("Generating ErrorTypeOldSpec.h...");
            File.WriteAllText(Path.Combine(cppSourceDir, @"..\include\ErrorTypeOldSpec.h".Replace('\\', Path.DirectorySeparatorChar)), new ErrorTypeHOldSpec().TransformText(), Encoding.UTF8);

            Console.WriteLine("Generating ErrorType.inl...");
            File.WriteAllText(Path.Combine(cppSourceDir, @"ErrorType.inl"), new ErrorTypeInlH().TransformText(), Encoding.UTF8);

            return 0;
        }
    }


}
