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

public class DirectP2PInfo : global::System.IDisposable {
  private global::System.Runtime.InteropServices.HandleRef swigCPtr;
  protected bool swigCMemOwn;

  public DirectP2PInfo(global::System.IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = new global::System.Runtime.InteropServices.HandleRef(this, cPtr);
  }

  public static global::System.Runtime.InteropServices.HandleRef getCPtr(DirectP2PInfo obj) {
    return (obj == null) ? new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero) : obj.swigCPtr;
  }

  ~DirectP2PInfo() {
    Dispose();
  }

  public virtual void Dispose() {
    lock(this) {
      if (swigCPtr.Handle != global::System.IntPtr.Zero) {
        if (swigCMemOwn) {
          swigCMemOwn = false;
          ProudNetClientPluginPINVOKE.delete_DirectP2PInfo(swigCPtr);
        }
        swigCPtr = new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero);
      }
      global::System.GC.SuppressFinalize(this);
    }
  }

  public AddrPort localUdpSocketAddr {
    set {
      ProudNetClientPluginPINVOKE.DirectP2PInfo_localUdpSocketAddr_set(swigCPtr, AddrPort.getCPtr(value));
      if (ProudNetClientPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetClientPluginPINVOKE.SWIGPendingException.Retrieve();
    } 
    get {
      global::System.IntPtr cPtr = ProudNetClientPluginPINVOKE.DirectP2PInfo_localUdpSocketAddr_get(swigCPtr);
      AddrPort ret = (cPtr == global::System.IntPtr.Zero) ? null : new AddrPort(cPtr, false);
      if (ProudNetClientPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetClientPluginPINVOKE.SWIGPendingException.Retrieve();
      return ret;
    } 
  }

  public AddrPort localToRemoteAddr {
    set {
      ProudNetClientPluginPINVOKE.DirectP2PInfo_localToRemoteAddr_set(swigCPtr, AddrPort.getCPtr(value));
      if (ProudNetClientPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetClientPluginPINVOKE.SWIGPendingException.Retrieve();
    } 
    get {
      global::System.IntPtr cPtr = ProudNetClientPluginPINVOKE.DirectP2PInfo_localToRemoteAddr_get(swigCPtr);
      AddrPort ret = (cPtr == global::System.IntPtr.Zero) ? null : new AddrPort(cPtr, false);
      if (ProudNetClientPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetClientPluginPINVOKE.SWIGPendingException.Retrieve();
      return ret;
    } 
  }

  public AddrPort remoteToLocalAddr {
    set {
      ProudNetClientPluginPINVOKE.DirectP2PInfo_remoteToLocalAddr_set(swigCPtr, AddrPort.getCPtr(value));
      if (ProudNetClientPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetClientPluginPINVOKE.SWIGPendingException.Retrieve();
    } 
    get {
      global::System.IntPtr cPtr = ProudNetClientPluginPINVOKE.DirectP2PInfo_remoteToLocalAddr_get(swigCPtr);
      AddrPort ret = (cPtr == global::System.IntPtr.Zero) ? null : new AddrPort(cPtr, false);
      if (ProudNetClientPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetClientPluginPINVOKE.SWIGPendingException.Retrieve();
      return ret;
    } 
  }

  public DirectP2PInfo() : this(ProudNetClientPluginPINVOKE.new_DirectP2PInfo(), true) {
    if (ProudNetClientPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetClientPluginPINVOKE.SWIGPendingException.Retrieve();
  }

  public bool HasBeenHolepunched() {
    bool ret = ProudNetClientPluginPINVOKE.DirectP2PInfo_HasBeenHolepunched(swigCPtr);
    if (ProudNetClientPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetClientPluginPINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

}

}
