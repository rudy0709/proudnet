#pragma once

const int MaxKeylen = 1024;
//대칭키
class CSymmetricEncrypt
{
private:
	void swap(BYTE *a, BYTE *b);
	void EncryptCalculation(BYTE *Buffer, int size);


	int			m_Keylen;// =256;
	BYTE		m_Key[MaxKeylen];
	BYTE		S[MaxKeylen];
	BYTE		m_CheckSumNum;

	void				Init2();
	virtual int			Getkeylen() {return m_Keylen;}

public:
	//////////////////////////////////체크섬 안씀 임시 고정/////////////////////////////////////
	virtual void		GetKey(BYTE *output) ;	

	BYTE*				GetTempKey();
	virtual void			SetKey(BYTE const *Key, int size) ;
	//////////////////////////////////체크섬 안씀 임시 고정/////////////////////////////////////


	virtual bool KeySetOK();
	void		GetKey(BYTE *Buffer, int *Keylen, BYTE *CheckSumNum);		//반드시 Keylen 사이즈 이상의 Buffer 일 것
	void		SetKey(BYTE const *Buffer, int Keylen, BYTE CheckSumNum);		//반드시 Keylen 사이즈 이상의 Buffer 일 것

	virtual void Init(int Keylen = MaxKeylen, BYTE *Key=NULL);
	void		EncryptBuffer(BYTE *Buffer, int BufferSize, BYTE* pBeforeCheck_Sum = NULL,  BYTE* pAfterCheck_Sum = NULL);
	bool		DecryptBuffer(BYTE *Buffer, int BufferSize, BYTE* pBeforeCheck_Sum = NULL,  BYTE* pAfterCheck_Sum = NULL);	//체크섬:  사용 옵션, Decrypt에서 아웃풋되지 않음


	CSymmetricEncrypt(void);
	~CSymmetricEncrypt(void);



	static int	Randint();		
	static BYTE	RandBYTE();
};
