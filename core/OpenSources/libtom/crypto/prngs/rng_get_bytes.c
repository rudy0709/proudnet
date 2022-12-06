/* LibTomCrypt, modular cryptographic library -- Tom St Denis
*
* LibTomCrypt is a library that provides various cryptographic
* algorithms in a highly modular and flexible manner.
*
* The library is free for all purposes without any express
* guarantee it works.
*
* Tom St Denis, tomstdenis@gmail.com, http://libtom.org
*/
#include "../headers/tomcrypt.h"

/**
@file rng_get_bytes.c
portable way to get secure random bits to feed a PRNG (Tom St Denis)
*/

#ifdef LTC_DEVRANDOM

#include <sys/ioctl.h>
#include <unistd.h>

// 리눅스에서 /dev/random을 읽을 경우 장시간 블록되는 문제가 있어서
// NonBlock으로 500ms 동안 읽다가 안되면 /dev/urandom으로 읽어서 값을 반환한다.
#define NONBLOCK_READ_RANDOM_COUNT				50			// 50회 read
#define NONBLOCK_READ_RANDOM_USLEEP_TIME		10000		// 10ms sleep

int SetBlocking(int fd, int isBlockingMode)
{
	// unix에서는 fcntl을 쓰라고 권유하고 있다. 그러나, 
	// fcntl()를 사용하면 GCC ARM에서 nonblock 임에도 불구하고 조금씩 block현상이 생김
	// fcntl() -> ioctl()로  수정함 modify by kdh
	//int flag = fcntl(m_socket, F_GETFL, 0);
	long argp = isBlockingMode ? 0 : 1;
	int flag = 1;
	flag = ioctl(fd, FIONBIO, &argp);

	return flag;
}

static unsigned long rng_linux_nonblock_random(unsigned char *buf, unsigned long len)
{
	FILE *f;
	unsigned long x, readLen;
	int i;

	f = fopen("/dev/random", "rb");

	if (f == NULL) {
		return 0;
	}

	if (SetBlocking(fileno(f), 0))
	{
		fclose(f);
		return 0;
	}

	//fcntl(fileno(f), F_SETFD, O_NONBLOCK);

	/* disable buffering */
	if (setvbuf(f, NULL, _IONBF, 0) != 0) {
		fclose(f);
		return 0;
	}

	x = 0;

	// 500ms 동안 NonBlock으로 random 파일 읽기
	// NOTE: 여기서 len은 "남은 바이트수"로 사용된다. 즉 0이 될 때까지, 읽은만큼 줄어든다. 
	for (i = 0; i < NONBLOCK_READ_RANDOM_COUNT; i++)
	{
		readLen = (unsigned long)fread(buf + x, 1, (size_t)len, f);
		x += readLen;
		len -= readLen;

		if (!len)
		{
			break;
		}

		usleep(NONBLOCK_READ_RANDOM_USLEEP_TIME);	// 10ms sleep
	}
	fclose(f);

	return x;
}

static unsigned long rng_linux_urandom(unsigned char *buf, unsigned long len)
{
	FILE *f;
	unsigned long x;

	f = fopen("/dev/urandom", "rb");

	if (f == NULL) {
		return 0;
	}

	/* disable buffering */
	if (setvbuf(f, NULL, _IONBF, 0) != 0) {
		fclose(f);
		return 0;
	}

	x = (unsigned long)fread(buf, 1, (size_t)len, f);
	fclose(f);

	return x;
}

/* on *NIX read /dev/random */
static unsigned long rng_nix(unsigned char *buf, unsigned long len,
	void(*callback)(void))
{
#ifdef LTC_NO_FILE
	return 0;
#else
	unsigned long x;

#ifdef TRY_URANDOM_FIRST
	
	x = rng_linux_urandom(buf, len);

	if (x == 0)
		x = rng_linux_nonblock_random(buf, len);

#else

	x = rng_linux_nonblock_random(buf, len);

	if (x != len)
		x = rng_linux_urandom(buf, len);

#endif /* TRY_URANDOM_FIRST */

	return x;

#endif /* LTC_NO_FILE */
}

#endif /* LTC_DEVRANDOM */

/* on ANSI C platforms with 100 < CLOCKS_PER_SEC < 10000 */
#if defined(CLOCKS_PER_SEC) && !defined(WINCE)

#define ANSI_RNG

static unsigned long rng_ansic(unsigned char *buf, unsigned long len,
	void(*callback)(void))
{
	clock_t t1;
	int l, acc, bits, a, b;

	if (XCLOCKS_PER_SEC < 100 || XCLOCKS_PER_SEC > 10000) {
		return 0;
	}

	l = len;
	bits = 8;
	acc = a = b = 0;
	while (len--) {
		if (callback != NULL) callback();
		while (bits--) {
			do {
				t1 = XCLOCK(); while (t1 == XCLOCK()) a ^= 1;
				t1 = XCLOCK(); while (t1 == XCLOCK()) b ^= 1;
			} while (a == b);
			acc = (acc << 1) | a;
		}
		*buf++ = acc;
		acc = 0;
		bits = 8;
	}
	acc = bits = a = b = 0;
	return l;
}

#endif 

#if defined(__ORBIS__)
#include <libsecure.h>

// PS4용 prng 준비
int init_sce_secure_prng()
{
	SceLibSecureErrorType error;

	static char buffer[1024 * 5];
	static SceLibSecureBlock sys_mem_block = { buffer, sizeof(buffer) };
	error = sceLibSecureInit(SCE_LIBSECURE_FLAGS_RANDOM_GENERATOR, &sys_mem_block);
	if (error != SCE_LIBSECURE_OK) {
		return -1;
	}

	return 0;
}

// 위 함수의 반대
int uninit_sce_secure_prng()
{
	SceLibSecureErrorType error;

	error = sceLibSecureDestroy();
	if (error != SCE_LIBSECURE_OK) {
		return -1;
	}
	return 0;
}

// 암호화용 높은 수준의 난수를 생성한다.
// buf에 len만큼의 크기를 채우고 채운 값을 리턴한다. 
// 생성 못하면 0을 리턴.
// 주의: register_sce_secure_prng를 사전에 1회 호출한 적 있어야 한다.
static unsigned long rng_orbis(unsigned char *buf, unsigned long len,
	void(*callback)(void))
{
	SceLibSecureErrorType error;

	SceLibSecureBlock mem_block = { buf, len }; // Set Buffer
	error = sceLibSecureRandom(&mem_block); // createRandom
	if (error != SCE_LIBSECURE_OK) {
		// 생성 실패. 따라서 생성한 바이트수를 0으로 리턴한다.
		return 0;
	}

	return mem_block.length;
}
#endif 

/* Try the Microsoft CSP */
#if defined(WIN32) || defined(WINCE)
//#define _WIN32_WINNT 0x0400
#ifdef WINCE
#define UNDER_CE
#define ARM
#endif
#include <windows.h>
#include <wincrypt.h>

static unsigned long rng_win32(unsigned char *buf, unsigned long len,
	void(*callback)(void))
{
	HCRYPTPROV hProv = 0;
	if (!CryptAcquireContext(&hProv, NULL, MS_DEF_PROV, PROV_RSA_FULL,
		(CRYPT_VERIFYCONTEXT | CRYPT_MACHINE_KEYSET)) &&
		!CryptAcquireContext(&hProv, NULL, MS_DEF_PROV, PROV_RSA_FULL,
		CRYPT_VERIFYCONTEXT | CRYPT_MACHINE_KEYSET | CRYPT_NEWKEYSET))
		return 0;

	if (CryptGenRandom(hProv, len, buf) == TRUE) {
		CryptReleaseContext(hProv, 0);
		return len;
	}
	else {
		CryptReleaseContext(hProv, 0);
		return 0;
	}
}

#endif /* WIN32 */

/**
Read the system RNG
@param out       Destination
@param outlen    Length desired (octets)
@param callback  Pointer to void function to act as "callback" when RNG is slow.  This can be NULL
@return Number of octets read
*/
unsigned long rng_get_bytes(unsigned char *out, unsigned long outlen,
	void(*callback)(void))
{
	unsigned long x;

	LTC_ARGCHK(out != NULL);

#if defined(LTC_DEVRANDOM)
	x = rng_nix(out, outlen, callback);   if (x != 0) { return x; }
#endif

#ifdef __ORBIS__
	x = rng_orbis(out, outlen, callback);   if (x != 0) { return x; }
#endif

#ifdef WIN32
	x = rng_win32(out, outlen, callback); if (x != 0) { return x; }
#endif

#ifdef ANSI_RNG
	x = rng_ansic(out, outlen, callback); if (x != 0) { return x; }
#endif
	return 0;
}

/* $Source: /cvs/libtom/libtomcrypt/src/prngs/rng_get_bytes.c,v $ */
/* $Revision: 1.7 $ */
/* $Date: 2007/05/12 14:32:35 $ */
