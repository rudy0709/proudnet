#include "stdafx.h"
#include "../include/pnguid.h"
#include "../include/Message.h"

namespace Proud
{
	bool Guid::ConvertStringToUUID(String uuidStr, Guid &uuid)
	{
		StringA a = StringT2A(uuidStr);

		a = a.TrimLeft('{');
		a = a.TrimRight('}');
		a.Replace(" ", "");  //사용자가 GUID 문자열에 빈칸 넣는 경우가 허다하다...

		if (a.GetLength() == 0)
		{
			uuid = Guid();
			return false;
		}

		// UuidToString() not used Win32 API for multi platforms
		/*-
		_GUID	{8D2909B7-FF2A-4BA7-B78B-893F6ECE0B49}	_GUID
		Data1	0x8d2909b7	unsigned long
		Data2	0xff2a	unsigned short
		Data3	0x4ba7	unsigned short
		-		Data4	0x0bfff6a4 unsigned char [8]
		[0x0]	0xb7 '?'	unsigned char
		[0x1]	0x8b '?'	unsigned char
		[0x2]	0x89 '?'	unsigned char
		[0x3]	0x3f '?'	unsigned char
		[0x4]	0x6e 'n'	unsigned char
		[0x5]	0xce '?'	unsigned char
		[0x6]	0x0b ''	unsigned char
		[0x7]	0x49 'I'	unsigned char
		*/
		int n = 0;
		int m2, m3;
		int x[8];
		n = _snscanf_s(a.GetBuffer(), a.GetLength(), "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
			&uuid.Data1,
			&m2,
			&m3,
			&x[0],
			&x[1],
			&x[2],
			&x[3],
			&x[4],
			&x[5],
			&x[6],
			&x[7]);
		if (n == 11)
		{
			uuid.Data2 = static_cast<uint16_t>(m2);
			uuid.Data3 = static_cast<uint16_t>(m3);
			// 'hh' prefix is only used to C99, so we have to typecast it with int type.
			for (int i = 0; i < 8; i++)
			{
				uuid.Data4[i] = static_cast<uint8_t>(x[i]);
			}
			return true;
		}
		else
			return false;
	}

	Guid::Guid()
	{
		Data1 = Data2 = Data3 = 0;
		memset(Data4, 0, sizeof(Data4));
	}

	Guid::Guid(PNGUID src)
	{
		*(PNGUID*)this = src;
	}

	Proud::String Guid::ToString() const
	{
		String ret;
		if (Guid::ConvertUUIDToString(*this, ret))
			return ret;
		else
			return _PNT("N/A");
	}

	Proud::String Guid::ToBracketString() const
	{
		String ret;
		if (Guid::ConvertUUIDToBracketString(*this, ret))
			return ret;
		else
			return _PNT("N/A");
	}

	bool Guid::ConvertUUIDToString(const Guid &uuid, String &uuidStr)
	{
		// UuidToString() not used Win32 API for multi platforms
		/*-
		_GUID	{8D2909B7-FF2A-4BA7-B78B-893F6ECE0B49}	_GUID
		Data1	0x8d2909b7	unsigned long
		Data2	0xff2a	unsigned short
		Data3	0x4ba7	unsigned short
		-		Data4	0x0bfff6a4 unsigned char [8]
		[0x0]	0xb7 '?'	unsigned char
		[0x1]	0x8b '?'	unsigned char
		[0x2]	0x89 '?'	unsigned char
		[0x3]	0x3f '?'	unsigned char
		[0x4]	0x6e 'n'	unsigned char
		[0x5]	0xce '?'	unsigned char
		[0x6]	0x0b ''	unsigned char
		[0x7]	0x49 'I'	unsigned char
		*/

		uuidStr.Format(_PNT("%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X"),
			uuid.Data1,
			uuid.Data2,
			uuid.Data3,
			uuid.Data4[0],
			uuid.Data4[1],
			uuid.Data4[2],
			uuid.Data4[3],
			uuid.Data4[4],
			uuid.Data4[5],
			uuid.Data4[6],
			uuid.Data4[7]);
		return true;
	}

	bool Guid::ConvertUUIDToBracketString(const Guid &uuid, String &uuidStr)
	{
		// UuidToString() not used Win32 API for multi platforms
		// {...} string format 
		String v;
		ConvertUUIDToString(uuid, v);
		uuidStr = _PNT("{") + v + _PNT("}");
		return true;
	}

	CMessage& operator>>(CMessage &a, Guid &b)
	{
		a.Read(b);
		return a;
	}

	CMessage& operator<<(CMessage &a, const Guid &b)
	{
		a.Write(b);
		return a;
	}

	void AppendTextOut(String &a, const Guid &b)
	{
		String uuidstr;
		bool r = Guid::ConvertUUIDToString(b, uuidstr);
		if (r)
			a += uuidstr;
		else
			a += _PNT("<BAD>");
	}


}