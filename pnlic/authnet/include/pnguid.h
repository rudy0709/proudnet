#pragma once 

#include "BasicTypes.h"
#include "PNString.h"

namespace Proud
{
	class CMessage;

	struct PNGUID
	{
		uint32_t Data1;
		uint16_t Data2;
		uint16_t Data3;
		uint8_t Data4[8];
	};

	/** Platform independent Global Unique ID object. 
	For Win32 features, refer to Proud::Win32Guid class.
	*/
	class Guid:public PNGUID
	{
	public:
		/**
		\~korean
		GUID를 랜덤으로 생성합니다. NewGuid()와 달리, 이 값은 고유성이 보장되지 않습니다.

		\~english
		Generates a 'random' GUID. Unlike NewGuid(), this does not guarantee uniqueness.

		\~chinese
		随机生成GUIDE。与NewGuid（）不同，此值不保障固有性。

		\~japanese
		\~
		*/
		PROUD_API static Guid RandomGuid();

		Guid();
		Guid(PNGUID src);

		/**
		\~korean
		Guid를 String값으로 변환해 줍니다.

		\~english TODO:translate needed.

		\~chinese
		把Guid转换成String值。

		\~japanese
		\~
		*/
		PROUD_API String ToString() const;

		/**
		\~korean
		Guid를 Bracket({}) 를 포함한 String으로 변환해 줍니다.

		\~english TODO:translate needed.

		\~chinese
		把Guid转换成包括Bracket({})的String。

		\~japanese
		\~
		*/
		PROUD_API String ToBracketString() const;

		/**
		\~korean
		Guid를 String값으로 변환해 줍니다.
		\param uuid Guid입니다.
		\param uuidStr uuid를 String형태로 변환한 정보를 담을 reference 입니다
		\return 변환에 성공 true, 실패 false

		\~english TODO:translate needed.

		\~chinese
		把Guid转换成String值。
		\param uuid 是Guid。
		\param uuidStr 是包含着把uuid转换成String形式信息的reference。
		\return 转换成功的话true，失败的话false。

		\~japanese
		\~
		*/
		PROUD_API static bool  ConvertUUIDToString(const Guid &uuid, String &uuidStr);

		/**
		\~korean
		Guid를 Bracket({}) 를 포함한 String으로 변환해 줍니다.
		\param uuid Guid입니다.
		\param uuidStr uuid를 String형태로 변환한 정보를 담을 reference 입니다
		\return 변환에 성공 true, 실패 false

		\~english TODO:translate needed.

		\~chinese
		把Guid转换成包括Bracket({}) 的String。
		\param uuid 是Guid。
		\param uuidStr 是包含着把uuid转换成String形式信息的reference。
		\return 转换成功的话true，失败的话false。

		\~japanese
		\~
		*/
		PROUD_API static bool  ConvertUUIDToBracketString(const Guid &uuid, String &uuidStr);

		/**
		\~korean
		String을 uuid값으로 변환합니다.
		\param uuidStr uuid의 String형태 입니다.
		\param uuidStr String에서 uuid로 변환한 정보를 담을 reference 입니다
		\return 변환에 성공 true, 실패 false

		\~english TODO:translate needed.

		\~chinese
		把String转换成uuid值。
		\param uuidStr 是uuid的String形式。
		\param uuidStr 是包含着从String转换成uuid信息的reference。
		\return 转换成功的话true，失败的话false。

		\~japanese
		\~
		*/
		PROUD_API static bool  ConvertStringToUUID(String uuidStr, Guid &uuid);

		/**
		\~korean
		Guid를 String값으로 변환해 줍니다.
		\param uuid Guid입니다.
		\return String

		\~english TODO:translate needed.

		\~chinese
		把Guid转换成String值。
		\param uuid 是Guid。
		\return String

		\~japanese
		\~
		*/
		static inline String GetString(const Guid &uuid)
		{
			String ret;
			if (ConvertUUIDToString(uuid, ret))
				return ret;
			else
				return String();
		}

		String GetString() const
		{
			return Guid::GetString(*this);
		}

		/**
		\~korean
		Guid를 Bracket({}) 를 포함한 String으로 변환해 줍니다.
		\param uuid Guid입니다.
		\return String

		\~english TODO:translate needed.

		\~chinese
		把Guid转换成包括Bracket({})的String。
		\param uuid 是Guid。
		\return String

		\~japanese
		\~
		*/
		static inline String GetBracketString(const Guid &uuid)
		{
			String ret;
			if (ConvertUUIDToBracketString(uuid, ret))
				return ret;
			else
				return String();
		}

		/**
		\~korean
		String을 uuid값으로 변환합니다.
		\param uuidStr uuid의 String형태 입니다.
		\return Guid 변환된 Guid 입니다.

		\~english TODO:translate needed.

		\~chinese
		把String转换成uuid值。
		\param uuidStr 是uuid的String形式。
		\return Guid被转换的Guid。

		\~japanese
		\~
		*/
		static inline Guid GetFromString(const PNTCHAR* uuidStr)
		{
			Guid uuid;
			if (ConvertStringToUUID(uuidStr, uuid))
				return uuid;
			else
				return Guid();
		}

	};

	inline bool operator<(const Guid& d1, const Guid& d2)
	{
		return memcmp(&d1, &d2, sizeof(Guid)) < 0;
	}

	inline bool operator<=(const Guid& d1, const Guid& d2)
	{
		return memcmp(&d1, &d2, sizeof(Guid)) <= 0;
	}

	inline bool operator>(const Guid& d1, const Guid& d2)
	{
		return memcmp(&d1, &d2, sizeof(Guid)) > 0;
	}

	inline bool operator>=(const Guid& d1, const Guid& d2)
	{
		return memcmp(&d1, &d2, sizeof(Guid)) >= 0;
	}

	inline bool operator==(const Guid& d1, const Guid& d2)
	{
		return memcmp(&d1, &d2, sizeof(Guid)) == 0;
	}

	inline bool operator!=(const Guid& d1, const Guid& d2)
	{
		return memcmp(&d1, &d2, sizeof(Guid)) != 0;
	}

	CMessage& operator>>(CMessage &a, Guid &b);
	CMessage& operator<<(CMessage &a, const Guid &b);
	void AppendTextOut(String &a, const Guid &b);


};

/**
\~korean
CFastMap 등에 쓰려면 이것이 필요하다.

\~english
This is needed to use CFastMap or others.

\~chinese
想用于 CFasTMap%等的话需要这个。

\~japanese
\~
*/
template<>
class CPNElementTraits < Proud::Guid >
{
public:
	typedef const Proud::Guid& INARGTYPE;
	typedef Proud::Guid& OUTARGTYPE;

	inline static uint32_t Hash(INARGTYPE element)
	{
		uint32_t ret = 0;
		uint32_t* data = (uint32_t*)&element;
		for (int i = 0; i < 4; i++)
		{
			ret ^= data[i];
		}

		return ret;
	}

	inline static bool CompareElements(INARGTYPE element1, INARGTYPE element2)
	{
		return (element1 == element2) ? true : false;
	}

	inline static int CompareElementsOrdered(INARGTYPE element1, INARGTYPE element2)
	{
		if (element1 < element2)
			return -1;
		else if (element1 > element2)
			return 1;
		else
			return 0;
	}
};

