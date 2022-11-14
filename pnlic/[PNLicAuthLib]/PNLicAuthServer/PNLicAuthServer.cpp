// PNAuthServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "PNLicAuthServer.h"

#include "../PNLicAuthCommon/pidl/C2S_common.cpp"
#include "../PNLicAuthCommon/pidl/S2C_common.cpp"
#include "../PNLicAuthCommon/pidl/S2C_proxy.cpp"
#include "../PNLicAuthCommon/pidl/C2S_proxy.cpp"
#include "../PNLicAuthCommon/pidl/C2S_stub.cpp"

DEFRMI_LicenseC2S_RequestAuthentication(CPNLicAuthServer)
{
	String externalIP = this->GetExternalIP(remote);
	LicenseMessageType ret = this->DBCheckAuthentication(cpuId, computerName, userName, key, externalIP, internalIP, licenseType);

	this->NotifyAutherized(remote, (LicenseMessageType)ret);

#if defined(_DEBUG) || defined(DEBUG) 
	cout << "success to receive authentication request() Past Ver. from " << externalIP.GetString() << " ret(" << ret << ")" << endl;
#endif
	return true;
}

DEFRMI_LicenseC2S_RequestRecordServerInfo(CPNLicAuthServer)
{
	String externalIP = this->GetExternalIP(remote);
	// 디비에 기록
	LicenseMessageType ret = this->DBRecordServerInfo(cpuId, companyName, projectName, licenseType, sigKey, computerName, userName, processName, externalIP, internalIP, ccuNumber);
	// 결과를 클라에 통보
	this->NotifyRecordServerInfo(remote, (LicenseMessageType)ret);

#if defined(_DEBUG) || defined(DEBUG) 
	cout << "success to receive authentication request() Past Ver. from " << externalIP.GetString() << " ret(" << ret << ")" << endl;
#endif
	return true;
}

CPNLicAuthServer::CPNLicAuthServer(void)
{

}

CPNLicAuthServer::~CPNLicAuthServer(void)
{
	m_server.Free();
}

bool CPNLicAuthServer::Start(CStartServerParameter p)
{
	m_server.Attach(CNetServer::Create());
	m_server->SetEventSink(this);
	m_server->AttachProxy(&m_s2cProxy);
	m_server->AttachStub(this);

	//m_server->EnableLog(_PNT("PNLicAuthServerLog.txt"));
	m_server->Start(p);

		// 10000 줄마다 로그 파일 새로 생성
		//m_logWriter = CLogWriter::New(L"C:\\AuthLog\\AuthServerLog", 10000);
//#if defined(__linux__)
//		m_logWriter = CLogWriter::New(_PNT("/tmp/AuthLog/AuthServerLog"), 10000);
//#else
//		m_logWriter = CLogWriter::New(_PNT("D:\\Temp\\AuthServerLog"), 10000);
//		if (m_logWriter == 0) {
//			cout << "logwriter is " << m_logWriter << endl;
//		}
//#endif

#if defined(_DEBUG) || defined(DEBUG) 
	cout << "success to start to run authserver(ip:" << StringT2A(p.m_serverAddrAtClient).GetString() << " port:" << p.m_tcpPorts.ElementAt(0) << ")" << endl;
#endif
	return true;
}

void CPNLicAuthServer::Stop()
{
	m_server->Stop();
#if defined(_DEBUG) || defined(DEBUG) 
	cout << "success to stop running authserver. " << endl;
#endif
}

void CPNLicAuthServer::OnClientJoin(CNetClientInfo *clientInfo)
{
#if defined(_DEBUG) || defined(DEBUG) 
	cout << "success to call OnClientJoin(" << clientInfo->m_HostID << ")" << endl;
#endif
}

void CPNLicAuthServer::OnClientLeave(CNetClientInfo *clientInfo, ErrorInfo *errorinfo, const ByteArray& comment)
{
#if defined(_DEBUG) || defined(DEBUG) 
	cout << "success to call OnClientLeave(" << clientInfo->m_HostID << ")" << endl;
#endif
}

// 접속한 클라이언트의 공인 주소를 얻는다.
Proud::String CPNLicAuthServer::GetExternalIP(HostID remote) const
{
	CNetClientInfo clientInfo;
	m_server->GetClientInfo(remote, clientInfo);
	return clientInfo.m_TcpAddrFromServer.ToDottedIP();
}

void CPNLicAuthServer::NotifyAutherized( HostID remote, LicenseMessageType resultType, const Proud::String &comment )
{
	m_s2cProxy.NotifyAuthenticationComment(remote, RmiContext::SecureReliableSend, (BYTE)resultType, comment);
#if defined(_DEBUG) || defined(DEBUG) 
	printf("success to notify authentication comment. result(%d) \n", resultType);
#endif
}

void CPNLicAuthServer::NotifyAutherized( HostID remote, LicenseMessageType resultType)
{
	m_s2cProxy.NotifyAuthentication(remote, RmiContext::SecureReliableSend, (BYTE)resultType);
#if defined(_DEBUG) || defined(DEBUG) 
	printf("success to notify authentication. result(%d) \n", resultType);
#endif
}

void CPNLicAuthServer::NotifyRecordServerInfo(HostID remote, LicenseMessageType resultType)
{
	m_s2cProxy.NotifyRecordServerInfo(remote, RmiContext::SecureReliableSend, (BYTE)resultType);
#if defined(_DEBUG) || defined(DEBUG) 
	printf("success to notify record server info. result(%d) \n", resultType);
#endif
}

// DB에 프넷 사용 기록을 남긴다.
LicenseMessageType CPNLicAuthServer::DBRecordServerInfo(const String &cpuId, const String &companyName, const String &projectName, const String &licenseType, const String &sigKey, const String &comName, const String &userName, const String &processName, const String &externalIP, const String &internalIP, const int8_t ccuNumber)
{
	LicenseMessageType result = LMType_Unexpected;
	CAdoConnection conn;

	/*Proud::String userIP;
	userIP.Format(_PNT("in :%s <br/> ex :%s"), internalIP.GetString(), externalIP.GetString());*/

	TSTRING userIP;
	Tstringstream strStream;
	strStream << _PNT("in :");
	strStream << internalIP.GetString();
	strStream << _PNT(" <br/> ex :");
	strStream << externalIP.GetString();
	userIP = strStream.str();
	
	try
	{
//222-TO-DO: 계정 정보를 다른 방식으로 관리하자. (암호화 필수)
		conn.Open(_PNT("Server=192.168.77.209;Database=ndnweb;User Id=webadmin;Password=nettentionfhzjf!@#4")); //Test용 Web팀 DB
		//conn.Open(_PNT("Server=175.125.93.117,5858;Database=ndnweb;User Id=webadmin;Password=nettenwebadmin2014")); //실 인증DB
		//conn.Open(_PNT("Data Source=.;Database=ndnweb;Trusted_Connection=yes")); //

		String strSigKey = sigKey;
		strSigKey.TrimLeft();
		strSigKey.TrimRight();
	
		CAdoCommand cmd;
		cmd.Prepare(conn, _PNT("ndnsp_record_serverinfo_over_ccu")); // 이 SP는 //web/main/aspnet/dbSchemaBackup 안에 있다.
		cmd.AppendParameter(_PNT("@cupid"), ADODB::adVarChar, ADODB::adParamInput, cpuId.GetString());
		cmd.AppendParameter(_PNT("@company_name"), ADODB::adVarChar, ADODB::adParamInput, companyName.GetString());
		cmd.AppendParameter(_PNT("@project_name"), ADODB::adVarChar, ADODB::adParamInput, projectName.GetString());
		cmd.AppendParameter(_PNT("@licensetype"), ADODB::adVarChar, ADODB::adParamInput, licenseType.GetString());
		cmd.AppendParameter(_PNT("@signkey"), ADODB::adVarChar, ADODB::adParamInput, strSigKey.GetString());
		cmd.AppendParameter(_PNT("@machine_name"), ADODB::adVarChar, ADODB::adParamInput, comName.GetString());
		cmd.AppendParameter(_PNT("@user_name"), ADODB::adVarChar, ADODB::adParamInput, userName.GetString());
		cmd.AppendParameter(_PNT("@process_name"), ADODB::adVarChar, ADODB::adParamInput, processName.GetString());
		cmd.AppendParameter(_PNT("@user_addr"), ADODB::adVarChar, ADODB::adParamInput, userIP.c_str()); //internal IP, external IP 
		cmd.AppendParameter(_PNT("@ccu_number"), ADODB::adInteger, ADODB::adParamInput, ccuNumber);

		ADODB::_Parameter *p = cmd.AppendParameter(_PNT("@valid"), ADODB::adInteger, ADODB::adParamOutput, sizeof(int));

		cmd.Execute();
		CVariant val = p->GetValue();
		result = (LicenseMessageType)(int)val;
	}
	catch (AdoException &e)
	{
#if defined(_DEBUG) || defined(DEBUG) 
		cout << "failed to record server info! ado_error result(" << e.what() << ")" << endl;
#endif
		return result;
	}
#if defined(__linux__)
#else
	catch (_com_error& e)
	{
#if defined(_DEBUG) || defined(DEBUG) 
		cout << "failed to record server info! _com_error result(" << e.Description() << ")" << endl;
#endif
		conn->Close();
		return result;
	}
#endif
	conn.Close();

#if defined(_DEBUG) || defined(DEBUG) 
	printf("success to record server info! result(%d) \n", result);
#endif
	return  result;
}


LicenseMessageType CPNLicAuthServer::DBCheckAuthentication(const String &cpuId, const String &comName, const String &userName, const String &key_2, const String &externalIP, const String &internalP, const String &licensetype)
{
	LicenseMessageType result = LMType_Unexpected;
	CAdoConnection conn;

	Proud::String userIP;
	userIP.Format(_PNT("in :%s <br/> ex :%s"), internalP.GetString(), externalIP.GetString());
	/* 위 코드는 사용자가 사용하는 부분이 아니기 때문에 큰 문제가 되진 않을 것 같습니다. 대신 대체 코드를 주석으로 남겨놓겠습니다.
	TSTRING userIP;
	Tstringstream strStream;
	strStream << _PNT("in :");
	strStream << internalIP.GetString();
	strStream << _PNT(" <br/> ex :");
	strStream << externalIP.GetString();
	userIP = strStream.str();
	*/

	try
	{
		conn.Open(_PNT("Server=192.168.77.209;Database=ndnweb;User Id=webadmin;Password=nettentionfhzjf!@#4")); //Test용 Web팀 DB
		//conn.Open(_PNT("Server=175.125.93.117,5858;Database=ndnweb;User Id=webadmin;Password=nettenwebadmin2014")); //실 인증DB
		//conn.Open(_PNT("Data Source=.;Database=ndnweb;Trusted_Connection=yes")); //

		String licensekey_2 = key_2;

		licensekey_2.TrimLeft();
		licensekey_2.TrimRight();
		licensekey_2.Replace(_PNT("\r\n"), _PNT("|"));
		licensekey_2.Replace(_PNT("\r"), _PNT("|"));
		licensekey_2.Replace(_PNT("\n"), _PNT("|"));
		
		CAdoCommand cmd;
		cmd.Prepare(conn, _PNT("ndnsp_check_authentication_ex")); // 이 SP는 //web/main/aspnet/dbSchemaBackup 안에 있다.
		cmd.AppendParameter(_PNT("@cupid"), ADODB::adVarChar, ADODB::adParamInput, cpuId.GetString());
		cmd.AppendParameter(_PNT("@machine_name"), ADODB::adVarChar, ADODB::adParamInput, comName.GetString());
		cmd.AppendParameter(_PNT("@user_name"), ADODB::adVarChar, ADODB::adParamInput, userName.GetString());
		cmd.AppendParameter(_PNT("@user_addr"), ADODB::adVarChar, ADODB::adParamInput, userIP.GetString()); //internal IP, external IP 
		cmd.AppendParameter(_PNT("@licensekey_2"), ADODB::adVarChar, ADODB::adParamInput, licensekey_2.GetString());
		cmd.AppendParameter(_PNT("@licensetype"), ADODB::adVarChar, ADODB::adParamInput, licensetype.GetString());

		ADODB::_Parameter *p = cmd.AppendParameter(_PNT("@valid"), ADODB::adInteger, ADODB::adParamOutput, sizeof(int));
		
		cmd.Execute();
		CVariant val = p->GetValue();
		result = (LicenseMessageType)(int)val;
	}
	catch (AdoException &e)
	{
#if defined(_DEBUG) || defined(DEBUG) 
		cout << "failed to check DBCheckAuthentication! ado_error result(" << e.what() << ")" << endl;
#endif
		return result;
	}
#if defined(__linux__)
#else
	catch (_com_error& e)
	{
#if defined(_DEBUG) || defined(DEBUG) 
		cout << "failed to check DBCheckAuthentication! _com_error result(" << e.Description() << ")" << endl;
#endif
		conn->Close();
		return result;
	}
#endif
	conn.Close();

#if defined(_DEBUG) || defined(DEBUG) 
	printf("success to check DBCheckAuthentication! result(%d) \n", result);
#endif
	return  result;
}

void CPNLicAuthServer::OnError(ErrorInfo *errorInfo)
{
	//m_logWriter->WriteLine(_PNT("Error: %s"), errorInfo->ToString());
}

void CPNLicAuthServer::OnWarning(ErrorInfo *errorInfo)
{
	//m_logWriter->WriteLine(_PNT("Warning: %s"), errorInfo->ToString());
}

void CPNLicAuthServer::OnInformation(ErrorInfo *errorInfo)
{
	//m_logWriter->WriteLine(_PNT("Information: %s"), errorInfo->ToString());
}

void CPNLicAuthServer::OnException(Exception &e)
{
	//m_logWriter->WriteLine(_PNT("Exception: %s"), StringA2T(e.what()));
}

void CPNLicAuthServer::OnNoRmiProcessed(RmiID rmiID)
{
	//m_logWriter->WriteLine(_PNT("NoRmProcessed, ID: %d"), rmiID);
}

