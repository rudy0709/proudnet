/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <new>
#include "../OpenSources/libtom/crypto/headers/tomcrypt.h"
#include "../include/CryptoRsa.h"

#if defined(__ORBIS__) && defined(_DEBUG)
#include <mspace.h>
#endif

namespace Proud
{
#if (_MSC_VER>=1400)
// 아래 주석처리된 pragma managed 전처리 구문은 C++/CLI 버전이 있었을 때에나 필요했던 것입니다.
// 현재는 필요없는 구문이고, 일부 환경에서 C3295 "#pragma managed는 전역 또는 네임스페이스 범위에서만 사용할 수 있습니다."라는 빌드에러를 일으킵니다.
//#pragma managed(push,off)
#endif

	class CRsaProvider :public CSingleton<CRsaProvider>
	{
	public:
		// m_prng : libtom prng 관련 함수 단에서 mutex 를 겁니다.
		prng_state m_prng;
		int m_prngIndex;
		int m_hashIndex;

		CRsaProvider();
		~CRsaProvider();
	};

	CRsaProvider::CRsaProvider()
	{
		// cipher, hash, prng 를 관리하는 크리티컬섹션을 초기화합니다.
		LTC_MUTEX_CREATE(&ltc_cipher_mutex);
		LTC_MUTEX_CREATE(&ltc_hash_mutex);
		LTC_MUTEX_CREATE(&ltc_prng_mutex);

		/* Q: 아니, 생성자에서 throw를?
		A: 아래 에러는 나면 안되는 상황입니다.
		강제로 크래시를 낼까 하다가 그냥 이렇게 했습니다.
		더 나은 것은, 암호화 시스템 초기화가 실패했으므로,
		암호화 관련 함수가 에러를 리턴하는게 더 정석입니다만.
		추후 손댑시다. */

		if (register_prng(&fortuna_desc) == -1)
		{
			throw Exception("REGISTER PRNG ERROR : SPRNG");
		}
		if (register_hash(&sha1_desc) == -1)
		{
			throw Exception("REGISTER HASH ERROR : SHA1");
		}

#if defined(__ORBIS__)
		if (init_sce_secure_prng() == -1)
		{
			throw Exception("REGISTER SCE SECURE ERROR : PRNG");
		}
#endif

		ltc_mp = ltm_desc;
		m_prngIndex = find_prng("fortuna");
		m_hashIndex = find_hash("sha1");

		// 128bit 의 엔트로피를 갖도록 설정합니다.
		if (rng_make_prng(128, m_prngIndex, &m_prng, NULL) != CRYPT_OK)
		{
			throw Exception("PRNG MAKE ERROR");
		}
	}

	CRsaProvider::~CRsaProvider()
	{
#if defined(__ORBIS__)
		uninit_sce_secure_prng();
#endif
		LTC_MUTEX_DELETE(&ltc_cipher_mutex);
		LTC_MUTEX_DELETE(&ltc_hash_mutex);
		LTC_MUTEX_DELETE(&ltc_prng_mutex);
		LTC_MUTEX_DELETE(&m_prng.fortuna.prng_lock);
	}

	CCryptoRsaKey::CCryptoRsaKey()
	{
		m_key = (Rsa_key*)malloc(sizeof(Rsa_key));

		if (m_key) {
			memset(m_key, 0, sizeof(Rsa_key));
		}
		else {
			throw Exception("Rsa_key malloc failed");
		}
	}

	CCryptoRsaKey::~CCryptoRsaKey()
	{
		if (m_key)
		{
			rsa_free(m_key);
			free(m_key);
		}
		//m_key = NULL;
	}

	bool CCryptoRsaKey::FromBlob(const ByteArray &blob)
	{
		// ltc_mt를 초기화한 적이 있어야 한다. 그것을 하는게 CRsaProvider의 생성자이다.
		// 그것을 하는 것을 유도한다.
		CRsaProvider::GetSharedPtr();

		// XXX 임시로 Unity 호환
		//return true;

		int rtn = rsa_import(blob.GetData(), (unsigned long)blob.GetCount(), m_key);
		if (rtn != CRYPT_OK)
		{
			return false;
		}

		return true;
	}

	bool CCryptoRsaKey::ToBlob_internal(ByteArray &outBlob, int keyType)
	{
		// ltc_mt를 초기화한 적이 있어야 한다. 그것을 하는게 CRsaProvider의 생성자이다.
		// 그것을 하는 것을 유도한다.
		CRsaProvider::GetSharedPtr();

		// XXX 임시로 Unity 호환
		//return true;

		// 1024bit : 140 , 2048bit : 270
		outBlob.SetCount(1024);

		unsigned long len = (unsigned long)outBlob.GetCount();
		if (rsa_export(outBlob.GetData(), &len, keyType, m_key) != CRYPT_OK)
		{
			// PUBLIC KEY EXPORT ERROR
			return false;
		}

		if (len > (unsigned long)outBlob.GetCount())
		{
			// PublicKey To Blob result is larger! Memory corruption may occur!
			return false;
		}

		outBlob.SetCount(len);
		return true;
	}

	bool CCryptoRsaKey::ToBlob(ByteArray &outBlob)
	{
		// ltc_mt를 초기화한 적이 있어야 한다. 그것을 하는게 CRsaProvider의 생성자이다.
		// 그것을 하는 것을 유도한다.
		CRsaProvider::GetSharedPtr();

		return ToBlob_internal(outBlob, PK_PUBLIC);
	}

	bool CCryptoRsaKey::ToBlob_privateKey(ByteArray &outBlob)
	{
		// ltc_mt를 초기화한 적이 있어야 한다. 그것을 하는게 CRsaProvider의 생성자이다.
		// 그것을 하는 것을 유도한다.
		CRsaProvider::GetSharedPtr();

		return ToBlob_internal(outBlob, PK_PRIVATE);
	}

	bool CCryptoRsa::CreatePublicAndPrivateKey(CCryptoRsaKey &outXchgKey, ByteArray& outPubKeyBlob)
	{
		// ltc_mt를 초기화한 적이 있어야 한다. 그것을 하는게 CRsaProvider의 생성자이다.
		// 그것을 하는 것을 유도한다.
		CRsaProvider::GetSharedPtr();

		// XXX 임시로 Unity 호환
		//return true;

		if (rsa_make_key(&CRsaProvider::GetUnsafeRef().m_prng, CRsaProvider::GetUnsafeRef().m_prngIndex, 1024 / 8, 65537, outXchgKey.m_key) != CRYPT_OK)
		{
			// RSA MAKE KEY ERROR
			return false;
		}

		// key를  ByteArray 로.
		if (outXchgKey.ToBlob(outPubKeyBlob) != true)
		{
			return false;
		}

		return true;
	}

	bool CCryptoRsa::EncryptSessionKeyByPublicKey(ByteArray& outEncryptedSessionKey, const ByteArray &randomBlock, const ByteArray &publicKeyBlob)
	{
		CCryptoRsaKey publicKey;

		// Unity + PS4 빌드 환경에서 런타임에 실패하여 정상 동작하지 않습니다.
		// 조건부 컴파일 조건도 Debug일 떄 활성화 되도록 되어 있기 때문에 릴리즈에서는 사용이 안됩니다.
		// 필요에 의해서 다시 주석을 푸는 등 논의를 진행합니다.
//#if defined(__ORBIS__) && defined(_DEBUG)
//		SceLibcMallocManagedSize mmsize;
//		SCE_LIBC_INIT_MALLOC_MANAGED_SIZE(mmsize);
//
//		if (malloc_stats(&mmsize)) {
//			throw Exception("1. Heap Memory Error!!!");
//		}
//#endif

		// ltc_mt를 초기화한 적이 있어야 한다. 그것을 하는게 CRsaProvider의 생성자이다.
		// 그것을 하는 것을 유도한다.
		CRsaProvider::GetSharedPtr();

		// XXX 임시로 Unity 호환
		//outEncryptedSessionKey.AddRange(randomBlock.GetData(), randomBlock.GetCount());
		//return true;

		if (publicKey.FromBlob(publicKeyBlob) != true)
		{
			return false;
		}

		unsigned long blobLen = (unsigned long)randomBlock.GetCount();
		unsigned long dataSize = blobLen;

		// outlen must be at least the size of the modulus
		dataSize = mp_unsigned_bin_size(publicKey.m_key->N);

		outEncryptedSessionKey.SetCount(dataSize);

		int rtn = rsa_encrypt_key(randomBlock.GetData(), blobLen, outEncryptedSessionKey.GetData(), &dataSize, NULL, 0,
			&CRsaProvider::GetUnsafeRef().m_prng, CRsaProvider::GetUnsafeRef().m_prngIndex, CRsaProvider::GetUnsafeRef().m_hashIndex, publicKey.m_key);
		if (rtn != CRYPT_OK)
		{
			return false;
		}

		if (dataSize > (unsigned long)outEncryptedSessionKey.GetCount())
		{
			// Encryption result is larger! Memory corruption may occur!
			return false;
		}

		outEncryptedSessionKey.SetCount(dataSize);

		// Unity + PS4 빌드 환경에서 런타임에 실패하여 정상 동작하지 않습니다.
		// 조건부 컴파일 조건도 Debug일 떄 활성화 되도록 되어 있기 때문에 릴리즈에서는 사용이 안됩니다.
		// 필요에 의해서 다시 주석을 푸는 등 논의를 진행합니다.
//#if defined(__ORBIS__) && defined(_DEBUG)
//		SCE_LIBC_INIT_MALLOC_MANAGED_SIZE(mmsize);
//		if (malloc_stats(&mmsize)) {
//			throw Exception("2. Heap Memory Error!!!");
//		}
//#endif

		return true;
	}

	/** public key로 암호화된 sessionKeyBlob을 private key로 암호 해제한 후 리턴한다.
	\param sessionKey 받은 결과물
	\return 에러 코드 or NULL */
	ErrorInfoPtr CCryptoRsa::DecryptSessionKeyByPrivateKey(ByteArray &outRandomBlock, const ByteArray &encryptedSessionKey, const CCryptoRsaKey &privateKey)
	{
		// ltc_mt를 초기화한 적이 있어야 한다. 그것을 하는게 CRsaProvider의 생성자이다.
		// 그것을 하는 것을 유도한다.
		CRsaProvider::GetSharedPtr();

		// XXX 임시로 Unity 호환
		//outRandomBlock.AddRange(encryptedSessionKey.GetData(), encryptedSessionKey.GetCount());
		//return ErrorInfoPtr();

		int stat = 0;

		unsigned long len = (unsigned long)encryptedSessionKey.GetCount();

		// decrypted msg 사이즈는 encrypted msg 보다 사이즈가 항상 작습니다.
		outRandomBlock.SetCount(len);
		if (rsa_decrypt_key(encryptedSessionKey.GetData(), (unsigned long)encryptedSessionKey.GetCount(), outRandomBlock.GetData(), &len, NULL, 0,
			CRsaProvider::GetUnsafeRef().m_hashIndex, &stat, privateKey.m_key) != CRYPT_OK)
		{
			String cmt;
			cmt.Format(_PNT("Error"));
			ErrorInfoPtr ret(ErrorInfo::From(ErrorType_DecryptFail, HostID_None, cmt));
			return ret;
		}

		// 메세지가 변조가 되었는지 확인합니다.
		if (stat != 1)
		{
			String cmt;
			cmt.Format(_PNT("Incorrect Packet"));
			ErrorInfoPtr ret(ErrorInfo::From(ErrorType_DecryptFail, HostID_None, cmt));
			return ret;
		}

		if (len > (uint32_t)encryptedSessionKey.GetCount())
		{
			String cmt;
			cmt.Format(_PNT("Decryption result is larger! Memory corruption may occur!"));
			ErrorInfoPtr ret(ErrorInfo::From(ErrorType_DecryptFail, HostID_None, cmt));
			return ret;
		}

		outRandomBlock.SetCount(len);

		return ErrorInfoPtr();
	}

	bool CCryptoRsa::CreateRandomBlock(ByteArray& output, int length)
	{
		// ltc_mt를 초기화한 적이 있어야 한다. 그것을 하는게 CRsaProvider의 생성자이다.
		// 그것을 하는 것을 유도한다.
		CRsaProvider::GetSharedPtr();

		length = length / 8;		// byte 로 변환
		output.SetCount(length);

		if (fortuna_read(output.GetData(), length, &CRsaProvider::GetUnsafeRef().m_prng) <= 0)
		{
			// Create Random Block Read Error
			return false;
		}

		return true;
	}

#if (_MSC_VER>=1400)
//#pragma managed(pop)
#endif

}
