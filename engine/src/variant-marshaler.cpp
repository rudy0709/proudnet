#include "stdafx.h"
#include "../include/variant-marshaler.h"
#include "../include/Message.h"

namespace Proud
{
#if defined(_WIN32)
	CMessage& operator<<(CMessage &a, const CVariant &b)
	{
		a.Write((uint16_t)b.m_val.vt);

		switch (b.m_val.vt)
		{
		case VT_NULL:
			// none to write
			break;
		case VT_I1:
		case VT_I2:
		case VT_I4:
		case VT_I8:
		case VT_INT:
			a.WriteScalar((int64_t)b);
			break;
		case VT_R4:
			a.Write((float)b);
			break;
		case VT_R8:
		case VT_DATE:
			a.Write((double)b);
			break;
		case VT_CY:
		{
			CY val = (CY)b.m_val;
			a.Write(val.int64);
		}
		break;
		case VT_UI1:
		case VT_UI2:
		case VT_UI4:
		case VT_UI8:
		case VT_UINT:
			a.WriteScalar((uint64_t)b);
			break;
		case VT_DECIMAL:
		{
			// ikpil.choi 2017-01-18 : VT_DECIMAL 타입의 마쉘링 추가 (칼럼 bigint 타입 지원) (N3738)
			a.Write_POD(&b.m_val.decVal);
		}
		break;
		case VT_BOOL:
			a.Write((BOOL)b);
			break;
		case VT_BSTR:
		case VT_LPWSTR:
		case VT_LPSTR:
		{
			String val = b;
			a.WriteString(val);
		}
		break;
		case VT_HRESULT:
			a.Write((int32_t)b);
			break;
		case VT_SAFEARRAY:
		case VT_ARRAY | VT_UI1:
		{
			ByteArray array;
			b.ToByteArray(array);
			a.Write(array);
		}
		break;
		case VT_CLSID:
		{
			a.Write((Guid)b);
		}
		break;
		default:
			throw Exception(StringA::NewFormat("Cannot serialize Variant type %d!", b.m_val.vt).GetString());
			break;
		}

		//to develop later...
		/*VT_DISPATCH	= 9,
		VT_ERROR	= 10,
		VT_VARIANT	= 12,
		VT_UNKNOWN	= 13,

		VT_VOID	= 24,
		VT_PTR	= 26,

		VT_USERDEFINED	= 29,
		VT_RECORD	= 36,
		VT_INT_PTR	= 37,
		VT_UINT_PTR	= 38,
		VT_FILETIME	= 64,
		VT_BLOB	= 65,
		VT_STREAM	= 66,
		VT_STORAGE	= 67,
		VT_STREAMED_OBJECT	= 68,
		VT_STORED_OBJECT	= 69,
		VT_BLOB_OBJECT	= 70,
		VT_CF	= 71,
		VT_VERSIONED_STREAM	= 73, */

		return a;

	}

	CMessage& operator>>(CMessage &a, CVariant &b)
	{
		//_variant_t var;
		uint16_t vt;
		a.Read(vt);
		//var.vt = vt;

		switch (vt)
		{
		case VT_NULL:
			b = CVariant(); // 이렇게 비워주어야 아래 ChangeType에서 안 망가진다.
			break;
		case VT_I1:
		case VT_I2:
		case VT_I4:
		case VT_I8:
		case VT_INT:
		{
			int64_t val;
			a.ReadScalar(val);

			b = val;
		}
		break;
		case VT_R4:
		{
			float val;
			a.Read(val);

			b = val;
		}
		break;
		case VT_R8:
		case VT_DATE:
		{
			double val;
			a.Read(val);

			b = val;
		}
		break;
		case VT_CY:
		{
			CY val;
			a.Read(val.int64);

			b.m_val = val;
		}
		break;
		case VT_UI1:
		case VT_UI2:
		case VT_UI4:
		case VT_UI8:
		case VT_UINT:
		{
			uint64_t val;
			a.ReadScalar(val);

			b = val;
		}
		break;
		case VT_DECIMAL:
		{
			// ikpil.choi 2017-01-18 : VT_DECIMAL 타입의 마쉘링 추가 (칼럼 bigint 타입 지원) (N3738)
			_variant_t v;
			a.Read_POD(&v.decVal);
			b.m_val = v;
		}
		break;
		case VT_BOOL:
		{
			BOOL val;
			a.Read(val);

			b = val;
		}
		break;
		case VT_BSTR:
		case VT_LPWSTR:
		case VT_LPSTR:
		{
			String val;
			a.ReadString(val);
			b = CVariant(val);
		}
		break;
		case VT_HRESULT:
		{
			int32_t val;
			a.Read(val);

			b.m_val = (HRESULT)val;
		}
		break;
		case VT_SAFEARRAY:
		case VT_ARRAY | VT_UI1:
		{
			ByteArray array;
			a.Read(array);

			b.FromByteArray(array);
		}
		break;
		case VT_CLSID:
		{
			Guid val;
			a.Read(val);

			b = val;
		}
		break;
		default:
			throw Exception(StringA::NewFormat("Cannot serialize Variant type %d!", vt).GetString());
			break;
		}

		b.m_val.ChangeType(vt);
		return a;
	}

	void AppendTextOut(String &a, const CVariant &b)
	{
		String f;
		f.Format(_PNT("Variant Type:%d"), b.m_val.vt);
		a += f;
	}
#endif // defined(_WIN32)
}
