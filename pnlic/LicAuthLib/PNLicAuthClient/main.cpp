#include "stdafx.h"
#include "PNLicAuthClient.h"
#include <fstream>
#include <iostream>

#include "../../AuthNetLib/include/ProudNetClient.h"

void ShowUsage()
{
	cout << endl << "Usage: PNLicAuthClient [ licensekey path ] " << endl;
}

int main(int argc, char* argv[])
{
	try
	{
		if (argc > 2) {
			throw Exception("too many arguments. errorcode: %d", PNErrorInvalidArgCount);
		}

		if (argc == 1) {
			ShowUsage();
			throw Exception("no input argument. errorcode: %d", PNErrorInvalidArgCount);
		}

		//RmiContext rmiContext;
		//rmiContext.m_compressMode = CM_Zip;
		//rmiContext.m_encryptMode = EM_Fast;
		Proud::String resultMessage;
		LicenseMessageType errorcode = LMType_Nothing;
		bool ret = false;
		bool isComplete = false;

		CPNLicAuthClient gameserver;
		//gameserver.Connect(g_serverIp, g_port);
		//printf("success to start to connect to authserver!\n");

		Proud::String stLicenseKey = _PNT("");
		Proud::String stLicenseKeyPath = StringA2T(argv[1]);
		std::ifstream fs(StringT2A(stLicenseKeyPath).GetString(), std::ifstream::in);
		if (!fs.is_open()) {
			throw Exception("cannot open the licensekey file. errno: %d", errno);
		}

		string strTempData;

		fs.seekg(0);
		getline(fs, strTempData, '\0');
		stLicenseKey = StringA2T(strTempData.c_str());
		fs.close();

		////printf("success to get file data[%s] size[%d] \n", stLicenseKey.c_str(), stLicenseKey.length());
		//while (gameserver.IsConnectedToAuthServer() == false) {
		//	gameserver.FrameMove();
		//	Sleep(1);
		//};
		//printf("success to connect with authserver!\n");

		gameserver.AuthorizationBlocking(stLicenseKey.GetString(), _PNT("PNLIBLICENSE"), resultMessage, errorcode, isComplete);

		if (isComplete == true) {
			switch (errorcode)
			{
			case LMType_RegSuccess:
				// 온라인 인증이 성공하여 true 를 반환한다. 
				ret = true;
				break;

			case LMType_AuthServerConnectionFailed:
				// 네트웍 장애로 온라인 인증을 할 수 없는 상태로 이 경우에만 특별히 오프라인 설치를 허용하기 위해 true 로 반환한다. 
				ret = true;
				break;

			default:
				cout << "Failed to activate online license (Reason: " << StringT2A(resultMessage).GetString() << ". errorcode: " << errorcode << ")" << endl;
				ret = false;
				break;
			}
		}
		else
		{
			ret = false;
		}

		printf("success to request authorization! \n");
		gameserver.Disconnect();
	}
	catch (Exception& e)
	{
		printf("failed to authorize the licensekey online. (reason: %s) \n", e.what());
		return -1;
	}

	return 0;
}
