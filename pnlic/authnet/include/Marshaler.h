﻿/*
ProudNet v1


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

#include <string>
#include "FakeClr.h"
#include "Message.h" // 이게 어쨌거나 필요. AppendTextOut 때문
#include "AddrPort.h"
#include "Vector3D.h"
#include "Enums.h"
#include "ErrorInfo.h"
#include "FastList.h"
#include "pnguid.h"

// NOTE: 여기서 stl관련 marshaler나 stl을 include하면 안된다. ProudNet은 어떤 버전의 stl을 사용자가 쓰는지에 의존하면 안되니까.
// 이것은 모든 C++ library의 특징이기도 하다. (이거 무시하다가 컴파일 에러 지옥을 경험하게 됨.)

namespace Proud
{
	/** \addtogroup net_group
	*  @{
	*/

	PROUD_API void ThrowExceptionOnReadString(int length);
	PROUD_API void ThrowExceptionOnReadArray(int64_t length);

	inline CMessage& operator<<(CMessage &a, const StringA &str)
	{
		a.WriteString(str);
		return a;
	}
	inline CMessage& operator>>(CMessage &a, StringA &str)
	{
		a.ReadString(str);
		return a;
	}
	inline CMessage& operator<<(CMessage &a, const StringW &str)
	{
		a.WriteString(str);
		return a;
	}
	inline CMessage& operator>>(CMessage &a, StringW &str)
	{
		a.ReadString(str);
		return a;
	}
	inline CMessage& operator<<(CMessage &a, const std::string &str)
	{
		a.WriteString(str);
		return a;
	}
	inline CMessage& operator>>(CMessage &a, std::string &str)
	{
		a.ReadString(str);
		return a;
	}
	inline CMessage& operator<<(CMessage &a, const std::wstring &str)
	{
		a.WriteString(str);
		return a;
	}
	inline CMessage& operator>>(CMessage &a, std::wstring &str)
	{
		a.ReadString(str);
		return a;
	}
	inline CMessage& operator<<(CMessage &a, bool b) {
		a.Write(b);
		return a;
	}
	inline CMessage& operator<<(CMessage &a, char b) {
		a.Write(b);
		return a;
	}
	inline CMessage& operator<<(CMessage &a, uint8_t b) {
		a.Write(b);
		return a;
	}
	inline CMessage& operator<<(CMessage &a, short b) {
		a.Write(b);
		return a;
	}
	inline CMessage& operator<<(CMessage &a, uint16_t b) {
		a.Write(b);
		return a;
	}
	inline CMessage& operator<<(CMessage &a, int b) {
		a.Write(b);
		return a;
	}
	inline CMessage& operator<<(CMessage &a, unsigned b) {
		a.Write(b);
		return a;
	}
	inline CMessage& operator<<(CMessage &a, uint64_t b) {
		a.Write(b);
		return a;
	}
	inline CMessage& operator<<(CMessage &a, int64_t b) {
		a.Write(b);
		return a;
	}
	inline CMessage& operator<<(CMessage &a, float b) {
		a.Write(b);
		return a;
	}
	inline CMessage& operator<<(CMessage &a, double b) {
		a.Write(b);
		return a;
	}
	inline CMessage& operator<<(CMessage &a, HostID b) {
		a.Write(b);
		return a;
	}
	//void Write(CMessage &a,Size^ b) { a.Write(b); return a; }
	inline CMessage& operator>>(CMessage &a, bool &b) {
		a.Read(b);
		return a;
	}
	inline CMessage& operator>>(CMessage &a, int8_t &b) {
		a.Read(b);
		return a;
	}
	inline CMessage& operator>>(CMessage &a, uint8_t &b) {
		a.Read(b);
		return a;
	}
	inline CMessage& operator>>(CMessage &a, int16_t &b) {
		a.Read(b);
		return a;
	}
	inline CMessage& operator>>(CMessage &a, uint16_t &b) {
		a.Read(b);
		return a;
	}
	inline CMessage& operator>>(CMessage &a, int32_t &b) {
		a.Read(b);
		return a;
	}
	inline CMessage& operator>>(CMessage &a, uint32_t &b) {
		a.Read(b);
		return a;
	}
    inline CMessage& operator>>(CMessage &a, uint64_t  &b) {
		a.Read(b);
		return a;
	}
	inline CMessage& operator>>(CMessage &a, int64_t  &b) {
		a.Read(b);
		return a;
	}
	inline CMessage& operator>>(CMessage &a, float &b) {
		a.Read(b);
		return a;
	}
	inline CMessage& operator>>(CMessage &a, double &b) {
		a.Read(b);
		return a;
	}
	inline CMessage& operator>>(CMessage &a, HostID &b) {
		a.Read(b);
		return a;
	}

	//inline CMessage& operator>>(CMessage &a,DateTime b)
	//{
	//    a.Read(b);
	//}

	//inline CMessage& operator<<(CMessage &a,DateTime b)
	//{
	//    a.Write(b);
	//}

	inline CMessage& operator>>(CMessage &a, AddrPort &b)
	{
		a.Read(b.m_binaryAddress);
		a.Read(b.m_port);
		return a;
	}
	inline CMessage& operator<<(CMessage &a, AddrPort b)
	{
		a.Write(b.m_binaryAddress);
		a.Write(b.m_port);
		return a;
	}

	inline CMessage& operator>>(CMessage &a, RelayDestList &b)
	{
		a.Read(b);
		return a;
	}

	inline CMessage& operator<<(CMessage &a, const RelayDestList &b)
	{
		a.Write(b);
		return a;
	}

	inline CMessage& operator>>(CMessage &a, ByteArray	&b)
	{
		a.Read(b);
		return a;
	}

	inline CMessage& operator<<(CMessage &a, const ByteArray &b)
	{
		a.Write(b);
		return a;
	}

	inline CMessage& operator>>(CMessage &a, HostIDArray &b)
	{
		a.Read(b);
		return a;
	}

	inline CMessage& operator<<(CMessage &a, const HostIDArray &b)
	{
		a.Write(b);
		return a;
	}

	// vector에서 사용할 수 있는 serialization functions
	// for output to stream
	template<typename elem, bool elem_in_ref, bool RAWTYPE, typename INDEXTYPE>
	inline CMessage& operator>>(CMessage &a, CFastArray<elem, elem_in_ref, RAWTYPE, INDEXTYPE> &b)
	{
		// 배열 크기를 얻는다.
		int64_t length;
		a.ReadScalar(length);

		// length가 해킹되어 말도 안되는 값이면 실패 처리하기
		// 물론 모든 경우를 잡지는 못하지만 (sizeof elem 무용지물) 그래도 최소 1바이트는 쓸테니.
		// 원소의 개수가 바이트 개수보다 많은 경우는 무시.
		if (length< 0 || length > a.GetLength() - a.GetReadOffset())
			ThrowExceptionOnReadArray(length);

		// 배열 크기 리셋 및 capacity set
		b.SetCount((INDEXTYPE)length);

		// 각 배열 항목을 읽는다.
		for ( INDEXTYPE i = 0; i < (INDEXTYPE)length; i++ )
		{
			a >> b[i];
		}
		return a;
	}

	// vector, list등 unary item elem 등에서 사용할 수 있는 serialization functions
	// for input from stream
	template<typename elem, bool elem_in_ref, bool RAWTYPE, typename INDEXTYPE>
	inline CMessage& operator<<(CMessage &a, const CFastArray<elem, elem_in_ref, RAWTYPE, INDEXTYPE> &b)
	{
		// 배열 크기를 기록한다.
		int64_t size = (int64_t)b.GetCount();
		a.WriteScalar(size);

		// 각 배열 인자를 기록한다.
		for ( INDEXTYPE i = 0; i < (INDEXTYPE)size; i++ )
		{
			a << b[i];
		}
		return a;
	}

	CMessage& operator<<(CMessage &a, const ByteArray &b);
	CMessage& operator>>(CMessage &a, ByteArray &b);
	CMessage& operator<<(CMessage& a, const NamedAddrPort &b);
	CMessage& operator>>(CMessage& a, NamedAddrPort &b);

	PROUD_API void AppendTextOut(String &a, const bool &b);
	PROUD_API void AppendTextOut(String &a, const char &b);
	PROUD_API void AppendTextOut(String &a, const unsigned char &b);
	PROUD_API void AppendTextOut(String &a, const short &b);
	PROUD_API void AppendTextOut(String &a, const unsigned short& b);
	PROUD_API void AppendTextOut(String &a, const int &b);
	PROUD_API void AppendTextOut(String &a, const unsigned int& b);
	PROUD_API void AppendTextOut(String &a, const int64_t &b);
	PROUD_API void AppendTextOut(String &a, const uint64_t &b);
	PROUD_API void AppendTextOut(String &a, const float  &b);
	PROUD_API void AppendTextOut(String &a, const double &b);
	PROUD_API void AppendTextOut(String &a, const char *&b);
	PROUD_API void AppendTextOut(String &a, const wchar_t *&b);
	PROUD_API void AppendTextOut(String &a, const StringA &b);
	PROUD_API void AppendTextOut(String &a, const StringW &b);
#if defined(_WIN32)
	PROUD_API void AppendTextOut(String &a, const std::string & b);
	PROUD_API void AppendTextOut(String &a, const std::wstring & b);
	PROUD_API void AppendTextOut(String &a, const POINT &b);
#endif



	template<typename elem>
	inline void AppendTextOut(String &a, CFastArray<elem> &b)
	{
		Proud::String x;
		x.Format(_PNT("<Array %d>"), b.GetCount());
		a += x;
	}

	PROUD_API void AppendTextOut(String &a, ByteArray &b);
	PROUD_API void AppendTextOut(String &a,HostIDArray &b);

	inline CMessage& operator>>(CMessage &a, LogCategory &b)
	{
		uint8_t x;
		a >> x;
		b = (LogCategory)x;

		return a;
	}

	inline CMessage& operator<<(CMessage &a, const LogCategory &b)
	{
		uint8_t x;
		x = (uint8_t)b;
		a << x;

		return a;
	}

	inline void AppendTextOut(String &a, const LogCategory &b)
	{
		a += ToString(b);
	}

	inline CMessage& operator>>(CMessage &a, ErrorType &b)
	{
		uint32_t x;
		a >> x;
		b = (ErrorType)x;

		return a;
	}

	inline CMessage& operator<<(CMessage &a, const ErrorType &b)
	{
		uint32_t x;
		x = (uint32_t)b;
		a << x;

		return a;
	}

	inline void AppendTextOut(String &a, const ErrorType &b)
	{
		a += ErrorInfo::TypeToString(b);
	}

	template <typename FloatType>
	inline CMessage& operator>>(CMessage &a, Vector3T<FloatType> &b)
	{
		a.Read(b.x);
		a.Read(b.y);
		a.Read(b.z);
		return a;
	}

	template <typename FloatType>
	inline CMessage& operator<<(CMessage &a, const Vector3T<FloatType> &b)
	{
		a.Write(b.x);
		a.Write(b.y);
		a.Write(b.z);
		return a;
	}

	inline void AppendTextOut(String &a, const FloatVector3 &b)
	{
		String f;
		f.Format(_PNT("{x=%f,y=%f,z=%f}"), b.x, b.y, b.z);
		a += f;
	}

	inline void AppendTextOut(String &a, const Vector3 &b)
	{
		String f;
		f.Format(_PNT("{x=%lf,y=%lf,z=%lf}"), b.x, b.y, b.z);
		a+=f;
	}

	template<typename K, typename V>
	__forceinline CMessage& operator>>(CMessage &a, CFastMap<K,V> &b)
	{
		int64_t length;
		a.ReadScalar(length);

		// length가 해킹되어 말도 안되는 값이면 실패 처리하기
		// 물론 모든 경우를 잡지는 못하지만 (sizeof elem 무용지물) 그래도 최소 1바이트는 쓸테니.
		// 원소의 개수가 바이트 개수보다 많은 경우는 무시.
		if (length<0 || length > a.GetLength() - a.GetReadOffset())
             ThrowExceptionOnReadArray(length);

		b.Clear();

		for(int64_t i=0;i<length;i++)
		{
			K key; V val;
			a>>key;
			a>>val;
			b.Add(key,val);
		}
		return a;
	}

	template<typename K, typename V>
	__forceinline CMessage& operator<<(CMessage &a, const CFastMap<K,V> &b)
	{
		int64_t length = (int64_t)b.GetCount();
		a.WriteScalar(length);

        typedef typename CFastMap<K,V>::const_iterator ConstIter;

		for(ConstIter i=b.begin();i!=b.end();i++)
		{
			a<<i.GetFirst();
			a<<i.GetSecond();
		}
		return a;
	}

	template<typename K, typename V>
	__forceinline void AppendTextOut(String &a,const CFastMap<K,V> &b)
	{
		String temp;
		temp.Format(_PNT("FastMap(Size=%d){"), b.GetCount());
		a += temp;

         typedef typename CFastMap<K,V>::const_iterator ConstIter;

		for(ConstIter i=b.begin();i!=b.end();i++)
		{
			a+=_PNT("(");
			AppendTextOut(a, i->GetFirst());
			a+=_PNT(",");
			AppendTextOut(a, i->GetSecond());
			a+=_PNT(")");
		}
		a+=_PNT("}");
	}

    template<typename elem>
    __forceinline CMessage& operator>>(CMessage &a, CFastList<elem> &b)
    {
		int64_t length;
		a.ReadScalar(length);

		// length가 해킹되어 말도 안되는 값이면 실패 처리하기
		// 물론 모든 경우를 잡지는 못하지만 (sizeof elem 무용지물) 그래도 최소 1바이트는 쓸테니.
		// 원소의 개수가 바이트 개수보다 많은 경우는 무시.
		if (length<0 || length > a.GetLength() - a.GetReadOffset())
             ThrowExceptionOnReadArray(length);

		b.Clear();

        for(int64_t i=0;i<length;i++)
        {
            elem e;
            a>>e;
            b.AddTail(e);
        }
        return a;
    }

    template<typename elem>
    __forceinline CMessage& operator<<(CMessage &a, const CFastList<elem> &b)
    {
		int64_t length = (int64_t)b.GetCount();
		a.WriteScalar(length);

        typedef typename CFastList<elem>::const_iterator ConstIter;

        for(ConstIter i=b.begin();i!=b.end();i++)
        {
            a<< (*i);
        }
        return a;
    }

    template<typename elem>
    __forceinline void AppendTextOut(String &a,const CFastList<elem> &b)
    {
        Proud::String x;
		x.Format(_PNT("<FastList %d>"), b.GetCount());
        a += x;
    }

	/**
	\~korean
	사용자가 만든 marshaler 메서드가 정상적으로 작동하는지 테스트합니다.
	디버깅 용도로 사용하세요.

	\param testee 사용자가 만든 변수 타입의 객체입니다. 최대한 복잡하게 값을 넣어서 테스트하는 것을 권합니다.
	이 객체 타입은 Equals() 메서드를 멤버로 갖고 있어야 합니다. Equals()메서드는 두 값을 비교해서 같으면 true를 리턴해야
	합니다.
	\return 오류가 없으면 빈 문자열을, 있는 경우 오류 내용을 담아서 리턴합니다.

	사용 예
	\code
		MyType a;
		a = ...; // 뭔가를 담는다.
		String err = TestMarshaler(a);
		if(!err.IsEmpty())
		show_error(err);
	\endcode

	\~english
	It tests marshaler method that created by user.
	Please use it for debugging.

	\param testee Variable type object that created by user. We recommend to put complex value for testing.
	This object time has to have Equals() method as member. It has return ture when Equals() method compare two value and if it is the same.
	\return If there in no error, it will send an empty string. If there is error then return information of error.
	Example
	\code
		MyType a;
		a = ...; // Put something in
		String err = TestMarshaler(a);
		if(!err.IsEmpty())
		show_error(err);
	\endcode

	\~chinese
	测试用户创建的marshaler方法是否正常运行。
	请使用于调试用途。

	\param testee 用户制造的变数类型对象。建议输入最大程度复杂的值进行测试。
	此对象类型要把Equals()方法拥有成成员。Equals()方法比较两个值，一样的话要返回true。
	\return 没有错误的话返回空字符串，有的话记载错误内容并返回。

	使用例
	\code
		MyType a;
		a = ...; // 记任何东西
		String err = TestMarshaler(a);
		if(!err.IsEmpty())
		show_error(err);

	\~japanese
	\~
	*/
	template<typename elem>
	String TestMarshaler(const elem& testee)
	{
		CMessage msg;
		msg.UseInternalBuffer();
		msg << testee;

		int msgLength = msg.GetLength();
		msg.SetReadOffset(0);

		elem readee;
		msg >> readee;

		String errText;

		if(msg.GetLength() != msg.GetReadOffset())
		{
			errText.Format(_PNT("An object is serialized into %d bytes of data, but %d bytes are used for deserialization. Its marshaler methods may run incorrectly."), msgLength, msg.GetReadOffset());
			return errText;
		}

		if(!testee.Equals(readee))
		{
			errText = _PNT("The serialzed and deserialized object is not the same to the original object! Its marshaler methods may run incorrectly.");
		}

		return errText;
	}

	/**  @} */
}

#define PROUDNET_SERIALIZE_ENUM(type) \
namespace Proud \
{ \
	inline CMessage& operator<<(CMessage& a,type b) \
	{ \
		a<<(int)b; \
		return a; \
	} \
	inline CMessage& operator>>(CMessage& a,type& b) \
	{ \
		int x; \
		a>>x; \
		b=(type)x; \
		return a; \
	} \
	inline void AppendTextOut(String &a,type b) \
	{ \
		String txt; \
		txt.Format(_PNT("%d"),(int)b); \
		a+=txt; \
	} \
}

#if defined(_WIN32) && defined(ATLASSERT)
    #include "MfcDependent.inl"
#endif
