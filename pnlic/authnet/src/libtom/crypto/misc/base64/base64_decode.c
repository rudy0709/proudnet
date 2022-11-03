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
#include "../../headers/tomcrypt.h"

/**
  @file base64_decode.c
  Compliant base64 code donated by Wayne Scott (wscott@bitmover.com)
*/


//#ifdef LTC_BASE64

//static const unsigned char map[256] = {
//255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
//255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
//255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
//255, 255, 255, 255, 255, 255, 255,  62, 255, 255, 255,  63,
// 52,  53,  54,  55,  56,  57,  58,  59,  60,  61, 255, 255,
//255, 254, 255, 255, 255,   0,   1,   2,   3,   4,   5,   6,
//  7,   8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,
// 19,  20,  21,  22,  23,  24,  25, 255, 255, 255, 255, 255,
//255,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,
// 37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,
// 49,  50,  51, 255, 255, 255, 255, 255, 255, 255, 255, 255,
//255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
//255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
//255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
//255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
//255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
//255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
//255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
//255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
//255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
//255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
//255, 255, 255, 255 };

static const unsigned char pr2six[256] =
{
	/* ASCII table */
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
	64, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
	64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
	41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
};

/**
   base64 decode a block of memory
   @param in       The base64 data to decode
   @param inlen    The length of the base64 data
   @param out      [out] The destination of the binary decoded data
   @param outlen   [in/out] The max size and resulting size of the decoded data
   @return CRYPT_OK if successful
*/

int Base64decode_len(const char *bufcoded)
{
	int nbytesdecoded;
	register const unsigned char *bufin;
	register int nprbytes;

	bufin = (const unsigned char *)bufcoded;
	while (pr2six[*(bufin++)] <= 63);

	nprbytes = (bufin - (const unsigned char *)bufcoded) - 1;
	nbytesdecoded = ((nprbytes + 3) / 4) * 3;

	return nbytesdecoded + 1;
}

int Base64decode(char *bufplain, const char *bufcoded)
{
	int nbytesdecoded;
	register const unsigned char *bufin;
	register unsigned char *bufout;
	register int nprbytes;

	bufin = (const unsigned char *)bufcoded;
	while (pr2six[*(bufin++)] <= 63);
	nprbytes = (bufin - (const unsigned char *)bufcoded) - 1;
	nbytesdecoded = ((nprbytes + 3) / 4) * 3;

	bufout = (unsigned char *)bufplain;
	bufin = (const unsigned char *)bufcoded;

	while (nprbytes > 4) {
		*(bufout++) =
			(unsigned char)(pr2six[*bufin] << 2 | pr2six[bufin[1]] >> 4);
		*(bufout++) =
			(unsigned char)(pr2six[bufin[1]] << 4 | pr2six[bufin[2]] >> 2);
		*(bufout++) =
			(unsigned char)(pr2six[bufin[2]] << 6 | pr2six[bufin[3]]);
		bufin += 4;
		nprbytes -= 4;
	}

	/* Note: (nprbytes == 1) would be an error, so just ingore that case */
	if (nprbytes > 1) {
		*(bufout++) =
			(unsigned char)(pr2six[*bufin] << 2 | pr2six[bufin[1]] >> 4);
	}
	if (nprbytes > 2) {
		*(bufout++) =
			(unsigned char)(pr2six[bufin[1]] << 4 | pr2six[bufin[2]] >> 2);
	}
	if (nprbytes > 3) {
		*(bufout++) =
			(unsigned char)(pr2six[bufin[2]] << 6 | pr2six[bufin[3]]);
	}

	*(bufout++) = '\0';
	nbytesdecoded -= (4 - nprbytes) & 3;
	return nbytesdecoded;
}

int base64_decode(const unsigned char *in,  unsigned long inlen, 
                        unsigned char *out, unsigned long *outlen)
{
	*outlen = Base64decode((char*)out, (char*)in);
	return 0;
   //unsigned long t, x, y, z;
   //unsigned char c;
   //int           g;

   //LTC_ARGCHK(in     != NULL);
   //LTC_ARGCHK(out    != NULL);
   //LTC_ARGCHK(outlen != NULL);

   //g = 3;
   //for (x = y = z = t = 0; x < inlen; x++) {
   //    c = map[in[x]&0xFF];
   //    if (c == 255) continue;
   //    /* the final = symbols are read and used to trim the remaining bytes */
   //    if (c == 254) { 
   //       c = 0; 
   //       /* prevent g < 0 which would potentially allow an overflow later */
   //       if (--g < 0) {
   //          return CRYPT_INVALID_PACKET;
   //       }
   //    } else if (g != 3) {
   //       /* we only allow = to be at the end */
   //       return CRYPT_INVALID_PACKET;
   //    }

   //    t = (t<<6)|c;

   //    if (++y == 4) {
   //       if (z + g > *outlen) { 
   //          return CRYPT_BUFFER_OVERFLOW; 
   //       }
   //       out[z++] = (unsigned char)((t>>16)&255);
   //       if (g > 1) out[z++] = (unsigned char)((t>>8)&255);
   //       if (g > 2) out[z++] = (unsigned char)(t&255);
   //       y = t = 0;
   //    }
   //}
   //if (y != 0) {
   //    return CRYPT_INVALID_PACKET;
   //}
   //*outlen = z;
   //return CRYPT_OK;
}

//#endif


/* $Source: /cvs/libtom/libtomcrypt/src/misc/base64/base64_decode.c,v $ */
/* $Revision: 1.6 $ */
/* $Date: 2007/05/12 14:32:35 $ */
