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

public class NativeThreadPool : global::System.IDisposable {
  private global::System.Runtime.InteropServices.HandleRef swigCPtr;
  protected bool swigCMemOwn;

  public NativeThreadPool(global::System.IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = new global::System.Runtime.InteropServices.HandleRef(this, cPtr);
  }

  public static global::System.Runtime.InteropServices.HandleRef getCPtr(NativeThreadPool obj) {
    return (obj == null) ? new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero) : obj.swigCPtr;
  }

  ~NativeThreadPool() {
    Dispose();
  }

  public virtual void Dispose() {
    lock(this) {
      if (swigCPtr.Handle != global::System.IntPtr.Zero) {
        if (swigCMemOwn) {
          swigCMemOwn = false;
          ProudNetServerPluginPINVOKE.delete_NativeThreadPool(swigCPtr);
        }
        swigCPtr = new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero);
      }
      global::System.GC.SuppressFinalize(this);
    }
  }

  public virtual void SetDesiredThreadCount(int threadCount) {
    ProudNetServerPluginPINVOKE.NativeThreadPool_SetDesiredThreadCount(swigCPtr, threadCount);
    if (ProudNetServerPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetServerPluginPINVOKE.SWIGPendingException.Retrieve();
  }

  public static NativeThreadPool Create(IThreadPoolEvent eventSink, int threadCount) {
    global::System.IntPtr cPtr = ProudNetServerPluginPINVOKE.NativeThreadPool_Create(IThreadPoolEvent.getCPtr(eventSink), threadCount);
    NativeThreadPool ret = (cPtr == global::System.IntPtr.Zero) ? null : new NativeThreadPool(cPtr, false);
    if (ProudNetServerPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetServerPluginPINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  public virtual void Process(int timeoutMs) {
    ProudNetServerPluginPINVOKE.NativeThreadPool_Process(swigCPtr, timeoutMs);
    if (ProudNetServerPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetServerPluginPINVOKE.SWIGPendingException.Retrieve();
  }

  public virtual void RunAsync(RunAsyncType type, SWIGTYPE_p_f_p_void__void func, global::System.IntPtr context) {
    ProudNetServerPluginPINVOKE.NativeThreadPool_RunAsync(swigCPtr, (int)type, SWIGTYPE_p_f_p_void__void.getCPtr(func), context);
    if (ProudNetServerPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetServerPluginPINVOKE.SWIGPendingException.Retrieve();
  }

}

}
