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
using System.Diagnostics;

namespace Nettention.Proud
{
    /** \addtogroup util_group
	*  @{
	*/

    /** 
	\~korean
	이 객체의 배열 크기가 증가할 때 가중치 타입
    
	\~english
	The weight type when the size of the array of this object.
    
	\~chinese
	此客体排列大小增加时的加重值类型。

	\~japanese
    
	\~
	 */
    public enum eGrowPolicy
    {
		/** 
		\~korean
		균형 
        
		\~english 
		Balance
            
		\~chinese
		均衡

		\~japanese
    
		\~
		*/
        Normal,

		/**
		\~korean 
		고성능. 대신 메모리 사용량이 많음 
        
		\~english 
		High performance but use more memory 
            
		\~chinese
		高性能，但内存使用量较大。
    
		\~japanese
    
		\~
		*/
        HighSpeed,

		/**
		\~korean
		메모리 절감. 대신 배열 크기가 증가시 복사 부하로 인한 저성능. 
        
		\~english 
		Save memory but when array size is increased, it shows low performance because of overload of copying 
            
		\~chinese
		节省内存，但在排列大小增加时会因复制负荷导致性能低下。

    
		\~japanese
    
		\~
		*/
        LowMemory
    }

    /** 
	\~korean
	배열 클래스

	주요 특징
	- C\#의 배열을 손쉽게 resize할수 있다.
	\param T 배열의 항목 타입
    
	\~english TODO:translate needed.
        
	\~chinese
	排列类
	主要特征
	- 可以轻松将 C# 排列进行 resize。
	\param T 排列项目类型。

	\~japanese
    
    \~
    */
    public class FastArray<T>:ICloneable
    {
		/**
		\~korean 
		내부 버퍼 입니다.

		\~english TODO:translate needed.
            
		\~chinese
		内部Buffer。

		\~japanese
    
		\~
		*/
        public T[] data = null;
        private int length = 0;
        private int capacity = 0;
        private int minCapacity = 0;

        private eGrowPolicy growPolicy = eGrowPolicy.Normal;

        Object ICloneable.Clone()
        {
            return this.Clone();
        }

		/**
		\~korean
		\brief 이 객체의 클론을 만들어 리턴합니다. 
        
		\~english TODO:translate needed.
        
		\~chinese
		\brief 制作此客体的Clone后Return。 

		\~japanese
    
        */
        public FastArray<T> Clone()
        {
            return (FastArray<T>)this.MemberwiseClone();
        }

        protected void InitVars()
        {
            this.data = null;
            this.length = 0;
            this.capacity = 0;
            this.minCapacity = 0;
        }

		/** 
		\~korean
		\brief 배열에서 사용자가 원하는 위치의 인자를 얻습니다.
        \param index Zero-base index입니다.
        \return 얻은 값
        
		\~english TODO:translate needed.
 
		\~chinese
		\brief 获得在排列中用户所希望的位置因子。
		\param index 是Zero-base index。
		\return 获得的值。

		\~japanese
    
        */
        public T this[int index]
        {
            get
            {
                BoundCheck(index);
                return data[index];
            }

            set
            {
                BoundCheck(index);
                data[index] = value;
            }
        }

		/** 
		\~korean
		\brief 배열의 갯수를 얻습니다. 
        
		\~english TODO:translate needed.
            
		\~chinese
		\brief 获得排列个数。
    
		\~japanese
    
		*/
        public int Count
        {
            get { return this.length; }
            set { SetCount(value); }
        }

		/**
		\~korean 
		배열의 가중치 타입 
        
		\~english TODO:translate needed.

		\~chinese
		排列的加重值类型。

		\~japanese
        
		\~
		*/
        public eGrowPolicy GrowPolicy
        {
            get { return this.growPolicy; }
            set { this.growPolicy = value; }
        }

		/** 
		\~korean 
		배열의 갯수를 0으로 Clear합니다. 
		주의!!! 버퍼는 그대로 남아 있으므로, data를 접근할때에 이점을 유의 하세요. 

		\~english TODO:translate needed.

		\~chinese
		将排列个数Clear为 0。 
		注意!!! Buffer一直存在，因此在接近data时请留意这一点。 

		\~japanese
        
		\~
        */
        public void Clear()
        {
            Count = 0;
        }

        protected void BoundCheck(int index)
        {
            if (index < 0 || index >= Count)
                Sysutil.ThrowArrayOutOfBoundException();
        }

		/**
		\~korean 
		배열 뒤에 배열 추가 
        
		\~english 
		Add array at the end of array 
        
		\~chinese
		在排列后面添加排列

		\~japanese
        
		\~
		*/
        public void AddRange(T[] data)
        {
            if (data == null || data.Length < 0)
            {
                Sysutil.ThrowInvalidArgumentException();
            }

            if (data.Length == 0)
                return;

            int oldCount = Count;

            SetCount(Count + data.Length);
            int indexAt = oldCount;

            Array.Copy(data, 0, this.data, indexAt, data.Length);
        }

		/** 
		\~korean 
		배열 뒤에 배열 추가 
        
		\~english 
		Add array at the end of array 
            
		\~chinese
		在排列后面添加排列

		\~japanese
    
		\~ 
		*/
        public void AddRange(T[] data, int dataLength)
        {
            if (data == null || data.Length < 0 || dataLength < 0 || data.Length < dataLength)
            {
                Sysutil.ThrowInvalidArgumentException();
            }

            if (dataLength == 0)
                return;

            int oldCount = Count;

            SetCount(Count + dataLength);
            int indexAt = oldCount;

            Array.Copy(data, 0, this.data, indexAt, dataLength);
        }

		/**
		\~korean 
		배열 중간에 배열 추가. offset가 가리키는 부분부터 추가한다. 
         
		\~english TODO:translate needed.
            
		\~chinese
		在排列中间添加排列，先从offset所指部分开始添加。 

		\~japanese
    
		\~
		*/
        public void AddRange(T[] data, int offset, int dataLength)
        {
            if (data == null || data.Length < 0 || dataLength < 0 || data.Length < dataLength
                || offset > data.Length)
            {
                Sysutil.ThrowInvalidArgumentException();
            }

            if (dataLength == 0)
                return;

            int oldCount = Count;

            SetCount(Count + dataLength);
            int indexAt = oldCount;

            Array.Copy(data, offset, this.data, indexAt, dataLength);
        }

		/** 
		\~korean 
		맨뒤에 추가
        
		\~english 
		Add at the end 
            
		\~chinese
		在最后面添加

		\~japanese
    
		\~
		*/
        public void Add(T value)
        {
            Insert(Count, value);
        }


		/**
		\~korean
		index에 해당하는 data를 제거한다.
		
		\~english
		Remove item at index
		
		\~chinese
		去除index的相应data。
		\~
		*/
        public void RemoveAt(int index)
        {
            BoundCheck(index);            

            for (int i = index; i < length; i++ )
            {
                this.data[i] = this.data[i + 1];
            }

            if (!typeof(T).IsValueType)
                data[length] = default(T);

            SetCount(length - 1);
        }
		/** 
		\~korean 
		indexAt이 가리키는 항목을 모두 한칸씩 뒤로 밀고 value를 indexAt이 가리키는 곳에 추가한다. 

		\~english
		Move back item that pointed by indexAt then add value to place that pointed by indexAt 
 
		\~chinese
		将indexAt所指项目全部往后移一格，之后将value添加到indexAt所指的地方。 

		\~japanese
    
		\~
		*/
        public void Insert(int indexAt, T value)
        {
            InsertRange(indexAt, value);
        }

		/**
		\~korean 
		indexAt이 가리키는 항목을 모두 한칸씩 뒤로 밀고 data를 indexAt이 가리키는 곳에 추가한다. 
        
		\~english
		Move back item that pointed by indexAt then add value to place that pointed by indexAt 
            
		\~chinese
		将indexAt所指项目全部往后移一格，之后将data添加到indexAt所指的地方。 

		\~japanese
    
		\~
		*/
        public void InsertRange(int indexAt, T data)
        {
            if (data == null || indexAt > Count || indexAt < 0)
                Sysutil.ThrowInvalidArgumentException();

            int oldCount = Count;

            SetCount(Count + 1);

            int moveAmount = oldCount - indexAt;

            if (moveAmount > 0)
            {
                for (int i = moveAmount - 1; i >= 0; i--)
                {
                    // 가속을 위해 GetData() 사용
                    this.data[i + indexAt + 1] = this.data[i + indexAt];
                }
            }

            this.data[indexAt] = data;
        }

		/**
		\~korean 
		배열 중간에 배열 추가. indexAt이 가리키는 부분을 뒤로 밀어놓고 틈새에 추가한다. 
        
		\~english 
		Add array to middle of array. Move back part that pointed by indexAt then add to the gap
            
		\~chinese
		在排列中间添加排列。将indexAt所指部分后移，之后在其空位中添加。 

		\~japanese
    
		\~
		*/
        public void InsertRange(int indexAt, T[] data)
        {
            if (data == null || indexAt > Count || indexAt < 0)
                Sysutil.ThrowInvalidArgumentException();

            int oldCount = Count;

            SetCount(Count + data.Length);

            int moveAmount = oldCount - indexAt;

            if (moveAmount > 0)
            {
                for (int i = moveAmount - 1; i >= 0; i--)
                {
                    // 가속을 위해 GetData() 사용
                    this.data[i + indexAt + 1] = this.data[i + indexAt];
                }
            }

            Array.Copy(data, 0, this.data, indexAt, data.Length);
        }

        /// \~korean index번째 항목부터 count만큼 제거한다. \~korean Remove from index th list as many as count \~
        public void RemoveRange(int index, int count)
        {
            if(index < 0 || count < 0)
                Sysutil.ThrowInvalidArgumentException();

            count = Math.Min(count, Count - index);

            // 앞으로 땡긴다.
            int amount = Count - (index + count);

            if (amount > 0)
            {
                for (int i = 0; i < amount; i++)
                {
                    this.data[i+index] = this.data[i+index+count];
                }

            }

            SetCount(Count - count);
        }

        private int GetRecommendedCapacity(int actualCount)
        {
            switch (growPolicy)
            {
                case eGrowPolicy.Normal:
                    {
                        // heuristically determine growth when nGrowBy == 0
                        //  (this avoids heap fragmentation in many situations)
                        int nGrowBy = length / 8;
                        nGrowBy = Math.Min(nGrowBy, 1024);
                        nGrowBy = Math.Max(nGrowBy, 4);

                        return Math.Max(minCapacity, actualCount + nGrowBy);
                    }
                case eGrowPolicy.HighSpeed:
                    {
                        int nGrowBy = length / 8;
                        
                        nGrowBy = Math.Max(nGrowBy, 16);
                        nGrowBy = Math.Max(nGrowBy, 64); // CMessage를 위해 이런 짓을 함


                        return Math.Max(minCapacity, actualCount + nGrowBy);
                    }
                case eGrowPolicy.LowMemory:
                    return Math.Max(minCapacity, actualCount);
            }

            Sysutil.ThrowInvalidArgumentException();
            return -1;
        }

        /** 
        \~korean
        배열 크기를 조절한다
        - 배열 크기 조절시 capacity가 충분히 증가한다.
        
        \~english
        Balalnces the size of array
        - When this is done, capacity increases enough.
            
		\~chinese
		调整排列大小
		- 在调整排列大小时 capacity将充分增加。
    
		\~japanese
    
        \~
         */
        public void SetCount(int newVal)
        {
            if (newVal < 0)
                Sysutil.ThrowInvalidArgumentException();

            if (newVal != length)
            {
                if (newVal > capacity)// 캐퍼를 늘려야 하면
                {
                    // 캐퍼를 늘리기.
                    int newCapacity = GetRecommendedCapacity(newVal);
                    Debug.Assert(newCapacity != capacity);
                    if (capacity == 0) // 캐퍼가 애당초 없었다면
                    {
                        // 0->N
                        data = new T[newCapacity];
                    }
                    else
                    {
                        // M->N(M<N) 있던 캐퍼의 증가
                        Array.Resize(ref data, newCapacity);
                    }
                    capacity = newCapacity;
                }

                // 실 사이즈 조절
                length = newVal;
            }
        }

        // 어차피 GC가 있어서 obj-pool을 안하고 있다. 하지만 이 함수가 있어야 지나친 memcpy를 예방할 수 있다.
        public void SetCapacity(int maxLength)
        {
            int oldCount = Count;
            SetCount(Math.Max(oldCount, maxLength));
            SetCount(oldCount);
        }

        public void SetMinCapacity(int newCapacity)
        {
            minCapacity = Math.Max(newCapacity, minCapacity);
        }


        /** 
        \~korean
        외부 버퍼를 세팅합니다.
        - 내부 버퍼를 외부 버퍼로 세팅합니다.
        - 현재 내부 버퍼에 값이 들어 있다면 exception이 발생합니다.
        
		\~english TODO:translate needed.
            
		\~chinese
		设置外部Buffer。
		- 将内部Buffer设置为外部Buffer。
		- 如果内部Buffer当中存在值，则会发生exception。

		\~japanese
    
        \~
        */
        public void UseExternalBuffer(ref T[] buffer,int capacity)
        {
            if(null != data)
                throw new Exception("FastArray.UseExternalBuffer");

            if (buffer == null || capacity == 0)
                return;

            this.capacity = capacity;
            data = buffer;
            length = 0;
        }
    }

    /** 
	\~korean
	ByteArray는 FastArray Byte와 같다.
    
	\~english TODO:translate needed.

	\~chinese
	ByteArray与FastArray Byte相同。

	\~japanese
    
	\~
	 */
    public class ByteArray : FastArray<Byte>
    {

        public ByteArray()
        {

        }

        public ByteArray(Byte[] inputData, int count)
        {
            GrowPolicy = eGrowPolicy.Normal;
            InitVars();

            AddRange(inputData, count);
        }

        public ByteArray(Byte[] inputData)
        {
            AddRange(inputData);
        }
      
        public new ByteArray Clone()
        {
            return (ByteArray)this.MemberwiseClone();
        }

        public static ByteArray CopyFrom(byte[] source)
        {
            ByteArray ret = new ByteArray();
            ret.SetCount(source.Length);
            for (int i = 0; i < source.Length; ++i)
            {
                ret.data[i] = source[i];
            }
            return ret;
        }

/*
        public unsafe void AddRange(Byte* srcData, int length)
        {
            if (srcData == null || length < 0)
            {
                Sysutil.ThrowInvalidArgumentException();
            }

            if (length == 0)
                return;

            int oldCount = Count;

            SetCount(Count + length);
            int indexAt = oldCount;

            fixed (Byte* pDst = &data[indexAt])
            {
                Sysutil.UnsafeMemCopy(srcData, pDst, length);
            }
        }
unsafe code 퇴역*/
    }
    /**  @} */
}
