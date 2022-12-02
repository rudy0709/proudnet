/*
ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.

ProudNet

This program is soley copyrighted by Nettention. 
Any use, correction, and distribution of this program are subject to the terms and conditions of the License Agreement.
Any violated use of this program is prohibited and will be cause of immediate termination according to the License Agreement.

** WARNING : PLEASE DO NOT REMOVE THE LEGAL NOTICE ABOVE. 

ProudNet

此程序的版权归Nettention公司所有。
与此程序的修改、使用、发布相关的事项要遵守此程序的所有权者的协议。
不遵守协议时要原则性的禁止擅自使用。
擅自使用的责任明示在与此程序所有权者的合同书里。

** 注意：不要移除关于制作物的上述明示。

*/
using System;
using System.Collections.Generic;
using System.Text;
using System.Net;
using System.Diagnostics;

namespace Nettention.Proud
{
    /** \addtogroup net_group
    *  @{
    */
    enum TestSplitter
    {
        Splitter = 254
    }

    /** 
    \~korean
    메시지 클래스. 
    - 버퍼를 두고 Write,Read 를 통해 버퍼에 읽고 쓰기를 합니다.
    
	\~english TODO:translate needed.
    Message class.
	- 
    
	\~chinese
	信息类
	- 放置Buffer，通过Write,Read在Buffer中进行读写。

	\~japanese
    
    \~
    */
    public class Message
    {
        //private static readonly int TEXTNCODE_CHAR = 1;
        //private static readonly int TEXTENCODE_WCHAR = 2;
        private static readonly int STRING_LENGTH_LIMIT = 1024*1024;


        //private int readBitOffset = 0;
        private int readByteOffset = 0;
        //private int bitLengthInOneByte = 0;

        private ByteArray msgBuffer = null;

        private bool enableTestSplitter = false;

        private bool isSimplePacketMode = false;

        public Message()
        {
            msgBuffer = new ByteArray();
            msgBuffer.SetMinCapacity(512);
        }

        /*public Message(ref Message src)
        {
            this.readBitOffset = src.readBitOffset;
            this.enableTestSplitter = src.enableTestSplitter;
            this.bitLengthInOneByte = src.bitLengthInOneByte;

            this.msgBuffer = src.msgBuffer;
        }*/

        public Message(ByteArray buffer)
        {
            msgBuffer = buffer;
        }


        private void ThrowArrayIsNullError()
        {
            throw new System.Exception("error: msgBuffer ArrayPtr is null!");
        }

        /*private void AdjustWriteOffsetByteAlign()
        {
            bitLengthInOneByte = 0;
        }*/

        public bool EnableTestSplitter
        {
            get { return enableTestSplitter; }
            set
            {
                enableTestSplitter = value;
            }
        }

        enum MessageSplitter { Splitter = 254 };
        private void WriteTestSplitterOnCase()
        {
            if (enableTestSplitter)
            {
                Byte s = (Byte)MessageSplitter.Splitter;
                Write_NoTestSplitter(BitConverter.GetBytes(s));
            }
        }

        private void Write_NoTestSplitter(Byte[] data)
        {
            if (null == msgBuffer)
            {
                ThrowArrayIsNullError();
            }

            msgBuffer.AddRange(data);
        }
        private void Write_NoTestSplitter(Byte[] data, int dataLength)
        {
            if (null == msgBuffer)
            {
                ThrowArrayIsNullError();
            }

            msgBuffer.AddRange(data, dataLength);
        }

        /** 
        \~korean
        외부 버퍼를 세팅합니다.
        
        \~english TODO:translate needed.

        \~chinese
        设置外部Buffer。

        \~japanese
        
        \~
        */
        public void UseExternalBuffer(ref Byte[] buf, int capacity)
        {
               // CNetConfig::MessageMaxLength 퇴역으로 메시지 작성 시에는 크기를 검사하지 않고 메시지를 전송 할 때 검사한다.
//             if (capacity > NetConfig.MessageMaxLength)
//                 throw new Exception("UseExternalBuffer failed due to too large capacity");

            msgBuffer.UseExternalBuffer(ref buf, capacity);
        }

        /** 
        \~korean
        내부 버퍼를 얻습니다.
        
        \~english TODO:translate needed.

        \~chinese
        获得内部Buffer。

        \~japanese
        
        \~
        */
        public ByteArray Data
        {
            get { return msgBuffer; }
        }

        /** 
        \~korean
        현재 메시지의 길이를 얻거나, 세팅합니다.
        
        \~english TODO:translate needed.

        \~chinese
        设置或获取现信息长度。

        \~japanese
        
        \~
        */
        public int Length
        {
            get { return msgBuffer.Count; }
            set
            {
                if (readByteOffset > value)
                    readByteOffset = value;

                msgBuffer.Count = value;
            }
        }

        /** 
        \~korean
        현재까지 읽은 길이를 얻거나, 세팅합니다.
        
        \~english TODO:translate needed.

        \~chinese
        设置或获取已读信息长度。

        \~japanese
        
        \~
        */
        public int ReadOffset
        {
            get { return readByteOffset; }
            set
            {
                if (value > msgBuffer.Count)
                {
                    throw new Exception("Too Big offset!");
                }

                readByteOffset = value;
            }
        }

        public bool SimplePacketMode
        {

            get { return isSimplePacketMode; }
            set { isSimplePacketMode = value; }
        }

        /** 
        \~korean
        count 만큼 읽을수 있는지 여부를 알려줍니다.
        
        \~english TODO:translate needed.

        \~chinese
        告知是否可以读取相当于count量的信息。

        \~japanese
        
        \~
        */
        public bool CanRead(int count)
        {
            // 아예 읽을게 없으면 splitter test 자체도 제낀다.
            if (count == 0)
                return true;

            int oldReadByteOffset = readByteOffset;

            //AdjustReadOffsetByteAlign();

            if (!CanRead_NoTestSplitter(count))
            {
                readByteOffset = oldReadByteOffset;
                return false;
            }
            ReadAndCheckTestSplitterOnCase();

            readByteOffset = oldReadByteOffset;

            return true;
        }

        private bool CanRead_NoTestSplitter(int count)
        {
            if (msgBuffer.data == null)
                throw new Exception("Cannot read from Null pointer message!");

            int oldReadByteOffset = readByteOffset;

            //AssureReadOffsetByteAlign();

            if (ReadOffset + count > msgBuffer.Count)
            {
                readByteOffset = oldReadByteOffset;
                return false;
            }

            readByteOffset = oldReadByteOffset;

            return true;
        }



        public void Write(Byte[] data)
        {
            if (null == data || data.Length == 0)
                return;

            //AdjustWriteOffsetByteAlign();

            Write_NoTestSplitter(data);

            WriteTestSplitterOnCase();
        }

        public void Write(Byte[] data, int dataLength)
        {
            if (null == data || dataLength <= 0)
                return;

            //AdjustWriteOffsetByteAlign();

            Write_NoTestSplitter(data, dataLength);

            WriteTestSplitterOnCase();
        }

        public void Write(bool b)
        {
            Write(BitConverter.GetBytes(b));
            //bool* ptr = &b;
            //Write((Byte*)ptr, sizeof(bool));
            //Write(BitConverter.GetBytes(b));
        }
        public void Write(sbyte b)
        {
            Write(BitConverter.GetBytes(b), sizeof(sbyte));
            //sbyte* ptr = &b;
            //Write((Byte*)ptr, sizeof(sbyte));
        }
        public void Write(Byte b)
        {
            //AdjustWriteOffsetByteAlign();

            msgBuffer.Add(b);

            WriteTestSplitterOnCase();

            //Write(BitConverter.GetBytes(b));
            //Byte* ptr = &b;
            //Write(ptr, sizeof(Byte));
        }
        public void Write(short b)
        {
            Write(BitConverter.GetBytes(b));
            //short* ptr = &b;
            //Write((Byte*)ptr, sizeof(short));
        }
        public void Write(ushort b)
        {
            Write(BitConverter.GetBytes(b));
            //ushort* ptr = &b;
            //Write((Byte*)ptr, sizeof(ushort));
        }
        public void Write(int b)
        {
            Write(BitConverter.GetBytes(b));
            //int* ptr = &b;
            //Write((Byte*)ptr, sizeof(int));
        }
        public void Write(long b)
        {
            Write(BitConverter.GetBytes(b));
            //Int64* ptr = &b;
            //Write((Byte*)ptr, sizeof(Int64));
        }
        public void Write(UInt64 b)
        {
            Write(BitConverter.GetBytes(b));
            //UInt64* ptr = &b;
            //Write((Byte*)ptr, sizeof(UInt64));
        }
        public void Write(uint b)
        {
            Write(BitConverter.GetBytes(b));
            //uint* ptr = &b;
            //Write((Byte*)ptr, sizeof(uint));
        }
        public void Write(float b)
        {
            Write(BitConverter.GetBytes(b));
            //float* ptr = &b;
            //Write((Byte*)ptr, sizeof(float));
        }
        public void Write(double b)
        {
            Write(BitConverter.GetBytes(b));
            //double* ptr = &b;
            //Write((Byte*)ptr, sizeof(double));
        }

        public void Write(EncryptMode b)
        {
            Write((Byte)b);
        }
        public void Write(System.Guid b)
        {
            Write(b.ToByteArray());
        }
        public void Write(HostID b)
        {
            Write((int)b);
        }
        public void Write(RmiID b)
        {
            Write((ushort)b);
        }
        //internal void Write(MessageType b)
        //{
        //    Write((Byte)b);
        //}
        public void Write(ErrorType b)
        {
            Write((uint)b);
        }


        //ipv4일때만 가능하다는 것을 주의하자.
        public void Write(IPEndPoint b)
        {
            byte[] byteIP = b.Address.GetAddressBytes();

            uint ip = (uint)byteIP[3] << 24;
            ip += (uint)byteIP[2] << 16;
            ip += (uint)byteIP[1] << 8;
            ip += (uint)byteIP[0];

            Write(ip);
            Write((ushort)b.Port);
        }

        public void Write(ByteArray b)
        {
            WriteScalar(b.Count);
            Write(b.data, b.Count);
        }

        public void Write(String b)
        {
            if (b.Length < 0 || b.Length > STRING_LENGTH_LIMIT)
            {
                throw new System.Exception(String.Format("Message.Write String failed! length = {0}", b.Length));
            }

            // 바이트 수로 전송합니다.
            byte [] writeStr = System.Text.Encoding.Unicode.GetBytes(b);
            WriteScalar(writeStr.Length);
            Write(writeStr);
        }

        //public void Write(FrameNumber b)
        //{
        //    Write((int)b);
        //}

        public void Write(NamedAddrPort b)
        {
            Write(b.addr);
            Write(b.port);
        }

        public void Write(MessagePriority b)
        {
            Write((Byte)b);
        }

        public void Write(HostIDArray b)
        {
            int count = b.Count;
            WriteScalar(count);

            for (int i = 0; i < count; ++i)
            {
                Write(b[i]);
            }
        }

        //long가 int64..
        public void WriteScalar(long a)
        {
            if (isSimplePacketMode)
            {
                Write(a);
                return;
            }

            CompactScalarValue comp = new CompactScalarValue();

            comp.MakeBlock(a);
            // 이를 메시지 객체에 넣기
            Write(comp.filledBlock, comp.filledBlockLength);
        }
        public void WriteScalar(int a)
        {
            WriteScalar((long)a);
        }

        public void WriteScalar(UInt32 a)
        {
            WriteScalar((long)a);
        }

        public void WriteScalar(UInt64 a)
        {
            WriteScalar((long)a);
        }

      
        private void ReadAndCheckTestSplitterOnCase()
        {
            if (enableTestSplitter)
            {
                Byte s = new Byte();
                if (!Read(out s) || s != (Byte)TestSplitter.Splitter)
                {
                    throw new System.Exception(String.Format("CMessage: Test splitter failure(reading={0},length={1})", ReadOffset, Length));
                }
            }
        }

        public bool SkipRead(int count)
        {
            ReadAndCheckTestSplitterOnCase();

            if (msgBuffer == null)
                throw new System.Exception("Read data from Null message");

            if (ReadOffset + count > msgBuffer.Count)
                return false;

            readByteOffset += count;

            return true;
        }

        public bool ReadScalar(ref long a)
        {
            if (isSimplePacketMode)
            {
                return Read(out a);
            }

            CompactScalarValue val = new CompactScalarValue();
            if (!val.ExtractValue(msgBuffer.data, ReadOffset, Length))
            {
                return false;
            }

            a = val.extractedValue;
            SkipRead(val.extracteeLength);

            return true;
        }

        public bool ReadScalar(ref int a)
        {
            long ba = 0;
            if (!ReadScalar(ref ba))
                return false;

            a = (int)ba;
            return true;
        }

        public bool ReadScalar(ref UInt64 a)
        {
            long ba = 0;
            if (!ReadScalar(ref ba))
                return false;

            a = (UInt64)ba;
            return true;
        }

        public bool Read(out Byte[] b, int count)
        {
            b = new Byte[count];
            if (count == 0)
                return true;

            //AdjustReadOffsetByteAlign();

            if (msgBuffer == null)
                throw new System.Exception("Cannot read from Null pointer message!");

            //AssureReadOffsetByteAlign();

            if (ReadOffset + count > msgBuffer.Count)
                return false;

            Array.Copy(msgBuffer.data, ReadOffset, b, 0, count);

            readByteOffset += count;

            ReadAndCheckTestSplitterOnCase();

            return true;
        }

        public bool Read(out bool b)
        {
            b = new bool();
            //AdjustReadOffsetByteAlign();

            if (msgBuffer == null)
                throw new System.Exception("Cannot read from Null pointer message!");

            //AssureReadOffsetByteAlign();

            if (ReadOffset + sizeof(bool) > msgBuffer.Count)
                return false;

            b = BitConverter.ToBoolean(msgBuffer.data, ReadOffset);

            readByteOffset += sizeof(bool);

            ReadAndCheckTestSplitterOnCase();

            return true;
        }

        public bool Read(out sbyte b)
        {
            b = new sbyte();
            //AdjustReadOffsetByteAlign();

            if (msgBuffer == null)
                throw new System.Exception("Cannot read from Null pointer message!");

            //AssureReadOffsetByteAlign();

            if (ReadOffset + sizeof(sbyte) > msgBuffer.Count)
                return false;

            b = (sbyte)msgBuffer.data[ReadOffset];

            readByteOffset += sizeof(sbyte);

            ReadAndCheckTestSplitterOnCase();

            return true;
        }

        public bool Read(out Byte b)
        {
            b = new Byte();
            //AdjustReadOffsetByteAlign();

            if (msgBuffer == null)
                throw new System.Exception("Cannot read from Null pointer message!");

            //AssureReadOffsetByteAlign();

            if (ReadOffset + sizeof(Byte) > msgBuffer.Count)
                return false;

            b = msgBuffer.data[ReadOffset];

            readByteOffset += sizeof(bool);

            ReadAndCheckTestSplitterOnCase();

            return true;
        }

        public bool Read(out short b)
        {
            b = new short();
            //AdjustReadOffsetByteAlign();

            if (msgBuffer == null)
                throw new System.Exception("Cannot read from Null pointer message!");

            //AssureReadOffsetByteAlign();

            if (ReadOffset + sizeof(short) > msgBuffer.Count)
                return false;

            b = BitConverter.ToInt16(msgBuffer.data, ReadOffset);

            readByteOffset += sizeof(short);

            ReadAndCheckTestSplitterOnCase();

            return true;
        }
        public bool Read(out ushort b)
        {
            b = new ushort();
            //AdjustReadOffsetByteAlign();

            if (msgBuffer == null)
                throw new System.Exception("Cannot read from Null pointer message!");

            //AssureReadOffsetByteAlign();

            if (ReadOffset + sizeof(ushort) > msgBuffer.Count)
                return false;

            b = BitConverter.ToUInt16(msgBuffer.data, ReadOffset);

            readByteOffset += sizeof(ushort);

            ReadAndCheckTestSplitterOnCase();

            return true;
        }

        public bool Read(out int b)
        {
            b = new int();
            //AdjustReadOffsetByteAlign();

            if (msgBuffer == null)
                throw new System.Exception("Cannot read from Null pointer message!");

            //AssureReadOffsetByteAlign();

            if (ReadOffset + sizeof(int) > msgBuffer.Count)
                return false;

            b = BitConverter.ToInt32(msgBuffer.data, ReadOffset);

            readByteOffset += sizeof(int);

            ReadAndCheckTestSplitterOnCase();

            return true;
        }
        public bool Read(out uint b)
        {
            b = new uint();
            //AdjustReadOffsetByteAlign();

            if (msgBuffer == null)
                throw new System.Exception("Cannot read from Null pointer message!");

            //AssureReadOffsetByteAlign();

            if (ReadOffset + sizeof(uint) > msgBuffer.Count)
                return false;

            b = BitConverter.ToUInt32(msgBuffer.data, ReadOffset);

            readByteOffset += sizeof(uint);

            ReadAndCheckTestSplitterOnCase();

            return true;
        }

        public bool Read(out long b)
        {
            b = new long();
            //AdjustReadOffsetByteAlign();

            if (msgBuffer == null)
                throw new System.Exception("Cannot read from Null pointer message!");

            //AssureReadOffsetByteAlign();

            if (ReadOffset + sizeof(long) > msgBuffer.Count)
                return false;

            b = BitConverter.ToInt64(msgBuffer.data, ReadOffset);

            readByteOffset += sizeof(long);

            ReadAndCheckTestSplitterOnCase();

            return true;
        }
        public bool Read(out UInt64 b)
        {
            b = new UInt64();
            //AdjustReadOffsetByteAlign();

            if (msgBuffer == null)
                throw new System.Exception("Cannot read from Null pointer message!");

            //AssureReadOffsetByteAlign();

            if (ReadOffset + sizeof(UInt64) > msgBuffer.Count)
                return false;

            b = BitConverter.ToUInt64(msgBuffer.data, ReadOffset);

            readByteOffset += sizeof(UInt64);

            ReadAndCheckTestSplitterOnCase();

            return true;
        }

        public bool Read(out float b)
        {
            b = new float();
            //AdjustReadOffsetByteAlign();

            if (msgBuffer == null)
                throw new System.Exception("Cannot read from Null pointer message!");

            //AssureReadOffsetByteAlign();

            if (ReadOffset + sizeof(float) > msgBuffer.Count)
                return false;

            b = BitConverter.ToSingle(msgBuffer.data, ReadOffset);

            readByteOffset += sizeof(float);

            ReadAndCheckTestSplitterOnCase();

            return true;
        }

        public bool Read(out double b)
        {
            b = new double();
            //AdjustReadOffsetByteAlign();

            if (msgBuffer == null)
                throw new System.Exception("Cannot read from Null pointer message!");

            //AssureReadOffsetByteAlign();

            if (ReadOffset + sizeof(double) > msgBuffer.Count)
                return false;

            b = BitConverter.ToDouble(msgBuffer.data, ReadOffset);

            readByteOffset += sizeof(double);

            ReadAndCheckTestSplitterOnCase();

            return true;
        }


        public bool Read(out RmiID b)
        {
            b = new RmiID();
            ushort a = 0;
            if (!Read(out a))
                return false;

            b = (RmiID)a;
            return true;
        }

        public bool Read(out EncryptMode b)
        {
            b = new EncryptMode();
            Byte a = 0;
            if (!Read(out a))
                return false;

            b = (EncryptMode)a;
            return true;
        }

        public bool Read(out HostID b)
        {
            b = new HostID();
            int a = 0;
            if (!Read(out a))
                return false;

            b = (HostID)a;
            return true;
        }

        public bool Read(out ByteArray b)
        {
            b = new ByteArray();
            int readLength = 0;
            if (!ReadScalar(ref readLength))
            {
                return false;
            }

            if (readLength < 0 || ReadOffset + readLength > Length)
                return false;

            b.Count = readLength;

            return Read(out b.data, readLength);
        }

        public bool Read(out IPEndPoint b)
        {
            b = new IPEndPoint(new IPAddress(0), 0);
            uint binaryAdress = 0;
            if (!Read(out binaryAdress))
                return false;

            ushort port = 0;

            if (!Read(out port))
                return false;

            b.Address = new IPAddress(binaryAdress);
            b.Port = port;

            return true;
        }

        public bool Read(out ErrorType b)
        {
            b = new ErrorType();
            uint uierror = 0;
            if (!Read(out uierror))
                return false;

            b = (ErrorType)uierror;

            return true;
        }

        public bool Read(out String b)
        {
            b = "";
            int readlength = 0;

            if (!ReadScalar(ref readlength))
                return false;

            if (readlength < 0 || readlength > STRING_LENGTH_LIMIT)
                throw new Exception(String.Format("Message Read String failed! length = {0}", readlength));

            if (readlength == 0)
                return true;

            // length 값은 전송된 바이트 길이이기 때문에 아래 부분을 주석처리 합니다.
            //readlength = sizeof(Char) * readlength;

            if (ReadOffset + readlength > msgBuffer.Count)
            {
                return false;
            }
            b = System.Text.Encoding.Unicode.GetString(msgBuffer.data, ReadOffset, readlength);

            readByteOffset += readlength;

            return true;
        }

        //public bool Read(out FrameNumber b)
        //{
        //    b = new FrameNumber();
        //    uint read = 0;
        //    if (!Read(out read))
        //        return false;

        //    b = (FrameNumber)read;

        //    return true;
        //}

        internal bool Read(out HostIDArray b)
        {
            b = new HostIDArray();
            int readLength = 0;
            if (!ReadScalar(ref readLength))
            {
                return false;
            }

            if (readLength < 0 || ReadOffset + readLength > Length)
                return false;

            b.Count = readLength;

            for (int i = 0; i < readLength; ++i)
            {
                if (!Read(out b.data[i]))
                    return false;
            }

            return true;
        } 

        private static int GUID_LENGTH = 16;
        public bool Read(out System.Guid b)
        {
            Byte[] read = new Byte[GUID_LENGTH];

            if (!Read(out read, GUID_LENGTH))
            {
                b = System.Guid.Empty;
                return false;
            }

            b = new System.Guid(read);

            return true;
        }

        public bool Read(out NamedAddrPort b)
        {
            b = new NamedAddrPort();

            String addr = null;
            ushort port = 0;

            if (Read(out addr) && Read(out port))
            {
                b.addr = addr;
                b.port = port;
                return true;
            }
            return false;
        }

        //internal bool Read(out MessageType b)
        //{
        //    b = MessageType.None;
        //    Byte bb = 0;

        //    if (!Read(out bb))
        //        return false;

        //    b = (MessageType)bb;

        //    return true;
        //}

        internal bool Read(out FallbackMethod b)
        {
            b = FallbackMethod.FallbackMethod_None;

            Byte bb = 0;
            if (!Read(out bb))
                return false;

            b = (FallbackMethod)bb;

            return true;
        }

        internal bool Read(out DirectP2PStartCondition b)
        {
            b = DirectP2PStartCondition.DirectP2PStartCondition_LAST;

            Byte bb = 0;
            if (!Read(out bb))
                return false;

            b = (DirectP2PStartCondition)bb;

            return true;
        }

        internal bool Read(out MessagePriority b)
        {
            b = MessagePriority.MessagePriority_LAST;

            Byte bb = 0;
            if (!Read(out bb))
                return false;

            b = (MessagePriority)bb;

            return true;
        }
    }
    /**  @} */
}
