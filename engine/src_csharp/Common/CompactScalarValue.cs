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

*/

using System;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;


namespace Nettention.Proud
{
	internal class CompactScalarValue
	{
		private Byte[] src = null;
		private int srcLength = 0;
		private int offset = 0;


		public Byte[] filledBlock = new Byte[100];
		public int filledBlockLength = 0;

		//long가 int64이다.
		public long extractedValue = 0;
		public int extracteeLength = 0;
		/*
				private unsafe void Write(Byte* data, int length)
				{
					fixed (Byte* dst = &filledBlock[filledBlockLength])
					{
						Sysutil.UnsafeMemCopy(data, dst, length);
					}

					filledBlockLength += length;
				}
				private unsafe void Write(sbyte a)
				{
					sbyte* ptr = &a;
					Write((Byte*)ptr, sizeof(sbyte));
				}
				private unsafe void Write(short a)
				{
					short* ptr = &a;
					Write((Byte*)ptr, sizeof(short));
				}
				private unsafe void Write(int a)
				{
					int* ptr = &a;
					Write((Byte*)ptr, sizeof(int));
				}
				private unsafe void Write(Int64 a)
				{
					Int64* ptr = &a;
					Write((Byte*)ptr, sizeof(Int64));
				}
		unsafecode 퇴역*/

		private void WriteByte(sbyte a)
		{
			filledBlock[filledBlockLength] = (Byte)a;
			filledBlockLength += sizeof(sbyte);
		}

		private bool ReadByte(out sbyte a)
		{
			a = new sbyte();
			if (extracteeLength + sizeof(sbyte) > srcLength - offset)
				return false;

			a = (sbyte)src[extracteeLength + offset];
			extracteeLength += sizeof(sbyte);

			return true;
		}

		public void MakeBlock(long src)
		{
			extractedValue = src;
			filledBlockLength = 0;

			bool bNegative;
			if (src < 0)
			{
				bNegative = true;
				src = ~src;
			}
			else
			{
				bNegative = false;
			}

			while (true)
			{
				sbyte oneByte = 0;
				oneByte = (sbyte)(src & 0x7f);
				src >>= 7;

				if (src != 0)
				{
					WriteByte((sbyte)((byte)oneByte | 0x80));
				}
				else
				{
					if ((oneByte & 0x40) != 0)
					{
						WriteByte((sbyte)((byte)oneByte | 0x80));
						oneByte = 0;
					}

					if (bNegative)
						oneByte |= 0x40;

					WriteByte(oneByte);
					break;
				}
			}
		}

		internal bool ExtractValue(Byte[] src, int offset, int length)
		{
			extracteeLength = 0;
			this.src = src;
			this.srcLength = length;
			this.extracteeLength = 0;
			this.offset = offset;

			sbyte oneByte = 0;
			long fillee = 0;
			int leftShiftOffset = 0;

			while (true)
			{
				if (extracteeLength == 10)
					return false;

				if (!ReadByte(out oneByte))
					return false;

				if ((oneByte & 0x80) == 0)
				{
					fillee |= ((long)(oneByte & 0x3f) << leftShiftOffset);

					if ((oneByte & 0x40) != 0)
						extractedValue = ~fillee;
					else
						extractedValue = fillee;

					return true;
				}
				else
				{
					fillee |= ((long)(oneByte & 0x7f) << leftShiftOffset);
					leftShiftOffset += 7;
				}
			}
		}
	}
}
