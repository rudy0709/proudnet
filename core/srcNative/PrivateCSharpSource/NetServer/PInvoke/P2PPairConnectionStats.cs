//------------------------------------------------------------------------------
// <auto-generated />
//
// This file was automatically generated by SWIG (http://www.swig.org).
// Version 3.0.7
//
// Do not make changes to this file unless you know what you are doing--modify
// the SWIG interface file instead.
//------------------------------------------------------------------------------

namespace Nettention.Proud {

public class P2PPairConnectionStats : global::System.IDisposable {
  private global::System.Runtime.InteropServices.HandleRef swigCPtr;
  protected bool swigCMemOwn;

  public P2PPairConnectionStats(global::System.IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = new global::System.Runtime.InteropServices.HandleRef(this, cPtr);
  }

  public static global::System.Runtime.InteropServices.HandleRef getCPtr(P2PPairConnectionStats obj) {
    return (obj == null) ? new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero) : obj.swigCPtr;
  }

  ~P2PPairConnectionStats() {
    Dispose();
  }

  public virtual void Dispose() {
    lock(this) {
      if (swigCPtr.Handle != global::System.IntPtr.Zero) {
        if (swigCMemOwn) {
          swigCMemOwn = false;
          ProudNetServerPluginPINVOKE.delete_P2PPairConnectionStats(swigCPtr);
        }
        swigCPtr = new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero);
      }
      global::System.GC.SuppressFinalize(this);
    }
  }

  public uint m_toRemoteBSendUdpMessageTrialCount {
    set {
      ProudNetServerPluginPINVOKE.P2PPairConnectionStats_m_toRemoteBSendUdpMessageTrialCount_set(swigCPtr, value);
      if (ProudNetServerPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetServerPluginPINVOKE.SWIGPendingException.Retrieve();
    } 
    get {
      uint ret = ProudNetServerPluginPINVOKE.P2PPairConnectionStats_m_toRemoteBSendUdpMessageTrialCount_get(swigCPtr);
      if (ProudNetServerPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetServerPluginPINVOKE.SWIGPendingException.Retrieve();
      return ret;
    } 
  }

  public uint m_toRemoteBSendUdpMessageSuccessCount {
    set {
      ProudNetServerPluginPINVOKE.P2PPairConnectionStats_m_toRemoteBSendUdpMessageSuccessCount_set(swigCPtr, value);
      if (ProudNetServerPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetServerPluginPINVOKE.SWIGPendingException.Retrieve();
    } 
    get {
      uint ret = ProudNetServerPluginPINVOKE.P2PPairConnectionStats_m_toRemoteBSendUdpMessageSuccessCount_get(swigCPtr);
      if (ProudNetServerPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetServerPluginPINVOKE.SWIGPendingException.Retrieve();
      return ret;
    } 
  }

  public uint m_toRemoteASendUdpMessageTrialCount {
    set {
      ProudNetServerPluginPINVOKE.P2PPairConnectionStats_m_toRemoteASendUdpMessageTrialCount_set(swigCPtr, value);
      if (ProudNetServerPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetServerPluginPINVOKE.SWIGPendingException.Retrieve();
    } 
    get {
      uint ret = ProudNetServerPluginPINVOKE.P2PPairConnectionStats_m_toRemoteASendUdpMessageTrialCount_get(swigCPtr);
      if (ProudNetServerPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetServerPluginPINVOKE.SWIGPendingException.Retrieve();
      return ret;
    } 
  }

  public uint m_toRemoteASendUdpMessageSuccessCount {
    set {
      ProudNetServerPluginPINVOKE.P2PPairConnectionStats_m_toRemoteASendUdpMessageSuccessCount_set(swigCPtr, value);
      if (ProudNetServerPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetServerPluginPINVOKE.SWIGPendingException.Retrieve();
    } 
    get {
      uint ret = ProudNetServerPluginPINVOKE.P2PPairConnectionStats_m_toRemoteASendUdpMessageSuccessCount_get(swigCPtr);
      if (ProudNetServerPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetServerPluginPINVOKE.SWIGPendingException.Retrieve();
      return ret;
    } 
  }

  public bool m_isRelayed {
    set {
      ProudNetServerPluginPINVOKE.P2PPairConnectionStats_m_isRelayed_set(swigCPtr, value);
      if (ProudNetServerPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetServerPluginPINVOKE.SWIGPendingException.Retrieve();
    } 
    get {
      bool ret = ProudNetServerPluginPINVOKE.P2PPairConnectionStats_m_isRelayed_get(swigCPtr);
      if (ProudNetServerPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetServerPluginPINVOKE.SWIGPendingException.Retrieve();
      return ret;
    } 
  }

  public P2PPairConnectionStats() : this(ProudNetServerPluginPINVOKE.new_P2PPairConnectionStats(), true) {
    if (ProudNetServerPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetServerPluginPINVOKE.SWIGPendingException.Retrieve();
  }

}

}
