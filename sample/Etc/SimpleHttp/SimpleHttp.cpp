// 2009.12.03 create by ulelio : http �� �����ϴ� �����Դϴ�. SMS�� �����ý��۵ Ȱ��ɼ� �ֽ��ϴ�.
// 2009.12.03 create by ulelio : http connecting sample. You can use it SMS or payment system.
//

#include "stdafx.h"

LPCWSTR ServerName=L"www.google.com";
int ServerPort=80;
// google���� ���ټ��� �˻��� �������� ����.
// Get searched page with "Nettention" from Google.
LPCSTR HttpMsgFmt = "/search?q=nettention";
LPCSTR requestMethod = "GET";

int _tmain(int argc, _TCHAR* argv[])
{

	CHttpConnector Connector;

	printf("ESC : End, a : Send a message to the site\n");

	while(true)
	{
		if(_kbhit())
		{
			switch(_getch())
			{
			case 27:
				return 0;
			case 'a':
				{
					Connector.Request(ServerName, requestMethod, HttpMsgFmt);
				}
				break;
			}
		}

		Connector.Update(0.0f);

	}

	return 0;
}

