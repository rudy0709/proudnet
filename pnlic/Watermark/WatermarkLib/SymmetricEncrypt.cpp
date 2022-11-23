#include "StdAfx.h"
#include "SymmetricEncrypt.h"
#include <time.h>
#include <stdlib.h>


CSymmetricEncrypt::CSymmetricEncrypt(void) : m_Keylen(0), m_CheckSumNum(0)
{
}

CSymmetricEncrypt::~CSymmetricEncrypt(void)
{
}

void CSymmetricEncrypt::swap(BYTE *a, BYTE *b)
{
	char temp;
	temp = *a;
	*a = *b;
	*b = temp;
}

bool CSymmetricEncrypt::KeySetOK()
{
	if(m_Keylen)//S값이 있는지 없는지 체크 해야 함 ByteArray값으로 변환 후에 처리
		return true;
	return false;
}
/*********************************
 * Name : initRC4 *
 * initialized keystream *
 *********************************/
void CSymmetricEncrypt::Init(int Keylen, BYTE *pKey)
{
	assert(Keylen);
	m_CheckSumNum = Randint()%256;
	m_Keylen = Keylen;
	/***************************** Key값 & 임시 벡터 T생성 *****************************/
	if(pKey == NULL)
	{
		for(int forNum_1 =0 ; forNum_1 < m_Keylen ; ++forNum_1)
		{
			m_Key[forNum_1] = Randint()%255+1;

		}
	}else 
	{
		memcpy(m_Key,pKey, Keylen);
	}
	Init2();
	
	srand((unsigned int)time(NULL));
	
	
}
void	 CSymmetricEncrypt::Init2()
{
	BYTE T[MaxKeylen];

	int forNum_1;
	int j,t=0;

	/* Initialization */
	for(forNum_1 = 0; forNum_1 < m_Keylen; forNum_1++)
	{
		S[forNum_1] = forNum_1%256;
		T[forNum_1] = m_Key[forNum_1 % m_Keylen];
	}
	/***************************** Key값 & 임시 벡터 T생성 *****************************/



	/********************************* S최초 치환 생성 *********************************/
	/*Initial permutation of s*/
	j=0;
	for(forNum_1 = 0;  forNum_1 < m_Keylen; forNum_1++)
	{
		j = ((j+S[forNum_1]+T[forNum_1]) % m_Keylen);
		swap( &S[forNum_1], &S[j] );
	}
	/********************************* S최초 치환 생성 *********************************/
}

void CSymmetricEncrypt::EncryptCalculation(BYTE *Buffer, int size)
{
	int i,j,m,len=0,l=0,t=0;
	BYTE k= 0;

	BYTE TempS[MaxKeylen];
	memcpy(TempS, S, m_Keylen);

// 		ATLTRACE("keylen:%d\n",m_Keylen);
//		ATLTRACE("sessionkey :");
// 		for( i = 0; i < m_Keylen; ++i)
// 			ATLTRACE(" %u", TempS[i]);
//  		ATLTRACE("sessionkeyEnd\n");

	/* stream Generation */
	i=0;
	j=0;

	for(m=0;m<size;m++)
	{
		i=(i+1) % m_Keylen;
		j=(j+TempS[i]) % m_Keylen;

		swap(&TempS[i], &TempS[j]);

		t=(TempS[i]+TempS[j]) % m_Keylen;
		k = TempS[t];
		Buffer[m] = k^Buffer[m];  
		
	}
}



void CSymmetricEncrypt::EncryptBuffer(BYTE *Buffer, int BufferSize, BYTE* pBeforeCheck_Sum,  BYTE* pAfterCheck_Sum)
{
	//암호화 하기 전 체크섬 생성
	//if(pBeforeCheck_Sum)
	//	*pBeforeCheck_Sum = CEncryptBasis::MakeCheckSum(Buffer, BufferSize, m_CheckSumNum );

	//암호화 계산
	EncryptCalculation(Buffer, BufferSize);


	//cout << endl << (int)*pAfterCheck_Sum;
	//암호화 후 체크섬 생성
	//if(pAfterCheck_Sum)
	//	*pAfterCheck_Sum = CEncryptBasis::MakeCheckSum(Buffer, BufferSize, m_CheckSumNum);
}

bool CSymmetricEncrypt::DecryptBuffer(BYTE *Buffer, int BufferSize, BYTE* pBeforeCheck_Sum,  BYTE* pAfterCheck_Sum )
{
	//암호화 후 체크섬 확인
	//if(pAfterCheck_Sum)
	//	if(m_CheckSumNum != CEncryptBasis::MakeCheckSum(Buffer, BufferSize, *pAfterCheck_Sum))
	//		return false;

	//복호화 계산
	EncryptCalculation(Buffer, BufferSize);


	//암호화 전 체크섬 생성
	//if(pBeforeCheck_Sum)
	//{
	//	if(m_CheckSumNum != CEncryptBasis::MakeCheckSum(Buffer, BufferSize, *pBeforeCheck_Sum))
	//		return false;
	//}

	return true;
}

void CSymmetricEncrypt::GetKey(BYTE *Buffer, int *Keylen, BYTE *CheckSumNum)
{
	*Keylen = m_Keylen;
	memcpy(Buffer, m_Key, m_Keylen);
	*CheckSumNum = m_CheckSumNum;
}
void CSymmetricEncrypt::SetKey(BYTE const *Buffer, int Keylen, BYTE CheckSumNum)
{
	m_Keylen = Keylen;
	memcpy( m_Key, Buffer, Keylen );
	m_CheckSumNum = CheckSumNum;

	Init2();
}

//////////////////////////////////체크섬 안씀 임시 고정/////////////////////////////////////
void CSymmetricEncrypt::GetKey(BYTE *output)
{
	assert(m_Keylen);
	memcpy( output, m_Key, m_Keylen );
}

void CSymmetricEncrypt::SetKey(BYTE const *Key, int Keylen)
{
	m_Keylen = Keylen;
	memcpy( m_Key, Key, Keylen );

	Init2();	
}
//////////////////////////////////체크섬 안씀 임시 고정/////////////////////////////////////

int CSymmetricEncrypt::Randint()
{	
	return rand();	
}

BYTE CSymmetricEncrypt::RandBYTE()
{
	return (BYTE)(rand()%8);
}