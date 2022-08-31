#include <jni.h>
#include "../include/NdkLog.h"
#ifdef __cplusplus
extern "C" {
#endif

	/*
	 * Class:     com_nettention_proud_NetClient
	 * Method:    createNativeInstance
	 * Signature: ()J
	 */
	JNIEXPORT jlong JNICALL Java_com_nettention_proud_NetClient_createNativeInstance(JNIEnv *env, jobject obj)
	{
		return (jlong) CNetClient::Create();
	}

	/*
	 * Class:     com_nettention_proud_NetClient
	 * Method:    destroyNativeInstance
	 * Signature: (J)V
	 */
	JNIEXPORT void JNICALL Java_com_nettention_proud_NetClient_destroyNativeInstance(JNIEnv *env, jobject obj, jlong netClient)
	{
		delete ((CNetClient *)netClient);
	}

	/*
	 * Class:     com_nettention_proud_NetClient
	 * Method:    nativeFrameMove
	 * Signature: (JLcom/nettention/proud/FrameMoveResult;)V
	 */
	JNIEXPORT void JNICALL Java_com_nettention_proud_NetClient_nativeFrameMove(JNIEnv *env, jobject obj, jlong netClient, jobject outResult)
	{
		((CNetClient *)netClient)->FrameMove();
	}

	/*
	 * Class:     com_nettention_proud_NetClient
	 * Method:    nativeGetLocalHostID
	 * Signature: (J)I
	 */
	JNIEXPORT jint JNICALL Java_com_nettention_proud_NetClient_nativeGetLocalHostID(JNIEnv *env, jobject obj, jlong netClient)
	{
		return ((CNetClient *)netClient)->GetLocalHostID();
	}

	/*
	 * Class:     com_nettention_proud_NetClient
	 * Method:    nativeAttachProxy
	 * Signature: (JJ)V
	 */
	JNIEXPORT void JNICALL Java_com_nettention_proud_NetClient_nativeAttachProxy(JNIEnv *env, jobject obj, jlong netClient, jlong proxy)
	{
		((CNetClient *)netClient)->AttachProxy((IRmiProxy*)proxy);
	}

	/*
	 * Class:     com_nettention_proud_NetClient
	* Method:    nativeDetachProxy
	* Signature: (JJ)V
	*/
	JNIEXPORT void JNICALL Java_com_nettention_proud_NetClient_nativeDetachProxy(JNIEnv *env, jobject obj, jlong netClient, jlong proxy)
	{
		((CNetClient *)netClient)->DetachProxy((IRmiProxy*)proxy);
	}

	/*
	 * Class:     com_nettention_proud_NetClient
	 * Method:    nativeAttachStub
	 * Signature: (JJ)V
	 */
	JNIEXPORT void JNICALL Java_com_nettention_proud_NetClient_nativeAttachStub(JNIEnv *env, jobject obj, jlong netClient, jlong stub)
	{
		((CNetClient *)netClient)->AttachStub((IRmiStub*)stub);
	}

	/*
	* Class:     com_nettention_proud_NetClient
	* Method:    nativeDetachStub
	* Signature: (JJ)V
	*/
	JNIEXPORT void JNICALL Java_com_nettention_proud_NetClient_nativeDetachStub(JNIEnv *env, jobject obj, jlong netClient, jlong stub)
	{
		((CNetClient *)netClient)->DetachStub((IRmiStub*)stub);
	}

	/*
	 * Class:     com_nettention_proud_NetClient
	 * Method:    nativeSetEventSink
	 * Signature: (JJ)V
	 */
	JNIEXPORT void JNICALL Java_com_nettention_proud_NetClient_nativeSetEventSink(JNIEnv *env, jobject obj, jlong netClient, jlong inetclientevent)
	{
		((CNetClient *)netClient)->SetEventSink((INetClientEvent*)inetclientevent);
	}

	/*
	 * Class:     com_nettention_proud_NetClient
	 * Method:    nativeConnet
	 * Signature: (JLcom/nettention/proud/NetConnectionParam;)Z
	 */
	JNIEXPORT jboolean JNICALL Java_com_nettention_proud_NetClient_nativeConnect(JNIEnv *env, jobject obj, jlong netClient, jobject param)
	{
		CNetConnectionParam para;

		/* serverIP */
		jclass JC_NetConnectionParam = env->GetObjectClass(param);
		jfieldID JF_serverIP = env->GetFieldID(JC_NetConnectionParam, "serverIP", "Ljava/lang/String;");
		jstring JO_serverIP = (jstring) env->GetObjectField(param, JF_serverIP);
		String m_serverIP = StringA2W(env->GetStringUTFChars(JO_serverIP, NULL));
		para.m_serverIP = m_serverIP;

		/* serverPort */
		jfieldID JF_serverPort = env->GetFieldID(JC_NetConnectionParam, "serverPort", "I");
		para.m_serverPort = env->GetIntField(param, JF_serverPort);

		/* guid */
		jfieldID JF_guid = env->GetFieldID(JC_NetConnectionParam, "protocolVersion", "Lcom/nettention/proud/Guid;");
		jobject JO_guid = env->GetObjectField(param, JF_guid);
		jclass JC_guid = env->GetObjectClass(JO_guid);
		jmethodID JM_guid_Most = env->GetMethodID(JC_guid, "getMostSignificantBits", "()J");
		jlong guidMost = env->CallLongMethod(JO_guid, JM_guid_Most);
		jmethodID JM_guid_Last = env->GetMethodID(JC_guid, "getLeastSignificantBits", "()J");
		jlong guidLast = env->CallLongMethod(JO_guid, JM_guid_Last);
		GUID guid = { (unsigned long)(guidMost >> 32L), (short)(guidMost >> 16L), (short)guidMost, { (unsigned char)(guidLast >> 56L), (unsigned char)(guidLast >> 48L), (unsigned char)(guidLast >> 40L), (unsigned char)(guidLast >> 32L),
			(unsigned char)(guidLast >> 24L), (unsigned char)(guidLast >> 16L), (unsigned char)(guidLast >> 8L), (unsigned char)guidLast } };
		para.m_protocolVersion = Guid::From(guid);

		return ((CNetClient *)netClient)->Connect(para);;
	}

	/*
	* Class:     com_nettention_proud_NetClient
	* Method:    nativeDisconnect
	* Signature: (J)V
	*/
	JNIEXPORT void JNICALL Java_com_nettention_proud_NetClient_nativeDisconnect(JNIEnv *env, jobject obj, jlong netClient)
	{
		((CNetClient *)netClient)->Disconnect();
	}

	/*
	* Class:     com_nettention_proud_NetClient
	* Method:    nativeDisconnectA
	* Signature: (JDLcom/nettention/proud/ByteArray;)V
	*/
	JNIEXPORT void JNICALL Java_com_nettention_proud_NetClient_nativeDisconnectA(JNIEnv *env, jobject obj, jlong netClient, jdouble gracefulDisconnectTimeout, jobject comment)
	{

		jclass ByteArrayClass = env->FindClass("com/nettention/proud/ByteArray");

		jfieldID lengthFieldID = env->GetFieldID(ByteArrayClass, "length", "I");

		jint length = env->GetIntField(comment, lengthFieldID);

		jfieldID dataFieldID = env->GetFieldID(ByteArrayClass, "data", "[B");

		jbyteArray data = (jbyteArray) env->GetObjectField(comment, dataFieldID);

		ByteArray tmp;
		if (data != NULL)
		{

			jbyte *data_jbyte = env->GetByteArrayElements(data, NULL);
			for(int i=0; i < length; i++)
			{
				tmp.Add((unsigned char)data_jbyte[i]);
			}
		}

		((CNetClient *)netClient)->Disconnect(gracefulDisconnectTimeout, tmp);
	}

	/*
	* Class:     com_nettention_proud_NetClient
	* Method:    nativeGetDirectP2PInfo
	* Signature: (JILcom/nettention/proud/DirectP2PInfo;)Z
	*/
	JNIEXPORT jboolean JNICALL Java_com_nettention_proud_NetClient_nativeGetDirectP2PInfo(JNIEnv *env, jobject obj, jlong netClient, jint remotePeerID, jobject outDirectP2PInfo)
	{
		jclass DirectP2PInfoClass = env->FindClass("com/nettention/proud/DirectP2PInfo");
		jclass InetSocketAddressClass = env->FindClass("java/net/InetSocketAddress");

		jmethodID InetSocketAddressConstructor = env->GetMethodID(InetSocketAddressClass, "<init>", "(Ljava/lang/String;I)V");

		jfieldID localUdpSocketAddrFieldID = env->GetFieldID(DirectP2PInfoClass, "localUdpSocketAddr", "Lcom/nettention/proud/DirectP2PInfo;");
		jfieldID localToRemoteAddrFieldID = env->GetFieldID(DirectP2PInfoClass, "localToRemoteAddr", "Lcom/nettention/proud/DirectP2PInfo;");
		jfieldID remoteToLocalAddrFieldID = env->GetFieldID(DirectP2PInfoClass, "remoteToLocalAddr", "Lcom/nettention/proud/DirectP2PInfo;");

		CDirectP2PInfo output;;
		jboolean ret = ((CNetClient *)netClient)->GetDirectP2PInfo((HostID)remotePeerID, output);

		/* localUdpSocketAddr */
		const char * localUdpSocketAddr_str = StringW2A(output.m_localUdpSocketAddr.IPToString());
		jstring localUdpSocketAddr_jstr = env->NewStringUTF(localUdpSocketAddr_str);
		jint localUdpSocketAddr_port = output.m_localUdpSocketAddr.m_port;
		jobject localUdpSocketAddr_obj = env->NewObject(InetSocketAddressClass, InetSocketAddressConstructor, localUdpSocketAddr_jstr, localUdpSocketAddr_port);
		env->SetObjectField(outDirectP2PInfo, localUdpSocketAddrFieldID, localUdpSocketAddr_obj);

		/* localToRemoteAddr */
		const char * localToRemoteAddr_str = StringW2A(output.m_localToRemoteAddr.IPToString());
		jstring localToRemoteAddr_jstr = env->NewStringUTF(localToRemoteAddr_str);
		jint localToRemoteAddr_port = output.m_localToRemoteAddr.m_port;
		jobject localToRemoteAddr_obj = env->NewObject(InetSocketAddressClass, InetSocketAddressConstructor, localToRemoteAddr_jstr, localToRemoteAddr_port);
		env->SetObjectField(outDirectP2PInfo, localToRemoteAddrFieldID, localToRemoteAddr_obj);

		/* remoteToLocalAddr */
		const char * remoteToLocalAddr_str = StringW2A(output.m_remoteToLocalAddr.IPToString());
		jstring remoteToLocalAddr_jstr = env->NewStringUTF(remoteToLocalAddr_str);
		jint remoteToLocalAddr_port = output.m_remoteToLocalAddr.m_port;
		jobject remoteToLocalAddr_obj = env->NewObject(InetSocketAddressClass, InetSocketAddressConstructor, remoteToLocalAddr_jstr, remoteToLocalAddr_port);
		env->SetObjectField(outDirectP2PInfo, remoteToLocalAddrFieldID, remoteToLocalAddr_obj);

		return ret;
	}

	/*
	* Class:     com_nettention_proud_NetClient
	* Method:    nativeGetGroupMembers
	* Signature: (JILcom/nettention/proud/HostIDArray;)V
	*/
	JNIEXPORT void JNICALL Java_com_nettention_proud_NetClient_nativeGetGroupMembers(JNIEnv *env, jobject obj, jlong netClient, jint groupHostID, jobject outHostArray)
	{
		jclass HostIDArrayClass = env->FindClass("com/nettention/proud/HostIDArray");

		jmethodID add = env->GetMethodID(HostIDArrayClass, "add", "(Ljava/lang/Integer);");

		jclass IntegerClass = env->FindClass("java/lang/Integer");
		jmethodID IntegerContructor = env->GetMethodID(IntegerClass, "<init>", "(I)V");

		HostIDArray groupMembers;

		((CNetClient*)netClient)->GetGroupMembers((HostID)groupHostID, groupMembers);

		for(int i=0; i < groupMembers.GetCount(); i++)
		{
			jobject value = env->NewObject(IntegerClass, IntegerContructor, (jint)(groupMembers.GetData()[i]));
			env->CallVoidMethod(outHostArray, add, value);
		}

	}

	/*
	* Class:     com_nettention_proud_NetClient
	* Method:    nativeGetIndirectServerTime
	* Signature: (JI)D
	*/
	JNIEXPORT jdouble JNICALL Java_com_nettention_proud_NetClient_nativeGetIndirectServerTime(JNIEnv *env, jobject obj, jlong netClient, jint peerHostID)
	{
		return (jdouble)((CNetClient*)netClient)->GetIndirectServerTime((HostID)peerHostID);
	}

	/*
	* Class:     com_nettention_proud_NetClient
	* Method:    nativeGetInternalVersion
	* Signature: (J)I
	*/
	JNIEXPORT jint JNICALL Java_com_nettention_proud_NetClient_nativeGetInternalVersion(JNIEnv *env, jobject obj, jlong netClient)
	{
		return ((CNetClient *)netClient)->GetInternalVersion();
	}

	/*
	* Class:     com_nettention_proud_NetClient
	* Method:    nativeGetLastPingMilisec
	* Signature: (JI)I
	*/
	JNIEXPORT jint JNICALL Java_com_nettention_proud_NetClient_nativeGetLastPingMilisec(JNIEnv *env, jobject obj, jlong netClient, jint remoteHostID)
	{
		return ((CNetClient*)netClient)->GetLastPingMilisec((HostID)remoteHostID);
	}

	/*
	* Class:     com_nettention_proud_NetClient
	* Method:    nativeGetLastPingSec
	* Signature: (JI)D
	*/
	JNIEXPORT jdouble JNICALL Java_com_nettention_proud_NetClient_nativeGetLastPingSec(JNIEnv *env, jobject obj, jlong netClient, jint remoteHostID)
	{
		return (jdouble)((CNetClient*)netClient)->GetLastPingSec((HostID)remoteHostID);
	}

	/*
	* Class:     com_nettention_proud_NetClient
	* Method:    nativeGetLocalJoinedP2PGroups
	* Signature: (JLcom/nettention/proud/HostIDArray;)V
	*/
	JNIEXPORT void JNICALL Java_com_nettention_proud_NetClient_nativeGetLocalJoinedP2PGroups(JNIEnv *env, jobject obj, jlong netClient, jobject outHostIDArray)
	{
		jclass HostIDArrayClass = env->FindClass("com/nettention/proud/HostIDArray");

		jmethodID add = env->GetMethodID(HostIDArrayClass, "add", "(Ljava/lang/Integer);");

		jclass IntegerClass = env->FindClass("java/lang/Integer");
		jmethodID IntegerContructor = env->GetMethodID(IntegerClass, "<init>", "(I)V");

		HostIDArray groupMembers;

		((CNetClient*)netClient)->GetLocalJoinedP2PGroups(groupMembers);

		for(int i=0; i < groupMembers.GetCount(); i++)
		{
			jobject value = env->NewObject(IntegerClass, IntegerContructor, (jint)(groupMembers.GetData()[i]));
			env->CallVoidMethod(outHostIDArray, add, value);
		}
	}

	/*
	* Class:     com_nettention_proud_NetClient
	* Method:    nativeGetMessageMaxLength
	* Signature: (J)I
	*/
	JNIEXPORT jint JNICALL Java_com_nettention_proud_NetClient_nativeGetMessageMaxLength(JNIEnv *env, jobject obj, jlong netClient)
	{
		return ((CNetClient*)netClient)->GetMessageMaxLength();
	}

	/*
	* Class:     com_nettention_proud_NetClient
	* Method:    nativeGetNatDeviceName
	* Signature: (J)Ljava/lang/String;
	*/
	JNIEXPORT jstring JNICALL Java_com_nettention_proud_NetClient_nativeGetNatDeviceName(JNIEnv *env, jobject obj, jlong netClient)
	{
		String NatDeviceName = ((CNetClient*)netClient)->GetNatDeviceName();
		const char * NatDeviceName_str = StringW2A(NatDeviceName);
		return env->NewStringUTF(NatDeviceName_str);
	}

	/*
	* Class:     com_nettention_proud_NetClient
	* Method:    nativeGetP2PServerTime
	* Signature: (JI)D
	*/
	JNIEXPORT jdouble JNICALL Java_com_nettention_proud_NetClient_nativeGetP2PServerTime(JNIEnv *env, jobject obj, jlong netClient, jint groupID)
	{
		return (jdouble)((CNetClient*)netClient)->GetP2PServerTime((HostID)groupID);
	}

	/*
	* Class:     com_nettention_proud_NetClient
	* Method:    nativeGetPeerInfo
	* Signature: (JILcom/nettention/proud/NetPeerInfo;)Z
	*/
	JNIEXPORT jboolean JNICALL Java_com_nettention_proud_NetClient_nativeGetPeerInfo(JNIEnv *env, jobject obj, jlong netClient, jint peerHostID, jobject outNetPeerInfo)
	{
		jclass NetPeerInfoClass = env->FindClass("com/nettention/proud/NetPeerInfo");
		jclass InetSocketAddressClass = env->FindClass("java/net/InetSocketAddress");
		jclass ObjectClass = env->FindClass("java/lang/Ojbejct");

		jfieldID udpAddrFromServerFieldID = env->GetFieldID(NetPeerInfoClass, "udpAddrFromServer", "Ljava/net/InetSocketAddress;");
		jfieldID udpAddrInternalFieldID = env->GetFieldID(NetPeerInfoClass, "udpAddrInternal", "Ljava/net/InetSocketAddress;");

		jmethodID InetSocketAddressConstructor = env->GetMethodID(InetSocketAddressClass, "<init>", "(Ljava/lang/String;I)V");
		jmethodID ObjectConstructor = env->GetMethodID(ObjectClass, "<init>", "()V");

		jfieldID hostIDFieldID = env->GetFieldID(NetPeerInfoClass, "hostID", "I");
		jfieldID relayedP2PFieldID = env->GetFieldID(NetPeerInfoClass, "relayedP2P", "Z");
		jfieldID isBehindNatFieldID = env->GetFieldID(NetPeerInfoClass, "isBehindNat", "Z");
		jfieldID realUdpEnabledFieldID = env->GetFieldID(NetPeerInfoClass, "realUdpEnabled", "Z");
		jfieldID recentPingFieldID = env->GetFieldID(NetPeerInfoClass, "recentPing", "D");
		jfieldID sendQueuedAmountInBytesFieldID = env->GetFieldID(NetPeerInfoClass, "sendQueuedAmountInBytes", "J");

		jfieldID hostTagFieldID = env->GetFieldID(NetPeerInfoClass, "hostTag", "Ljava/lang/Object;");

		jfieldID directP2PPeerFrameRateFieldID = env->GetFieldID(NetPeerInfoClass, "directP2PPeerFrameRate", "D");
		jfieldID toRemotePeerSendUdpMessageTrialCountFieldID = env->GetFieldID(NetPeerInfoClass, "toRemotePeerSendUdpMessageTrialCount", "J");
		jfieldID toRemotePeerSendUdpMessageSuccessCountFieldID = env->GetFieldID(NetPeerInfoClass, "toRemotePeerSendUdpMessageSuccessCount", "J");

		CNetPeerInfo tmp;
		jboolean ret = ((CNetClient*)netClient)->GetPeerInfo((HostID)peerHostID, tmp);

		/* udpAddrFromServer */
		const char * udpAddrFromServer_str = StringW2A(tmp.m_UdpAddrFromServer.IPToString());
		jstring udpAddrFromServer_jstr = env->NewStringUTF(udpAddrFromServer_str);
		jint udpAddrFromServer_port = tmp.m_UdpAddrFromServer.m_port;
		jobject udpAddrFromServer_obj = env->NewObject(InetSocketAddressClass, InetSocketAddressConstructor, udpAddrFromServer_jstr, udpAddrFromServer_port);
		env->SetObjectField(outNetPeerInfo, udpAddrFromServerFieldID, udpAddrFromServer_obj);

		/* udpAddrInternal */
		const char * udpAddrInternal_str = StringW2A(tmp.m_UdpAddrInternal.IPToString());
		jstring udpAddrInternal_jstr = env->NewStringUTF(udpAddrInternal_str);
		jint udpAddrInternal_port = tmp.m_UdpAddrInternal.m_port;
		jobject udpAddrInternal_obj = env->NewObject(InetSocketAddressClass, InetSocketAddressConstructor, udpAddrInternal_jstr, udpAddrInternal_port);
		env->SetObjectField(outNetPeerInfo, udpAddrInternalFieldID, udpAddrInternal_obj);

		/* hostTag */
		/* ?????? */

		env->SetIntField(outNetPeerInfo, hostIDFieldID, tmp.m_HostID);
		env->SetBooleanField(outNetPeerInfo, relayedP2PFieldID, tmp.m_RelayedP2P);
		env->SetBooleanField(outNetPeerInfo, isBehindNatFieldID, tmp.m_isBehindNat);
		env->SetBooleanField(outNetPeerInfo, realUdpEnabledFieldID, tmp.m_realUdpEnabled);
		env->SetDoubleField(outNetPeerInfo, recentPingFieldID, (jdouble)tmp.m_recentPing);
		env->SetLongField(outNetPeerInfo, sendQueuedAmountInBytesFieldID, (jlong)tmp.m_sendQueuedAmountInBytes);
		env->SetDoubleField(outNetPeerInfo, directP2PPeerFrameRateFieldID, (jdouble)tmp.m_directP2PPeerFrameRate);
		env->SetLongField(outNetPeerInfo, toRemotePeerSendUdpMessageTrialCountFieldID, (jlong)tmp.m_toRemotePeerSendUdpMessageTrialCount);
		env->SetLongField(outNetPeerInfo, toRemotePeerSendUdpMessageSuccessCountFieldID, (jlong)tmp.m_toRemotePeerSendUdpMessageSuccessCount);

		return ret;
	}

	/*
	* Class:     com_nettention_proud_NetClient
	* Method:    nativeGetPeerReliableUdpStats
	* Signature: (JILcom/nettention/proud/ReliableUdpHostStats;)Z
	*/
	JNIEXPORT jboolean JNICALL Java_com_nettention_proud_NetClient_nativeGetPeerReliableUdpStats(JNIEnv *env, jobject obj, jlong netClient, jint peerID, jobject outReliableUdpHostStats)
	{
		jclass ReliableUdpHostStatsClass = env->FindClass("com/nettention/proud/ReliableUdpHostStats");

		jfieldID receivedFrameCountFieldID = env->GetFieldID(ReliableUdpHostStatsClass, "receivedFrameCount", "I");
		jfieldID receivedStreamCountFieldID = env->GetFieldID(ReliableUdpHostStatsClass, "receivedStreamCount", "I");
		jfieldID totalReceivedStreamLengthFieldID = env->GetFieldID(ReliableUdpHostStatsClass, "totalReceivedStreamLength", "I");
		jfieldID totalAckFrameCountFieldID = env->GetFieldID(ReliableUdpHostStatsClass, "totalAckFrameCount", "I");
		jfieldID recentReceiveSpeedFieldID = env->GetFieldID(ReliableUdpHostStatsClass, "recentReceiveSpeed", "I");

		jfieldID expectedFrameNumberFieldID = env->GetFieldID(ReliableUdpHostStatsClass, "expectedFrameNumber", "I");
		jfieldID lastReceivedDataFrameNumberFieldID = env->GetFieldID(ReliableUdpHostStatsClass, "lastReceivedDataFrameNumber", "I");
		jfieldID sendStreamCountFieldID = env->GetFieldID(ReliableUdpHostStatsClass, "sendStreamCount", "I");
		jfieldID firstSendFrameCountFieldID = env->GetFieldID(ReliableUdpHostStatsClass, "firstSendFrameCount", "I");
		jfieldID resendFrameCountFieldID = env->GetFieldID(ReliableUdpHostStatsClass, "resendFrameCount", "I");

		jfieldID totalSendStreamLengthFieldID = env->GetFieldID(ReliableUdpHostStatsClass, "totalSendStreamLength", "I");
		jfieldID totalResendCountFieldID = env->GetFieldID(ReliableUdpHostStatsClass, "totalResendCount", "I");
		jfieldID totalFirstSendCountFieldID = env->GetFieldID(ReliableUdpHostStatsClass, "totalFirstSendCount", "I");
		jfieldID recentSendFrameToUdpSpeedFieldID = env->GetFieldID(ReliableUdpHostStatsClass, "recentSendFrameToUdpSpeed", "I");
		jfieldID sendSpeedLimitFieldID = env->GetFieldID(ReliableUdpHostStatsClass, "sendSpeedLimit", "I");

		jfieldID firstSenderWindowLastFrameFieldID = env->GetFieldID(ReliableUdpHostStatsClass, "firstSenderWindowLastFrame", "I");
		jfieldID resendWindowLastFrameFieldID = env->GetFieldID(ReliableUdpHostStatsClass, "resendWindowLastFrame", "I");
		jfieldID lastExpectedFrameNumberAtSenderFieldID = env->GetFieldID(ReliableUdpHostStatsClass, "lastExpectedFrameNumberAtSender", "I");
		jfieldID totalReceiveDataCountFieldID = env->GetFieldID(ReliableUdpHostStatsClass, "totalReceiveDataCount", "I");


		ReliableUdpHostStats tmp;
		jboolean ret = ((CNetClient*)netClient)->GetPeerReliableUdpStats((HostID)peerID, tmp);

		env->SetIntField(outReliableUdpHostStats, receivedFrameCountFieldID, tmp.m_receivedFrameCount);
		env->SetIntField(outReliableUdpHostStats, receivedStreamCountFieldID, tmp.m_receivedStreamCount);
		env->SetIntField(outReliableUdpHostStats, totalReceivedStreamLengthFieldID, tmp.m_totalReceivedStreamLength);
		env->SetIntField(outReliableUdpHostStats, totalAckFrameCountFieldID, tmp.m_totalAckFrameCount);
		env->SetIntField(outReliableUdpHostStats, recentReceiveSpeedFieldID, tmp.m_recentReceiveSpeed);

		env->SetIntField(outReliableUdpHostStats, expectedFrameNumberFieldID, tmp.m_expectedFrameNumber);
		env->SetIntField(outReliableUdpHostStats, lastReceivedDataFrameNumberFieldID, tmp.m_lastReceivedDataFrameNumber);
		env->SetIntField(outReliableUdpHostStats, sendStreamCountFieldID, tmp.m_sendStreamCount);
		env->SetIntField(outReliableUdpHostStats, firstSendFrameCountFieldID, tmp.m_firstSendFrameCount);
		env->SetIntField(outReliableUdpHostStats, resendFrameCountFieldID, tmp.m_resendFrameCount);

		env->SetIntField(outReliableUdpHostStats, totalSendStreamLengthFieldID, tmp.m_totalSendStreamLength);
		env->SetIntField(outReliableUdpHostStats, totalResendCountFieldID, tmp.m_totalResendCount);
		env->SetIntField(outReliableUdpHostStats, totalFirstSendCountFieldID, tmp.m_totalFirstSendCount);
		env->SetIntField(outReliableUdpHostStats, recentSendFrameToUdpSpeedFieldID, tmp.m_recentSendFrameToUdpSpeed);
		env->SetIntField(outReliableUdpHostStats, sendSpeedLimitFieldID, tmp.m_sendSpeedLimit);

		env->SetIntField(outReliableUdpHostStats, firstSenderWindowLastFrameFieldID, tmp.m_firstSenderWindowLastFrame);
		env->SetIntField(outReliableUdpHostStats, resendWindowLastFrameFieldID, tmp.m_resendWindowLastFrame);
		env->SetIntField(outReliableUdpHostStats, lastExpectedFrameNumberAtSenderFieldID, tmp.m_lastExpectedFrameNumberAtSender);
		env->SetIntField(outReliableUdpHostStats, totalReceiveDataCountFieldID, tmp.m_totalReceiveDataCount);

		return ret;
	}

	/*
	* Class:     com_nettention_proud_NetClient
	* Method:    nativeGetRecentPingMilisec
	* Signature: (JI)I
	*/
	JNIEXPORT jint JNICALL Java_com_nettention_proud_NetClient_nativeGetRecentPingMilisec(JNIEnv *env, jobject obj, jlong netClient, jint remoteHostID)
	{
		return ((CNetClient*)netClient)->GetRecentPingMilisec((HostID)remoteHostID);
	}

	/*
	* Class:     com_nettention_proud_NetClient
	* Method:    nativeGetRecentPingSec
	* Signature: (JI)D
	*/
	JNIEXPORT jdouble JNICALL Java_com_nettention_proud_NetClient_nativeGetRecentPingSec(JNIEnv *env, jobject obj, jlong netClient, jint remoteHostID)
	{
		return (jdouble)((CNetClient*)netClient)->GetRecentPingSec((HostID)remoteHostID);
	}

	/*
	* Class:     com_nettention_proud_NetClient
	* Method:    nativeGetSendToServerSpeed
	* Signature: (J)D
	*/
	JNIEXPORT jdouble JNICALL Java_com_nettention_proud_NetClient_nativeGetSendToServerSpeed(JNIEnv *env, jobject obj, jlong netClient)
	{
		return (jdouble)((CNetClient*)netClient)->GetSendToServerSpeed();
	}

	/*
	* Class:     com_nettention_proud_NetClient
	* Method:    nativeGetServerConnectionState
	* Signature: (JLcom/nettention/proud/ServerConnectionState;)Lcom/nettention/proud/ConnectionState;
	*/
	JNIEXPORT jobject JNICALL Java_com_nettention_proud_NetClient_nativeGetServerConnectionState(JNIEnv *env, jobject obj, jlong netClient, jobject outServerConnectionState)
	{
		jclass ServerConnectionStateClass = env->FindClass("com/nettention/proud/ServerConnectionState");
		jclass ConnctionStateClass = env->FindClass("com/nettention/proud/ConnectionState");

		jfieldID realUdpEnabledFieldID = env->GetFieldID(ServerConnectionStateClass, "realUdpEnabled", "Z");

		CServerConnectionState tmp;
		ConnectionState tmp2 = ((CNetClient*)netClient)->GetServerConnectionState(tmp);

		env->SetBooleanField(outServerConnectionState, realUdpEnabledFieldID, (jboolean)tmp.m_realUdpEnabled);
		jobject ret;
		jfieldID ConnectionStateFieldID;
		switch (tmp2)
		{
		case ConnectionState_Connected :
			ConnectionStateFieldID = env->GetStaticFieldID(ConnctionStateClass, "Connected", "Lcom/nettention/proud/ServerConnectionState;");
			ret = env->GetStaticObjectField(ConnctionStateClass, ConnectionStateFieldID);
			break;
		case ConnectionState_Connecting :
			ConnectionStateFieldID = env->GetStaticFieldID(ConnctionStateClass, "Connecting", "Lcom/nettention/proud/ServerConnectionState;");
			ret = env->GetStaticObjectField(ConnctionStateClass, ConnectionStateFieldID);
			break;
		case ConnectionState_Disconnected :
			ConnectionStateFieldID = env->GetStaticFieldID(ConnctionStateClass, "Disconnected", "Lcom/nettention/proud/ServerConnectionState;");
			ret = env->GetStaticObjectField(ConnctionStateClass, ConnectionStateFieldID);
			break;
		case ConnectionState_Disconnecting :
			ConnectionStateFieldID = env->GetStaticFieldID(ConnctionStateClass, "Disconnecting", "Lcom/nettention/proud/ServerConnectionState;");
			ret = env->GetStaticObjectField(ConnctionStateClass, ConnectionStateFieldID);
			break;
		}

		return ret;
	}

	/*
	* Class:     com_nettention_proud_NetClient
	* Method:    nativeGetServerTime
	* Signature: (J)D
	*/
	JNIEXPORT jdouble JNICALL Java_com_nettention_proud_NetClient_nativeGetServerTime(JNIEnv *env, jobject obj, jlong netClient)
	{
		return (jdouble)((CNetClient*)netClient)->GetServerTime();
	}

	/*
	* Class:     com_nettention_proud_NetClient
	* Method:    nativeGetServerTimeDiff
	* Signature: (J)D
	*/
	JNIEXPORT jdouble JNICALL Java_com_nettention_proud_NetClient_nativeGetServerTimeDiff(JNIEnv *env, jobject obj, jlong netClient)
	{
		return (jdouble)((CNetClient*)netClient)->GetServerTimeDiff();
	}

	/*
	* Class:     com_nettention_proud_NetClient
	* Method:    nativeGetStats
	* Signature: (JLcom/nettention/proud/NetClientStats;)V
	*/
	JNIEXPORT void JNICALL Java_com_nettention_proud_NetClient_nativeGetStats(JNIEnv *env, jobject obj, jlong netClient, jobject outNetClientStats)
	{
		jclass NetClientStatsClass = env->FindClass("com/nettention/proud/NetClientStats");
		jfieldID totalTcpReceiveBytesFieldID = env->GetFieldID(NetClientStatsClass, "totalTcpReceiveBytes", "J");
		jfieldID totalTcpSendBytesFieldID = env->GetFieldID(NetClientStatsClass, "totalTcpSendBytes", "J");
		jfieldID totalUdpSendCountFieldID = env->GetFieldID(NetClientStatsClass, "totalUdpSendCount", "J");
		jfieldID totalUdpSendBytesFieldID = env->GetFieldID(NetClientStatsClass, "totalUdpSendBytes", "J");
		jfieldID totalUdpReceiveCountFieldID = env->GetFieldID(NetClientStatsClass, "totalUdpReceiveCount", "J");
		jfieldID totalUdpReceiveBytesFieldID = env->GetFieldID(NetClientStatsClass, "totalUdpReceiveBytes", "J");
		jfieldID remotePeerCountFieldID = env->GetFieldID(NetClientStatsClass, "remotePeerCount", "I");
		jfieldID serverUdpEnabledFieldID = env->GetFieldID(NetClientStatsClass, "serverUdpEnabled", "Z");
		jfieldID directP2PEnabledPeerCountFieldID = env->GetFieldID(NetClientStatsClass, "directP2PEnabledPeerCount", "I");

		CNetClientStats tmp;
		((CNetClient*)netClient)->GetStats(tmp);

		env->SetLongField(outNetClientStats, totalTcpReceiveBytesFieldID, tmp.GetTotalReceiveBytes());
		env->SetLongField(outNetClientStats, totalTcpSendBytesFieldID, tmp.m_totalTcpSendBytes);
		env->SetLongField(outNetClientStats, totalUdpSendCountFieldID, tmp.m_totalUdpSendCount);
		env->SetLongField(outNetClientStats, totalUdpSendBytesFieldID, tmp.m_totalUdpSendBytes);
		env->SetLongField(outNetClientStats, totalUdpReceiveCountFieldID, tmp.m_totalUdpReceiveCount);
		env->SetLongField(outNetClientStats, totalUdpReceiveBytesFieldID, tmp.m_totalUdpReceiveBytes);
		env->SetIntField(outNetClientStats, remotePeerCountFieldID, tmp.m_remotePeerCount);
		env->SetBooleanField(outNetClientStats, serverUdpEnabledFieldID, tmp.m_serverUdpEnabled);
		env->SetIntField(outNetClientStats, directP2PEnabledPeerCountFieldID, tmp.m_directP2PEnabledPeerCount);

	}

	/*
	* Class:     com_nettention_proud_NetClient
	* Method:    nativeHasServerConnection
	* Signature: (J)Z
	*/
	JNIEXPORT jboolean JNICALL Java_com_nettention_proud_NetClient_nativeHasServerConnection(JNIEnv *env, jobject obj, jlong netClient)
	{
		return ((CNetClient*)netClient)->HasServerConnection();
	}

	/*
	* Class:     com_nettention_proud_NetClient
	* Method:    nativeInvalidateUdpSocket
	* Signature: (JILcom/nettention/proud/DirectP2PInfo;)Z
	*/
	JNIEXPORT jboolean JNICALL Java_com_nettention_proud_NetClient_nativeInvalidateUdpSocket(JNIEnv *env, jobject obj, jlong netClient, jint peerID, jobject outDirectoryP2PInfo)
	{
		jclass DirectP2PInfoClass = env->FindClass("com/nettention/proud/DirectP2PInfo");
		jfieldID localUdpSocketAddr = env->GetFieldID(DirectP2PInfoClass, "localUdpSocketAddr", "Ljava/net/InetSocketAddress;");
		jfieldID localToRemoteAddr = env->GetFieldID(DirectP2PInfoClass, "localToRemoteAddr", "Ljava/net/InetSocketAddress;");
		jfieldID remoteToLocalAddr = env->GetFieldID(DirectP2PInfoClass, "remoteToLocalAddr", "Ljava/net/InetSocketAddress;");

		jclass InetSocketAddressClass = env->FindClass("java/net/InetSocketAddress");
		jmethodID InetSocketAddressContructor = env->GetMethodID(InetSocketAddressClass, "<init>", "(Ljava/lang/String;I)V");

		CDirectP2PInfo tmp;
		jboolean ret = ((CNetClient*)netClient)->InvalidateUdpSocket((HostID)peerID, tmp);

		/* localRemoteAddr */
		String localToRemoteAddr_str = tmp.m_localToRemoteAddr.IPToString();
		const char * localToRemoteAddr_cstr = StringW2A(localToRemoteAddr_str);
		jstring localToRemoteAddr_jstr = env->NewStringUTF(localToRemoteAddr_cstr);
		jint localToRemoteAddr_port = tmp.m_localToRemoteAddr.m_port;

		jobject localToRemoteAddr_obj = env->NewObject(InetSocketAddressClass, InetSocketAddressContructor, localToRemoteAddr_jstr, localToRemoteAddr_port);
		env->SetObjectField(outDirectoryP2PInfo, localToRemoteAddr, localToRemoteAddr_obj);

		/* localUdpSocketAddr */
		String localUdpSocketAddr_str = tmp.m_localUdpSocketAddr.IPToString();
		const char * localUdpSocketAddr_cstr = StringW2A(localUdpSocketAddr_str);
		jstring localUdpSocketAddr_jstr = env->NewStringUTF(localUdpSocketAddr_cstr);
		jint localUdpSocketAddr_port = tmp.m_localUdpSocketAddr.m_port;

		jobject localUdpSocketAddr_obj = env->NewObject(InetSocketAddressClass, InetSocketAddressContructor, localUdpSocketAddr_jstr, localUdpSocketAddr_port);
		env->SetObjectField(outDirectoryP2PInfo, localUdpSocketAddr, localUdpSocketAddr_obj);

		/* remoteToLocalAddr */
		String remoteToLocalAddr_str = tmp.m_remoteToLocalAddr.IPToString();
		const char * remoteToLocalAddr_cstr = StringW2A(remoteToLocalAddr_str);
		jstring remoteToLocalAddr_jstr = env->NewStringUTF(remoteToLocalAddr_cstr);
		jint remoteToLocalAddr_port = tmp.m_remoteToLocalAddr.m_port;

		jobject remoteToLocalAddr_obj = env->NewObject(InetSocketAddressClass, InetSocketAddressContructor, remoteToLocalAddr_jstr, remoteToLocalAddr_port);
		env->SetObjectField(outDirectoryP2PInfo, remoteToLocalAddr, remoteToLocalAddr_obj);

		return ret;
	}

	/*
	* Class:     com_nettention_proud_NetClient
	* Method:    nativeSetUseNetworkerThread
	* Signature: (JZ)V
	*/
	JNIEXPORT void JNICALL Java_com_nettention_proud_NetClient_nativeSetUseNetworkerThread(JNIEnv *env, jobject obj, jlong netClient, jboolean value)
	{
		CNetClient::UseNetworkerThread_EveryInstance(value);
	}

#ifdef __cplusplus
}
#endif
