//////////////////////////////////////////////////////////////////////////
//

#pragma once

// http connection level
// http ���� �ܰ�
enum eHttpProgress
{
	eHttp_None,
	eHttp_Connecting,
	eHttp_Sending,
	eHttp_Receiving,
	eHttp_End
};


class CSocketDg:public ISocketDelegate
{
public:
	// Socket Warning Event
	virtual void OnSocketWarning(CSocket* socket,String msg)
	{
		printf("Socket Warning!! Message : %ls\n", msg.GetString());
	}
};

typedef RefCount<CSocket> SocketPtr;

// Object that Connect with http with using Socket
// Socket�� �̿��Ͽ� http�� Connect�ϴ� ��ü
class CHttpConnector
{
public:
	CHttpConnector();
	~CHttpConnector();

private:
	SocketPtr m_Socket;			// CSocket refcount 
	CSocketDg m_SocketDg;

	// �޼����� ���� Server url name
	// Server url name that sending message
	String	m_strServerName;	
	StringA m_HttpText;			// message text

	eHttpProgress m_httpProgress; 

	// send�Ҷ��� offset��
	// offset value when you do send
	int m_sendProgress;			

	// ���� ����
	// Receiving buffer
	ByteArray m_ReceiveBuffer;	

	bool	TryConnect();
	void	Send();
	void	Progress_None();
	void	Progress_Connecting();
	void	Progress_Sending();
	void	Progress_Receiving();
public:
	bool Request(String strServerName, StringA requestMethod, StringA strSend);
	void	Update(float fDelta);

};
