//////////////////////////////////////////////////////////////////////////
// 2009.10.27 create by ulelio : It works same as Simple sample in Sample folder.
// You should include compiled common.cs, stub.cs, proxy.cs from TestPIDL in this project.
// 2009.10.27 create by ulelio : Sample 폴더의 Simple 예제와 똑같이 동작함.
// TestPIDL에서 컴파일된 common.cs, stub.cs, proxy.cs를 이 프로젝트에 포함시켜야합니다.

using System;
using System.Collections.Generic;
//using System.Linq;
using System.Text;
using Nettention.Proud;

namespace SimpleCSharp
{
    public class Vars
    {
        public static System.Guid m_Version;
        public static int m_serverPort;

        public Vars()
        {
            // {3AE33249-ECC6-4980-BC5D-7B0A999C0739}
            m_Version = new System.Guid("{ 0x3ae33249, 0xecc6, 0x4980, { 0xbc, 0x5d, 0x7b, 0xa, 0x99, 0x9c, 0x7, 0x39 } }");
            m_serverPort = 33334;
        }
    }

    // User defined class
    // 사용자 정의 클래스
    public class MyClass
    {
        public int a;
        public float b;
        public double c;
    }

    /** Inherit Marshaler of ProudClr then re-define brandnew defined MyClass type Read, Write.
    - Define it at PIDL file.
    - e.g. C2C.PIDL

    /** ProudClr의 Marshaler를 상속받아 새로정의한 MyClass형 Read, Write를 재정의 합니다.
    -이것을 PIDL파일에서 정의해주면 됩니다.
    -예 C2C.PIDL

        [marshaler=SimpleCSharp.CMyMarshaler] global C2C 3000  // client-to-client RMI, First message ID = 3000
        {
            P2PChat([in] System.String a,[in] int b, [in] float c);
    
            P2PChat2([in] SimpleCSharp.MyClass a);
        }     
     */
    public class CMyMarshaler : Nettention.Proud.Marshaler
    {
        public static bool Read(Nettention.Proud.Message msg, out MyClass value)
        {
            value = new MyClass();
            if ( msg.Read( out value.a) && msg.Read(out value.b) && msg.Read(out value.c))
                return true;

            return false;
        }

        public static void Write(Nettention.Proud.Message msg, MyClass value)
        {
            msg.Write(value.a);
            msg.Write(value.b);
            msg.Write(value.c);
        }

        public static void Read(Nettention.Proud.Message msg, out List<int> value)
        {
            value = new List<int>();

            int count = 0;
            msg.ReadScalar(ref count);

            for (int i = 0; i < count; ++i)
            {
                int elem = 0;
                msg.Read(out elem);
                value.Add(elem);
            }
        }

        public static void Write(Nettention.Proud.Message msg, List<int> value)
        {
            int size = value.Count;

            msg.WriteScalar(size);

            foreach (int temp in value)
            {
                msg.Write(temp);
            }
        }

        public static void Read(Nettention.Proud.Message msg, out Dictionary<int, float> value)
        {
            value = new Dictionary<int, float>();

            int count = 0;
            msg.ReadScalar(ref count);

            for (int i = 0; i < count; ++i)
            {
                int key = 0;
                float elem = 0;
                msg.Read(out key);
                msg.Read(out elem);
                value.Add(key, elem);
            }
        }

        public static void Write(Nettention.Proud.Message msg, Dictionary<int, float> value)
        {
            int size = value.Count;

            msg.WriteScalar(size);

            foreach (KeyValuePair<int, float> pair in value)
            {
                msg.Write(pair.Key);
                msg.Write(pair.Value);
            }
        }
    }
}
