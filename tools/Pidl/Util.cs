using System;
using System.IO;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace PIDL
{
    class Util
    {
        // searchString이 가리키는 파일 검색어로 파일들을 찾아 리턴한다.
        // 절대,상대 경로 모두 사용 가능. wildcard는 물론.
        // 유닉스도 지원함.
        // 출력은 절대경로임.
        public static string[] GetFiles(string searchString)
        {
            var path = Path.Combine(Directory.GetCurrentDirectory(), searchString);
            var directory = Path.GetDirectoryName(path);
            var fileName = Path.GetFileName(path);
            var ret = Directory.GetFiles(directory, fileName);

            return ret;
        }

        // ikpil.choi 2017-01-10 : PIDL에서 커맨드 라인의 -output 의 인자 인식을 범용적으로 쓸수 있게 개선(N3726)
        public static string GetDirectoryName(string outputPath)
        {
            string directory = outputPath;

            // outputPath 가 ../a/b 로 입력 되면 GetDirectoryName 의 리턴이 ../a 이므로, 수동으로 변경할 필요가 있음
            // '/' 이나 '\\' 이 들어 왔을 때, 모두 현 플랫폼에 맞는 경로 구문자 문자로 치환시킨다.
            directory = directory.Replace('/', Path.DirectorySeparatorChar);
            directory = directory.Replace('\\', Path.DirectorySeparatorChar);

            return directory;
        }

        // 파일 이름의 확장자를 바꾼다
        // fn; 바꿀 파일의 이름. 이것은 src,dest의 역할을 한다
        //newExtName; 확장자는 요걸로 바뀐다.
        static public void ModifyFileName(string fileName, string postfix, string newExtName, string outDir, out string outPathName, out string outFileName, out string outFileNameWithoutExtension)
        {
            // fn의 순수 파일 이름에 add를 붙이고, 확장자를 newExtName이 가리키는 것으로 바꾼다.
            var pure_fn = Path.GetFileNameWithoutExtension(fileName);
            var newFullPath = Path.ChangeExtension(Path.Combine(outDir, pure_fn + postfix), newExtName);

            outPathName = newFullPath;
            outFileName = Path.GetFileName(newFullPath);
            outFileNameWithoutExtension = Path.GetFileNameWithoutExtension(newFullPath);
        }

    }
}
