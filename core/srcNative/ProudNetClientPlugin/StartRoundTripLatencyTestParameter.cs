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

public class StartRoundTripLatencyTestParameter : global::System.IDisposable {
  private global::System.Runtime.InteropServices.HandleRef swigCPtr;
  protected bool swigCMemOwn;

  public StartRoundTripLatencyTestParameter(global::System.IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = new global::System.Runtime.InteropServices.HandleRef(this, cPtr);
  }

  public static global::System.Runtime.InteropServices.HandleRef getCPtr(StartRoundTripLatencyTestParameter obj) {
    return (obj == null) ? new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero) : obj.swigCPtr;
  }

  ~StartRoundTripLatencyTestParameter() {
    Dispose();
  }

  public virtual void Dispose() {
    lock(this) {
      if (swigCPtr.Handle != global::System.IntPtr.Zero) {
        if (swigCMemOwn) {
          swigCMemOwn = false;
          ProudNetClientPluginPINVOKE.delete_StartRoundTripLatencyTestParameter(swigCPtr);
        }
        swigCPtr = new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero);
      }
      global::System.GC.SuppressFinalize(this);
    }
  }

  public int testDurationMs {
    set {
      ProudNetClientPluginPINVOKE.StartRoundTripLatencyTestParameter_testDurationMs_set(swigCPtr, value);
      if (ProudNetClientPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetClientPluginPINVOKE.SWIGPendingException.Retrieve();
    } 
    get {
      int ret = ProudNetClientPluginPINVOKE.StartRoundTripLatencyTestParameter_testDurationMs_get(swigCPtr);
      if (ProudNetClientPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetClientPluginPINVOKE.SWIGPendingException.Retrieve();
      return ret;
    } 
  }

  public int pingIntervalMs {
    set {
      ProudNetClientPluginPINVOKE.StartRoundTripLatencyTestParameter_pingIntervalMs_set(swigCPtr, value);
      if (ProudNetClientPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetClientPluginPINVOKE.SWIGPendingException.Retrieve();
    } 
    get {
      int ret = ProudNetClientPluginPINVOKE.StartRoundTripLatencyTestParameter_pingIntervalMs_get(swigCPtr);
      if (ProudNetClientPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetClientPluginPINVOKE.SWIGPendingException.Retrieve();
      return ret;
    } 
  }

  public StartRoundTripLatencyTestParameter() : this(ProudNetClientPluginPINVOKE.new_StartRoundTripLatencyTestParameter(), true) {
    if (ProudNetClientPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetClientPluginPINVOKE.SWIGPendingException.Retrieve();
  }

}

}
