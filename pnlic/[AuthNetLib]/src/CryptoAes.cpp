﻿/* ProudNet

이 프로그램의 저작권은 넷텐션에게 있습니다.
이 프로그램의 수정, 사용, 배포에 관련된 사항은 본 프로그램의 소유권자와의 계약을 따르며,
계약을 준수하지 않는 경우 원칙적으로 무단 사용을 금지합니다.
무단 사용에 의한 책임은 본 프로그램의 소유권자와의 계약서에 명시되어 있습니다.

** 주의: 저작물에 관한 위의 명시를 제거하지 마십시오.
*/

#include "stdafx.h"
#include <new>
#include "../include/CryptoAes.h"
#include "../include/Message.h"
#include "Crc.h"

namespace Proud
{
	//CONSTRUCTOR
	CCryptoAes::CCryptoAes()
	{
	}

	//DESTRUCTOR
	CCryptoAes::~CCryptoAes()
	{
	}

	//Expand a user-supplied key material into a session key.
	// key        - The 128/192/256-bit user-key to use.
	// keylength  - 16, 24 or 32 bytes
	// blockSize  - The block size in bytes of this Rijndael (16, 24 or 32 bytes).
	bool CCryptoAes::ExpandFrom(CCryptoAesKey &outKey, const uint8_t* const key, int keyLength, int blockSize)
	{
		// 키 길이는 세종류입니다.
		if (!(keyLength == 16 || keyLength == 24 || keyLength == 32))
		{
			// Incorrect key length
			return false;
		}

		if (!(blockSize == 16 || blockSize == 24 || blockSize == 32))
		{
			// Incorrect block length
			return false;
		}

		outKey.m_blockSize = blockSize;

		//Calculate Number of Rounds
		// 키 값길이에 따라 라운드수가 정해진다.
		switch (keyLength)
		{
		case 16:
			outKey.m_rounds = (outKey.m_blockSize == 16) ? 10 : (outKey.m_blockSize == 24 ? 12 : 14);
			break;

		case 24:
			outKey.m_rounds = (outKey.m_blockSize != 32) ? 12 : 14;
			break;

		default: // 32 bytes = 256 bits
			outKey.m_rounds = 14;
			break;
		}

		int bc = outKey.m_blockSize / 4;
		int i, j;
		// Encrypt 라운드키 초기화
		for (i = 0; i <= outKey.m_rounds; i++)
		{
			for (j = 0; j < bc; j++)
			{
				outKey.m_ke[i][j] = 0;
			}
		}
		// Decrypt 라운드키 초기화
		for (i = 0; i <= outKey.m_rounds; i++)
		{
			for (j = 0; j < bc; j++)
			{
				outKey.m_kd[i][j] = 0;
			}
		}
		// 라운드 키의 수 : Nr +1 
		int roundKeyCount = (outKey.m_rounds + 1) * bc;
		int kc = keyLength / 4;

		//Auxiliary private use buffers
		int tk[MAX_KC];
		//Copy user material bytes into temporary ints
		int* pi = tk;			// 버퍼 포인터
		uint8_t const* pc = key;	// 키 버퍼 포인터의 내용이 바뀌지 않게 안전 처리
		for (i = 0; i < kc; i++)
		{
			*pi = *(pc++) << 24;
			*pi |= *(pc++) << 16;
			*pi |= *(pc++) << 8;
			*(pi++) |= *(pc++);
		}
		//Copy values into round key arrays
		// 키 스테이트 블럭 세팅
		int t = 0;
		for (j = 0; (j < kc) && (t < roundKeyCount); j++, t++)
		{
			outKey.m_ke[t / bc][t%bc] = tk[j];
			outKey.m_kd[outKey.m_rounds - (t / bc)][t%bc] = tk[j];
		}
		int tt, rconpointer = 0;
		while (t < roundKeyCount)
		{
			// S-Box 치환
			//Extrapolate using phi (the round key evolution function)
			tt = tk[kc - 1];
			tk[0] ^= (m_s[(tt >> 16) & 0xFF] & 0xFF) << 24 ^
				(m_s[(tt >> 8) & 0xFF] & 0xFF) << 16 ^
				(m_s[tt & 0xFF] & 0xFF) << 8 ^
				(m_s[(tt >> 24) & 0xFF] & 0xFF) ^
				(m_rcon[rconpointer++] & 0xFF) << 24;
			if (kc != 8)
			{
				for (i = 1, j = 0; i < kc;)
				{
					tk[i++] ^= tk[j++];
				}
			}
			else
			{
				for (i = 1, j = 0; i < kc / 2;)
				{
					tk[i++] ^= tk[j++];
				}
				tt = tk[kc / 2 - 1];
				tk[kc / 2] ^= (m_s[tt & 0xFF] & 0xFF) ^
					(m_s[(tt >> 8) & 0xFF] & 0xFF) << 8 ^
					(m_s[(tt >> 16) & 0xFF] & 0xFF) << 16 ^
					(m_s[(tt >> 24) & 0xFF] & 0xFF) << 24;
				for (j = kc / 2, i = j + 1; i < kc;)
				{
					tk[i++] ^= tk[j++];
				}
			}
			//Copy values into round key arrays
			for (j = 0; (j < kc) && (t < roundKeyCount); j++, t++)
			{
				outKey.m_ke[t / bc][t%bc] = tk[j];
				outKey.m_kd[outKey.m_rounds - (t / bc)][t%bc] = tk[j];
			}
		}
		//?
		//Inverse MixColumn where needed
		for (int r = 1; r < outKey.m_rounds; r++)
		{
			for (j = 0; j < bc; j++)
			{
				tt = outKey.m_kd[r][j];
				outKey.m_kd[r][j] = m_u1[(tt >> 24) & 0xFF] ^
					m_u2[(tt >> 16) & 0xFF] ^
					m_u3[(tt >> 8) & 0xFF] ^
					m_u4[tt & 0xFF];
			}
		}

		outKey.m_keyLength = keyLength;

		return true;
	}

	int CCryptoAes::GetEncryptSize(const CCryptoAesKey &key, int inputLength)
	{
		// 블럭암호화 이므로 padding 이 들어가며 padding 안에는 crc값, padding사이즈를 넣습니다.
		uint8_t paddingSize = uint8_t(key.GetBlockSize() - ((inputLength + sizeof(Crc32) + sizeof(paddingSize)) % key.GetBlockSize()));

		return inputLength + sizeof(Crc32)+sizeof(paddingSize)+paddingSize;
	}

	bool CCryptoAes::Encrypt(
		const CCryptoAesKey &key,
		const uint8_t* const input,
		int inputLength,
		uint8_t *output,
		int &outputLength,
		uint8_t * initializationVector /*= NULL*/,
		int initializationVectorLength /*= 0*/,
		EncryptMode encryptMode /*= ECB*/
		)
	{
		if (key.KeyExists() != true)
		{
			// aes key is not initialized
			return false;
		}

		if (key.GetBlockSize() == 0)
		{
			return false;
		}

		// 암호화할 데이터는 BlockSize 로 나누어져야 하므로 padding 사이즈를 구합니다.
		uint8_t paddingSize = key.GetBlockSize() - ((inputLength + sizeof(Crc32) + sizeof(paddingSize)) % key.GetBlockSize());

		if (outputLength == 0 || outputLength < inputLength + (int)(sizeof(Crc32) + sizeof(paddingSize) + paddingSize))
		{
			return false;
		}

		// 암호화가 끝난후의 데이터 사이즈입니다.
		outputLength = inputLength + sizeof(Crc32) + sizeof(paddingSize) + paddingSize;

		// crc 값을 구합니다.
		Crc32 crc = CCrc::Crc32(input, inputLength);

		// input 데이터를 output 에 복사합니다.
		memcpy(output, input, inputLength);

		// padding 을 포함한 끝 부분에 crc, padding 를 넣습니다.
		memcpy(output + inputLength + paddingSize, &crc, sizeof(crc));
		memcpy(output + inputLength + paddingSize + sizeof(crc), &paddingSize, sizeof(paddingSize));

		// 이제 암호화를 시작합니다.

		// 채인 사용모드 앞의 문장과 xor연산을 함으로서 모든 블럭이 연계된다.
		// CBC와 CFB의 차이는 Xor를 먼저 하느냐 아니면 암호화 후에 하느냐의 차이가 있다.
		// 한비트의 오류가 전체 블럭에 파급을 주게되나 암호화의 안정성이 높아진다.
		int position = 0;
		int total = outputLength / key.GetBlockSize();

		switch (encryptMode)
		{
		case ECB:
		{
			//ECB mode, not using the Chain
			for (int i = 0; i < total; i++)
			{
				EncryptBlock(key, output + position, output + position);
				position += key.GetBlockSize();
			}
		}
		break;
		case CBC:
			// CBC mode, using the Chain
		{
			uint8_t temp[MAX_BLOCK_SIZE] = { 0, };
			if (initializationVector != NULL && initializationVectorLength > 0)
			{
				memcpy(temp, initializationVector, PNMIN(MAX_BLOCK_SIZE, initializationVectorLength));
			}
			for (int i = 0; i < total; i++)
			{
				Xor(temp, output + position, key.m_blockSize);
				EncryptBlock(key, temp, output + position);
				memcpy(temp, output + position, key.m_blockSize);
				position += key.GetBlockSize();
			}
		}
		break;
		case CFB:
			// CFB mode, using the Chain
		{
			uint8_t temp[MAX_BLOCK_SIZE] = { 0, };
			if (initializationVector != NULL && initializationVectorLength > 0)
			{
				memcpy(temp, initializationVector, PNMIN(MAX_BLOCK_SIZE, initializationVectorLength));
			}

			// chain 으로 암호화 후 input + crc + padding 으로 다시 XOR을 해야 하므로 미리 만들어둔 output 의 클론을 떠둡니다.
			ByteArray outputClone;
			outputClone.SetCount(outputLength);
			memcpy(outputClone.GetData(), output, outputLength);
			for (int i = 0; i < total; i++)
			{
				EncryptBlock(key, temp, output + position);
				Xor(output + position, outputClone.GetData() + position, key.m_blockSize);
				memcpy(temp, output + position, key.m_blockSize);
				position += key.GetBlockSize();
			}
		}
		break;
		default:
			return false;
		}

		return true;
	}

  bool CCryptoAes::Decrypt(
	  const CCryptoAesKey &key, 
	  const uint8_t* const input,
	  int inputLength, 
	  uint8_t *output,
	  int &outputLength, 
	  uint8_t * initializationVector /*= NULL*/,
	  int initializationVectorLength /*= 0*/, 
	  EncryptMode encryptMode /*= ECB*/
	  )
	{
		if (key.GetBlockSize() == 0)
		{
			return false;
		}

		// 복호화할 데이터가 BlockSize 로 나누어지는지 확인합니다.
		if (inputLength == 0 || inputLength % key.GetBlockSize())
		{
			return false;
		}

		// 복호화 후 데이터의 길이가 충분한지 확인합니다.
		if (outputLength < inputLength)
		{
			// 실제 복호화후 사이즈는 작아지지만 복호화시 inputLength사이즈가 필요합니다.
			return false;
		}

		// 이제 복호화를 시작합니다.

		int position = 0;
		int total = inputLength / key.GetBlockSize();

		switch (encryptMode)
		{
		case ECB:
		{
			//ECB mode, not using the Chain
			for (int i = 0; i < total; i++)
			{
				DecryptBlock(key, input + position, output + position);
				position += key.GetBlockSize();
			}
		}
		break;
		case CBC:
			// CBC mode, using the Chain
		{
					uint8_t temp[MAX_BLOCK_SIZE] = { 0, };
 					if (initializationVector != NULL && initializationVectorLength > 0)
 					{
						memcpy(temp, initializationVector, PNMIN(MAX_BLOCK_SIZE, initializationVectorLength));
 					}

			// FIXME 초기 chain값을 조정 가능
			for (int i = 0; i < total; i++)
			{
				DecryptBlock(key, input + position, output + position);
				Xor(output + position, temp, key.GetBlockSize());
				memcpy(temp, input + position, key.m_blockSize);
				position += key.GetBlockSize();
			}
		}
		break;
		case CFB:
			// CFB mode, using the Chain
		{
			uint8_t temp[MAX_BLOCK_SIZE] = { 0, };
			if (initializationVector != NULL && initializationVectorLength > 0)
			{
				memcpy(temp, initializationVector, PNMIN(MAX_BLOCK_SIZE, initializationVectorLength));
			}

			for (int i = 0; i < total; i++)
			{
				EncryptBlock(key, temp, output + position);
				Xor(output + position, input + position, key.m_blockSize);
				memcpy(temp, input + position, key.m_blockSize);
				position += key.GetBlockSize();
			}
		}
		break;
		default:
			return false;
		}

		// crc 값을 체크합니다.
		Crc32 crc;
		uint8_t paddingSize;
		memcpy(&crc, output + inputLength - sizeof(crc) - sizeof(paddingSize), sizeof(crc));
		memcpy(&paddingSize, output + inputLength - sizeof(paddingSize), sizeof(paddingSize));

		outputLength = inputLength - paddingSize - sizeof(crc) - sizeof(paddingSize);

		if (outputLength < 0)
			return false;

		Crc32 newCrc = CCrc::Crc32(output, outputLength);
		if (crc != newCrc)
		{
			return false;
		}

		return true;
	}

  bool CCryptoAes::EncryptByteArray(
	  const CCryptoAesKey &key, 
	  const ByteArray &input, 
	  ByteArray &output, 
	  uint8_t * initializationVector /*= NULL*/,
	  int initializationVectorLength /*= 0*/, 
	  EncryptMode encryptMode /*= ECB*/
	  )
	{
		int outputLength = CCryptoAes::GetEncryptSize(key, (int)input.GetCount());
		output.SetCount(outputLength);
		return CCryptoAes::Encrypt(key, input.GetData(), (int)input.GetCount(), output.GetData(), outputLength, initializationVector, initializationVectorLength, encryptMode);
	}

  bool CCryptoAes::DecryptByteArray(
	  const CCryptoAesKey &key, 
	  const ByteArray &input, 
	  ByteArray &output, 
	  uint8_t * initializationVector /*= NULL*/,
	  int initializationVectorLength /*= 0*/, 
	  EncryptMode encryptMode /*= ECB*/
	  )
	{
		int outputLength = (int)input.GetCount();
		output.SetCount(outputLength);
		if (CCryptoAes::Decrypt(key, input.GetData(), outputLength, output.GetData(), outputLength, initializationVector, initializationVectorLength, encryptMode) != true)
		{
			return false;
		}
		output.SetCount(outputLength);
		return true;
	}

  bool CCryptoAes::EncryptMessage(
	  const CCryptoAesKey &key, 
	  const CMessage &input, 
	  CMessage &output, 
	  int offset, 
	  uint8_t * initializationVector /*= NULL*/,
	  int initializationVectorLength /*= 0*/, 
	  EncryptMode encryptMode /*= ECB*/
	  )
	{
		if (input.GetLength() - offset <= 0)
		{
			return false;
		}

		int outputLength = CCryptoAes::GetEncryptSize(key, input.GetLength() - offset);
		output.SetLength(outputLength);

		return CCryptoAes::Encrypt(key, input.GetData() + offset, input.GetLength() - offset, output.GetData(), outputLength, initializationVector, initializationVectorLength, encryptMode);
	}

  bool CCryptoAes::DecryptMessage(
	  const CCryptoAesKey &key, 
	  const CMessage &input, 
	  CMessage &output, 
	  int offset, 
	  uint8_t * initializationVector /*= NULL*/,
	  int initializationVectorLength /*= 0*/, 
	  EncryptMode encryptMode /*= ECB*/
	  )
	{
		int outputLength = input.GetLength() - offset;
		if (outputLength <= 0)
		{
			return false;
		}

		output.SetLength(outputLength);
		//int tmpsize = outputLength;
		if (CCryptoAes::Decrypt(key, input.GetData() + offset, input.GetLength() - offset, output.GetData(), outputLength, initializationVector, initializationVectorLength, encryptMode) != true)
		{
			return false;
		}
		output.SetLength(outputLength);
		return true;
	}

	/** Auxiliary Functions */
	/** Multiply two elements of GF(2^m) */
	int CCryptoAes::Mul(int a, int b)
	{
		return (a != 0 && b != 0) ? m_alog[(m_log[a & 0xFF] + m_log[b & 0xFF]) % 255] : 0;
	}

	/** Convenience method used in generating TransPosition Boxes */
	int CCryptoAes::Mul4(int a, char b[])
	{
		if (a == 0)
		{
			return 0;
		}
		a = m_log[a & 0xFF];
		int a0 = (b[0] != 0) ? m_alog[(a + m_log[b[0] & 0xFF]) % 255] & 0xFF : 0;
		int a1 = (b[1] != 0) ? m_alog[(a + m_log[b[1] & 0xFF]) % 255] & 0xFF : 0;
		int a2 = (b[2] != 0) ? m_alog[(a + m_log[b[2] & 0xFF]) % 255] & 0xFF : 0;
		int a3 = (b[3] != 0) ? m_alog[(a + m_log[b[3] & 0xFF]) % 255] & 0xFF : 0;
		return a0 << 24 | a1 << 16 | a2 << 8 | a3;
	}

	//Auxiliary Function
	void CCryptoAes::Xor(uint8_t *buff, const uint8_t* chain, const int blockSize)
	{
		for (int i = 0; i < blockSize; i++)
		{
			*(buff++) ^= *(chain++);
		}
	}

	// Convenience method to encrypt exactly one block of plaintext, assuming
	// Rijndael's default block size (128-bit).
	// in         - The plaintext
	// result     - The ciphertext generated from a plaintext using the key
	bool CCryptoAes::DefaultEncryptBlock(const CCryptoAesKey &key, const uint8_t* in, uint8_t *result)
	{
		if (key.KeyExists() == false)
		{
			// aes key is not initialized
			return false;
		}

		const int* keRound = key.m_ke[0];
		int t0 = (*(in++) << 24);
		t0 |= (*(in++) << 16);
		t0 |= (*(in++) << 8);
		(t0 |= *(in++)) ^= keRound[0];
		int t1 = (*(in++) << 24);
		t1 |= (*(in++) << 16);
		t1 |= (*(in++) << 8);
		(t1 |= *(in++)) ^= keRound[1];
		int t2 = (*(in++) << 24);
		t2 |= (*(in++) << 16);
		t2 |= (*(in++) << 8);
		(t2 |= *(in++)) ^= keRound[2];
		int t3 = (*(in++) << 24);
		t3 |= (*(in++) << 16);
		t3 |= (*(in++) << 8);
		(t3 |= *(in++)) ^= keRound[3];
		int a0, a1, a2, a3;
		//Apply Round Transforms
		for (int r = 1; r < key.m_rounds; r++)
		{
			keRound = key.m_ke[r];
			a0 = (m_t1[(t0 >> 24) & 0xFF] ^
				m_t2[(t1 >> 16) & 0xFF] ^
				m_t3[(t2 >> 8) & 0xFF] ^
				m_t4[t3 & 0xFF]) ^ keRound[0];
			a1 = (m_t1[(t1 >> 24) & 0xFF] ^
				m_t2[(t2 >> 16) & 0xFF] ^
				m_t3[(t3 >> 8) & 0xFF] ^
				m_t4[t0 & 0xFF]) ^ keRound[1];
			a2 = (m_t1[(t2 >> 24) & 0xFF] ^
				m_t2[(t3 >> 16) & 0xFF] ^
				m_t3[(t0 >> 8) & 0xFF] ^
				m_t4[t1 & 0xFF]) ^ keRound[2];
			a3 = (m_t1[(t3 >> 24) & 0xFF] ^
				m_t2[(t0 >> 16) & 0xFF] ^
				m_t3[(t1 >> 8) & 0xFF] ^
				m_t4[t2 & 0xFF]) ^ keRound[3];
			t0 = a0;
			t1 = a1;
			t2 = a2;
			t3 = a3;
		}
		//Last Round is special
		keRound = key.m_ke[key.m_rounds];
		int tt = keRound[0];
		result[0] = m_s[(t0 >> 24) & 0xFF] ^ (tt >> 24);
		result[1] = m_s[(t1 >> 16) & 0xFF] ^ (tt >> 16);
		result[2] = m_s[(t2 >> 8) & 0xFF] ^ (tt >> 8);
		result[3] = m_s[t3 & 0xFF] ^ tt;
		tt = keRound[1];
		result[4] = m_s[(t1 >> 24) & 0xFF] ^ (tt >> 24);
		result[5] = m_s[(t2 >> 16) & 0xFF] ^ (tt >> 16);
		result[6] = m_s[(t3 >> 8) & 0xFF] ^ (tt >> 8);
		result[7] = m_s[t0 & 0xFF] ^ tt;
		tt = keRound[2];
		result[8] = m_s[(t2 >> 24) & 0xFF] ^ (tt >> 24);
		result[9] = m_s[(t3 >> 16) & 0xFF] ^ (tt >> 16);
		result[10] = m_s[(t0 >> 8) & 0xFF] ^ (tt >> 8);
		result[11] = m_s[t1 & 0xFF] ^ tt;
		tt = keRound[3];
		result[12] = m_s[(t3 >> 24) & 0xFF] ^ (tt >> 24);
		result[13] = m_s[(t0 >> 16) & 0xFF] ^ (tt >> 16);
		result[14] = m_s[(t1 >> 8) & 0xFF] ^ (tt >> 8);
		result[15] = m_s[t2 & 0xFF] ^ tt;

		return true;
	}

	// Convenience method to decrypt exactly one block of plaintext, assuming
	// Rijndael's default block size (128-bit).
	// in         - The ciphertext.
	// result     - The plaintext generated from a ciphertext using the session key.
	bool CCryptoAes::DefaultDecryptBlock(const CCryptoAesKey &key, const uint8_t* in, uint8_t *result)
	{
		if (key.KeyExists() == false)
		{
			// aes key is not initialized
			return false;
		}

		const int* Kdr = key.m_kd[0];
		int t0 = (*(in++) << 24);
		t0 = t0 | (*(in++) << 16);
		t0 |= (*(in++) << 8);
		(t0 |= *(in++)) ^= Kdr[0];
		int t1 = (*(in++) << 24);
		t1 |= (*(in++) << 16);
		t1 |= (*(in++) << 8);
		(t1 |= *(in++)) ^= Kdr[1];
		int t2 = (*(in++) << 24);
		t2 |= (*(in++) << 16);
		t2 |= (*(in++) << 8);
		(t2 |= *(in++)) ^= Kdr[2];
		int t3 = (*(in++) << 24);
		t3 |= (*(in++) << 16);
		t3 |= (*(in++) << 8);
		(t3 |= *(in++)) ^= Kdr[3];
		int a0, a1, a2, a3;
		for (int r = 1; r < key.m_rounds; r++) // apply round transforms
		{
			Kdr = key.m_kd[r];
			a0 = (m_t5[(t0 >> 24) & 0xFF] ^
				m_t6[(t3 >> 16) & 0xFF] ^
				m_t7[(t2 >> 8) & 0xFF] ^
				m_t8[t1 & 0xFF]) ^ Kdr[0];
			a1 = (m_t5[(t1 >> 24) & 0xFF] ^
				m_t6[(t0 >> 16) & 0xFF] ^
				m_t7[(t3 >> 8) & 0xFF] ^
				m_t8[t2 & 0xFF]) ^ Kdr[1];
			a2 = (m_t5[(t2 >> 24) & 0xFF] ^
				m_t6[(t1 >> 16) & 0xFF] ^
				m_t7[(t0 >> 8) & 0xFF] ^
				m_t8[t3 & 0xFF]) ^ Kdr[2];
			a3 = (m_t5[(t3 >> 24) & 0xFF] ^
				m_t6[(t2 >> 16) & 0xFF] ^
				m_t7[(t1 >> 8) & 0xFF] ^
				m_t8[t0 & 0xFF]) ^ Kdr[3];
			t0 = a0;
			t1 = a1;
			t2 = a2;
			t3 = a3;
		}
		//Last Round is special
		Kdr = key.m_kd[key.m_rounds];
		int tt = Kdr[0];
		result[0] = m_si[(t0 >> 24) & 0xFF] ^ (tt >> 24);
		result[1] = m_si[(t3 >> 16) & 0xFF] ^ (tt >> 16);
		result[2] = m_si[(t2 >> 8) & 0xFF] ^ (tt >> 8);
		result[3] = m_si[t1 & 0xFF] ^ tt;
		tt = Kdr[1];
		result[4] = m_si[(t1 >> 24) & 0xFF] ^ (tt >> 24);
		result[5] = m_si[(t0 >> 16) & 0xFF] ^ (tt >> 16);
		result[6] = m_si[(t3 >> 8) & 0xFF] ^ (tt >> 8);
		result[7] = m_si[t2 & 0xFF] ^ tt;
		tt = Kdr[2];
		result[8] = m_si[(t2 >> 24) & 0xFF] ^ (tt >> 24);
		result[9] = m_si[(t1 >> 16) & 0xFF] ^ (tt >> 16);
		result[10] = m_si[(t0 >> 8) & 0xFF] ^ (tt >> 8);
		result[11] = m_si[t3 & 0xFF] ^ tt;
		tt = Kdr[3];
		result[12] = m_si[(t3 >> 24) & 0xFF] ^ (tt >> 24);
		result[13] = m_si[(t2 >> 16) & 0xFF] ^ (tt >> 16);
		result[14] = m_si[(t1 >> 8) & 0xFF] ^ (tt >> 8);
		result[15] = m_si[t0 & 0xFF] ^ tt;

		return true;
	}

	//Encrypt exactly one block of plaintext.
	// in           - The plaintext.
	// result       - The ciphertext generated from a plaintext using the key.
	bool CCryptoAes::EncryptBlock(const CCryptoAesKey &key, const uint8_t* in, uint8_t *result)
	{
		if (key.KeyExists() == false)
		{
			// aes key is not initialized
			return false;
		}

		if (key.m_blockSize == DEFAULT_BLOCK_SIZE)
		{
			return DefaultEncryptBlock(key, in, result);
		}

		int bc = key.m_blockSize / 4;
		int sc = (bc == 4) ? 0 : (bc == 6 ? 1 : 2);
		int s1 = m_shifts[sc][1][0];
		int s2 = m_shifts[sc][2][0];
		int s3 = m_shifts[sc][3][0];
		//Temporary Work Arrays
		int i;
		int tt;

		//Auxiliary private use buffers
		int t[MAX_BC];

		int* pi = t;
		for (i = 0; i < bc; i++)
		{
			*pi = (*(in++) << 24);
			*pi |= (*(in++) << 16);
			*pi |= (*(in++) << 8);
			(*(pi++) |= *(in++)) ^= key.m_ke[0][i];
		}
		//Auxiliary private use buffers
		int a[MAX_BC];
		//Apply Round Transforms
		for (int r = 1; r < key.m_rounds; r++)
		{
			for (i = 0; i < bc; i++)
				a[i] = (m_t1[(t[i] >> 24) & 0xFF] ^
				m_t2[(t[(i + s1) % bc] >> 16) & 0xFF] ^
				m_t3[(t[(i + s2) % bc] >> 8) & 0xFF] ^
				m_t4[t[(i + s3) % bc] & 0xFF]) ^ key.m_ke[r][i];
			memcpy(t, a, 4 * bc);
		}
		int j;
		//Last Round is Special
		for (i = 0, j = 0; i < bc; i++)
		{
			tt = key.m_ke[key.m_rounds][i];
			result[j++] = m_s[(t[i] >> 24) & 0xFF] ^ (tt >> 24);
			result[j++] = m_s[(t[(i + s1) % bc] >> 16) & 0xFF] ^ (tt >> 16);
			result[j++] = m_s[(t[(i + s2) % bc] >> 8) & 0xFF] ^ (tt >> 8);
			result[j++] = m_s[t[(i + s3) % bc] & 0xFF] ^ tt;
		}

		return true;
	}

	//Decrypt exactly one block of ciphertext.
	// in         - The ciphertext.
	// result     - The plaintext generated from a ciphertext using the session key.
	bool CCryptoAes::DecryptBlock(const CCryptoAesKey &key, const uint8_t* in, uint8_t *result)
	{
		if (key.KeyExists() == false)
		{
			// aes key is not initialized
			return false;
		}

		if (DEFAULT_BLOCK_SIZE == key.m_blockSize)
		{
			return DefaultDecryptBlock(key, in, result);
		}

		int bc = key.m_blockSize / 4;
		int sc = bc == 4 ? 0 : (bc == 6 ? 1 : 2);
		int s1 = m_shifts[sc][1][1];
		int s2 = m_shifts[sc][2][1];
		int s3 = m_shifts[sc][3][1];
		//Temporary Work Arrays
		int i;
		int tt;

		//Auxiliary private use buffers
		int t[MAX_BC];

		int* pi = t;
		for (i = 0; i < bc; i++)
		{
			*pi = (*(in++) << 24);
			*pi |= (*(in++) << 16);
			*pi |= (*(in++) << 8);
			(*(pi++) |= *(in++)) ^= key.m_kd[0][i];
		}
		//Auxiliary private use buffers
		int a[MAX_BC];
		//Apply Round Transforms
		for (int r = 1; r < key.m_rounds; r++)
		{
			for (i = 0; i < bc; i++)
			{
				a[i] = (m_t5[(t[i] >> 24) & 0xFF] ^
					m_t6[(t[(i + s1) % bc] >> 16) & 0xFF] ^
					m_t7[(t[(i + s2) % bc] >> 8) & 0xFF] ^
					m_t8[t[(i + s3) % bc] & 0xFF]) ^ key.m_kd[r][i];
			}
			memcpy(t, a, 4 * bc);
		}
		int j;
		//Last Round is Special
		for (i = 0, j = 0; i < bc; i++)
		{
			tt = key.m_kd[key.m_rounds][i];
			result[j++] = m_si[(t[i] >> 24) & 0xFF] ^ (tt >> 24);
			result[j++] = m_si[(t[(i + s1) % bc] >> 16) & 0xFF] ^ (tt >> 16);
			result[j++] = m_si[(t[(i + s2) % bc] >> 8) & 0xFF] ^ (tt >> 8);
			result[j++] = m_si[t[(i + s3) % bc] & 0xFF] ^ tt;
		}

		return true;
	}

	int CCryptoAesKey::GetKeyLength() const
	{
		if (KeyExists() == false)
		{
			return -1;
		}

		return m_keyLength;
	}

	int	CCryptoAesKey::GetBlockSize() const
	{
		if (KeyExists() == false)
		{
			return -1;
		}

		return m_blockSize;
	}

	//Number of Rounds
	int CCryptoAesKey::GetRounds() const
	{
		if (KeyExists() == false)
		{
			return -1;
		}

		return m_rounds;
	}

	// Is Key Init?
	bool CCryptoAesKey::KeyExists() const
	{
		if (m_keyLength > 0)
		{
			return true;
		}
		else {
			return false;
		}
	}

	void CCryptoAesKey::Clear()
	{
		m_keyLength = 0;
	}

	const int CCryptoAes::m_alog[256] =
	{
		1, 3, 5, 15, 17, 51, 85, 255, 26, 46, 114, 150, 161, 248, 19, 53,
		95, 225, 56, 72, 216, 115, 149, 164, 247, 2, 6, 10, 30, 34, 102, 170,
		229, 52, 92, 228, 55, 89, 235, 38, 106, 190, 217, 112, 144, 171, 230, 49,
		83, 245, 4, 12, 20, 60, 68, 204, 79, 209, 104, 184, 211, 110, 178, 205,
		76, 212, 103, 169, 224, 59, 77, 215, 98, 166, 241, 8, 24, 40, 120, 136,
		131, 158, 185, 208, 107, 189, 220, 127, 129, 152, 179, 206, 73, 219, 118, 154,
		181, 196, 87, 249, 16, 48, 80, 240, 11, 29, 39, 105, 187, 214, 97, 163,
		254, 25, 43, 125, 135, 146, 173, 236, 47, 113, 147, 174, 233, 32, 96, 160,
		251, 22, 58, 78, 210, 109, 183, 194, 93, 231, 50, 86, 250, 21, 63, 65,
		195, 94, 226, 61, 71, 201, 64, 192, 91, 237, 44, 116, 156, 191, 218, 117,
		159, 186, 213, 100, 172, 239, 42, 126, 130, 157, 188, 223, 122, 142, 137, 128,
		155, 182, 193, 88, 232, 35, 101, 175, 234, 37, 111, 177, 200, 67, 197, 84,
		252, 31, 33, 99, 165, 244, 7, 9, 27, 45, 119, 153, 176, 203, 70, 202,
		69, 207, 74, 222, 121, 139, 134, 145, 168, 227, 62, 66, 198, 81, 243, 14,
		18, 54, 90, 238, 41, 123, 141, 140, 143, 138, 133, 148, 167, 242, 13, 23,
		57, 75, 221, 124, 132, 151, 162, 253, 28, 36, 108, 180, 199, 82, 246, 1
	};

	const int CCryptoAes::m_log[256] =
	{
		0, 0, 25, 1, 50, 2, 26, 198, 75, 199, 27, 104, 51, 238, 223, 3,
		100, 4, 224, 14, 52, 141, 129, 239, 76, 113, 8, 200, 248, 105, 28, 193,
		125, 194, 29, 181, 249, 185, 39, 106, 77, 228, 166, 114, 154, 201, 9, 120,
		101, 47, 138, 5, 33, 15, 225, 36, 18, 240, 130, 69, 53, 147, 218, 142,
		150, 143, 219, 189, 54, 208, 206, 148, 19, 92, 210, 241, 64, 70, 131, 56,
		102, 221, 253, 48, 191, 6, 139, 98, 179, 37, 226, 152, 34, 136, 145, 16,
		126, 110, 72, 195, 163, 182, 30, 66, 58, 107, 40, 84, 250, 133, 61, 186,
		43, 121, 10, 21, 155, 159, 94, 202, 78, 212, 172, 229, 243, 115, 167, 87,
		175, 88, 168, 80, 244, 234, 214, 116, 79, 174, 233, 213, 231, 230, 173, 232,
		44, 215, 117, 122, 235, 22, 11, 245, 89, 203, 95, 176, 156, 169, 81, 160,
		127, 12, 246, 111, 23, 196, 73, 236, 216, 67, 31, 45, 164, 118, 123, 183,
		204, 187, 62, 90, 251, 96, 177, 134, 59, 82, 161, 108, 170, 85, 41, 157,
		151, 178, 135, 144, 97, 190, 220, 252, 188, 149, 207, 205, 55, 63, 91, 209,
		83, 57, 132, 60, 65, 162, 109, 71, 20, 42, 158, 93, 86, 242, 211, 171,
		68, 17, 146, 217, 35, 32, 46, 137, 180, 124, 184, 38, 119, 153, 227, 165,
		103, 74, 237, 222, 197, 49, 254, 24, 13, 99, 140, 128, 192, 247, 112, 7
	};

	/** Encrypt Key S-Box */
	const int8_t CCryptoAes::m_s[256] =
	{
		99, 124, 119, 123, -14, 107, 111, -59, 48, 1, 103, 43, -2, -41, -85, 118,
		-54, -126, -55, 125, -6, 89, 71, -16, -83, -44, -94, -81, -100, -92, 114, -64,
		-73, -3, -109, 38, 54, 63, -9, -52, 52, -91, -27, -15, 113, -40, 49, 21,
		4, -57, 35, -61, 24, -106, 5, -102, 7, 18, -128, -30, -21, 39, -78, 117,
		9, -125, 44, 26, 27, 110, 90, -96, 82, 59, -42, -77, 41, -29, 47, -124,
		83, -47, 0, -19, 32, -4, -79, 91, 106, -53, -66, 57, 74, 76, 88, -49,
		-48, -17, -86, -5, 67, 77, 51, -123, 69, -7, 2, 127, 80, 60, -97, -88,
		81, -93, 64, -113, -110, -99, 56, -11, -68, -74, -38, 33, 16, -1, -13, -46,
		-51, 12, 19, -20, 95, -105, 68, 23, -60, -89, 126, 61, 100, 93, 25, 115,
		96, -127, 79, -36, 34, 42, -112, -120, 70, -18, -72, 20, -34, 94, 11, -37,
		-32, 50, 58, 10, 73, 6, 36, 92, -62, -45, -84, 98, -111, -107, -28, 121,
		-25, -56, 55, 109, -115, -43, 78, -87, 108, 86, -12, -22, 101, 122, -82, 8,
		-70, 120, 37, 46, 28, -90, -76, -58, -24, -35, 116, 31, 75, -67, -117, -118,
		112, 62, -75, 102, 72, 3, -10, 14, 97, 53, 87, -71, -122, -63, 29, -98,
		-31, -8, -104, 17, 105, -39, -114, -108, -101, 30, -121, -23, -50, 85, 40, -33,
		-116, -95, -119, 13, -65, -26, 66, 104, 65, -103, 45, 15, -80, 84, -69, 22
	};

	/** Decrypt Key S-Box */
	const int8_t CCryptoAes::m_si[256] =
	{
		82, 9, 106, -43, 48, 54, -91, 56, -65, 64, -93, -98, -127, -13, -41, -5,
		124, -29, 57, -126, -101, 47, -1, -121, 52, -114, 67, 68, -60, -34, -23, -53,
		84, 123, -108, 50, -90, -62, 35, 61, -18, 76, -107, 11, 66, -6, -61, 78,
		8, 46, -95, 102, 40, -39, 36, -78, 118, 91, -94, 73, 109, -117, -47, 37,
		114, -8, -10, 100, -122, 104, -104, 22, -44, -92, 92, -52, 93, 101, -74, -110,
		108, 112, 72, 80, -3, -19, -71, -38, 94, 21, 70, 87, -89, -115, -99, -124,
		-112, -40, -85, 0, -116, -68, -45, 10, -9, -28, 88, 5, -72, -77, 69, 6,
		-48, 44, 30, -113, -54, 63, 15, 2, -63, -81, -67, 3, 1, 19, -118, 107,
		58, -111, 17, 65, 79, 103, -36, -22, -105, -14, -49, -50, -16, -76, -26, 115,
		-106, -84, 116, 34, -25, -83, 53, -123, -30, -7, 55, -24, 28, 117, -33, 110,
		71, -15, 26, 113, 29, 41, -59, -119, 111, -73, 98, 14, -86, 24, -66, 27,
		-4, 86, 62, 75, -58, -46, 121, 32, -102, -37, -64, -2, 120, -51, 90, -12,
		31, -35, -88, 51, -120, 7, -57, 49, -79, 18, 16, 89, 39, -128, -20, 95,
		96, 81, 127, -87, 25, -75, 74, 13, 45, -27, 122, -97, -109, -55, -100, -17,
		-96, -32, 59, 77, -82, 42, -11, -80, -56, -21, -69, 60, -125, 83, -103, 97,
		23, 43, 4, 126, -70, 119, -42, 38, -31, 105, 20, 99, 85, 33, 12, 125
	};

	/** Encrypt Message S-Box 1~4 */
	const int CCryptoAes::m_t1[256] =
	{
		-966564955, -126059388, -294160487, -159679603,
		-855539, -697603139, -563122255, -1849309868,
		1613770832, 33620227, -832084055, 1445669757,
		-402719207, -1244145822, 1303096294, -327780710,
		-1882535355, 528646813, -1983264448, -92439161,
		-268764651, -1302767125, -1907931191, -68095989,
		1101901292, -1277897625, 1604494077, 1169141738,
		597466303, 1403299063, -462261610, -1681866661,
		1974974402, -503448292, 1033081774, 1277568618,
		1815492186, 2118074177, -168298750, -2083730353,
		1748251740, 1369810420, -773462732, -101584632,
		-495881837, -1411852173, 1647391059, 706024767,
		134480908, -1782069422, 1176707941, -1648114850,
		806885416, 932615841, 168101135, 798661301,
		235341577, 605164086, 461406363, -538779075,
		-840176858, 1311188841, 2142417613, -361400929,
		302582043, 495158174, 1479289972, 874125870,
		907746093, -596742478, -1269146898, 1537253627,
		-1538108682, 1983593293, -1210657183, 2108928974,
		1378429307, -572267714, 1580150641, 327451799,
		-1504488459, -1177431704, 0, -1041371860,
		1075847264, -469959649, 2041688520, -1235526675,
		-731223362, -1916023994, 1740553945, 1916352843,
		-1807070498, -1739830060, -1336387352, -2049978550,
		-1143943061, -974131414, 1336584933, -302253290,
		-2042412091, -1706209833, 1714631509, 293963156,
		-1975171633, -369493744, 67240454, -25198719,
		-1605349136, 2017213508, 631218106, 1269344483,
		-1571728909, 1571005438, -2143272768, 93294474,
		1066570413, 563977660, 1882732616, -235539196,
		1673313503, 2008463041, -1344611723, 1109467491,
		537923632, -436207846, -34344178, -1076702611,
		-2117218996, 403442708, 638784309, -1007883217,
		-1101045791, 899127202, -2008791860, 773265209,
		-1815821225, 1437050866, -58818942, 2050833735,
		-932944724, -1168286233, 840505643, -428641387,
		-1067425632, 427917720, -1638969391, -1545806721,
		1143087718, 1412049534, 999329963, 193497219,
		-1941551414, -940642775, 1807268051, 672404540,
		-1478566279, -1134666014, 369822493, -1378100362,
		-606019525, 1681011286, 1949973070, 336202270,
		-1840690725, 201721354, 1210328172, -1201906460,
		-1614626211, -1110191250, 1135389935, -1000185178,
		965841320, 831886756, -739974089, -226920053,
		-706222286, -1949775805, 1849112409, -630362697,
		26054028, -1311386268, -1672589614, 1235855840,
		-663982924, -1403627782, -202050553, -806688219,
		-899324497, -193299826, 1202630377, 268961816,
		1874508501, -260540280, 1243948399, 1546530418,
		941366308, 1470539505, 1941222599, -1748580783,
		-873928669, -1579295364, -395021156, 1042226977,
		-1773450275, 1639824860, 227249030, 260737669,
		-529502064, 2084453954, 1907733956, -865704278,
		-1874310952, 100860677, -134810111, 470683154,
		-1033805405, 1781871967, -1370007559, 1773779408,
		394692241, -1715355304, 974986535, 664706745,
		-639508168, -336005101, 731420851, 571543859,
		-764843589, -1445340816, 126783113, 865375399,
		765172662, 1008606754, 361203602, -907417312,
		-2016489911, -1437248001, 1344809080, -1512054918,
		59542671, 1503764984, 160008576, 437062935,
		1707065306, -672733647, -2076032314, -798463816,
		-2109652541, 697932208, 1512910199, 504303377,
		2075177163, -1470868228, 1841019862, 739644986
	};

	const int CCryptoAes::m_t2[256] =
	{
		-1513725085, -2064089988, -1712425097, -1913226373,
		234877682, -1110021269, -1310822545, 1418839493,
		1348481072, 50462977, -1446090905, 2102799147,
		434634494, 1656084439, -431117397, -1695779210,
		1167051466, -1658879358, 1082771913, -2013627011,
		368048890, -340633255, -913422521, 201060592,
		-331240019, 1739838676, -44064094, -364531793,
		-1088185188, -145513308, -1763413390, 1536934080,
		-1032472649, 484572669, -1371696237, 1783375398,
		1517041206, 1098792767, 49674231, 1334037708,
		1550332980, -195975771, 886171109, 150598129,
		-1813876367, 1940642008, 1398944049, 1059722517,
		201851908, 1385547719, 1699095331, 1587397571,
		674240536, -1590192490, 252314885, -1255171430,
		151914247, 908333586, -1692696448, 1038082786,
		651029483, 1766729511, -847269198, -1612024459,
		454166793, -1642232957, 1951935532, 775166490,
		758520603, -1294176658, -290170278, -77881184,
		-157003182, 1299594043, 1639438038, -830622797,
		2068982057, 1054729187, 1901997871, -1760328572,
		-173649069, 1757008337, 0, 750906861,
		1614815264, 535035132, -931548751, -306816165,
		-1093375382, 1183697867, -647512386, 1265776953,
		-560706998, -728216500, -391096232, 1250283471,
		1807470800, 717615087, -447763798, 384695291,
		-981056701, -677753523, 1432761139, -1810791035,
		-813021883, 283769337, 100925954, -2114027649,
		-257929136, 1148730428, -1171939425, -481580888,
		-207466159, -27417693, -1065336768, -1979347057,
		-1388342638, -1138647651, 1215313976, 82966005,
		-547111748, -1049119050, 1974459098, 1665278241,
		807407632, 451280895, 251524083, 1841287890,
		1283575245, 337120268, 891687699, 801369324,
		-507617441, -1573546089, -863484860, 959321879,
		1469301956, -229267545, -2097381762, 1199193405,
		-1396153244, -407216803, 724703513, -1780059277,
		-1598005152, -1743158911, -778154161, 2141445340,
		1715741218, 2119445034, -1422159728, -2096396152,
		-896776634, 700968686, -747915080, 1009259540,
		2041044702, -490971554, 487983883, 1991105499,
		1004265696, 1449407026, 1316239930, 504629770,
		-611169975, 168560134, 1816667172, -457679780,
		1570751170, 1857934291, -280777556, -1497079198,
		-1472622191, -1540254315, 936633572, -1947043463,
		852879335, 1133234376, 1500395319, -1210421907,
		-1946055283, 1689376213, -761508274, -532043351,
		-1260884884, -89369002, 133428468, 634383082,
		-1345690267, -1896580486, -381178194, 403703816,
		-714097990, -1997506440, 1867130149, 1918643758,
		607656988, -245913946, -948718412, 1368901318,
		600565992, 2090982877, -1662487436, 557719327,
		-577352885, -597574211, -2045932661, -2062579062,
		-1864339344, 1115438654, -999180875, -1429445018,
		-661632952, 84280067, 33027830, 303828494,
		-1547542175, 1600795957, -106014889, -798377543,
		-1860729210, 1486471617, 658119965, -1188585826,
		953803233, 334231800, -1288988520, 857870609,
		-1143838359, 1890179545, -1995993458, -1489791852,
		-1238525029, 574365214, -1844082809, 550103529,
		1233637070, -5614251, 2018519080, 2057691103,
		-1895592820, -128343647, -2146858615, 387583245,
		-630865985, 836232934, -964410814, -1194301336,
		-1014873791, -1339450983, 2002398509, 287182607,
		-881086288, -56077228, -697451589, 975967766
	};

	const int CCryptoAes::m_t3[256] =
	{
		1671808611, 2089089148, 2006576759, 2072901243,
		-233963534, 1807603307, 1873927791, -984313403,
		810573872, 16974337, 1739181671, 729634347,
		-31856642, -681396777, -1410970197, 1989864566,
		-901410870, -2103631998, -918517303, 2106063485,
		-99225606, 1508618841, 1204391495, -267650064,
		-1377025619, -731401260, -1560453214, -1343601233,
		-1665195108, -1527295068, 1922491506, -1067738176,
		-1211992649, -48438787, -1817297517, 644500518,
		911895606, 1061256767, -150800905, -867204148,
		878471220, -1510714971, -449523227, -251069967,
		1905517169, -663508008, 827548209, 356461077,
		67897348, -950889017, 593839651, -1017209405,
		405286936, -1767819370, 84871685, -1699401830,
		118033927, 305538066, -2137318528, -499261470,
		-349778453, 661212711, -1295155278, 1973414517,
		152769033, -2086789757, 745822252, 439235610,
		455947803, 1857215598, 1525593178, -1594139744,
		1391895634, 994932283, -698239018, -1278313037,
		695947817, -482419229, 795958831, -2070473852,
		1408607827, -781665839, 0, -315833875,
		543178784, -65018884, -1312261711, 1542305371,
		1790891114, -884568629, -1093048386, 961245753,
		1256100938, 1289001036, 1491644504, -817199665,
		-798245936, -282409489, -1427812438, -82383365,
		1137018435, 1305975373, 861234739, -2053893755,
		1171229253, -116332039, 33948674, 2139225727,
		1357946960, 1011120188, -1615190625, -1461498968,
		1374921297, -1543610973, 1086357568, -1886780017,
		-1834139758, -1648615011, 944271416, -184225291,
		-1126210628, -1228834890, -629821478, 560153121,
		271589392, -15014401, -217121293, -764559406,
		-850624051, 202643468, 322250259, -332413972,
		1608629855, -1750977129, 1154254916, 389623319,
		-1000893500, -1477290585, 2122513534, 1028094525,
		1689045092, 1575467613, 422261273, 1939203699,
		1621147744, -2120738431, 1339137615, -595614756,
		577127458, 712922154, -1867826288, -2004677752,
		1187679302, -299251730, -1194103880, 339486740,
		-562452514, 1591917662, 186455563, -612979237,
		-532948000, 844522546, 978220090, 169743370,
		1239126601, 101321734, 611076132, 1558493276,
		-1034051646, -747717165, -1393605716, 1655096418,
		-1851246191, -1784401515, -466103324, 2039214713,
		-416098841, -935097400, 928607799, 1840765549,
		-1920204403, -714821163, 1322425422, -1444918871,
		1823791212, 1459268694, -200805388, -366620694,
		1706019429, 2056189050, -1360443474, 135794696,
		-1160417350, 2022240376, 628050469, 779246638,
		472135708, -1494132826, -1261997132, -967731258,
		-400307224, -579034659, 1956440180, 522272287,
		1272813131, -1109630531, -1954148981, -1970991222,
		1888542832, 1044544574, -1245417035, 1722469478,
		1222152264, 50660867, -167643146, 236067854,
		1638122081, 895445557, 1475980887, -1177523783,
		-2037311610, -1051158079, 489110045, -1632032866,
		-516367903, -132912136, -1733088360, 288563729,
		1773916777, -646927911, -1903622258, -1800981612,
		-1682559589, 505560094, -2020469369, -383727127,
		-834041906, 1442818645, 678973480, -545610273,
		-1936784500, -1577559647, -1988097655, 219617805,
		-1076206145, -432941082, 1120306242, 1756942440,
		1103331905, -1716508263, 762796589, 252780047,
		-1328841808, 1425844308, -1143575109, 372911126
	};

	const int CCryptoAes::m_t4[256] =
	{
		1667474886, 2088535288, 2004326894, 2071694838,
		-219017729, 1802223062, 1869591006, -976923503,
		808472672, 16843522, 1734846926, 724270422,
		-16901657, -673750347, -1414797747, 1987484396,
		-892713585, -2105369313, -909557623, 2105378810,
		-84273681, 1499065266, 1195886990, -252703749,
		-1381110719, -724277325, -1566376609, -1347425723,
		-1667449053, -1532692653, 1920112356, -1061135461,
		-1212693899, -33743647, -1819038147, 640051788,
		909531756, 1061110142, -134806795, -859025533,
		875846760, -1515850671, -437963567, -235861767,
		1903268834, -656903253, 825316194, 353713962,
		67374088, -943238507, 589522246, -1010606435,
		404236336, -1768513225, 84217610, -1701137105,
		117901582, 303183396, -2139055333, -488489505,
		-336910643, 656894286, -1296904833, 1970642922,
		151591698, -2088526307, 741110872, 437923380,
		454765878, 1852748508, 1515908788, -1600062629,
		1381168804, 993742198, -690593353, -1280061827,
		690584402, -471646499, 791638366, -2071685357,
		1398011302, -774805319, 0, -303223615,
		538992704, -50585629, -1313748871, 1532751286,
		1785380564, -875870579, -1094788761, 960056178,
		1246420628, 1280103576, 1482221744, -808498555,
		-791647301, -269538619, -1431640753, -67430675,
		1128514950, 1296947098, 859002214, -2054843375,
		1162203018, -101117719, 33687044, 2139062782,
		1347481760, 1010582648, -1616922075, -1465326773,
		1364325282, -1549533603, 1077985408, -1886418427,
		-1835881153, -1650607071, 943212656, -168491791,
		-1128472733, -1229536905, -623217233, 555836226,
		269496352, -58651, -202174723, -757961281,
		-842183551, 202118168, 320025894, -320065597,
		1600119230, -1751670219, 1145359496, 387397934,
		-993765485, -1482165675, 2122220284, 1027426170,
		1684319432, 1566435258, 421079858, 1936954854,
		1616945344, -2122213351, 1330631070, -589529181,
		572679748, 707427924, -1869567173, -2004319477,
		1179044492, -286381625, -1195846805, 336870440,
		-555845209, 1583276732, 185277718, -606374227,
		-522175525, 842159716, 976899700, 168435220,
		1229577106, 101059084, 606366792, 1549591736,
		-1027449441, -741118275, -1397952701, 1650632388,
		-1852725191, -1785355215, -454805549, 2038008818,
		-404278571, -926399605, 926374254, 1835907034,
		-1920103423, -707435343, 1313788572, -1448484791,
		1819063512, 1448540844, -185333773, -353753649,
		1701162954, 2054852340, -1364268729, 134748176,
		-1162160785, 2021165296, 623210314, 774795868,
		471606328, -1499008681, -1263220877, -960081513,
		-387439669, -572687199, 1953799400, 522133822,
		1263263126, -1111630751, -1953790451, -1970633457,
		1886425312, 1044267644, -1246378895, 1718004428,
		1212733584, 50529542, -151649801, 235803164,
		1633788866, 892690282, 1465383342, -1179004823,
		-2038001385, -1044293479, 488449850, -1633765081,
		-505333543, -117959701, -1734823125, 286339874,
		1768537042, -640061271, -1903261433, -1802197197,
		-1684294099, 505291324, -2021158379, -370597687,
		-825341561, 1431699370, 673740880, -539002203,
		-1936945405, -1583220647, -1987477495, 218961690,
		-1077945755, -421121577, 1111672452, 1751693520,
		1094828930, -1717981143, 757954394, 252645662,
		-1330590853, 1414855848, -1145317779, 370555436
	};

	/** Decrypt Message S-Box 5~8 */
	const int CCryptoAes::m_t5[256] =
	{
		1374988112, 2118214995, 437757123, 975658646,
		1001089995, 530400753, -1392879445, 1273168787,
		540080725, -1384747530, -1999866223, -184398811,
		1340463100, -987051049, 641025152, -1251826801,
		-558802359, 632953703, 1172967064, 1576976609,
		-1020300030, -2125664238, -1924753501, 1809054150,
		59727847, 361929877, -1083344149, -1789765158,
		-725712083, 1484005843, 1239443753, -1899378620,
		1975683434, -191989384, -1722270101, 666464733,
		-1092530250, -259478249, -920605594, 2110667444,
		1675577880, -451268222, -1756286112, 1649639237,
		-1318815776, -1150570876, -25059300, -116905068,
		1883793496, -1891238631, -1797362553, 1383856311,
		-1418472669, 1917518562, -484470953, 1716890410,
		-1293211641, 800440835, -2033878118, -751368027,
		807962610, 599762354, 33778362, -317291940,
		-1966138325, -1485196142, -217582864, 1315562145,
		1708848333, 101039829, -785096161, -995688822,
		875451293, -1561111136, 92987698, -1527321739,
		193195065, 1080094634, 1584504582, -1116860335,
		1042385657, -1763899843, -583137874, 1306967366,
		-1856729675, 1908694277, 67556463, 1615861247,
		429456164, -692196969, -1992277044, 1742315127,
		-1326955843, 126454664, -417768648, 2043211483,
		-1585706425, 2084704233, -125559095, 0,
		159417987, 841739592, 504459436, 1817866830,
		-49348613, 260388950, 1034867998, 908933415,
		168810852, 1750902305, -1688513327, 607530554,
		202008497, -1822955761, -1259432238, 463180190,
		-2134850225, 1641816226, 1517767529, 470948374,
		-493635062, -1063245083, 1008918595, 303765277,
		235474187, -225720403, 766945465, 337553864,
		1475418501, -1351284916, -291906117, -1551933187,
		-150919521, 1551037884, 1147550661, 1543208500,
		-1958532746, -886847780, -1225917336, -1192955549,
		-684598070, 1113818384, 328671808, -2067394272,
		-2058738563, -759480840, -1359400431, -953573011,
		496906059, -592301837, 226906860, 2009195472,
		733156972, -1452230247, 294930682, 1206477858,
		-1459843900, -1594867942, 1451044056, 573804783,
		-2025238841, -650587711, -1932877058, -1730933962,
		-1493859889, -1518674392, -625504730, 1068351396,
		742039012, 1350078989, 1784663195, 1417561698,
		-158526526, -1864845080, 775550814, -2101104651,
		-1621262146, 1775276924, 1876241833, -819653965,
		-928212677, 270040487, -392404114, -616842373,
		-853116919, 1851332852, -325404927, -2091935064,
		-426414491, -1426069890, 566021896, -283776794,
		-1159226407, 1248802510, -358676012, 699432150,
		832877231, 708780849, -962227152, 899835584,
		1951317047, -58537306, -527380304, 866637845,
		-251357110, 1106041591, 2144161806, 395441711,
		1984812685, 1139781709, -861254316, -459930401,
		-1630423581, 1282050075, -1054072904, 1181045119,
		-1654724092, 25965917, -91786125, -83148498,
		-1285087910, -1831087534, -384805325, 1842759443,
		-1697160820, 933301370, 1509430414, -351060855,
		-827774994, -1218328267, -518199827, 2051518780,
		-1663901863, 1441952575, 404016761, 1942435775,
		1408749034, 1610459739, -549621996, 2017778566,
		-894438527, -1184316354, 941896748, -1029488545,
		371049330, -1126030068, 675039627, -15887039,
		967311729, 135050206, -659233636, 1683407248,
		2076935265, -718096784, 1215061108, -793225406
	};

	const int CCryptoAes::m_t6[256] =
	{
		1347548327, 1400783205, -1021700188, -1774573730,
		-885281941, -249586363, -1414727080, -1823743229,
		1428173050, -156404115, -1853305738, 636813900,
		-61872681, -674944309, -2144979644, -1883938141,
		1239331162, 1730525723, -1740248562, -513933632,
		46346101, 310463728, -1551022441, -966011911,
		-419197089, -1793748324, -339776134, -627748263,
		768917123, -749177823, 692707433, 1150208456,
		1786102409, 2029293177, 1805211710, -584599183,
		-1229004465, 401639597, 1724457132, -1266823622,
		409198410, -2098914767, 1620529459, 1164071807,
		-525245321, -2068091986, 486441376, -1795618773,
		1483753576, 428819965, -2020286868, -1219331080,
		598438867, -495826174, 1474502543, 711349675,
		129166120, 53458370, -1702443653, -1512884472,
		-231724921, -1306280027, -1174273174, 1559041666,
		730517276, -1834518092, -252508174, -1588696606,
		-848962828, -721025602, 533804130, -1966823682,
		-1657524653, -1599933611, 839224033, 1973745387,
		957055980, -1438621457, 106852767, 1371368976,
		-113368694, 1033297158, -1361232379, 1179510461,
		-1248766835, 91341917, 1862534868, -10465259,
		605657339, -1747534359, -863420349, 2003294622,
		-1112479678, -2012771957, 954669403, -612775698,
		1201765386, -377732593, -906460130, 0,
		-2096529274, 1211247597, -1407315600, 1315723890,
		-67301633, 1443857720, 507358933, 657861945,
		1678381017, 560487590, -778347692, 975451694,
		-1324610969, 261314535, -759894378, -1642357871,
		1333838021, -1570644960, 1767536459, 370938394,
		182621114, -440360918, 1128014560, 487725847,
		185469197, -1376613433, -1188186456, -938205527,
		-2057834215, 1286567175, -1141990947, -39616672,
		-1611202266, -1134791947, -985373125, 878443390,
		1988838185, -590666810, 1756818940, 1673061617,
		-891866660, 272786309, 1075025698, 545572369,
		2105887268, -120407235, 296679730, 1841768865,
		1260232239, -203640272, -334657966, -797457949,
		1814803222, -1716948807, -99511224, 575138148,
		-995558260, 446754879, -665420500, -282971248,
		-947435186, -1042728751, -24327518, 915985419,
		-811141759, 681933534, 651868046, -1539330625,
		-466863459, 223377554, -1687527476, 1649704518,
		-1024029421, -393160520, 1580087799, -175979601,
		-1096852096, 2087309459, -1452288723, -1278270190,
		1003007129, -1492117379, 1860738147, 2077965243,
		164439672, -194094824, 32283319, -1467789414,
		1709610350, 2125135846, 136428751, -420538904,
		-642062437, -833982666, -722821367, -701910916,
		-1355701070, 824852259, 818324884, -1070226842,
		930369212, -1493400886, -1327460144, 355706840,
		1257309336, -146674470, 243256656, 790073846,
		-1921626666, 1296297904, 1422699085, -538667516,
		-476130891, 457992840, -1195299809, 2135319889,
		77422314, 1560382517, 1945798516, 788204353,
		1521706781, 1385356242, 870912086, 325965383,
		-1936009375, 2050466060, -1906706412, -1981082820,
		-288446169, 901210569, -304014107, 1014646705,
		1503449823, 1062597235, 2031621326, -1082931401,
		-363595827, 1533017514, 350174575, -2038938405,
		-2117423117, 1052338372, 741876788, 1606591296,
		1914052035, 213705253, -1960297399, 1107234197,
		1899603969, -569897805, -1663519516, -1872472383,
		1635502980, 1893020342, 1950903388, 1120974935
	};

	const int CCryptoAes::m_t7[256] =
	{
		-1487908364, 1699970625, -1530717673, 1586903591,
		1808481195, 1173430173, 1487645946, 59984867,
		-95084496, 1844882806, 1989249228, 1277555970,
		-671330331, -875051734, 1149249077, -1550863006,
		1514790577, 459744698, 244860394, -1058972162,
		1963115311, -267222708, -1750889146, -104436781,
		1608975247, -1667951214, 2062270317, 1507497298,
		-2094148418, 567498868, 1764313568, -935031095,
		-1989511742, 2037970062, 1047239000, 1910319033,
		1337376481, -1390940024, -1402549984, 984907214,
		1243112415, 830661914, 861968209, 2135253587,
		2011214180, -1367032981, -1608712575, 731183368,
		1750626376, -48656571, 1820824798, -122203525,
		-752637069, 48394827, -1890065633, -1423284651,
		671593195, -1039978571, 2073724613, 145085239,
		-2014171096, -1515052097, 1790575107, -2107839210,
		472615631, -1265457287, -219090169, -492745111,
		-187865638, -1093335547, 1646252340, -24460122,
		1402811438, 1436590835, -516815478, -344611594,
		-331805821, -274055072, -1626972559, 273792366,
		-1963377119, 104699613, 95345982, -1119466010,
		-1917480620, 1560637892, -730921978, 369057872,
		-81520232, -375925059, 1137477952, -1636341799,
		1119727848, -1954019447, 1530455833, -287606328,
		172466556, 266959938, 516552836, 0,
		-2038232704, -314035669, 1890328081, 1917742170,
		-262898, 945164165, -719438418, 958871085,
		-647755249, -1507760036, 1423022939, 775562294,
		1739656202, -418409641, -1764576018, -1851909221,
		-984645440, 547512796, 1265195639, 437656594,
		-1173691757, 719700128, -532464606, 387781147,
		218828297, -944901493, -1464259146, -1446505442,
		428169201, 122466165, -574886247, 1627235199,
		648017665, -172204942, 1002783846, 2117360635,
		695634755, -958608605, -60246291, -245122844,
		-590686415, -2062531997, 574624663, 287343814,
		612205898, 1039717051, 840019705, -1586641111,
		793451934, 821288114, 1391201670, -472877119,
		376187827, -1181111952, 1224348052, 1679968233,
		-1933268740, 1058709744, 752375421, -1863376333,
		1321699145, -775825096, -1560376118, 188127444,
		-2117097739, -567761542, -1910056265, -1079754835,
		-1645990854, -1844621192, -862229921, 1180849278,
		331544205, -1192718120, -144822727, -1342864701,
		-2134991011, -1820562992, 766078933, 313773861,
		-1724135252, 2108100632, 1668212892, -1149510853,
		2013908262, 418672217, -1224610662, -1700232369,
		1852171925, -427906305, -821550660, -387518699,
		-1680229657, 919489135, 164948639, 2094410160,
		-1297141340, 590424639, -1808742747, 1723872674,
		-1137216434, -895026046, -793714544, -669699161,
		-1739919100, -621329940, 1343127501, -164685935,
		-695372211, -1337113617, 1297403050, 81781910,
		-1243373871, -2011476886, 532201772, 1367295589,
		-368796322, 895287692, 1953757831, 1093597963,
		492483431, -766340389, 1446242576, 1192455638,
		1636604631, 209336225, 344873464, 1015671571,
		669961897, -919226527, -437395172, -1321436601,
		-547775278, 1933530610, -830924780, 935293895,
		-840281097, -1436852227, 1863638845, -611944380,
		-209597777, -1002522264, 875313188, 1080017571,
		-1015933411, 621591778, 1233856572, -1790836979,
		24197544, -1277294580, -459482956, -1047501738,
		-2073986101, -1234119374, 1551124588, 1463996600
	};

	const int CCryptoAes::m_t8[256] =
	{
		-190361519, 1097159550, 396673818, 660510266,
		-1418998981, -1656360673, -94852180, -486304949,
		821712160, 1986918061, -864644728, 38544885,
		-438830001, 718002117, 893681702, 1654886325,
		-1319482914, -1172609243, -368142267, -20913827,
		796197571, 1290801793, 1184342925, -738605461,
		-1889540349, -1835231979, 1836772287, 1381620373,
		-1098699308, 1948373848, -529979063, -909622130,
		-1031181707, -1904641804, 1480485785, -1183720153,
		-514869570, -2001922064, 548169417, -835013507,
		-548792221, 439452389, 1362321559, 1400849762,
		1685577905, 1806599355, -2120213250, 137073913,
		1214797936, 1174215055, -563312748, 2079897426,
		1943217067, 1258480242, 529487843, 1437280870,
		-349698126, -1245576401, -981755258, 923313619,
		679998000, -1079659997, 57326082, 377642221,
		-820237430, 2041877159, 133361907, 1776460110,
		-621490843, 96392454, 878845905, -1493267772,
		777231668, -212492126, -1964953083, -152341084,
		-2081670901, 1626319424, 1906247262, 1846563261,
		562755902, -586793578, 1040559837, -423803315,
		1418573201, -1000536719, 114585348, 1343618912,
		-1728371687, -1108764714, 1078185097, -643926169,
		-398279248, -1987344377, 425408743, -923870343,
		2081048481, 1108339068, -2078357000, 0,
		-2138668279, 736970802, 292596766, 1517440620,
		251657213, -2059905521, -1361764803, 758720310,
		265905162, 1554391400, 1532285339, 908999204,
		174567692, 1474760595, -292105548, -1684955621,
		-1060810880, -601841055, 2001430874, 303699484,
		-1816524062, -1607801408, 585122620, 454499602,
		151849742, -1949848078, -1230456531, 514443284,
		-249985705, 1963412655, -1713521682, 2137062819,
		19308535, 1928707164, 1715193156, -75615141,
		1126790795, 600235211, -302225226, -453942344,
		836553431, 1669664834, -1759363053, -971956092,
		1243905413, -1153566510, -114159186, 698445255,
		-1641067747, -1305414692, -2041385971, -1042034569,
		-1290376149, 1891211689, -1807156719, -379313593,
		-57883480, -264299872, 2100090966, 865136418,
		1229899655, 953270745, -895287668, -737462632,
		-176042074, 2061379749, -1215420710, -1379949505,
		983426092, 2022837584, 1607244650, 2118541908,
		-1928084746, -658970480, 972512814, -1011878526,
		1568718495, -795640727, -718427793, 621982671,
		-1399243832, 410887952, -1671205144, 1002142683,
		645401037, 1494807662, -1699282452, 1335535747,
		-1787927066, -1671510, -1127282655, 367585007,
		-409216582, 1865862730, -1626745622, -1333995991,
		-1531793615, 1059270954, -1517014842, -1570324427,
		1320957812, -2100648196, -1865371424, -1479011021,
		77089521, -321194175, -850391425, -1846137065,
		1305906550, -273658557, -1437772596, -1778065436,
		-776608866, 1787304780, 740276417, 1699839814,
		1592394909, -1942659839, -2022411270, 188821243,
		1729977011, -606973294, 274084841, -699985043,
		-681472870, -1593017801, -132870567, 322734571,
		-1457000754, 1640576439, 484830689, 1202797690,
		-757114468, -227328171, 349075736, -952647821,
		-137500077, -39167137, 1030690015, 1155237496,
		-1342996022, 1757691577, 607398968, -1556062270,
		499347990, -500888388, 1011452712, 227885567,
		-1476300487, 213114376, -1260086056, 1455525988,
		-880516741, 850817237, 1817998408, -1202240816
	};

	const int CCryptoAes::m_u1[256] =
	{
		0, 235474187, 470948374, 303765277,
		941896748, 908933415, 607530554, 708780849,
		1883793496, 2118214995, 1817866830, 1649639237,
		1215061108, 1181045119, 1417561698, 1517767529,
		-527380304, -291906117, -58537306, -225720403,
		-659233636, -692196969, -995688822, -894438527,
		-1864845080, -1630423581, -1932877058, -2101104651,
		-1459843900, -1493859889, -1259432238, -1159226407,
		-616842373, -718096784, -953573011, -920605594,
		-484470953, -317291940, -15887039, -251357110,
		-1418472669, -1518674392, -1218328267, -1184316354,
		-1822955761, -1654724092, -1891238631, -2125664238,
		1001089995, 899835584, 666464733, 699432150,
		59727847, 226906860, 530400753, 294930682,
		1273168787, 1172967064, 1475418501, 1509430414,
		1942435775, 2110667444, 1876241833, 1641816226,
		-1384747530, -1551933187, -1318815776, -1083344149,
		-1789765158, -1688513327, -1992277044, -2025238841,
		-583137874, -751368027, -1054072904, -819653965,
		-451268222, -351060855, -116905068, -150919521,
		1306967366, 1139781709, 1374988112, 1610459739,
		1975683434, 2076935265, 1775276924, 1742315127,
		1034867998, 866637845, 566021896, 800440835,
		92987698, 193195065, 429456164, 395441711,
		1984812685, 2017778566, 1784663195, 1683407248,
		1315562145, 1080094634, 1383856311, 1551037884,
		101039829, 135050206, 437757123, 337553864,
		1042385657, 807962610, 573804783, 742039012,
		-1763899843, -1730933962, -1966138325, -2067394272,
		-1359400431, -1594867942, -1293211641, -1126030068,
		-426414491, -392404114, -91786125, -191989384,
		-558802359, -793225406, -1029488545, -861254316,
		1106041591, 1340463100, 1576976609, 1408749034,
		2043211483, 2009195472, 1708848333, 1809054150,
		832877231, 1068351396, 766945465, 599762354,
		159417987, 126454664, 361929877, 463180190,
		-1585706425, -1351284916, -1116860335, -1285087910,
		-1722270101, -1756286112, -2058738563, -1958532746,
		-785096161, -549621996, -853116919, -1020300030,
		-384805325, -417768648, -184398811, -83148498,
		-1697160820, -1797362553, -2033878118, -1999866223,
		-1561111136, -1392879445, -1092530250, -1326955843,
		-358676012, -459930401, -158526526, -125559095,
		-759480840, -592301837, -827774994, -1063245083,
		2051518780, 1951317047, 1716890410, 1750902305,
		1113818384, 1282050075, 1584504582, 1350078989,
		168810852, 67556463, 371049330, 404016761,
		841739592, 1008918595, 775550814, 540080725,
		-325404927, -493635062, -259478249, -25059300,
		-725712083, -625504730, -928212677, -962227152,
		-1663901863, -1831087534, -2134850225, -1899378620,
		-1527321739, -1426069890, -1192955549, -1225917336,
		202008497, 33778362, 270040487, 504459436,
		875451293, 975658646, 675039627, 641025152,
		2084704233, 1917518562, 1615861247, 1851332852,
		1147550661, 1248802510, 1484005843, 1451044056,
		933301370, 967311729, 733156972, 632953703,
		260388950, 25965917, 328671808, 496906059,
		1206477858, 1239443753, 1543208500, 1441952575,
		2144161806, 1908694277, 1675577880, 1842759443,
		-684598070, -650587711, -886847780, -987051049,
		-283776794, -518199827, -217582864, -49348613,
		-1485196142, -1452230247, -1150570876, -1251826801,
		-1621262146, -1856729675, -2091935064, -1924753501
	};

	const int CCryptoAes::m_u2[256] =
	{
		0, 185469197, 370938394, 487725847,
		741876788, 657861945, 975451694, 824852259,
		1483753576, 1400783205, 1315723890, 1164071807,
		1950903388, 2135319889, 1649704518, 1767536459,
		-1327460144, -1141990947, -1493400886, -1376613433,
		-1663519516, -1747534359, -1966823682, -2117423117,
		-393160520, -476130891, -24327518, -175979601,
		-995558260, -811141759, -759894378, -642062437,
		2077965243, 1893020342, 1841768865, 1724457132,
		1474502543, 1559041666, 1107234197, 1257309336,
		598438867, 681933534, 901210569, 1052338372,
		261314535, 77422314, 428819965, 310463728,
		-885281941, -1070226842, -584599183, -701910916,
		-419197089, -334657966, -249586363, -99511224,
		-1823743229, -1740248562, -2057834215, -1906706412,
		-1082931401, -1266823622, -1452288723, -1570644960,
		-156404115, -39616672, -525245321, -339776134,
		-627748263, -778347692, -863420349, -947435186,
		-1361232379, -1512884472, -1195299809, -1278270190,
		-2098914767, -1981082820, -1795618773, -1611202266,
		1179510461, 1296297904, 1347548327, 1533017514,
		1786102409, 1635502980, 2087309459, 2003294622,
		507358933, 355706840, 136428751, 53458370,
		839224033, 957055980, 605657339, 790073846,
		-1921626666, -2038938405, -1687527476, -1872472383,
		-1588696606, -1438621457, -1219331080, -1134791947,
		-721025602, -569897805, -1021700188, -938205527,
		-113368694, -231724921, -282971248, -466863459,
		1033297158, 915985419, 730517276, 545572369,
		296679730, 446754879, 129166120, 213705253,
		1709610350, 1860738147, 1945798516, 2029293177,
		1239331162, 1120974935, 1606591296, 1422699085,
		-146674470, -61872681, -513933632, -363595827,
		-612775698, -797457949, -848962828, -966011911,
		-1355701070, -1539330625, -1188186456, -1306280027,
		-2096529274, -2012771957, -1793748324, -1642357871,
		1201765386, 1286567175, 1371368976, 1521706781,
		1805211710, 1620529459, 2105887268, 1988838185,
		533804130, 350174575, 164439672, 46346101,
		870912086, 954669403, 636813900, 788204353,
		-1936009375, -2020286868, -1702443653, -1853305738,
		-1599933611, -1414727080, -1229004465, -1112479678,
		-722821367, -538667516, -1024029421, -906460130,
		-120407235, -203640272, -288446169, -440360918,
		1014646705, 930369212, 711349675, 560487590,
		272786309, 457992840, 106852767, 223377554,
		1678381017, 1862534868, 1914052035, 2031621326,
		1211247597, 1128014560, 1580087799, 1428173050,
		32283319, 182621114, 401639597, 486441376,
		768917123, 651868046, 1003007129, 818324884,
		1503449823, 1385356242, 1333838021, 1150208456,
		1973745387, 2125135846, 1673061617, 1756818940,
		-1324610969, -1174273174, -1492117379, -1407315600,
		-1657524653, -1774573730, -1960297399, -2144979644,
		-377732593, -495826174, -10465259, -194094824,
		-985373125, -833982666, -749177823, -665420500,
		2050466060, 1899603969, 1814803222, 1730525723,
		1443857720, 1560382517, 1075025698, 1260232239,
		575138148, 692707433, 878443390, 1062597235,
		243256656, 91341917, 409198410, 325965383,
		-891866660, -1042728751, -590666810, -674944309,
		-420538904, -304014107, -252508174, -67301633,
		-1834518092, -1716948807, -2068091986, -1883938141,
		-1096852096, -1248766835, -1467789414, -1551022441,
	};

	const int CCryptoAes::m_u3[256] =
	{
		0, 218828297, 437656594, 387781147,
		875313188, 958871085, 775562294, 590424639,
		1750626376, 1699970625, 1917742170, 2135253587,
		1551124588, 1367295589, 1180849278, 1265195639,
		-793714544, -574886247, -895026046, -944901493,
		-459482956, -375925059, -24460122, -209597777,
		-1192718120, -1243373871, -1560376118, -1342864701,
		-1933268740, -2117097739, -1764576018, -1680229657,
		-1149510853, -1234119374, -1586641111, -1402549984,
		-1890065633, -2107839210, -1790836979, -1739919100,
		-752637069, -567761542, -919226527, -1002522264,
		-418409641, -368796322, -48656571, -267222708,
		1808481195, 1723872674, 1910319033, 2094410160,
		1608975247, 1391201670, 1173430173, 1224348052,
		59984867, 244860394, 428169201, 344873464,
		935293895, 984907214, 766078933, 547512796,
		1844882806, 1627235199, 2011214180, 2062270317,
		1507497298, 1423022939, 1137477952, 1321699145,
		95345982, 145085239, 532201772, 313773861,
		830661914, 1015671571, 731183368, 648017665,
		-1119466010, -1337113617, -1487908364, -1436852227,
		-1989511742, -2073986101, -1820562992, -1636341799,
		-719438418, -669699161, -821550660, -1039978571,
		-516815478, -331805821, -81520232, -164685935,
		-695372211, -611944380, -862229921, -1047501738,
		-492745111, -274055072, -122203525, -172204942,
		-1093335547, -1277294580, -1530717673, -1446505442,
		-1963377119, -2014171096, -1863376333, -1645990854,
		104699613, 188127444, 472615631, 287343814,
		840019705, 1058709744, 671593195, 621591778,
		1852171925, 1668212892, 1953757831, 2037970062,
		1514790577, 1463996600, 1080017571, 1297403050,
		-621329940, -671330331, -1058972162, -840281097,
		-287606328, -472877119, -187865638, -104436781,
		-1297141340, -1079754835, -1464259146, -1515052097,
		-2038232704, -1954019447, -1667951214, -1851909221,
		172466556, 122466165, 273792366, 492483431,
		1047239000, 861968209, 612205898, 695634755,
		1646252340, 1863638845, 2013908262, 1963115311,
		1446242576, 1530455833, 1277555970, 1093597963,
		1636604631, 1820824798, 2073724613, 1989249228,
		1436590835, 1487645946, 1337376481, 1119727848,
		164948639, 81781910, 331544205, 516552836,
		1039717051, 821288114, 669961897, 719700128,
		-1321436601, -1137216434, -1423284651, -1507760036,
		-2062531997, -2011476886, -1626972559, -1844621192,
		-647755249, -730921978, -1015933411, -830924780,
		-314035669, -532464606, -144822727, -95084496,
		-1224610662, -1173691757, -1390940024, -1608712575,
		-2094148418, -1910056265, -1724135252, -1808742747,
		-547775278, -766340389, -984645440, -935031095,
		-344611594, -427906305, -245122844, -60246291,
		1739656202, 1790575107, 2108100632, 1890328081,
		1402811438, 1586903591, 1233856572, 1149249077,
		266959938, 48394827, 369057872, 418672217,
		1002783846, 919489135, 567498868, 752375421,
		209336225, 24197544, 376187827, 459744698,
		945164165, 895287692, 574624663, 793451934,
		1679968233, 1764313568, 2117360635, 1933530610,
		1343127501, 1560637892, 1243112415, 1192455638,
		-590686415, -775825096, -958608605, -875051734,
		-387518699, -437395172, -219090169, -262898,
		-1265457287, -1181111952, -1367032981, -1550863006,
		-2134991011, -1917480620, -1700232369, -1750889146
	};

	const int CCryptoAes::m_u4[256] =
	{
		0, 151849742, 303699484, 454499602,
		607398968, 758720310, 908999204, 1059270954,
		1214797936, 1097159550, 1517440620, 1400849762,
		1817998408, 1699839814, 2118541908, 2001430874,
		-1865371424, -1713521682, -2100648196, -1949848078,
		-1260086056, -1108764714, -1493267772, -1342996022,
		-658970480, -776608866, -895287668, -1011878526,
		-57883480, -176042074, -292105548, -409216582,
		1002142683, 850817237, 698445255, 548169417,
		529487843, 377642221, 227885567, 77089521,
		1943217067, 2061379749, 1640576439, 1757691577,
		1474760595, 1592394909, 1174215055, 1290801793,
		-1418998981, -1570324427, -1183720153, -1333995991,
		-1889540349, -2041385971, -1656360673, -1807156719,
		-486304949, -368142267, -249985705, -132870567,
		-952647821, -835013507, -718427793, -601841055,
		1986918061, 2137062819, 1685577905, 1836772287,
		1381620373, 1532285339, 1078185097, 1229899655,
		1040559837, 923313619, 740276417, 621982671,
		439452389, 322734571, 137073913, 19308535,
		-423803315, -273658557, -190361519, -39167137,
		-1031181707, -880516741, -795640727, -643926169,
		-1361764803, -1479011021, -1127282655, -1245576401,
		-1964953083, -2081670901, -1728371687, -1846137065,
		1305906550, 1155237496, 1607244650, 1455525988,
		1776460110, 1626319424, 2079897426, 1928707164,
		96392454, 213114376, 396673818, 514443284,
		562755902, 679998000, 865136418, 983426092,
		-586793578, -737462632, -820237430, -971956092,
		-114159186, -264299872, -349698126, -500888388,
		-1787927066, -1671205144, -2022411270, -1904641804,
		-1319482914, -1202240816, -1556062270, -1437772596,
		-321194175, -438830001, -20913827, -137500077,
		-923870343, -1042034569, -621490843, -738605461,
		-1531793615, -1379949505, -1230456531, -1079659997,
		-2138668279, -1987344377, -1835231979, -1684955621,
		2081048481, 1963412655, 1846563261, 1729977011,
		1480485785, 1362321559, 1243905413, 1126790795,
		878845905, 1030690015, 645401037, 796197571,
		274084841, 425408743, 38544885, 188821243,
		-681472870, -563312748, -981755258, -864644728,
		-212492126, -94852180, -514869570, -398279248,
		-1626745622, -1778065436, -1928084746, -2078357000,
		-1153566510, -1305414692, -1457000754, -1607801408,
		1202797690, 1320957812, 1437280870, 1554391400,
		1669664834, 1787304780, 1906247262, 2022837584,
		265905162, 114585348, 499347990, 349075736,
		736970802, 585122620, 972512814, 821712160,
		-1699282452, -1816524062, -2001922064, -2120213250,
		-1098699308, -1215420710, -1399243832, -1517014842,
		-757114468, -606973294, -1060810880, -909622130,
		-152341084, -1671510, -453942344, -302225226,
		174567692, 57326082, 410887952, 292596766,
		777231668, 660510266, 1011452712, 893681702,
		1108339068, 1258480242, 1343618912, 1494807662,
		1715193156, 1865862730, 1948373848, 2100090966,
		-1593017801, -1476300487, -1290376149, -1172609243,
		-2059905521, -1942659839, -1759363053, -1641067747,
		-379313593, -529979063, -75615141, -227328171,
		-850391425, -1000536719, -548792221, -699985043,
		836553431, 953270745, 600235211, 718002117,
		367585007, 484830689, 133361907, 251657213,
		2041877159, 1891211689, 1806599355, 1654886325,
		1568718495, 1418573201, 1335535747, 1184342925
	};

	const int8_t CCryptoAes::m_rcon[30] =
	{
		1, 2, 4, 8, 16, 32,
		64, -128, 27, 54, 108, -40,
		-85, 77, -102, 47, 94, -68,
		99, -58, -105, 53, 106, -44,
		-77, 125, -6, -17, -59, -111
	};

	const int CCryptoAes::m_shifts[3][4][2] =
	{
		{ { 0, 0 }, { 1, 3 }, { 2, 2 }, { 3, 1 } },
		{ { 0, 0 }, { 1, 5 }, { 2, 4 }, { 3, 3 } },
		{ { 0, 0 }, { 1, 7 }, { 3, 5 }, { 4, 4 } }
	};



}
