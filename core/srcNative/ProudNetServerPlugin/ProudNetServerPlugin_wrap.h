/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 3.0.7
 *
 * This file is not intended to be easily readable and contains a number of
 * coding conventions designed to improve portability and efficiency. Do not make
 * changes to this file unless you know what you are doing--modify the SWIG
 * interface file instead.
 * ----------------------------------------------------------------------------- */

#ifndef SWIG_ProudNetServerPlugin_WRAP_H_
#define SWIG_ProudNetServerPlugin_WRAP_H_

class SwigDirector_INetCoreEvent : public Proud::INetCoreEvent, public Swig::Director {

public:
    SwigDirector_INetCoreEvent();
    virtual ~SwigDirector_INetCoreEvent();
    virtual void OnError(Proud::ErrorInfo *errorInfo);
    virtual void OnWarning(Proud::ErrorInfo *errorInfo);
    virtual void OnInformation(Proud::ErrorInfo *errorInfo);
    virtual void OnException(Exception const &e);
    virtual void OnNoRmiProcessed(Proud::RmiID rmiID);
    virtual void OnReceiveUserMessage(Proud::HostID sender, Proud::RmiContext const &rmiContext, uint8_t *payload, int payloadLength);
    virtual void OnTick(void *arg0);
    virtual void OnUserWorkerThreadCallbackBegin(Proud::CUserWorkerThreadCallbackContext *arg0);
    virtual void OnUserWorkerThreadCallbackEnd(Proud::CUserWorkerThreadCallbackContext *arg0);

    typedef void (SWIGSTDCALL* SWIG_Callback0_t)(void *);
    typedef void (SWIGSTDCALL* SWIG_Callback1_t)(void *);
    typedef void (SWIGSTDCALL* SWIG_Callback2_t)(void *);
    typedef void (SWIGSTDCALL* SWIG_Callback3_t)(void *);
    typedef void (SWIGSTDCALL* SWIG_Callback4_t)(unsigned short);
    typedef void (SWIGSTDCALL* SWIG_Callback5_t)(int, void *, void *, int);
    typedef void (SWIGSTDCALL* SWIG_Callback6_t)(void *);
    typedef void (SWIGSTDCALL* SWIG_Callback7_t)(void *);
    typedef void (SWIGSTDCALL* SWIG_Callback8_t)(void *);
    void swig_connect_director(SWIG_Callback0_t callbackOnError, SWIG_Callback1_t callbackOnWarning, SWIG_Callback2_t callbackOnInformation, SWIG_Callback3_t callbackOnException, SWIG_Callback4_t callbackOnNoRmiProcessed, SWIG_Callback5_t callbackOnReceiveUserMessage, SWIG_Callback6_t callbackOnTick, SWIG_Callback7_t callbackOnUserWorkerThreadCallbackBegin, SWIG_Callback8_t callbackOnUserWorkerThreadCallbackEnd);

private:
    SWIG_Callback0_t swig_callbackOnError;
    SWIG_Callback1_t swig_callbackOnWarning;
    SWIG_Callback2_t swig_callbackOnInformation;
    SWIG_Callback3_t swig_callbackOnException;
    SWIG_Callback4_t swig_callbackOnNoRmiProcessed;
    SWIG_Callback5_t swig_callbackOnReceiveUserMessage;
    SWIG_Callback6_t swig_callbackOnTick;
    SWIG_Callback7_t swig_callbackOnUserWorkerThreadCallbackBegin;
    SWIG_Callback8_t swig_callbackOnUserWorkerThreadCallbackEnd;
    void swig_init_callbacks();
};

class SwigDirector_INetServerEvent : public Proud::INetServerEvent, public Swig::Director {

public:
    SwigDirector_INetServerEvent();
    virtual ~SwigDirector_INetServerEvent();
    virtual void OnError(Proud::ErrorInfo *errorInfo);
    virtual void OnWarning(Proud::ErrorInfo *errorInfo);
    virtual void OnInformation(Proud::ErrorInfo *errorInfo);
    virtual void OnException(Exception const &e);
    virtual void OnNoRmiProcessed(Proud::RmiID rmiID);
    virtual void OnReceiveUserMessage(Proud::HostID sender, Proud::RmiContext const &rmiContext, uint8_t *payload, int payloadLength);
    virtual void OnTick(void *arg0);
    virtual void OnUserWorkerThreadCallbackBegin(Proud::CUserWorkerThreadCallbackContext *arg0);
    virtual void OnUserWorkerThreadCallbackEnd(Proud::CUserWorkerThreadCallbackContext *arg0);
    virtual void OnClientJoin(Proud::CNetClientInfo *clientInfo);
    virtual void OnClientLeave(Proud::CNetClientInfo *clientInfo, Proud::ErrorInfo *errorinfo, Proud::ByteArray const &comment);
    virtual void OnClientOffline(CRemoteOfflineEventArgs &arg0);
    virtual void OnClientOnline(CRemoteOnlineEventArgs &arg0);
    virtual bool OnConnectionRequest(AddrPort arg0, Proud::ByteArray &arg1, Proud::ByteArray &arg2);
    virtual void OnP2PGroupJoinMemberAckComplete(Proud::HostID groupHostID, Proud::HostID memberHostID, Proud::ErrorType result);
    virtual void OnUserWorkerThreadBegin();
    virtual void OnUserWorkerThreadEnd();
    virtual void OnClientHackSuspected(Proud::HostID arg0, Proud::HackType arg1);
    virtual void OnP2PGroupRemoved(Proud::HostID arg0);

    typedef void (SWIGSTDCALL* SWIG_Callback0_t)(void *);
    typedef void (SWIGSTDCALL* SWIG_Callback1_t)(void *);
    typedef void (SWIGSTDCALL* SWIG_Callback2_t)(void *);
    typedef void (SWIGSTDCALL* SWIG_Callback3_t)(void *);
    typedef void (SWIGSTDCALL* SWIG_Callback4_t)(unsigned short);
    typedef void (SWIGSTDCALL* SWIG_Callback5_t)(int, void *, void *, int);
    typedef void (SWIGSTDCALL* SWIG_Callback6_t)(void *);
    typedef void (SWIGSTDCALL* SWIG_Callback7_t)(void *);
    typedef void (SWIGSTDCALL* SWIG_Callback8_t)(void *);
    typedef void (SWIGSTDCALL* SWIG_Callback9_t)(void *);
    typedef void (SWIGSTDCALL* SWIG_Callback10_t)(void *, void *, void *);
    typedef void (SWIGSTDCALL* SWIG_Callback11_t)(void *);
    typedef void (SWIGSTDCALL* SWIG_Callback12_t)(void *);
    typedef unsigned int (SWIGSTDCALL* SWIG_Callback13_t)(void *, void *, void *);
    typedef void (SWIGSTDCALL* SWIG_Callback14_t)(int, int, int);
    typedef void (SWIGSTDCALL* SWIG_Callback15_t)();
    typedef void (SWIGSTDCALL* SWIG_Callback16_t)();
    typedef void (SWIGSTDCALL* SWIG_Callback17_t)(int, int);
    typedef void (SWIGSTDCALL* SWIG_Callback18_t)(int);
    void swig_connect_director(SWIG_Callback0_t callbackOnError, SWIG_Callback1_t callbackOnWarning, SWIG_Callback2_t callbackOnInformation, SWIG_Callback3_t callbackOnException, SWIG_Callback4_t callbackOnNoRmiProcessed, SWIG_Callback5_t callbackOnReceiveUserMessage, SWIG_Callback6_t callbackOnTick, SWIG_Callback7_t callbackOnUserWorkerThreadCallbackBegin, SWIG_Callback8_t callbackOnUserWorkerThreadCallbackEnd, SWIG_Callback9_t callbackOnClientJoin, SWIG_Callback10_t callbackOnClientLeave, SWIG_Callback11_t callbackOnClientOffline, SWIG_Callback12_t callbackOnClientOnline, SWIG_Callback13_t callbackOnConnectionRequest, SWIG_Callback14_t callbackOnP2PGroupJoinMemberAckComplete, SWIG_Callback15_t callbackOnUserWorkerThreadBegin, SWIG_Callback16_t callbackOnUserWorkerThreadEnd, SWIG_Callback17_t callbackOnClientHackSuspected, SWIG_Callback18_t callbackOnP2PGroupRemoved);

private:
    SWIG_Callback0_t swig_callbackOnError;
    SWIG_Callback1_t swig_callbackOnWarning;
    SWIG_Callback2_t swig_callbackOnInformation;
    SWIG_Callback3_t swig_callbackOnException;
    SWIG_Callback4_t swig_callbackOnNoRmiProcessed;
    SWIG_Callback5_t swig_callbackOnReceiveUserMessage;
    SWIG_Callback6_t swig_callbackOnTick;
    SWIG_Callback7_t swig_callbackOnUserWorkerThreadCallbackBegin;
    SWIG_Callback8_t swig_callbackOnUserWorkerThreadCallbackEnd;
    SWIG_Callback9_t swig_callbackOnClientJoin;
    SWIG_Callback10_t swig_callbackOnClientLeave;
    SWIG_Callback11_t swig_callbackOnClientOffline;
    SWIG_Callback12_t swig_callbackOnClientOnline;
    SWIG_Callback13_t swig_callbackOnConnectionRequest;
    SWIG_Callback14_t swig_callbackOnP2PGroupJoinMemberAckComplete;
    SWIG_Callback15_t swig_callbackOnUserWorkerThreadBegin;
    SWIG_Callback16_t swig_callbackOnUserWorkerThreadEnd;
    SWIG_Callback17_t swig_callbackOnClientHackSuspected;
    SWIG_Callback18_t swig_callbackOnP2PGroupRemoved;
    void swig_init_callbacks();
};


#endif
