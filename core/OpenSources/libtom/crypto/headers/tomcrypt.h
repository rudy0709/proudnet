#ifndef TOMCRYPT_H_
#define TOMCRYPT_H_


#ifdef _WIN32
#define LTC_WINTHREAD
#else
#define LTC_PTHREAD
#endif // _WIN32

#if !defined(_WIN32) && !defined(__ORBIS__) // dev/random이 사용 가능한 모든 OS
#define LTC_DEVRANDOM    // DEVRANDOM 옵션을 안쓰면 ios의 rng_get_bytes에서 대박 시간 오래걸림(ndk에서도 해당옵션 사용가능한지 확인할것.)
#ifdef __ANDROID__
#define TRY_URANDOM_FIRST // DEVRANDOM 에서의 /dev/random 을 사용하게 되면 엔트로피가 모일때까지 blocking 되므로 사용을 안함. urandom 을 사용하면 엔트로피가 모일때까지 블럭을 안해도 되지만 난수값이 고르지 못해 사용하기가 애매하므로 urandom 보다는 기존 clock 함수를 이용한 난수생성을 사용해봄 일단 ->(테스트 결과 속도 문제 생기지 않음: 안드로이드)
#endif
#endif

#define LTM_DESC
#define LTC_SOURCE
#define USE_LTM

#define LTC_NO_PROTOTYPES
#define LTC_NO_TEST
#define LTC_NO_CIPHERS
#define LTC_NO_MODES
#define LTC_NO_HASHES
#define LTC_NO_PRNGS
#define LTC_NO_PK
#define LTC_NO_MACS

#define LTC_RIJNDAEL	// FORTUNA required
#define LTC_HMAC		// RSA required
#define LTC_OMAC		// RSA required
#define LTC_CCM_MODE	// RSA required
#define LTC_SHA1		// RSA required
#define LTC_SHA256		// FORTUNA required
#define LTC_FORTUNA		// RSA required

/* reseed every N calls to the read function */
#define LTC_FORTUNA_WD    10
/* number of pools (4..32) can save a bit of ram by lowering the count */
#define LTC_FORTUNA_POOLS 32

/* Include RSA support */
#define LTC_MRSA

#define ARGTYPE			4	// arg null check


#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>
#include "ProudTomCrypto.h"
/* use configuration data */
#include "tomcrypt_custom.h"

#ifdef __cplusplus
extern "C" {
#endif

	/* version */
#define CRYPT   0x0117
#define SCRYPT  "1.17"

	/* max size of either a cipher/hash block or symmetric key [largest of the two] */
#define MAXBLOCKSIZE  128

	/* descriptor table size */
#define TAB_SIZE      32

	/* error codes [will be expanded in future releases] */
	enum {
		CRYPT_OK=0,             /* Result OK */
		CRYPT_ERROR,            /* Generic Error */
		CRYPT_NOP,              /* Not a failure but no operation was performed */

		CRYPT_INVALID_KEYSIZE,  /* Invalid key size given */
		CRYPT_INVALID_ROUNDS,   /* Invalid number of rounds */
		CRYPT_FAIL_TESTVECTOR,  /* Algorithm failed test vectors */

		CRYPT_BUFFER_OVERFLOW,  /* Not enough space for output */
		CRYPT_INVALID_PACKET,   /* Invalid input packet given */

		CRYPT_INVALID_PRNGSIZE, /* Invalid number of bits for a PRNG */
		CRYPT_ERROR_READPRNG,   /* Could not read enough from PRNG */

		CRYPT_INVALID_CIPHER,   /* Invalid cipher specified */
		CRYPT_INVALID_HASH,     /* Invalid hash specified */
		CRYPT_INVALID_PRNG,     /* Invalid PRNG specified */

		CRYPT_MEM,              /* Out of memory */

		CRYPT_PK_TYPE_MISMATCH, /* Not equivalent types of PK keys */
		CRYPT_PK_NOT_PRIVATE,   /* Requires a private PK key */

		CRYPT_INVALID_ARG,      /* Generic invalid argument */
		CRYPT_FILE_NOTFOUND,    /* File Not Found */

		CRYPT_PK_INVALID_TYPE,  /* Invalid type of PK key */
		CRYPT_PK_INVALID_SYSTEM,/* Invalid PK system specified */
		CRYPT_PK_DUP,           /* Duplicate key already in key ring */
		CRYPT_PK_NOT_FOUND,     /* Key not found in keyring */
		CRYPT_PK_INVALID_SIZE,  /* Invalid size input for PK parameters */

		CRYPT_INVALID_PRIME_SIZE,/* Invalid size of prime requested */
		CRYPT_PK_INVALID_PADDING /* Invalid padding on input */
	};

#include "tomcrypt_cfg.h"
#include "tomcrypt_macros.h"
#include "tomcrypt_cipher.h"
#include "tomcrypt_hash.h"
#include "tomcrypt_mac.h"
#include "tomcrypt_prng.h"
#include "tomcrypt_pk.h"
#include "tomcrypt_math.h"
#include "tomcrypt_misc.h"
#include "tomcrypt_argchk.h"
#include "tomcrypt_pkcs.h"

#ifdef __cplusplus
}
#endif

#endif /* TOMCRYPT_H_ */


/* $Source: /cvs/libtom/libtomcrypt/src/headers/tomcrypt.h,v $ */
/* $Revision: 1.21 $ */
/* $Date: 2006/12/16 19:34:05 $ */