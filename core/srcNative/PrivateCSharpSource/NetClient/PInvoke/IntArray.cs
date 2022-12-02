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

public class IntArray : global::System.IDisposable {
  private global::System.Runtime.InteropServices.HandleRef swigCPtr;
  protected bool swigCMemOwn;

  public IntArray(global::System.IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = new global::System.Runtime.InteropServices.HandleRef(this, cPtr);
  }

  public static global::System.Runtime.InteropServices.HandleRef getCPtr(IntArray obj) {
    return (obj == null) ? new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero) : obj.swigCPtr;
  }

  ~IntArray() {
    Dispose();
  }

  public virtual void Dispose() {
    lock(this) {
      if (swigCPtr.Handle != global::System.IntPtr.Zero) {
        if (swigCMemOwn) {
          swigCMemOwn = false;
          ProudNetClientPluginPINVOKE.delete_IntArray(swigCPtr);
        }
        swigCPtr = new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero);
      }
      global::System.GC.SuppressFinalize(this);
    }
  }

  public IntArray() : this(ProudNetClientPluginPINVOKE.new_IntArray(), true) {
    if (ProudNetClientPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetClientPluginPINVOKE.SWIGPendingException.Retrieve();
  }

  public void SuspendShrink() {
    ProudNetClientPluginPINVOKE.IntArray_SuspendShrink(swigCPtr);
    if (ProudNetClientPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetClientPluginPINVOKE.SWIGPendingException.Retrieve();
  }

  public void OnRecycle() {
    ProudNetClientPluginPINVOKE.IntArray_OnRecycle(swigCPtr);
    if (ProudNetClientPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetClientPluginPINVOKE.SWIGPendingException.Retrieve();
  }

  public void OnDrop() {
    ProudNetClientPluginPINVOKE.IntArray_OnDrop(swigCPtr);
    if (ProudNetClientPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetClientPluginPINVOKE.SWIGPendingException.Retrieve();
  }

  public void AddCount(int addLength) {
    ProudNetClientPluginPINVOKE.IntArray_AddCount(swigCPtr, addLength);
    if (ProudNetClientPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetClientPluginPINVOKE.SWIGPendingException.Retrieve();
  }

  public void resize(int sz) {
    ProudNetClientPluginPINVOKE.IntArray_resize(swigCPtr, sz);
    if (ProudNetClientPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetClientPluginPINVOKE.SWIGPendingException.Retrieve();
  }

  public int GetCount() {
    int ret = ProudNetClientPluginPINVOKE.IntArray_GetCount(swigCPtr);
    if (ProudNetClientPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetClientPluginPINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  public int size() {
    int ret = ProudNetClientPluginPINVOKE.IntArray_size(swigCPtr);
    if (ProudNetClientPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetClientPluginPINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  public bool IsEmpty() {
    bool ret = ProudNetClientPluginPINVOKE.IntArray_IsEmpty(swigCPtr);
    if (ProudNetClientPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetClientPluginPINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  public void Clear() {
    ProudNetClientPluginPINVOKE.IntArray_Clear(swigCPtr);
    if (ProudNetClientPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetClientPluginPINVOKE.SWIGPendingException.Retrieve();
  }

  public void RemoveAt(int index) {
    ProudNetClientPluginPINVOKE.IntArray_RemoveAt(swigCPtr, index);
    if (ProudNetClientPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetClientPluginPINVOKE.SWIGPendingException.Retrieve();
  }

  public void Add(int value) {
    ProudNetClientPluginPINVOKE.IntArray_Add(swigCPtr, value);
    if (ProudNetClientPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetClientPluginPINVOKE.SWIGPendingException.Retrieve();
  }

  public int Get(int index) {
    int ret = ProudNetClientPluginPINVOKE.IntArray_Get(swigCPtr, index);
    if (ProudNetClientPluginPINVOKE.SWIGPendingException.Pending) throw ProudNetClientPluginPINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  public static readonly uint TYPE_SIZE = ProudNetClientPluginPINVOKE.IntArray_TYPE_SIZE_get();
}

}
