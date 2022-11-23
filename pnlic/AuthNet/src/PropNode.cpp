/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include "../include/variant-marshaler.h"
#if defined(_WIN32)
#include <atlcomcli.h>

#include "../include/PropNode.h"
#include "../include/Exception.h"
#include "../include/Message.h"
#include "FastList2.h"

using namespace std;

namespace Proud
{
#ifdef _DEBUG
#define VERIFY(x) assert(x)
#else
#define VERIFY(x) __noop
#endif

	PROUD_API BOOL GetBinaryFromVariant(CComVariant & ovData, uint8_t ** ppBuf,
		unsigned long * pcBufLen);
	PROUD_API BOOL PutBinaryIntoVariant(CComVariant * ovData, uint8_t * pBuf,
		unsigned long cBufLen);

	CProperty::operator _variant_t() const 
	{
		CMessage pk;
		pk.UseInternalBuffer();
		pk<<*this;
		CComVariant r;
		VERIFY(PutBinaryIntoVariant(&r, (uint8_t*)pk.GetData(), pk.GetLength()));
		return r;
	}

	CProperty::operator ByteArrayPtr() const 
	{
		CMessage pk;
		pk.UseInternalBuffer();
		pk<<*this;
		ByteArrayPtr r;
		r.SetCount(pk.GetLength());
		memcpy(r.GetData(),pk.GetData(),pk.GetLength());
		return r;
	}

	void CProperty::FromVariant( _variant_t from)
	{
		AssertThreadID(eAccessMode_Write);

		m_map.Clear();

		if(from.vt!=VT_EMPTY && from.vt!=VT_NULL)
		{
			CComVariant from2(from);
			uint8_t* buf = NULL; DWORD buflen = 0;
			VERIFY(GetBinaryFromVariant(from2,&buf,&buflen));
			CMessage pk;
			pk.SetLength(buflen);
			memcpy(pk.GetData(),buf,buflen);
			pk>>*this;
			delete buf;
		}

		ClearThreadID();
	}

	void CProperty::FromByteArray(const uint8_t* data, int length)
	{
		AssertThreadID(eAccessMode_Write);

		m_map.Clear();

		if(length>0)
		{
			CMessage pk;
			pk.UseExternalBuffer((uint8_t*)data, length);
			pk.SetLength(length);
			pk>>(*this);
		}

		ClearThreadID();
	}

	void CProperty::FromByteArray( const ByteArray &from )
	{
		FromByteArray(from.GetData(),(int)from.GetCount());
	}
	//////////
	// These functions are copied from MSDN / HOWTO: Accessing Binary Data Using dbDao(Q152294).
	//Extensive error checking is left out to make the code more readable
	BOOL GetBinaryFromVariant(CComVariant & ovData, uint8_t ** ppBuf,
		unsigned long * pcBufLen)
	{
		BOOL fRetVal = FALSE;

		//Binary data is stored in the variant as an array of unsigned char
		if(ovData.vt == (VT_ARRAY|VT_UI1))  // (OLE SAFEARRAY)
		{
			//Retrieve size of array
			*pcBufLen = ovData.parray->rgsabound[0].cElements;

			*ppBuf = new uint8_t[*pcBufLen]; //Allocate a buffer to store the data
			if(*ppBuf != NULL)
			{
				void * pArrayData;

				//Obtain safe pointer to the array
				SafeArrayAccessData(ovData.parray,&pArrayData);

				//Copy the bitmap into our buffer
				memcpy(*ppBuf, pArrayData, *pcBufLen);

				//Unlock the variant data
				SafeArrayUnaccessData(ovData.parray);
				fRetVal = TRUE;
			}
		}
		return fRetVal;
	}

	BOOL PutBinaryIntoVariant(CComVariant * ovData, uint8_t * pBuf,
		unsigned long cBufLen)
	{
		BOOL fRetVal = FALSE;

		VARIANT var;
		VariantInit(&var);  //Initialize our variant

		//Set the type to an array of unsigned chars (OLE SAFEARRAY)
		var.vt = VT_ARRAY | VT_UI1;

		//Set up the bounds structure
		SAFEARRAYBOUND  rgsabound[1];

		rgsabound[0].cElements = cBufLen;
		rgsabound[0].lLbound = 0;

		//Create an OLE SAFEARRAY
		var.parray = SafeArrayCreate(VT_UI1,1,rgsabound);

		if(var.parray != NULL)
		{
			void * pArrayData = NULL;

			//Get a safe pointer to the array
			SafeArrayAccessData(var.parray,&pArrayData);

			//Copy bitmap to it
			memcpy(pArrayData, pBuf, cBufLen);

			//Unlock the variant data
			SafeArrayUnaccessData(var.parray);

			*ovData = var;  // Create a CComVariant based on our variant
			VariantClear(&var);
			fRetVal = TRUE;
		}

		return fRetVal;
	}

	CVariant CProperty::GetField(const String &name)
	{
		AssertThreadID(eAccessMode_Read);

		MapType::const_iterator i = m_map.find(String(name).MakeUpper());
		
		if(i!=m_map.end())
		{
			ClearThreadID();
			return i->GetSecond();
		}

		ClearThreadID();
		return CVariant(VT_NULL);
	}

	void CProperty::SetField(const String &name,const CVariant &value)
	{
		AssertThreadID(eAccessMode_Write);

		const String pszName = CStringPool::Instance().GetString(String(name).MakeUpper());
		MapType::iterator i=m_map.find(pszName);

		if(i==m_map.end())
		{	
			m_map.Add(pszName,value);
			ClearThreadID();
			return;
		}
		
		i->SetSecond(value);
		ClearThreadID();
	}

	void CProperty::RemoveField(const String &name)
	{
		AssertThreadID(eAccessMode_Write);
		m_map.Remove(String(name).MakeUpper());
		ClearThreadID();
	}

	CProperty::CProperty()		
	{
#ifdef _DEBUG
		m_RWAccessChecker = RefCount<CReaderWriterAccessChecker>(new CReaderWriterAccessChecker);
#endif // _DEBUG
	}

	CProperty::CProperty( _variant_t from )
	{
#ifdef _DEBUG
		m_RWAccessChecker = RefCount<CReaderWriterAccessChecker>(new CReaderWriterAccessChecker);
#endif // _DEBUG
		FromVariant(from);
	}

	CProperty::CProperty( const CProperty& rhs )
	{
#ifdef _DEBUG
		m_RWAccessChecker = RefCount<CReaderWriterAccessChecker>(new CReaderWriterAccessChecker);
#endif // _DEBUG
		rhs.AssertThreadID(eAccessMode_Read);
		AssertThreadID(eAccessMode_Write);
		m_map = rhs.m_map;
		ClearThreadID();
		rhs.ClearThreadID();
	}
	void CProperty::ToByteArray( ByteArray &output )
	{
		AssertThreadID(eAccessMode_Read);
		CMessage pk;
		pk.UseInternalBuffer();
		pk<<*this;
		output.SetCount(pk.GetLength());
		memcpy(output.GetData(),pk.GetData(),pk.GetLength());
		ClearThreadID();
	}

	
	Proud::String CProperty::GetDumpedText()
	{
		AssertThreadID(eAccessMode_Read);
		String ret;
		for(const_iterator i=begin();i!=end();i++)
		{
			String fmt;
			String name=i.Key;

			CVariant value=i.Value;

			if(value.m_val.vt==VT_NULL || value.m_val.vt==VT_EMPTY)
				fmt.Format(_PNT("[%s:null]"),name);
			else
				fmt.Format(_PNT("[%s:%s]"),name,(String)value);

			ret+=fmt;
		}
		
		ClearThreadID();
		return ret;
	}

	CProperty::~CProperty()
	{
	}

	void CProperty::AssertThreadID(eAccessMode eMode) const
	{
#ifdef _DEBUG
		m_RWAccessChecker->AssertThreadID(eMode);
#endif // _DEBUG
	}

	void CProperty::ClearThreadID() const
	{
#ifdef _DEBUG
		m_RWAccessChecker->ClearThreadID();
#endif // _DEBUG
	}

	CMessage& operator<<(CMessage& packet, const CProperty &rhs)
	{
		rhs.AssertThreadID(eAccessMode_Read);
		int sz=(int)rhs.GetCount();
		packet<<sz;

		for (CProperty::const_iterator i=rhs.begin();i!=rhs.end();i++)		
		{
			String key = String(i.Key);
			packet << key;
			packet << i.Value;
		}
		rhs.ClearThreadID();
		return packet;
	}

	CMessage& operator>>(CMessage& packet,CProperty &rhs)
	{
		int sz;
		packet>>sz;

		String s;
		CVariant v;
		rhs.AssertThreadID(eAccessMode_Write);
		
		rhs.Clear();
		
		for(int i=0;i<sz;i++)
		{
			packet>>s;
			packet>>v;
			rhs.Add(s,v);
		}

		rhs.ClearThreadID();
		
		return packet;
	}

	

	//////////////////////////////////////////////////////////////////////////
	// CPropNode

	CPropNode::CPropNode(const PNTCHAR* TypeName) :
	CProperty(),
	m_sibling(NULL),
	m_child(NULL),
	m_parent(NULL),
	m_endSibling(NULL),
	m_UUID(),
	m_ownerUUID(),
	m_RootUUID(),
	m_INTERNAL_TypeName(CStringPool::Instance().GetString(TypeName)),
	m_dirtyFlag(false),
	m_issuedSoft(false)
	{

	}

	CPropNode::CPropNode( const CPropNode& from ):
	CProperty()
	{
		AssertThreadID(eAccessMode_Write);
		m_map = from.m_map;
		m_child = from.m_child;
		m_sibling = from.m_sibling;
		m_endSibling = from.m_endSibling;
		m_INTERNAL_TypeName = CStringPool::Instance().GetString(String(from.m_INTERNAL_TypeName).MakeUpper());
		m_UUID = from.m_UUID;
		m_ownerUUID = from.m_ownerUUID;
		m_RootUUID = from.m_RootUUID;
		m_dirtyFlag = from.m_dirtyFlag;
		ClearThreadID();
	}

	CPropNode::~CPropNode()
	{
	
	}

	void CPropNode::ToByteArray( ByteArray &output )
	{
		AssertThreadID(eAccessMode_Read);

		CMessage pk;
		pk.UseInternalBuffer();

		if(NULL == m_sibling)
			pk << Guid();
		else
			pk << m_sibling->m_UUID;

		pk<<*this;

		output.SetCount(pk.GetLength());
		memcpy(output.GetData(),pk.GetData(),pk.GetLength());

		ClearThreadID();
	}

	void CPropNode::FromByteArray(const uint8_t* data, int length)
	{
		Guid trashid;
		FromByteArray(data,length,trashid);
	}

	void CPropNode::FromByteArray(const uint8_t* data, int length, Guid& siblingUUID)
	{
		if(length>0)
		{
			AssertThreadID(eAccessMode_Write);

			CMessage pk;
			pk.UseExternalBuffer((uint8_t*)data, length);
			pk.SetLength(length);

			pk>>siblingUUID;
			pk>>(*this);

			ClearThreadID();
		}
	}

	void CPropNode::FromByteArray( const ByteArray &from )
	{
		FromByteArray(from.GetData(),(int)from.GetCount());
	}
	void CPropNode::FromVariant( _variant_t from )
	{
		__super::FromVariant(from);
	}

	void CPropNode::SetTypeName( const PNTCHAR* str )
	{
		AssertThreadID(eAccessMode_Write);

		m_INTERNAL_TypeName = CStringPool::Instance().GetString(String(str).MakeUpper());

		ClearThreadID();
	}

	void CPropNode::SetStringTypeName( String str )
	{
		AssertThreadID(eAccessMode_Write);

		m_INTERNAL_TypeName = CStringPool::Instance().GetString(str.MakeUpper());

		ClearThreadID();
	}

	void CPropNode::SetField( const String &name,const CVariant &value )
	{
		AssertThreadID(eAccessMode_Write);
		m_dirtyFlag = true; 
		ClearThreadID();
		__super::SetField(name, value);
	}

	void CPropNode::RemoveField( const String &name )
	{
		AssertThreadID(eAccessMode_Write);
		m_dirtyFlag = true;
		ClearThreadID();
		__super::RemoveField(name);
	}

	Proud::CPropNodePtr CPropNode::CloneNoChildren()
	{
		CPropNodePtr clone = CPropNodePtr(new CPropNode(TypeName));

		*clone = *this;

		clone->m_child = NULL;
		clone->m_sibling = NULL;
		clone->m_parent = NULL;

		return clone;
	}

	CMessage& operator<<( CMessage& packet,CPropNode &rhs )
	{	
		rhs.AssertThreadID(eAccessMode_Read);
		packet<<String(rhs.m_INTERNAL_TypeName);
		packet<<rhs.m_UUID;
		packet<<rhs.m_ownerUUID;
		packet<<rhs.m_RootUUID;
		rhs.ClearThreadID();

		packet<<*(static_cast<CProperty*>(&rhs));
		return packet;
	}
	CMessage& operator>>( CMessage& packet,CPropNode &rhs )
	{
		String strName;
		rhs.AssertThreadID(eAccessMode_Write);
		packet >> strName;
		rhs.m_INTERNAL_TypeName = CStringPool::Instance().GetString(String(strName).MakeUpper());
		packet >> rhs.m_UUID;
		packet >> rhs.m_ownerUUID;
		packet >> rhs.m_RootUUID;
		rhs.ClearThreadID();

		packet >> *(static_cast<CProperty*>(&rhs));
		return packet;
	}

}
#endif // _WIN32