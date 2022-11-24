/*
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

#include "FastArrayPtr.h"
#include "ByteArray.h"

#if defined(_MSC_VER)
#pragma warning(disable:4290) // 이 경고는 VC++에서만 발생하니 막자
#endif

namespace Proud
{

	/** \addtogroup util_group
	*  @{
	*/

	/**
	\~korean
	ByteArray 에 smart pointer 기능이 추가된 형태. 네트워크 패킷 형태로 쓰이므로 32-64 변환에도 민감. 따라서 intPtr이 아닌 int32 타입이다.
	ByteArray 가 typedef가 아닌 상속 클래스이기 때문에 필요한 메서드를 이 객체도 오버라이드해야
	빌드 에러를 피한다.

	\~english
	It is the form that smart pointer function has been added to ByteArray. It is very sensitive to conversion from 32 to 64 because of being used as the form of network packet, so its form is int32 (not intPtr).
	To avoid build error, this object has to override require method because ByreArray is inherit class instead of typedef.

	\~chinese
	在 ByteArray%添加了smartpointer技能的形态。因其也用于网络数据包的形态所以对32-64变换较为敏感。因此其不是inPtr而是int32类型。
	因为 ByteArray%不是typedef而是接续类，因此将必要的方法超越此客体才能避开Build错误。

	\~japanese
	\~
	 */
	class ByteArrayPtr
	{
	public:
		class Tombstone
		{
		public:
			ByteArray m_objByteArray;
			PROUDNET_VOLATILE_ALIGN int32_t m_refCount;

			inline Tombstone():m_objByteArray()
			{
				m_refCount = 0;
			}

			DECLARE_NEW_AND_DELETE
		};
	private:
		Tombstone* m_tombstone;
		CArrayWithExternalBuffer<uint8_t, false, true, int> m_externalBuffer;

		static Tombstone* NewTombstone();
		static void FreeTombstone(Tombstone* tombstone);
	public:

		/**
		\~korean
		기본 생성자

		\~english
		Default constructor

		\~chinese
		基本生成者。

		\~japanese
		\~
		 */
		inline ByteArrayPtr()
		{
			m_tombstone = NULL;
		}

		/**
		\~korean
		복사 생성자
		- src의 성향도 복사해온다.

		\~english
		Copy constructor
		- Also copies the nature of src

		\~chinese
		复制生成者。
		也复制-src 的倾向。

		\~japanese
		\~
		 */
		inline ByteArrayPtr(const ByteArrayPtr& src)
		{
			if(src.m_tombstone)
			{
				AtomicIncrement32(&src.m_tombstone->m_refCount);

				m_tombstone = src.m_tombstone;
			}
			else
			{
				m_tombstone = NULL;
				src.m_externalBuffer.ShareTo(m_externalBuffer);
			}
		}

		inline ByteArrayPtr& operator=(const ByteArrayPtr& src)
		{
			if(src.m_tombstone != m_tombstone)
			{
				UninitInternalBuffer();

				if(src.m_tombstone != NULL)
				{
					AtomicIncrement32(&src.m_tombstone->m_refCount);

					m_tombstone = src.m_tombstone;
				}
			}
			else
				src.m_externalBuffer.ShareTo(m_externalBuffer);

			return *this;
		}
	private:
		void UninitInternalBuffer();
	public:
		~ByteArrayPtr()
		{
			UninitInternalBuffer();
		}

		inline int GetCapacity() const
		{
			if (m_externalBuffer.IsNull() == false)
				return m_externalBuffer.GetCapacity();
			else if(m_tombstone)
				return m_tombstone->m_objByteArray.GetCapacity();
			else
			{
				ThrowArrayIsNullError();
			}

			return -1;
		}

		inline int GetCount() const
		{
			if(m_tombstone)
				return m_tombstone->m_objByteArray.GetCount();
			else if(m_externalBuffer.IsNull())
				ThrowArrayIsNullError();

				return m_externalBuffer.GetCount();
		}

#if defined(_MSC_VER)
		__declspec(property(get = GetCount)) int Count;
#endif

		inline void MustNotNull() const
		{
			if (IsNull())
			{
				ThrowArrayIsNullError();
			}
		}

		inline void MustNull() const /*throw(Exception)*/
		{
			if (!IsNull())
				ThrowException(ArrayPtrIsNotNullError);
		}

		/**
		\~korean
		\param length 변경할 Capacity 사이즈

		\~english TODO:translate needed.
		\param length

		\~chinese
		\param length 要变换的Capacity大小。

		\~japanese
		\~
		*/
		inline void SetCapacity(int length)
		{
			if(length < 0)
				ThrowInvalidArgumentException();

			if (!m_externalBuffer.IsNull())
			{
				// 아무것도 안함
			}
			else
			{
				if (m_tombstone != NULL)
					m_tombstone->m_objByteArray.SetCapacity(length);
			}
		}

		/**
		\~korean
		배열의 사이즈를 setting 한다.
		\param length 변경할 배열의 사이즈

		\~english TODO:translate needed.

		\~chinese
		Setting 排列的大小。
		\param length 要变换的排列之大小。

		\~japanese
		\~
		*/
		inline void SetCount(int length)
		{
			if(length < 0)
				ThrowInvalidArgumentException();

			if (!m_externalBuffer.IsNull())
				m_externalBuffer.SetCount(length);
			else
			{
				if(length > 0)
				{
					if(m_tombstone)
						m_tombstone->m_objByteArray.SetCount(length);
					else
					{
						ThrowArrayIsNullError();
					}
				}
				else
				{
					if(m_tombstone)
						m_tombstone->m_objByteArray.SetCount(0);
				}
			}
		}

		/**
		\~korean
		1 BYTE 단위의 Data를 추가합니다.
		\param data 추가할 BYTE

		\~english TODO:translate needed.

		\~chinese
		添加1BYTE单位的Data。
		\param data 要添加的BYTE。

		\~japanese
		\~
		*/
		inline void Add(uint8_t data)
		{
			AddRange(&data, 1);
		}

		/**
		\~korean
		현재의 배열에 BYTE배열 data를 추가 합니다.
		\param data 추가할 BYTE배열 포인터
		\param count 배열의 크기

		\~english TODO:translate needed.

		\~chinese
		在现有的排列上添加BYTE排列data。
		\param data 要添加的BYTE排列指针。
		\param count 排列的大小。

		\~japanese
		\~
		*/
		inline void AddRange(const uint8_t* data, int count)
		{
			// InsertRange보다 AddRange바로 콜이 더 빠름
			if (m_externalBuffer.IsNull() == false)
			{
				m_externalBuffer.AddRange(data, count);
			}
			else
			{
				if(m_tombstone)
				{
					m_tombstone->m_objByteArray.AddRange(data, count);
				}
				else
				{
					ThrowArrayIsNullError();
				}
			}
		}

		/*
		\~korean
		현재의 배열에 BYTE배열 data를 추가 합니다.
		\param indexAt 배열내에서 indexAt번째뒤에 삽입합니다. 원래의 indexAt뒤의 데이터는 삽입된 데이터 뒤로 밀립니다.
		\param data 추가할 BYTE배열 포인터
		\param count 배열의 크기

		\~english TODO:translate needed.

		\~chinese
		在现有的排列上添加BYTE排列data。
		\param indexAt 在排列中的indexAt后插入。原有的indexAt后的数据将会推到插入的数据之后。
		\param data 要添加的BYTE排列指针。
		\param count 排列的大小。

		\~japanese
		\~
		*/
		inline void InsertRange(int indexAt, const uint8_t* data, int count)
		{
			if (m_externalBuffer.IsNull() == false)
			{
				m_externalBuffer.InsertRange(indexAt, data, count);
			}
			else
			{
				if(m_tombstone)
					m_tombstone->m_objByteArray.InsertRange(indexAt, data, count);
				else
				{
					ThrowArrayIsNullError();
				}
			}
		}

		/**
		\~korean
		데이터들을 제거 합니다.
		\param indexAt 제거한 data의 index입니다.
		\param count 제거할 배열의 수 입니다.

		\~english TODO:translate needed.

		\~chinese
		删除Data。
		\param indexAt 是删除的Data index。
		\param count  要删除的排列数。

		\~japanese
		\~
		*/
		inline void RemoveRange(int indexAt, int count)
		{
			if (m_externalBuffer.IsNull() == false)
			{
				m_externalBuffer.RemoveRange(indexAt, count);
			}
			else
			{
				if(m_tombstone)
					m_tombstone->m_objByteArray.RemoveRange(indexAt, count);
				else
				{
					ThrowArrayIsNullError();
				}
			}
		}

		/**
		\~korean
		배열내의 하나의 데이터를 제거합니다.
		\param index 제거할 배열의 index값

		\~english TODO:translate needed.

		\~chinese
		删除排列中的一个Data。
		\param index 要删除的排列的index值。

		\~japanese
		\~
		*/
		inline void RemoveAt(int index)
		{
			RemoveRange(index, 1);
		}

		/**
		\~korean
		배열을 비웁니다. capacity는 변화하지 않습니다.

		\~english TODO:translate needed.

		\~chinese
		清空排列。Capacity 不会变。

		\~japanese
		\~
		*/
		inline void Clear()
		{
			SetCount(0);
			// capacity는 그대로 두어야 함. 안그러면 성능이 곳곳에서 크게 저하된다!
		}

		/**
		\~korean
		\return Data배열의 포인터를 리턴합니다.

		\~english TODO:translate needed.

		\~chinese
		\return return Data排列的指针。

		\~japanese
		\~
		*/
		inline uint8_t* GetData()
		{
			if (!m_externalBuffer.IsNull())
				return m_externalBuffer.GetData();
			else if (m_tombstone == NULL)
				ThrowArrayIsNullError();

			return m_tombstone->m_objByteArray.GetData();
		}

		/**
		\~korean
		\return Data배열의 const 포인터를 리턴합니다.

		\~english TODO:translate needed.

		\~chinese
		\return return Data排列的const指针。

		\~japanese
		\~
		*/
		inline const uint8_t* GetData() const
		{
			if (m_externalBuffer.IsNull() == false)
				return m_externalBuffer.GetData();
			else if (m_tombstone == NULL)
			{
				ThrowArrayIsNullError();
			}

			return m_tombstone->m_objByteArray.GetData();
		}

		/**
		\~korean
		\return 배열의 복사본을 리턴합니다.

		\~english TODO:translate needed.

		\~chinese
		\return  return排列的复本。

		\~japanese
		\~
		*/
		inline ByteArrayPtr Clone()
		{
			if(!m_externalBuffer.IsNull())
				ThrowException(MustNotExternalBufferError);

			ByteArrayPtr ret;

			if(m_tombstone)
			{
				ret.UseInternalBuffer();
				ret.SetCount(GetCount());
			}
			CopyRangeTo(ret, 0, GetCount());

			return ret;
		}

		/**
		\~korean
		데이터들을 dest로 복사합
		\param dest 복사받을 객체
		\param srcOffset 복사를 시작할 원본 배열의 위치
		\param count 복사 받을 배열의 크기

		\~english TODO:translate needed.

		\~chinese
		将数据复制到dest。
		\param dest  接受复制的客体。
		\param srcOffset  要开始复制的原本的排列位置。
		\param count 接受复制的排列的大小。

		\~japanese
		\~
		*/
		template<typename BYTEARRAYT>
		inline void CopyRangeToT(BYTEARRAYT &dest, int srcOffset, int count) const
		{
			if (count <= 0)
				return;

			if (dest.GetCount() < srcOffset + count)
				ThrowArrayOutOfBoundException();

			if (!GetData() || !dest.GetData())
				ThrowInvalidArgumentException();

			UnsafeFastMemmove(dest.GetData(), GetData() + srcOffset, count * sizeof(uint8_t));
		}

		/**
		\~korean
		데이터들을 dest로 복사합
		\param dest 복사받을 ByteArrayPtr
		\param srcOffset 복사를 시작할 원본 배열의 위치
		\param count 복사 받을 배열의 크기

		\~english TODO:translate needed.

		\~chinese
		将数据复制到dest。
		\param dest 接受复制的 ByteArrayPtr
		\param srcOffset 要开始复制的原本排列的位置。
		\param count 接受复制的排列的大小。

		\~japanese
		\~
		*/
		void CopyRangeTo(ByteArrayPtr &dest, int srcOffset, int count) const
		{
			CopyRangeToT<ByteArrayPtr>(dest,srcOffset,count);
		}

		/**
		\~korean
		데이터들을 dest로 복사합
		\param dest 복사받을 ByteArray
		\param srcOffset 복사를 시작할 원본 배열의 위치
		\param count 복사 받을 배열의 크기

		\~english TODO:translate needed.

		\~chinese
		将数据复制到dest。
		\param dest 要接受复制的 ByteArray
		\param srcOffset 要开始复制的原本的排列位置。
		\param count 要接受复制的排列的大小。

		\~japanese
		\~
		*/
		void CopyRangeTo(ByteArray& dest, int srcOffset, int count) const
		{
			CopyRangeToT<ByteArray>(dest,srcOffset,count);
		}

		inline uint8_t& operator[](int index)
		{
			if (m_externalBuffer.IsNull()==false)
				return m_externalBuffer[index];
			else
			{
				if(m_tombstone)
					return m_tombstone->m_objByteArray.operator[](index);
				else
				{
					ThrowArrayIsNullError();
					return m_externalBuffer[0]; // unreachable code. just preventing 'Control may reach end of non-void function' warning
				}
			}
		}

		inline const uint8_t operator[](int index) const
		{
			if (m_externalBuffer.IsNull() == false)
				return m_externalBuffer[index];
			else
			{
				if (m_tombstone)
					return m_tombstone->m_objByteArray.operator[](index);
				else
				{
					ThrowArrayIsNullError();
					return m_externalBuffer[0]; // unreachable code. just preventing 'Control may reach end of non-void function' warning
				}
			}
		}


		/**
		\~korean
		\return 이 객체의 배열 크기가 증가할 때 가중치 타입

		\~english TODO:translate needed.

		\~chinese
		\return 当此客体的排列大小增加时的加重值类型。

		\~japanese
		\~
		*/
		inline GrowPolicy GetGrowPolicy() const
		{
			if (m_tombstone != NULL)
				m_tombstone->m_objByteArray.GetGrowPolicy();

			return GrowPolicy_Normal;
		}

		/**
		\~korean
		이 객체의 배열 크기가 증가할 때 가중치 타입을 설정한다.
		자세한 내용은 GrowPolicy 을 참조
		\param val 증가할 때의 가중치 타입

		\~english TODO:translate needed.

		\~chinese
		设置此客体的排列大小增加时的加重值。
		详细内容参考GrowPolicy。
		\param val 增加时的加重值的类型。

		\~japanese
		\~
		*/
		void SetGrowPolicy(GrowPolicy val)
		{
			if (m_tombstone != NULL)
				m_tombstone->m_objByteArray.SetGrowPolicy(val);
			else
			{
				// none
			}
		}

		/**
		\~korean
		최소 버퍼 크기를 설정한다. 버퍼(capacity)크기가 증가할 때 최소한 이 사이즈보다 크게 설정한다.
		\param val 최소 Capacity size

		\~english TODO:translate needed.

		\~chinese
		设置最小buffer的大小。当Buffer（Capacity）大小增加时至少要设置成比此大小大的值。
		\param val 最小Capacity size

		\~japanese
		\~
		*/
		void SetMinCapacity(int val)
		{
			if(m_tombstone)
				m_tombstone->m_objByteArray.SetMinCapacity(val);
			else
			{
				// none
			}
		}

		/**
		\~korean
		이것을 초기에 호출하면 내부 버퍼를 생성하여 사용한다.

		\~english TODO:translate needed.

		\~chinese
		在初期呼出这个，会生成并使用内部Buffer。

		\~japanese
		\~
		*/
		inline void UseInternalBuffer()
		{
			if(!m_externalBuffer.IsNull())
				ThrowException(AlreadyHasExternalBufferError);

			if(!m_tombstone)
			{
				m_tombstone = NewTombstone();
				m_tombstone->m_refCount = 1;
			}
		}

		/**
		\~korean
		이걸 초기에 호출하면 이 객체는 외부 버퍼를 사용한다.

		\~english
		If you call this at the beginning, this object use external buffer

		\~chinese
		在初期呼出这个，此客体将使用外部Buffer。

		\~japanese
		\~
		*/
		inline void UseExternalBuffer(uint8_t* buf, int capacity)
		{
			if (m_tombstone != NULL)
				ThrowException(AlreadyHasInternalBufferError);
			else if (m_externalBuffer.IsNull() == false)
				ThrowException(AlreadyHasExternalBufferError);

			m_externalBuffer.Init(buf, capacity);
		}

		/**
		\~korean
		UseInternalBuffer, UseExternalBuffer 를 재사용하려면 이 메서드를 호출할 것.

		\~english
		Call this method if you want to reuse UseInternalBuffer,UseExternalBuffer

		\~chinese
		如果想再次使用UseInternalBuffer , UseExternalBuffer ， 呼出此方法。

		\~japanese
		\~
		*/
		void UninitBuffer()
		{
			UninitInternalBuffer();
			m_externalBuffer.Uninit();
		}

		/**
		\~korean
		\return NULL이면 true, NULL이 아니면 false를 리턴한다.

		\~english TODO:translate needed.

		\~chinese
		\return  NULL的话true,不是NULL的话return false。

		\~japanese
		\~
		*/
		inline bool IsNull() const
		{
			return (m_tombstone == NULL && m_externalBuffer.IsNull());
		}

// 		inline ByteArray& GetInternalBufferRef()
// 		{
// 			MustInternalBuffer();
// 			return *m_tombstone;
// 		}

		/**
		\~korean
		내부 버퍼를 사용하고 있는 것이 아니면 예외를 발생시킨다.

		\~english TODO:translate needed.

		\~chinese
		如果不是使用内部Buffer，使之生成例外。

		\~japanese
		\~
		*/
		inline void MustInternalBuffer()
		{
			if (m_tombstone == NULL)
			{
				ThrowException(MustInternalBufferError);
			}
		}

		/**
		\~korean
		rhs와 내용이 동일한지 체크한다.
		- 주의: 단순 메모리 비교다. 이 점을 주의할 것.
		\param rhs 비교할 ByteArrayPtr
		\return 같으면 true 다르면 false를 리턴한다.

		\~english TODO:translate needed.
		 Checks if this contains same as rhs
		- Note: This is a simple memory comparison. This is crucial.
		\param
		\return

		\~chinese
		检查是否与rhs的内容一致。
		- 注意：只是简单的内存比较。要注意这一点。
		\param rhs 要比较的 ByteArrayPtr
		\return 如果一样的话true，否则return false。

		\~japanese
		\~
		*/
		inline bool Equals(const ByteArrayPtr &rhs) const
		{
			if(rhs.GetCount()!=GetCount())
				return false;

			// 단순 메모리 비교다.
			const uint8_t* p_rhs = (uint8_t*)rhs.GetData();
			const uint8_t* p_lhs = (uint8_t*)GetData();
			return memcmp(p_rhs, p_lhs, rhs.GetCount() * sizeof(uint8_t)) == 0;
		}


	};

	/**  @} */


}
