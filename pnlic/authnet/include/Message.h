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

#pragma once

#if defined(_WIN32)
//#include <string>
#endif
#include <string>
#include "FastArray.h"
#include "FastArrayPtr.h"
#include "AddrPort.h"
#include "EncryptEnum.h"
#include "FakeClr.h"
#include "HostIDArray.h"
#include "NetConfig.h"
#include "Enums.h"
#include "BasicTypes.h"

//#pragma pack(push,8)

namespace Proud
{
#if (_MSC_VER>=1400)
#pragma managed(push, off)
#endif
	/** \addtogroup net_group
	*  @{
	*/
	class CMessage;
	class CSendFragRefs;
	class RelayDestList;
	class CNetSettings;


	struct AddrPort;
	struct RelayDest;
	
	// non-vc++ 호환 위해, 전방 선언을 했음. 구현은 이 함수 저 아래에.
	template<typename T>
	inline bool CMessage_RawRead(CMessage &a, T &b);

// 	template<typename T>
// 	void Buffer_AddRange(ByteArrayPtr buffer, T value)
// 	{
// 		buffer->AddRange((BYTE*)&value, sizeof(T));
// 	}

	/** 
	\~korean
	메시지 클래스. 자세한 내용은 \ref message_class 를 참고.

	\~english TODO:translate needed.

	\~chinese
	信息类。详细请参考\ref message_class%。

	\~japanese
	\~
	*/
	class CMessage
	{
		int m_readBitOffset;
		int m_bitLengthInOneByte; 

		// 이 값이 true이면 read/write scalar가 비활성되어 무조건 int64로 기재되는 등,
		// 패킷 구조는 사용자가 알 수 있도록 단순화되되 필요한 효율적인 기능들이 사라진다.
		bool m_isSimplePacketMode;

		friend void AdjustReadOffsetByteAlign(CMessage& msg);
		friend void AssureReadOffsetByteAlign(CMessage& msg);


		template<typename T, bool RAWTYPE, typename COLLT>
		static inline bool ReadArrayT(CMessage &a, COLLT &b)
		{
			int length;
			if (a.ReadScalar(length) == false)
				return false;
			
			// count가 해킹되어 말도 안되는 값이면 실패 처리하기
			// 물론 모든 경우를 잡지는 못하지만 (sizeof elem 무용지물) 그래도 최소 1바이트는 쓸테니.
			if (length<0 || length > a.GetLength() - a.GetReadOffset())
				return false;

			b.SetCount(length);

			if(RAWTYPE)
			{
				return a.Read((uint8_t*)b.GetData(), sizeof(T) * length);
			}
			else 
			{
				for (int i = 0; i < length; i++)
				{
					if (a.Read(b[i]) == false)
						return false;
				}
				return true;
			}
		}

		template<typename T, bool RAWTYPE, typename COLLT>
		inline void WriteArrayT(CMessage &a, const COLLT &b)
		{
			int length = (int)b.GetCount();
			a.WriteScalar(length);

			if(RAWTYPE)
				a.Write((uint8_t*)b.GetData(), sizeof(T) * length);
			else
			{
				for (int i = 0; i < length; i++)
					a.Write(b[i]);
			}
		}

	public:
		ByteArrayPtr m_msgBuffer;

		PROUD_API CMessage();
		PROUD_API CMessage(const CMessage &src);

		inline bool DataEquals(const CMessage& rhs)
		{
			return m_msgBuffer.Equals(rhs.m_msgBuffer);
		}
		/** 
		\~korean
		메시지로부터 데이터를 읽는다.
		\param data 읽은 데이터가 저장될 곳
		\param count data의 바이트 단위 크기 
		\return count만큼 성공적으로 읽었으면 true.

		\~english TODO:translate needed.

		\~chinese
		从信息读取数据。
		\param data 所读取的数据储存的位置。
		\param count data的byte单位大小。 
		\return 读到最后的话true。

		\~japanese
		\~
		*/
		PROUD_API bool Read( uint8_t* data, int count );

		PROUD_API bool CanRead( int count );		
		
		/** 
		\~korean
		메시지에 데이터를 기록한다.

		\~english TODO:translate needed.

		\~chinese
		记录信息数据。

		\~japanese
		\~
		*/
		inline void Write(const uint8_t* data, int count)
		{
			// 아예 쓸게 없으면 splitter test 자체도 제낀다.
			if (count == 0)
				return;

			AdjustWriteOffsetByteAlign();

			Write_NoTestSplitter(data, count);

			WriteTestSplitterOnCase();
		}
		PROUD_API void CopyFromArrayAndResetReadOffset(const uint8_t* src, int srcLength);
		//PROUD_API void ResetWritePointer();

		/** 
		\~korean
		읽기 지점을 강제 조정한다. 바이트 단위이다.

		\~english TODO:translate needed.

		\~chinese
		强制调整读取地点。Byte 单位。

		\~japanese
		\~
		*/
		PROUD_API void SetReadOffset(int offset);

		// 사용자는 이 함수를 호출하지 말 것
		PROUD_API void SetSimplePacketMode(bool isSimplePacketMode);

		/** 
		\~korean
		메시지의 현재 크기를 설정한다.

		\~english TODO:translate needed.

		\~chinese
		设置信息现在的大小。

		\~japanese
		\~
		*/
		PROUD_API void SetLength(int count);
		PROUD_API int GetWriteOffset();
		PROUD_API void EnableTestSplitter(bool enable);
		PROUD_API bool IsTestSplitterEnabled() {
			return m_enableTestSplitter;
		}
	private:
		bool m_enableTestSplitter;
		enum { Splitter = 254 };

		PROUD_API void ReadAndCheckTestSplitterOnCase();
		inline void WriteTestSplitterOnCase()
		{
			if (m_enableTestSplitter)
			{
				uint8_t s = Splitter;
				Write_NoTestSplitter(&s, sizeof(s));
			}
		}
		PROUD_API bool Read_NoTestSplitter( uint8_t* data , int count);
		PROUD_API bool CanRead_NoTestSplitter( int count);
		inline void Write_NoTestSplitter( const uint8_t* data, int count )
		{
			m_msgBuffer.MustNotNull();

			m_msgBuffer.AddRange(data, count);

			/* 		// CS lock이 큰지 이게 큰지 비교할라구. 임시다.
			BYTE* p = m_msgBuffer.GetData();
			int len = m_msgBuffer.Count;
			for(int i=0;i<len;i++)
			{
			p[i]++;
			}
			for(int i=0;i<len;i++)
			{
			p[i]--;
			} */
		}

		inline void AdjustWriteOffsetByteAlign()
		{
			// 비트 단위 기록을 하던 중이라 쓰기 포인트가 byte align이 아니면, 조정한다.
			// 단, 이것을 호출 후에는 반드시 최소 1 바이트를 기록해야 한다. 안그러면 기록하던 비트들은 유실된다.
			m_bitLengthInOneByte=0;
		}

	public:
		/** 
		\~korean
		이 객체의 메모리 버퍼를 output이 length 크기만큼 읽어가는 양상으로 만들되,
		output은 외부로서 이 객체의 버퍼를 참조하도록 만든다. 
		성능을 위해 사용되는 함수다.

		\~english TODO:translate needed.

		\~chinese
		把此对象的内存buffer制造成output读取相当于length大小的局面，但是output作为外部制造成参考此对象的buffer。
		为了性能使用的函数。

		\~japanese
		\~
		*/
		PROUD_API bool ReadWithShareBuffer(CMessage& output, int length);

		/** 
		\~korean
		메시지 객체에 데이터를 추가 기록한다.
		bool 타입 뿐만 아니라 다양한 타입을 지원한다.

		\~english TODO:translate needed.

		\~chinese
		往信息对象添加记录数据。
		不只是bool类型，还支持各种类型。

		\~japanese
		\~
		*/
		inline void Write(bool b) {
			Write((const uint8_t*)&b, sizeof(b));
		}
		inline void Write(char b) {
			Write((const uint8_t*)&b, sizeof(b));
		}
		inline void Write(uint8_t b) {
			Write((const uint8_t*)&b, sizeof(b));
		}
		inline void Write(int16_t b) {
			Write((const uint8_t*)&b, sizeof(b));
		}
		inline void Write(uint16_t b) {
			Write((const uint8_t*)&b, sizeof(b));
		}
		inline void Write(int32_t b) {
			Write((const uint8_t*)&b, sizeof(b));
		}
		inline void Write(uint32_t b) {
			Write((const uint8_t*)&b, sizeof(b));
		}
		/* Q: 왜 long type에 대해서는 Write & Read가 없나요?
		A: long, unsigned long은 몇몇 컴파일러에서 크기가 서로 다릅니다. 
		VC++ x64에서는 32bit이지만, 다른 컴파일러 중 하나는 x64에서 64bit로 처리됩니다.
		이러면 네트워크로 주고받는 데이터 타입으로 부적절입니다.
		대신 int64_t, uint64_t를 쓰시기 바랍니다. 아니면 (unsigned)int or (u)int32_t를 쓰시던지.
		*/
		inline void Write(int64_t b) {
			Write((const uint8_t*)&b, sizeof(b));
		}
		inline void Write(uint64_t b) {
			Write((const uint8_t*)&b, sizeof(b));
		}
		inline void Write(float b) {
			Write((const uint8_t*)&b, sizeof(b));
		}
		inline void Write(double b) {
			Write((const uint8_t*)&b, sizeof(b));
		}
		inline void Write(HostID b) {
			Write((int)b);
		}
#if _WIN32
private:
		inline void Write(_MessageType b) {
			Write((char)b);
		}
public:
#endif
		const static int MessageTypeFieldLength = 1;
#if _WIN32
private:
		PROUD_API bool Read(_MessageType &b);
public:
#endif
		inline void Write(MessagePriority b) {
			Write((uint8_t)b);
		}
		PROUD_API bool Read(MessagePriority &b);

		inline void Write(DirectP2PStartCondition b) {
			Write((uint8_t)b);
		}
		PROUD_API bool Read(DirectP2PStartCondition &b);

		inline void Write(FallbackMethod b) {
			Write((uint8_t)b);
		}
		PROUD_API bool Read(FallbackMethod &b);

		inline void Write(EncryptMode b) {
			Write((uint8_t)b);
		}
		PROUD_API bool Read(EncryptMode &b);

		inline void Write(ReliableUdpFrameType b) {
			Write((char)b);
		}
		PROUD_API bool Read(ReliableUdpFrameType &b);
#if _WIN32
private:
		inline void Write(_LocalEventType b) {
			Write((uint16_t)b);
		}
		PROUD_API bool Read(_LocalEventType &b);
public:
#endif
		inline void Write(RmiID b) {
			Write((uint16_t)b);
		}
		PROUD_API bool Read(RmiID &b);
		PROUD_API void Write(const AddrPort &b);
		PROUD_API bool Read(AddrPort &b);
		
		bool Read(ByteArray &b)
		{
			return ReadArrayT<uint8_t, true, ByteArray>(*this,b);
		}
		void Write(const ByteArray &b)
		{
			WriteArrayT<uint8_t,true,ByteArray>(*this,b);
		}

		void Write(ByteArrayPtr b)
		{
			// b는 buffer usage를 미리 지정했어야 함
			WriteArrayT<uint8_t,true,ByteArrayPtr>(*this,b);
		}

		bool Read(ByteArrayPtr &b)
		{
			// b는 buffer usage를 미리 지정했어야 함
			return ReadArrayT<uint8_t, true, ByteArrayPtr>(*this,b);
		}

		bool Read(CNetSettings &b);
		void Write(const CNetSettings &b);

		/** 
		\~korean
		scalar compression 기법으로 값을 읽는다.

		\~english
		Reads with scalar compression technique

		\~chinese
		用scalar compression技术读入值。

		\~japanese
		\~
		 */
		PROUD_API bool ReadScalar(int64_t &a);
		/** 
		\~korean
		scalar compression 기법으로 값을 기록한다. 

		\~english
		Writes with scalar compression technique

		\~chinese
		用scalar compression技术记录值。

		\~japanese
		\~
		*/
		PROUD_API void WriteScalar(int64_t a);

		inline void WriteScalar(int32_t a)
		{
			WriteScalar((int64_t)a);
		}

		inline void WriteScalar(uint32_t a)
		{
			WriteScalar((int64_t)a);
		}
		
		inline bool ReadScalar(int &a)
		{
			int64_t ba;
			if(!ReadScalar(ba))
				return false;
			else
			{

				a = (int) ba;
				return true;
			}
		}

		inline void WriteScalar(uint64_t a)
		{
			WriteScalar((int64_t)a);
		}

		inline bool ReadScalar(uint64_t &a)
		{
			int64_t ba;
			if(!ReadScalar(ba))
				return false;
			else
			{

				a = (uint64_t) ba;
				return true;
			}
		}

		//bool Read(out CFastArray<BYTE> b)
		//{
		//	b = new CFastArray<BYTE>();
		//	int length;
		//	if (Read(out length) == false)
		//		return false;

		//	for (int i = 0; i < length; i++)
		//	{
		//		BYTE p = 0;
		//		if (Read(out p) == false)
		//			return false;
		//		b.Add(p);
		//	}

		//	return true;
		//}

		//void Write(CFastArray<BYTE> b)
		//{
		//	int length = b.GetCount();
		//	Write(length);
		//	for (int i = 0; i < length; i++)
		//	{
		//		Write(b[i]);
		//	}
		//}

		//PROUD_API bool ReadFrameNumberArray(CFastArrayPtr<int,true> &b);
		//PROUD_API void WriteFrameNumberArray(const int* arr, int count);

		/** 
		\~korean
		메시지 객체에 데이터를 추가 기록한다.
		bool 타입 뿐만 아니라 다양한 타입을 지원한다. 
		\return 완전히 읽는데 성공하면 true 

		\~english
		Additionally writes data to message object
		Suuports various types including bool type 
		\return True if successful in thorough reading

		\~chinese
		往信息对象添加记录数据。
		不仅是bool类型，支持各种类型。
		\return 完全的读入成功的话true

		\~japanese
		\~
		*/
		inline bool Read(bool &b) {
			return CMessage_RawRead(*this, b);
		}
		inline bool Read(char &b) {
			return CMessage_RawRead(*this, b);
		}
		inline bool Read(int8_t &b) {
			return CMessage_RawRead(*this, b);
		}
		inline bool Read(uint8_t &b) {
			return CMessage_RawRead(*this, b);
		}
		inline bool Read(short &b) {
			return CMessage_RawRead(*this, b);
		}
		inline bool Read(uint16_t &b) {
			return CMessage_RawRead(*this, b);
		}
		inline bool Read(int &b) {
			return CMessage_RawRead(*this, b);
		}
		inline bool Read(unsigned int &b) {
			return CMessage_RawRead(*this, b);
		}
		//bool Read(unsigned &b) { return CMessage_RawRead(*this,b); }
		inline bool Read(int64_t &b) {
			return CMessage_RawRead(*this, b);
		}
		inline bool Read(uint64_t &b) {
			return CMessage_RawRead(*this, b);
		}
		inline bool Read(float &b) {
			return CMessage_RawRead(*this, b);
		}
		inline bool Read(double &b) {
			return CMessage_RawRead(*this, b);
		}

		void Write(const Guid &b);
		bool Read(Guid &b);

		PROUD_API bool Read(HostID &b);

		//void ClonedTo(CMessage_RawRead()
		//{
		//	BYTE[] o = new BYTE[m_buffer.GetCount()];
		//	m_buffer.CopyTo(o);
		//	return o;
		//}

		PROUD_API void Write(const RelayDestList &relayDestList);
		PROUD_API bool Read(RelayDestList &relayDestList);
		PROUD_API void Write(const RelayDest &rd);
		PROUD_API bool Read(RelayDest &rd);

		inline void Write(const HostIDArray &lst)
		{
			WriteArrayT<HostID,true,HostIDArray>(*this, lst);
		}

		inline bool Read(HostIDArray &lst)
		{
			return ReadArrayT<HostID,true,HostIDArray>(*this, lst);
		}

		PROUD_API void Write(NamedAddrPort n);
		PROUD_API bool Read(NamedAddrPort &n);

// 		PROUD_API void WriteMessageContent(const CMessage &msg);
// 		PROUD_API bool ReadMessageContent(CMessage &msg);

		/** 
		\~korean
		메시지의 길이를 얻는다. 

		\~english
		Gets the length of message

		\~chinese
		获取信息的长度。

		\~japanese
		\~
		*/
		inline int GetLength() const
		{
			return (int)m_msgBuffer.GetCount();
		}

		/** 
		\~korean
		메시지의 현재 읽기 지점을 구한다. 리턴값은 바이트 단위다.

		\~english
		Calculates current read point of message. The return value is in byte. 

		\~chinese
		求信息的现在读取地点。返回值是byte单位。

		\~japanese
		\~
		 */
		inline int GetReadOffset() const
		{
			return m_readBitOffset>>3; // 나누기 8. signed도 >>연산자가 잘 작동하는구마이!
		}

		PROUD_API void AppendByteArray(const uint8_t* fragment, int fragmentLength);
		PROUD_API void AppendFragments(const CSendFragRefs& fragments);

		inline uint8_t* GetData()
		{
			return m_msgBuffer.GetData();
		}

		inline const uint8_t* GetData() const
		{
			return m_msgBuffer.GetData();
		}

		template<typename BYTEARRAY>
		void CopyToT(BYTEARRAY& dest)
		{
			dest.SetCount(GetLength());
			UnsafeFastMemcpy(dest.GetData(), GetData(), GetLength());
		}

		void CopyTo(ByteArray& dest)
		{
			CopyToT<ByteArray>(dest);
		}
		void CopyTo(ByteArrayPtr& dest)
		{
			CopyToT<ByteArrayPtr>(dest);
		}
		void CopyTo(CMessage& dest)
		{
			CopyToT<ByteArrayPtr>(dest.m_msgBuffer);
			dest.m_readBitOffset = m_readBitOffset;
			dest.m_isSimplePacketMode = m_isSimplePacketMode;
		}

		/*		PROUD_API void CloneTo(CMessage& dest);
		PROUD_API ByteArrayPtr ToByteArray();
		PROUD_API void ToByteArray(ByteArray &ret);
		PROUD_API void CopyTo(ByteArray& dest); */

		void Clear()
		{
			m_msgBuffer.Clear();
		}

		PROUD_API void ShareFromAndResetReadOffset(ByteArrayPtr data);

		/** 
		\~korean
		외부 버퍼를 사용하도록 선언한다.
		\param buf 외부 버퍼 포인터
		\param capacity 외부 버퍼의 크기

		\~english TODO:translate needed.
		Declares to use external buffer



		\~chinese
		宣言使用外部buffer。
		\param buf 外部buffer指针。
		\param capacity 外部buffer大小。

		\~japanese
		\~
		 */
		PROUD_API void UseExternalBuffer(uint8_t* buf, int capacity);
		
		/** 
		\~korean
		내부 버퍼를 사용하도록 선언한다.

		\~english
		Declares to use internal buffer

		\~chinese
		宣言使用内部buffer。

		\~japanese
		\~
		 */
		PROUD_API void UseInternalBuffer();

		/** 
		\~korean
		사용하던 버퍼를 완전히 리셋하고, 새로 외부 또는 내부 버퍼를 사용하도록 선언한다.
		- UseExternalBuffer, UseInternalBuffer를 재사용하려면 반드시 이것을 호출해야 한다.
		이러한 메서드가 따로 있는 이유는 사용자가 실수하는 것을 방지하기 위해서다. 
		- 이 메서드 호출 후에는 read offset과 메시지 크기가 초기화된다.

		\~english
		Completely resets the PROUD_API void UseExternalBuffer(BYTE* buf, int capacity)r
		- This must be called in order to re-use UseExternalBuffer and UseInternalBuffer.
		  The reasone why this kind of method exists is to prevent for user to make mistakes.
		- After this method is called, the size of read offset and message will be initialized.

		\~chinese
		完全复位使用的buffer，宣言重新使用外部或者内部buffer。
		- 想再使用UseExternalBuffer, UseInternalBuffer的话必须呼叫这个。
		另有这种方法的理由是为了防止用户失误。
		- 呼叫此方法呼叫以后会初始化read offset和信息大小。

		\~japanese

		 */
		PROUD_API void UninitBuffer();

		/**
		\~korean
		현재 읽은 데이터 크기에 count를 더한다.

		\~english TODO:translate needed.

		\~chinese
		往现在所读数据大小上添加count。

		\~japanese
		\~
		*/
		PROUD_API bool SkipRead(int count);

		/**
		\~korean
		문자열 넣기

		\~english
		Write string

		\~chinese
		输入字符串。

		\~japanese
		\~
		*/
		PROUD_API void WriteString(const char* str);

		/**
		\~korean
		문자열 넣기

		\~english
		Write string

		\~chinese
		输入字符串。

		\~japanese
		\~
		*/
		PROUD_API void WriteString(const wchar_t* str);

		/**
		\~korean
		문자열 넣기

		\~english
		Write string

		\~chinese
		输入字符串。

		\~japanese
		\~
		*/
		PROUD_API void WriteString(const Proud::StringA &str);

		/**
		\~korean
		문자열 넣기

		\~english
		Write string

		\~chinese
		输入字符串。

		\~japanese
		\~
		*/
		PROUD_API void WriteString(const Proud::StringW &str);
		
		/**
		\~korean
		문자열 넣기

		\~english
		Write string

		\~chinese
		输入字符串。

		\~japanese
		\~
		*/
		PROUD_API void WriteString(const std::string &str)
		{
			WriteString(str.c_str());
		}

		/**
		\~korean
		문자열 넣기

		\~english
		Write string

		\~chinese
		输入字符串。

		\~japanese
		\~
		*/
		PROUD_API void WriteString(const std::wstring &str)
		{	
			WriteString(str.c_str());
		}

		/**
		\~korean
		문자열 꺼내기

		\~english
		Read string

		\~chinese
		导出字符串。

		\~japanese
		\~
		*/
		PROUD_API bool ReadString(Proud::StringA &str);

		/**
		\~korean
		문자열 꺼내기

		\~english
		Read string

		\~chinese
		导出字符串。

		\~japanese
		\~
		*/
		PROUD_API bool ReadString(Proud::StringW &str);
		
		/**
		\~korean
		문자열 꺼내기

		\~english
		Read string

		\~chinese
		导出字符串。

		\~japanese
		\~
		*/
		PROUD_API bool ReadString(std::string &str)
		{
			StringA str2;
			bool ret = ReadString(str2);
			if(ret)
			{
				str = str2;
			}
			return ret;
		}

		/**
		\~korean
		문자열 꺼내기

		\~english
		Read string 

		\~chinese
		导出字符串。

		\~japanese
		\~
		*/
		PROUD_API bool ReadString(std::wstring &str)
		{
			StringW str2;
			bool ret = ReadString(str2);
			if(ret)
			{
				str = str2;
			}
			return ret;
		}

		PROUD_API bool ReadStringA(Proud::StringA &str);
		PROUD_API bool ReadStringW(Proud::StringW &str);
		PROUD_API void WriteStringA(const char* str);
		PROUD_API void WriteStringW(const wchar_t* str);

		/** 
		\~korean
		bitCount 만큼의 비트 데이터를 기록한다.
		- 만약 앞서 bit를 기록하던 중이라 bit offset > 0인 경우 사용 가능한 잔여 비트에 이어서 기록된다.
		- 만약 bit offset > 0인 상태에서 Write()로 인해 byte 단위 쓰기를 할 경우 잔여 비트는 무시되고
		새 바이트에서 쓰기가 재개된다. 즉 byte assign으로 조정된다. 
		따라서 효과적인 비트 사용을 위해서 비트 필드과 비트가 아닌 필드를 끼리끼리 모아서 읽기/쓰기를 해야 한다.

		\~english
		Writes bit data as muuch of bitCount
		- If bit offset > 0 due to prior bit writing then writes continuous to usable remaining bits.
		- When writing byte unit due to Write() when bit offset > 0 then remaining bits will be ignored and the writing will be continued at a new byte. In other words, it will be modified to byte assign. 
		Therefore, to use bits effectively, it must collectively read/write those fields, not fields and bits.

		\~chinese
		记录相当于bitCount的bit数据。
		- 如果之前是记录bit之中而bit offset > 0的情况的话，会接着在可以使用的残留bit上进行记录。
		- 如果从bit offset > 0的情况，因为Write()，要写成byte单位的话，残留bit会被忽视，从新的bit开始写入。即调整为byte assign。
		为了有效地使用bit，聚集是bit领域和不是bit的领域之后进行读/写。

		\~japanese
		\~
		 */
		PROUD_API void WriteBits(uint8_t* src,int srcBitLength);

		/** 
		\~korean
		bitCount만큼의 비트 데이터를 읽어들인다.
		\return 읽는데 성공하면 true를 리턴한다. 읽을 bit가 모자라면 false를 리턴.
		- 만약 앞서 bit를 읽던 중이라 bit offset > 0인 경우 사용 가능한 잔여 비트에 이어서 읽는다.
		- 만약 bit offset>0인 상태에서 Read()로 인해 byte 단위 읽기를 할 경우 잔여 비트는 무시되고
		새 바이트에서 읽기가 재개된다. 즉 byte assign으로 조정된다. 
		따라서 효과적인 비트 사용을 위해서 비트 필드과 비트가 아닌 필드를 끼리끼리 모아서 읽기/쓰기를 해야 한다. 

		\~english
		Reads bit data as much of bitCount
		\return Returns true if reading is successful. Returns false when there are not enough bits to read.
		- If bit offset > 0 due to prior bit reading then reads continuous to usable remaining bits.
		- When reading byte unit due to Read() when bit offset > 0 then remaining bits will be ignored and the reading will be continued at a new byte. In other words, it will be modified to byte assign. 
		Therefore, to use bits effectively, it must collectively read/write those fields, not fields and bits.

		\~chinese
		读入相当于bitCount的bit数据。
		\return 读入成功的话返回ture。缺少要读的bit的话返回false。
		- 如果之前是记录bit之中而bit offset > 0的情况的话，接着从可以使用的残留bit中读取。		-如果是bit offset > 0的情况，因为Read()，要写成byte单位的话，残留bit会被忽视，从新bit上开始读取。即调整为byte assign。
		为了有效地使用bit，聚集是bit领域和不是bit的领域之后进行读/写。

		\~japanese
		\~
		*/
		PROUD_API bool ReadBits(uint8_t* output,int outputBitLength);

		inline bool ReadBits(bool &output,int outputBitLength) {return ReadBits((uint8_t*)&output,outputBitLength);}
		inline void WriteBits(bool src,int srcBitLength) { WriteBits((uint8_t*)&src,srcBitLength);}

		inline bool ReadBits(char &output,int outputBitLength) {return ReadBits((uint8_t*)&output,outputBitLength);}
		inline void WriteBits(char src,int srcBitLength) { WriteBits((uint8_t*)&src,srcBitLength);}

		inline bool ReadBits(unsigned char &output,int outputBitLength) {return ReadBits((uint8_t*)&output,outputBitLength);}
		inline void WriteBits(unsigned char src,int srcBitLength) { WriteBits((uint8_t*)&src,srcBitLength);}

		inline bool ReadBits(short &output,int outputBitLength) {return ReadBits((uint8_t*)&output,outputBitLength);}
		inline void WriteBits(short src,int srcBitLength) { WriteBits((uint8_t*)&src,srcBitLength);}

		inline bool ReadBits(unsigned short &output,int outputBitLength) {return ReadBits((uint8_t*)&output,outputBitLength);}
		inline void WriteBits(unsigned short src,int srcBitLength) { WriteBits((uint8_t*)&src,srcBitLength);}

		inline bool ReadBits(int &output,int outputBitLength) {return ReadBits((uint8_t*)&output,outputBitLength);}
		inline void WriteBits(int src,int srcBitLength) { WriteBits((uint8_t*)&src,srcBitLength);}

		inline bool ReadBits(unsigned int &output,int outputBitLength) {return ReadBits((uint8_t*)&output,outputBitLength);}
		inline void WriteBits(unsigned int src,int srcBitLength) { WriteBits((uint8_t*)&src,srcBitLength);}

		inline bool ReadBits(float &output,int outputBitLength) {return ReadBits((uint8_t*)&output,outputBitLength);}
		inline void WriteBits(float src,int srcBitLength) { WriteBits((uint8_t*)&src,srcBitLength);}

		inline bool ReadBits(double &output,int outputBitLength) {return ReadBits((uint8_t*)&output,outputBitLength);}
		inline void WriteBits(double src,int srcBitLength) { WriteBits((uint8_t*)&src,srcBitLength);}

	private:
		int WriteBitsOfOneByte(const uint8_t* src, int srcBitLength, int srcBitOffset);
		int ReadBitsOfOneByte(uint8_t* output, int outputBitLength, int outputBitOffset);
		void ThrowBitOperationError(const char* where);
	

};
	
	//// 로컬 변수로 잠깐 쓸 경우 유용=>더 이상 안씀. UseInternalBuffer자체가 obj-pool기능으로 인해 relloc횟수가 점차 줄어드니까.
	//class CSendingMessage: public CMessage
	//{
	//public:
	//	CSendingMessage()
	//	{
	//		UseInternalBuffer();
	//		// 미리 공간을 확 확보한 후 다시 해제해버린다. 이렇게 해서 재할당 횟수를 0으로 만든다.
	//		// 예전에는 Proud::CNetConfig::MessageMaxLength를 넣었으나, 이것을 넣으면 CRT heap을 직접 접근해버리므로 캐삽질이 된다.
	//		// 따라서 고속 할당을 하는 CFastHeap::DefaultAccelBlockSizeLimit를 써야 한다.
	//		SetLength(CFastHeap::DefaultAccelBlockSizeLimit);
	//		SetLength(0);
	//	}
	//};

	// ------- 이건 더 이상 안쓴다. ProudNetConfig::MessageMaxLength를 런타임에서 수정할 수 있어야 하니까.
	// class CSendingMessage: public CMessage 
	// {
	// 	BYTE m_stackBuffer[CNetConfig::MessageMaxLength];
	// 
	// public:
	// 	CSendingMessage()
	// 	{
	// 		UseExternalBuffer(m_stackBuffer, sizeof(m_stackBuffer));
	// 	}
	// };

	// 로컬 변수로 잠깐 쓸 경우 유용
	// 주의: 버퍼 크기가 매우 작으므로 조심해서 쓸 것! 가변 크기 데이터를 넣거나 해서는 곤란!
	// ** PS4&UE4 포팅하던 상황에서, 이것이 고정 크기 배열을 내장했더니, 오류가 발생했었다. 그리고 이 클래스는
	// obj-pool 기능이 없던 시절에 들어갔던 기능이다. 따라서 지금은 이 클래스를 없애도 된다.
	class CSmallStackAllocMessage: public CMessage
	{
	public:
		CSmallStackAllocMessage()
		{
			UseInternalBuffer();
		}
	};

	/* 
	\~korean
	임시적으로 splitter test를 끄고자 할 때 이 객체를 로컬 변수로 둬서 쓰도록 한다.
	- 주로 serialize 함수 내부에서 쓴다.
	- CSendFragRefs에서 필요해서 넣은 클래스 

	\~english
	To temporarily turn off splitter test, this object can be as a local variable.
	- Mainly use in inside of serialize function
	- Class added since required by CSendFragRefs

	\~chinese
	想临时关闭splitter test的时候，要把此对象留为本地变数而使用。
	- 主要在serialize函数内部使用。
	- 在 CSendFragRefs%里需要而输入的类。

	\~japanese
	\~
	*/
	class CMessageTestSplitterTemporaryDisabler
	{
		CMessage* m_msg;
		bool m_oldVal;
	public:
		PROUD_API CMessageTestSplitterTemporaryDisabler(CMessage& msg);
		PROUD_API ~CMessageTestSplitterTemporaryDisabler();
	};
	/**  @} */
	
	template<typename T>
	inline bool CMessage_RawRead(CMessage &a, T &b)
	{
		T t;
		bool r = a.Read((uint8_t*) & t, sizeof(t));
		if (r == false)
			return false;
		b = t;
		return true;
	}
	
#if (_MSC_VER>=1400)
#pragma managed(pop)
#endif
}


//#pragma pack(pop)
