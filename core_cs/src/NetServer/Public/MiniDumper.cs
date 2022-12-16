/*
ProudNet HERE_SHALL_BE_EDITED_BY_BUILD_HELPER


이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의 : 저작물에 관한 위의 명시를 제거하지 마십시오.


This program is soley copyrighted by Nettention.
Any use, correction, and distribution of this program are subject to the terms and conditions of the License Agreement.
Any violated use of this program is prohibited and will be cause of immediate termination according to the License Agreement.

** WARNING : PLEASE DO NOT REMOVE THE LEGAL NOTICE ABOVE.


此程序的版权归Nettention公司所有。
与此程序的修改、使用、发布相关的事项要遵守此程序的所有权者的协议。
不遵守协议时要原则性的禁止擅自使用。
擅自使用的责任明示在与此程序所有权者的合同书里。

** 注意：不要移除关于制作物的上述明示。


このプログラムの著作権はNettentionにあります。
このプログラムの修正、使用、配布に関する事項は本プログラムの所有権者との契約に従い、
契約を遵守しない場合、原則的に無断使用を禁じます。
無断使用による責任は本プログラムの所有権者との契約書に明示されています。

** 注意：著作物に関する上記の明示を除去しないでください。

*/

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
//using System.Threading.Tasks;
using System.Runtime.InteropServices;
using System.Net;

namespace Nettention.Proud
{
    public enum MinidumpType
    {
        MiniDumpNormal                         = 0x00000000,
        MiniDumpWithDataSegs                   = 0x00000001,
        MiniDumpWithFullMemory                 = 0x00000002,
        MiniDumpWithHandleData                 = 0x00000004,
        MiniDumpFilterMemory                   = 0x00000008,
        MiniDumpScanMemory                     = 0x00000010,
        MiniDumpWithUnloadedModules            = 0x00000020,
        MiniDumpWithIndirectlyReferencedMemory = 0x00000040,
        MiniDumpFilterModulePaths              = 0x00000080,
        MiniDumpWithProcessThreadData          = 0x00000100,
        MiniDumpWithPrivateReadWriteMemory     = 0x00000200,
        MiniDumpWithoutOptionalData            = 0x00000400,
        MiniDumpWithFullMemoryInfo             = 0x00000800,
        MiniDumpWithThreadInfo                 = 0x00001000,
        MiniDumpWithCodeSegs                   = 0x00002000,
        MiniDumpWithoutAuxiliaryState          = 0x00004000,
        MiniDumpWithFullAuxiliaryState         = 0x00008000,
        MiniDumpWithPrivateWriteCopyMemory     = 0x00010000,
        MiniDumpIgnoreInaccessibleMemory       = 0x00020000,
        MiniDumpWithTokenInformation           = 0x00040000,
        MiniDumpWithModuleHeaders              = 0x00080000,
        MiniDumpFilterTriage                   = 0x00100000,
        MiniDumpValidTypeFlags                 = 0x001fffff,
    }

    public class MiniDumper
    {
        public static Nettention.Proud.MiniDumpAction StartUp(String dumpFileName, MinidumpType miniDumpType = MinidumpType.MiniDumpNormal)
        {
            Nettention.Proud.MiniDumpAction action = Nettention.Proud.MiniDumpAction.MiniDumpAction_None;

            try
            {
                action = ProudNetServerPlugin.StartUpDump(dumpFileName, (int)miniDumpType);
            }
            catch (System.TypeInitializationException ex)
            {
                // c++ ProudNetServerPlugin.dll, ProudNetClientPlugin.dll 파일이 작업 경로에 없을 때
                throw new System.Exception(ServerNativeExceptionString.TypeInitializationExceptionString);
            }

            return action;
        }

        /***
		\~korean
		유저 호출에 의해 미니 덤프 파일을 생성한다. 단, 메모리 전체 덤프를 하므로 용량이 큰 파일이 생성된다.

		\~english
		Create mini dump file by user calling, however, the capacity of file becomes larger because the whole memory is dumped.

		\~chinese
		被用户呼叫生成微型转储文件。但是，因为进行内存全部转储，会生成容量大的文件。

		\~japanese
		ユーザの呼び出しによりミニダンプファイルを生成します。ただし、メモリー全体をダンプするので容量の大きいファイルが生成されます。
		*/
        public static void ManualFullDump()
        {
            ProudNetServerPlugin.ManualFullDump();
        }

        /**
		\~korean
		유저 호출에 의해 미니 덤프 파일을 생성한다.

		\~english
		Create mini dump file by user calling.

		\~chinese
		被用户呼叫生成微型转储文件。

		\~japanese
		ユーザの呼び出しによりミニダンプファイルを生成します。
		*/
        public static void ManualMiniDump()
        {
            ProudNetServerPlugin.ManualMiniDump();
        }

        /**
		\~korean
		이 함수를 호출한 시점에서의 프로세스 상태를 덤프 파일로 남긴다.
		\param fileName 덤프 파일 이름. 확장자는 dmp로 지정할 것.

		\~english
		Process status of calling this function as dump file
		\param fileName Name of dump file. Its extension must be .dmp.

		\~chinese
		从呼叫此函数的时点的程序状态留为转储文件。
		\param filename 转储文件名。扩展者指定为dmp。

		\~japanese
		この関数を呼び出した時点でのプロセス状態をダンプファイルで残します。
		\param fileNameダンプファイルの名前。拡張子はdmpで指定すること。
		*/
        public static void WriteDumpFromHere(String fileName, bool fullDump = false)
        {
            ProudNetServerPlugin.WriteDumpFromHere(fileName, fullDump);
        }
    }
}
