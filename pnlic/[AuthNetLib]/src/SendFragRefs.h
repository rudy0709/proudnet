#pragma once

#include "../include/pnstdint.h"
#include "../include/ExternalBufferArray.h"
#include "../include/Message.h"
#include "SocketUtil.h"
#include "../include/ClassBehavior.h"

namespace Proud
{
	/* 송신 처리를 위한 fragmented data block 객체. data block segment의 집합이다.
	- 순수하게 송신시 fragmented data를 gather하는 횟수를 최소화하기 위한 목적으로 만들어진 클래스다. 
	  따라서 이것을 통해 scattered receive를 쓰는 것으로의 용도는 피한다.
	- 성능을 위한 것이므로, 여기에 다른 버퍼를 추가해도 pointer share를 한다. 
	  따라서 이 객체가 참조하는 버퍼의 존재는 다른 객체들이 보장해줘야 한다. */
	class CSendFragRefs
	{
	
		NO_COPYABLE(CSendFragRefs)
	public:		
		template<typename T>
		friend void CSendFragRefs_ToByteArrayT(const CSendFragRefs& src, T& ret);
	public:
		// 1개의 segment.
		class CFrag
		{
			// 주의: 이거 수정시 이걸 참조하는 CFastArray도 수정해야 한다!
		public:
			const uint8_t* m_data;
			int m_length;

			inline CFrag()
			{
				m_data = NULL;
				m_length = 0;
			}

			inline CFrag(const uint8_t* fragment, int length)
			{
				m_data = fragment;
				m_length = length;
			}

			inline const uint8_t* GetData() const {
				return m_data;
			}
			inline int GetLength() const {
				return m_length;
			}
		};
	private:
		typedef CFastArray<CFrag, true, true, int> FragArray;
		/* (예전에는 고정 크기 배열과 그것을 참조하는 배열 객체였지만,
		TCP send queue에서 빼올 때 100을 넘는 경우가 흔하며, 이때는 상당한 realloc & copy 로 인해 성능상 이익이 불투명.
		code profile 결과 heap/free 부하도 제법 크다.
		따라서 obj-pool한 배열을 사용. */
		FragArray* m_fragArray; // 배열의 배열
		inline FragArray& Array() { return *m_fragArray; }
		inline const FragArray& Array() const { return *m_fragArray; }
	public:
		// vs2003 오류 의심으로 구현부를 cpp로 옮김
		CSendFragRefs();
		CSendFragRefs(const CMessage &msg);

		~CSendFragRefs();

		inline CFrag* GetFragmentData()
		{
			return Array().GetData();
		}
		inline const CFrag* GetFragmentData() const
		{
			return Array().GetData();
		}

		inline void SetFragmentCount(int count)
		{
			Array().SetCount(count);
		}

		inline void Add(const CFrag& src)
		{
			Array().Add(src);
		}

		inline void Add(const uint8_t* fragment, int length)
		{
			Array().Add(CFrag(fragment, length));
		}

		inline void Add(const CSendFragRefs &src)
		{
			// 반복 Add가 코드 프로필링 결과 크게 나옴. 그냥 한번에 다 넣어버리자.
			int cnt = (int)src.GetFragmentCount();
			int offset = (int)GetFragmentCount();
			SetFragmentCount(offset + cnt);

			CFrag* pDest = Array().GetData();

			UnsafeFastMemcpy(pDest + offset, src.GetFragmentData(), cnt * sizeof(CFrag));

			/*for (int i = 0;i < cnt;i++)
			{
			pDest[offset + i] = src[i];
			}*/
		}

		void AppendFrag_ShareBuffer(CSendFragRefs &appendee);

		inline CFrag &operator[](int index)
		{
			return Array()[index];
		}

		inline const CFrag &operator[](int index) const
		{
			return Array()[index];
		}

		inline intptr_t GetFragmentCount() const {
			return Array().GetCount();
		}

		int GetTotalLength() const;

		inline void Insert(int indexAt, CSendFragRefs &insertee)
		{
			Array().InsertRange(indexAt, insertee.m_fragArray->GetData(), insertee.m_fragArray->GetCount());
		}

		inline void Insert(int indexAt, CMessage &msg)
		{
			Insert(indexAt, msg.GetData(), msg.GetLength());
		}

		inline void Insert(int indexAt, uint8_t* fragment, int length)
		{
			Array().Insert(indexAt, CFrag(fragment, length));
		}

		inline void Clear()
		{
			Array().SetCount(0);
		}

		void CloneButBufferShared(CSendFragRefs &ret);
		void CopyTo(ByteArray &ret) const;
		void CopyTo(ByteArrayPtr &ret) const;
		void ToAssembledMessage(CMessage &ret) const;
		void ToAssembledByteArray(ByteArray &output) const;
		//		CMessage ToAssembledMessage() const;
		// 		void AddRangeToByteArray(ByteArrayPtr target) const;
		// 		void AddRangeToByteArray(ByteArray &target) const;

	};

	//CMessage& operator<<(CMessage& msg,const CSendFragRefs& v);
	//CMessage& operator>>(CMessage& msg,CSendFragRefs& v);
	void AppendTextOut(String& output, const CSendFragRefs& v);

}