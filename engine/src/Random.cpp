// CRandom.cpp: Implementierung der Klasse CRandom.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <new>
#include "../include/Random.h"
#include "../include/sysutil.h"

#if !defined(_WIN32)
	#include <sys/time.h>
#endif

#define N				(624)					// length of state vector
#define M				(397)					// a period parameter
#define K				(0x9908B0DFU)			// a magic constant
#define hiBit(u)		((u) & 0x80000000U)		// mask all but highest   bit of u
#define loBit(u)		((u) & 0x00000001U)		// mask all but lowest    bit of u
#define loBits(u)		((u) & 0x7FFFFFFFU)		// mask     the highest   bit of u
#define mixBits(u, v)	(hiBit(u)|loBits(v))	// move hi bit of u to hi bit of v


//////////////////////////////////////////////////////////////////////
// Konstruktion/Destruktion
//////////////////////////////////////////////////////////////////////

namespace Proud
{
	CRandom::CRandom()
	{
		m_lLeft = -2; // InitializeSeed를 필요시 부름
	}


	CRandom::~CRandom()
	{

	}

	/** 랜덤 seed 값을 세팅한다.
	- 하드 디스크와 시스템 클럭을 기반으로 랜덤 초기값을 세팅한다. */
	void CRandom::InitializeSeed()
	{
		CriticalSectionLock lock(m_cs,true);
		// We need to seed our algorithm with something random from the operating system
		uint16_t wHigh = 0;
		uint16_t wLow = 0;

		// GetTickCount() returns the number of milliseconds the machine has been turned on
		wHigh = LOWORD(CFakeWin32::GetTickCount());

#if !defined(_WIN32)
		struct timeval val;
		gettimeofday(&val, NULL);

		unsigned char chigh = 0;
		unsigned char clow = 0;

		chigh = LOBYTE(val.tv_usec);
		clow = static_cast<unsigned char>((val.tv_usec >> 8) + val.tv_sec );
		wLow = MAKEWORD(clow,chigh);

#else
		DWORD dwNumberOfSectorsPerCluster = 0;
		DWORD dwNumberOfBytesPerSector = 0;
		DWORD dwNumberOfFreeClusters = 0;
		DWORD dwTotalNumberOfClusters = 0;

		// Different folks have different amounts of free space on their hard drives
		if ( ::GetDiskFreeSpace(NULL, &dwNumberOfSectorsPerCluster, &dwNumberOfBytesPerSector, &dwNumberOfFreeClusters, &dwTotalNumberOfClusters) != FALSE)
		{
			wLow = LOWORD(dwNumberOfFreeClusters);

			// This is a nice number but not nice enough. It won't vary enough
			// from call to call. So, let's scramble it a bit (no pun intented)
			// Anther call to GetTickCount() should do the trick because the
			// GetDiskFreeSpace() will take a varying amount of time.
			wLow  ^= static_cast<uint16_t>(CFakeWin32::GetTickCount());
		} else {
			// This is a fail-safe condition. We resort to time because it is kinda
			// random but it is guessable.
			SYSTEMTIME SystemTime;

			// ikpil.choi : 2016-11-03, memory safe function
			//ZeroMemory(&SystemTime, sizeof(SYSTEMTIME));
			memset_s(&SystemTime, sizeof(SystemTime), 0, sizeof(SystemTime));

			GetSystemTime(&SystemTime);

			// The most random part of the system time is the milliseconds then seconds
			uint8_t cHigh = 0;
			uint8_t cLow = 0;

			cHigh = LOBYTE(SystemTime.wMilliseconds);
			cLow = static_cast<uint8_t>((SystemTime.wMilliseconds >> 8) + SystemTime.wSecond);

			wLow = MAKEWORD(cLow, cHigh);
		}
#endif

		SetSeed(MAKELONG(wLow, wHigh));
	}


	uint32_t CRandom::LoadMersenneTwister()
	{
		/*register*/ uint32_t *p0 = m_dwState;
		/*register*/ uint32_t *p2 = m_dwState + 2;
		/*register*/ uint32_t *pM = m_dwState + M;
		/*register*/ uint32_t s0 = 0;
		/*register*/ uint32_t s1 = 0;
		/*register*/ int j = 0;

		if (m_lLeft < -1)
			InitializeSeed();

		m_lLeft = N - 1;
		m_pdwNext = m_dwState + 1;

		for ( s0 = m_dwState[ 0 ], s1 = m_dwState[ 1 ], j = N - M + 1; --j; s0 = s1, s1 = *p2++ )
			* p0++ = *pM++ ^ (mixBits(s0, s1) >> 1) ^ (loBit(s1) ? K : 0U);

		for ( pM = m_dwState, j = M; --j; s0 = s1, s1 = *p2++ )
			* p0++ = *pM++ ^ (mixBits(s0, s1) >> 1) ^ (loBit(s1) ? K : 0U);

		s1 = m_dwState[0], *p0 = *pM ^ (mixBits(s0, s1) >> 1) ^ (loBit(s1) ? K : 0U);
		s1 ^= (s1 >> 11);
		s1 ^= (s1 <<  7) & 0x9D2C5680U;
		s1 ^= (s1 << 15) & 0xEFC60000U;

		return s1 ^ (s1 >> 18);
	}

	int CRandom::GetInt()
	{
		// 내부 state를 바꾸는 메서드이므로 thread safe를 강제한다.
		CriticalSectionLock lock(m_cs,true);

		uint32_t dwResult = 0;

		if (--m_lLeft < 0 )
			return LoadMersenneTwister();

		dwResult  = *m_pdwNext++;
		dwResult ^= (dwResult >> 11);
		dwResult ^= (dwResult <<  7) & 0x9D2C5680U;
		dwResult ^= (dwResult << 15) & 0xEFC60000U;

		return dwResult ^ (dwResult >> 18);
	}

	/* 랜덤값을 만든다. 대략 측정해본 바, -2147483008~2147483008 사이 값이 나오는 것 같다. */
	double CRandom::GetFloat_INTERNAL()
	{
		CriticalSectionLock lock(m_cs,true);
		union
		{
			double dResult;
			unsigned char cResultBytes[8];
		} Result;

		Result.dResult = GetInt();

		uint32_t dwTemp = GetInt();

		// This works for Intel-Endian machines
		Result.cResultBytes[0] ^= HIBYTE(HIWORD(dwTemp));
		Result.cResultBytes[1] ^= HIBYTE(LOWORD(dwTemp));
		Result.cResultBytes[2] ^= LOBYTE(HIWORD(dwTemp));

		return Result.dResult;
	}

	double abs(double a)
	{
		if (a < 0)
			return -a;
		return a;
	}
	double CRandom::GetFloat()
	{
		double a = GetFloat_INTERNAL() / 2147483008;
		return abs(a);
	}

	void CRandom::SetSeed(uint32_t dwSeed)
	{
		CriticalSectionLock lock(m_cs,true);
		//
		// We initialize state[0..(N-1)] via the generator
		//
		//   x_new = (69069 * x_old) mod 2^32
		//
		// from Line 15 of Table 1, p. 106, Sec. 3.3.4 of Knuth's
		// _The Art of Computer Programming_, Volume 2, 3rd ed.
		//
		// Notes (SJC): I do not know what the initial state requirements
		// of the Mersenne Twister are, but it seems this seeding generator
		// could be better.  It achieves the maximum period for its modulus
		// (2^30) iff x_initial is odd (p. 20-21, Sec. 3.2.1.2, Knuth); if
		// x_initial can be even, you have sequences like 0, 0, 0, ...;
		// 2^31, 2^31, 2^31, ...; 2^30, 2^30, 2^30, ...; 2^29, 2^29 + 2^31,
		// 2^29, 2^29 + 2^31, ..., etc. so I force seed to be odd below.
		//
		// Even if x_initial is odd, if x_initial is 1 mod 4 then
		//
		//   the          lowest bit of x is always 1,
		//   the  next-to-lowest bit of x is always 0,
		//   the 2nd-from-lowest bit of x alternates      ... 0 1 0 1 0 1 0 1 ... ,
		//   the 3rd-from-lowest bit of x 4-cycles        ... 0 1 1 0 0 1 1 0 ... ,
		//   the 4th-from-lowest bit of x has the 8-cycle ... 0 0 0 1 1 1 1 0 ... ,
		//    ...
		//
		// and if x_initial is 3 mod 4 then
		//
		//   the          lowest bit of x is always 1,
		//   the  next-to-lowest bit of x is always 1,
		//   the 2nd-from-lowest bit of x alternates      ... 0 1 0 1 0 1 0 1 ... ,
		//   the 3rd-from-lowest bit of x 4-cycles        ... 0 0 1 1 0 0 1 1 ... ,
		//   the 4th-from-lowest bit of x has the 8-cycle ... 0 0 1 1 1 1 0 0 ... ,
		//    ...
		//
		// The generator's potency (min. s>=0 with (69069-1)^s = 0 mod 2^32) is
		// 16, which seems to be alright by p. 25, Sec. 3.2.1.3 of Knuth.  It
		// also does well in the dimension 2..5 spectral tests, but it could be
		// better in dimension 6 (Line 15, Table 1, p. 106, Sec. 3.3.4, Knuth).
		//
		// Note that the random number user does not see the values generated
		// here directly since reloadMT() will always munge them first, so maybe
		// none of all of this matters.  In fact, the seed values made here could
		// even be extra-special desirable if the Mersenne Twister theory says
		// so-- that's why the only change I made is to restrict to odd seeds.
		//

		/*register*/ uint32_t x = (dwSeed | 1) & 0xFFFFFFFF;
		/*register*/ uint32_t *s = m_dwState;
		/*register*/ int j = 0;

		for (m_lLeft = 0, *s++ = x, j = N; --j;
			*s++ = (x *= 69069U) & 0xFFFFFFFF);
	}

	int64_t CRandom::GetInt64()
	{
		CriticalSectionLock lock(m_cs,true);
		int64_t ret;
		double* part = (double*) & ret;
		*part = GetFloat_INTERNAL();
		return ret;
	}

	class CGlobalRandom:public CSingleton<CGlobalRandom>
	{
	public:
		CRandom m_rand;
	};

	int CRandom::StaticGetInt()
	{
		// GetSharedPtr을 쓰는 것이 더 바람직하나, 이 함수 자체가 static 함수인지라, 딱히 파괴순서를 보장할
		return CGlobalRandom::GetUnsafeRef().m_rand.GetInt();
	}

	double CRandom::StaticGetFloat_INTERNAL()
	{
		return CGlobalRandom::GetUnsafeRef().m_rand.GetFloat_INTERNAL();
	}

	int64_t CRandom::StaticGetInt64()
	{
		return CGlobalRandom::GetUnsafeRef().m_rand.GetInt64();
	}

	double CRandom::StaticGetFloat()
	{
		return abs(StaticGetFloat_INTERNAL() / 2147483008);
	}
}
